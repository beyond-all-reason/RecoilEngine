/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#define LUA_SYNCED_ONLY

#include "LuaUnitScript.h"

#include "CobDefines.h"
#include "CobInstance.h"
#include "LuaInclude.h"
#include "NullUnitScript.h"
#include "UnitScriptFactory.h"
#include "LuaScriptNames.h"
#include "Lua/LuaConfig.h"
#include "Lua/LuaCallInCheck.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaHandleSynced.h"
#include "Lua/LuaRules.h"
#include "Lua/LuaUtils.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "System/ContainerUtil.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "Rendering/Models/3DModelPiece.hpp"
#include "Sim/IK/IKSolver.hpp"

#include "System/Misc/TracyDefs.h"

#include <new>

CR_BIND_DERIVED(CLuaUnitScript, CUnitScript, )

CR_REG_METADATA(CLuaUnitScript, (
	CR_IGNORED(handle),
	CR_IGNORED(L),
	CR_MEMBER(scriptIndex),
	CR_MEMBER(scriptNames),
	CR_IGNORED(inKilled),
	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad),
	CR_PREALLOC(GetUnit)
))

void CLuaUnitScript::PostLoad()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (unit == nullptr)
		return;

	pieces.reserve(unit->localModel.pieces.size());

	for (auto& p: unit->localModel.pieces) {
		pieces.push_back(&p);
		if (!p.parent)
			rootPiece = &p;
	}
	assert(rootPiece);

	L = handle->GetLuaState();

	hasSetSFXOccupy  = scriptIndex[LUAFN_SetSFXOccupy ] != LUA_NOREF;
	hasRockUnit      = scriptIndex[LUAFN_RockUnit     ] != LUA_NOREF;
	hasStartBuilding = scriptIndex[LUAFN_StartBuilding] != LUA_NOREF;
}


void CLuaUnitScript::Serialize(creg::ISerializer* s) {
	RECOIL_DETAILED_TRACY_ZONE;
	bool isLuaGaia;
	if (s->IsWriting()) {
		isLuaGaia = luaGaia != nullptr && handle == &luaGaia->syncedLuaHandle;
		const bool isLuaRules = luaRules != nullptr && handle == &luaRules->syncedLuaHandle;
		assert(isLuaGaia || isLuaRules);
	}
	s->SerializeInt(&isLuaGaia, sizeof(isLuaGaia));
	if (!s->IsWriting()) {
		CSplitLuaHandle* slh = isLuaGaia ? (CSplitLuaHandle*) luaGaia : (CSplitLuaHandle*) luaRules;
		assert(slh != nullptr);
		handle = &slh->syncedLuaHandle;
		L = handle->GetLuaState();
	}
}


static inline LocalModelPiece* ParseLocalModelPiece(lua_State* L, CUnitScript* script, const char* caller)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int piece = luaL_checkint(L, 1) - 1;

	auto* p = script->SafeGetPiece(piece);
	if (!p)
		luaL_error(L, "%s(): Invalid piecenumber", caller);

	return p;
}

static inline int ToLua(lua_State* L, const float3& v)
{
	RECOIL_DETAILED_TRACY_ZONE;
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	lua_pushnumber(L, v.z);
	return 3;
}

static inline IK::Skeleton* toSkeleton(lua_State* L, int idx)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto ud = static_cast<std::shared_ptr<IK::Skeleton>*>(luaL_checkudata(L, idx, "IKSkeleton"));
	if (*ud == nullptr)
		luaL_error(L, "attempt to use a deleted IK skeleton");
	return ud->get();
}

static inline IK::Chain* toChain(lua_State* L, int idx)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto ud = static_cast<std::shared_ptr<IK::Chain>*>(luaL_checkudata(L, idx, "IKChain"));
	if (*ud == nullptr)
		luaL_error(L, "attempt to use a deleted IK chain");
	return ud->get();
}

static inline std::shared_ptr<IK::Chain> toChainShared(lua_State* L, int idx)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto ud = static_cast<std::shared_ptr<IK::Chain>*>(luaL_checkudata(L, idx, "IKChain"));
	if (*ud == nullptr)
		luaL_error(L, "attempt to use a deleted IK chain");
	return *ud;
}

struct IKSolution {
	IK::ChainSolution sol;
	std::shared_ptr<IK::Chain> chain;
};

static inline IKSolution* toSolution(lua_State* L, int idx)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return static_cast<IKSolution*>(luaL_checkudata(L, idx, "IKSolution"));
}

static int PushChainSolution(lua_State* L, const IK::ChainSolution& solution, std::shared_ptr<IK::Chain> chain)
{
	auto* ud = static_cast<IKSolution*>(lua_newuserdata(L, sizeof(IKSolution)));
	new (ud) IKSolution{ solution, std::move(chain) };

	luaL_getmetatable(L, "IKSolution");
	lua_setmetatable(L, -2);
	return 1;
}


/*

some notes:
- all piece numbers are 1 based, to be consistent with other parts of interface
- all axes are 1 based, so 1 = x, 2 = y, 3 = z
- destination, speed for Move are in world coords, and NOT in COB coords
- therefore, compared to COB, the X axis for the Move callout is mirrored
- destination, speed, accel, decel for Turn, Spin, StopSpin are in radians
- GetUnitCOBValue(PLAY_SOUND, ...) does NOT work for Lua unit scripts,
  use Spring.PlaySound instead (synced code can call unsynced funcs!).
- Because in current design CBCobThreadFinish can impossibly be called, certain
  state changes which normally happen immediately when script returns should
  be triggered through a call to a callOut when using Lua scripts.
  This applies to:
  * Spring.SetUnitShieldState(unitID, false|true) replaces return value 0|1 of
    COB's AimWeaponX function for plasma repulsers.
  * Spring.SetUnitWeaponState(unitID, weaponNum, "aimReady", 0|1) replaces
    return value 0|1 of COB's AimWeaponX function for all other weapons.
  * Spring.UnitScript.SetDeathScriptFinished(wreckLevel) replaces
    return value of wreckLevel from Killed function.
    This MUST be called, otherwise zombie units will eat your Spring!


callIn notes:
- Killed takes recentDamage,maxHealth instead of severity, to calculate
  severity use: 'local severity = recentDamage / maxHealth'
- SetMaxReloadTime doesn't exist, max reload time can be calculated in Lua
  (without the WTFs that are present in max reload time calculation for COB)
- RockUnit takes x,z instead of 500z,500x
- HitByWeapon takes x,z instead of 500z,500x
- HitByWeaponId takes x,z,weaponDefID,damage instead of 500z,500x,tdfID,damage
- HitByWeaponId returns the new damage, instead of a percentage of old damage
- QueryLandingPadCount doesn't exist
- QueryLandingPad should return an array (table) of all pieces
- BeginTransport and QueryTransport take unitID instead of unit->height*65536,
  use 'local height = Spring.GetUnitHeight(unitID)' to get the height.
- TransportDrop takes x,y,z instead of PACKXZ(x,z)
- AimWeapon for a shield (plasma repulser) takes no arguments instead of 0,0
- Shot takes no arguments instead of 0
- new callins MoveFinished and TurnFinished, see below


docs for callins defined in this file:

  TODO: document other callins properly

TurnFinished(number piece, number axis)
	Called after a turn finished for this unit/piece/axis (not a turn-now!)
	Should resume coroutine of the particular thread which called the Lua
	WaitForTurn function (see below).

MoveFinished(number piece, number axis)
	Called after a move finished for this unit/piece/axis (not a move-now!)
	Should resume coroutine of the particular thread which called the Lua
	WaitForMove function (see below).


docs for callouts defined in this file:

Spring.UnitScript.SetUnitValue(...)
	see wiki for Spring.SetUnitCOBValue (unchanged)

Spring.UnitScript.GetUnitValue(...)
	see wiki for Spring.GetUnitCOBValue (unchanged)

Spring.UnitScript.SetPieceVisibility(number piece, boolean visible) -> nil
	Set's piece visibility.  Same as COB's hide/show.

Spring.UnitSript.EmitSfx(number piece, number type) -> nil
	Same as COB's emit-sfx.

Spring.UnitScript.AttachUnit(number piece, number transporteeID) -> nil
	Same as COB's attach-unit.

Spring.UnitScript.DropUnit(number transporteeID) -> nil
	Same as COB's drop-unit.

Spring.UnitScript.Explode(number piece, number flags) -> nil
	Same as COB's explode.

Spring.UnitScript.ShowFlare(number piece) -> nil
	Same as COB's show _inside_ FireWeaponX.

Spring.UnitScript.Spin(number piece, number axis, number speed[, number accel]) -> nil
	Same as COB's spin.  If accel isn't given spinning starts at the desired speed.

Spring.UnitScript.StopSpin(number piece, number axis[, number decel]) -> nil
	Same as COB's stop-spin.  If decel isn't given spinning stops immediately.

Spring.UnitScript.Turn(number piece, number axis, number destination[, number speed]) -> nil
	Same as COB's turn iff speed is given and not zero, and turn-now otherwise.

Spring.UnitScript.Move(number piece, number axis, number destination[, number speed]) -> nil
	Same as COB's move iff speed is given and not zero, and move-now otherwise.

Spring.UnitScript.Scale(number piece, number destination[, number speed]) -> nil
	Same as COB's scale iff speed is given and not zero, and scale-now otherwise.

Spring.UnitScript.IsInTurn(number piece, number axis) -> boolean
Spring.UnitScript.IsInMove(number piece, number axis) -> boolean
Spring.UnitScript.IsInSpin(number piece, number axis) -> boolean
Spring.UnitScript.IsInScale(number piece) -> boolean
	Returns true iff such an animation exists, false otherwise.

Spring.UnitScript.WaitForTurn(number piece, number axis) -> boolean
	Returns true iff such an animation exists, false otherwise.  Iff it returns
	true, the TurnFinished callIn will be called once the turn completes.

Spring.UnitScript.WaitForMove(number piece, number axis) -> boolean
	Returns true iff such an animation exists, false otherwise.  Iff it returns
	true, the MoveFinished callIn will be called once the move completes.

Spring.UnitScript.WaitForScale(number piece) -> boolean
	Returns true iff such an animation exists, false otherwise.  Iff it returns
	true, the ScaleFinished callIn will be called once the scale completes.

Spring.UnitScript.SetDeathScriptFinished(number wreckLevel])
	Tells Spring the Killed script finished, and which wreckLevel to use.
	If wreckLevel is not given no wreck is created.

Spring.UnitScript.CreateScript(number unitID, table callIns) -> nil
	Replaces the current unit script (independent of type, also replaces COB)
	with the unit script given by a table of callins for the unit.
	Callins are similar to COB functions, e.g. a number of predefined names are
	called by the engine if they exist in the table.

Spring.UnitScript.UpdateCallIn(number unitID, string fname[, function callIn]) -> number|boolean
	Iff callIn is a function, a single callIn is replaced or added, and the
	new functionID is returned.  If callIn isn't given or is nil, the callIn is
	nilled, returns true if it was removed, or false if the callin didn't exist.
	See also Spring.UnitScript.CreateScript.
*/


#define LUA_TRACE(m) \
	do { \
		if (unit) { \
			LOG_L(L_DEBUG, "%s: %d: %s", __func__, unit->id, m); \
		} else { \
			LOG_L(L_DEBUG, "%s: %s", __func__, m); \
		} \
	} while (false)


CUnit* CLuaUnitScript::activeUnit;
CUnitScript* CLuaUnitScript::activeScript;


/******************************************************************************/
/******************************************************************************/


CLuaUnitScript::CLuaUnitScript(lua_State* L, CUnit* unit)
	: CUnitScript(unit)
	, handle(CLuaHandle::GetHandle(L)), L(L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	scriptIndex.fill(LUA_NOREF);
	scriptNames.reserve(scriptIndex.size());
	pieces.reserve(unit->localModel.pieces.size());

	for (lua_pushnil(L); lua_next(L, 2) != 0; /*lua_pop(L, 1)*/) {
		const std::string& fname = lua_tostring(L, -2);
		const int r = luaL_ref(L, LUA_REGISTRYINDEX);

		scriptNames.emplace(fname, r);
		UpdateCallIn(fname, r);
	}
	for (auto& p: unit->localModel.pieces) {
		pieces.push_back(&p);
		if (!p.parent)
			rootPiece = &p;
	}
	assert(rootPiece);
}


CLuaUnitScript::~CLuaUnitScript()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// if L is NULL then the lua_State is closed/closing (see HandleFreed)
	if (L == nullptr)
		return;

	// notify Lua the script is going down
	Destroy();

	for (auto it = scriptNames.begin(); it != scriptNames.end(); ++it) {
		luaL_unref(L, LUA_REGISTRYINDEX, it->second);
	}
}


void CLuaUnitScript::HandleFreed(CLuaHandle* handle)
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (CUnit* u: unitHandler.GetActiveUnits()) {
		CUnitScript* script = u->script;
		CLuaUnitScript* luaScript = dynamic_cast<CLuaUnitScript*>(script);

		// kill only the Lua scripts running in this handle
		if (luaScript == nullptr)
			continue;
		if (luaScript->handle != handle)
			continue;

		// we don't have anything better ...
		u->script = &CNullUnitScript::value;

		// signal the destructor it shouldn't unref refs
		luaScript->L = nullptr;

		spring::SafeDestruct(script);
	}
}


int CLuaUnitScript::UpdateCallIn()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const std::string& fname = lua_tostring(L, 2);
	const bool remove = lua_isnoneornil(L, 3);

	auto it = scriptNames.find(fname);
	int r = LUA_NOREF;

	if (it != scriptNames.end()) {
		luaL_unref(L, LUA_REGISTRYINDEX, it->second);
		if (remove) {
			// removing existing callIn
			scriptNames.erase(it);
			lua_pushboolean(L, 1);
		}
		else {
			// replacing existing callIn
			r = luaL_ref(L, LUA_REGISTRYINDEX);
			it->second = r;
		}
	}
	else if (remove) {
		// removing nonexisting callIn (== no-op)
		lua_pushboolean(L, 0);
	}
	else {
		// adding new callIn
		r = luaL_ref(L, LUA_REGISTRYINDEX);
		scriptNames.emplace(fname, r);
	}

	UpdateCallIn(fname, r);

	if (!remove) {
		// the reference doubles as the functionId, as expected by RealCall
		// from Lua this can be used with e.g. Spring.CallCOBScript
		lua_pushnumber(L, r);
	}
	return 1;
}


void CLuaUnitScript::UpdateCallIn(const std::string& fname, int ref)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// Map common function names to indices
	int num = CLuaUnitScriptNames::GetScriptNumber(fname);

	// Check upper bound too in case user calls UpdateCallIn with nonexisting weapon.
	// (we only allocate slots in scriptIndex for the number of weapons the unit has)
	if (num >= 0 && num < int(scriptIndex.size())) {
		scriptIndex[num] = ref;
	}

	switch (num) {
		case LUAFN_SetSFXOccupy:  hasSetSFXOccupy  = (ref != LUA_NOREF); break;
		case LUAFN_RockUnit:      hasRockUnit      = (ref != LUA_NOREF); break;
		case LUAFN_StartBuilding: hasStartBuilding = (ref != LUA_NOREF); break;
	}

	//LUA_TRACE(fname.c_str());
}


void CLuaUnitScript::RemoveCallIn(const std::string& fname)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = scriptNames.find(fname);

	if (it != scriptNames.end()) {
		luaL_unref(L, LUA_REGISTRYINDEX, it->second);
		scriptNames.erase(it);
		UpdateCallIn(fname, LUA_NOREF);
	}

	//LUA_TRACE(fname.c_str());
}


void CLuaUnitScript::ShowScriptError(const std::string& msg)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// if we are in the same handle, we can truly raise an error
	if (handle->IsRunning()) {
		luaL_error(L, "Lua UnitScript error: %s", msg.c_str());
	}
	else {
		LOG_L(L_ERROR, "%s", msg.c_str());
	}
}


bool CLuaUnitScript::HasBlockShot(int weaponNum) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return HasFunction(LUAFN_BlockShot);
}


bool CLuaUnitScript::HasTargetWeight(int weaponNum) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return HasFunction(LUAFN_TargetWeight);
}


/******************************************************************************/
/******************************************************************************/


inline float CLuaUnitScript::PopNumber(int fn, float def)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!lua_israwnumber(L, -1)) {
		const std::string& fname = CLuaUnitScriptNames::GetScriptName(fn);

		LOG_L(L_ERROR, "%s: bad return value, expected number", fname.c_str());
		RemoveCallIn(fname);

		lua_pop(L, 1);
		return def;
	}

	const float ret = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return ret;
}


inline bool CLuaUnitScript::PopBoolean(int fn, bool def)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!lua_isboolean(L, -1)) {
		const std::string& fname = CLuaUnitScriptNames::GetScriptName(fn);

		LOG_L(L_ERROR, "%s: bad return value, expected boolean", fname.c_str());
		RemoveCallIn(fname);

		lua_pop(L, 1);
		return def;
	}

	const bool ret = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return ret;
}


inline void CLuaUnitScript::RawPushFunction(int functionId)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// Push Lua function on the stack
	lua_rawgeti(L, LUA_REGISTRYINDEX, functionId);
}


inline void CLuaUnitScript::PushFunction(int id)
{
	RECOIL_DETAILED_TRACY_ZONE;
	RawPushFunction(scriptIndex[id]);
}


inline void CLuaUnitScript::PushUnit(const CUnit* targetUnit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (targetUnit) {
		lua_pushnumber(L, targetUnit->id);
	}
	else {
		lua_pushnil(L);
	}
}


inline bool CLuaUnitScript::RunCallIn(int id, int inArgs, int outArgs)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return RawRunCallIn(scriptIndex[id], inArgs, outArgs);
}


int CLuaUnitScript::RunQueryCallIn(int fn)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!HasFunction(fn))
		return -1;

	LUA_CALL_IN_CHECK(L, -1);
	lua_checkstack(L, 1);

	PushFunction(fn);

	if (!RunCallIn(fn, 0, 1))
		return -1;

	const int scriptPieceNum = (int)PopNumber(fn, 0) - 1;

	// if (LOG_IS_ENABLED(L_DEBUG)) {
		// if (PieceExists(scriptPieceNum)) {
			// LocalModelPiece* piece = GetScriptLocalModelPiece(scriptPieceNum);
			// LOG_L(L_DEBUG, "%s: %d %s",
					// CLuaUnitScriptNames::GetScriptName(fn).c_str(),
					// scriptPieceNum,
					// (piece->original) ? piece->original->name.c_str() : "n/a");
		// }
	// }

	return scriptPieceNum;
}


int CLuaUnitScript::RunQueryCallIn(int fn, float arg1)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!HasFunction(fn))
		return -1;

	LUA_CALL_IN_CHECK(L, -1);
	lua_checkstack(L, 2);

	PushFunction(fn);
	lua_pushnumber(L, arg1);

	if (!RunCallIn(fn, 1, 1))
		return -1;

	const int scriptPieceNum = (int)PopNumber(fn, 0) - 1;

	// if (LOG_IS_ENABLED(L_DEBUG)) {
		// if (PieceExists(scriptPieceNum)) {
			// LocalModelPiece* piece = GetScriptLocalModelPiece(scriptPieceNum);
			// LOG_L(L_DEBUG, "%s: %d %s",
					// CLuaUnitScriptNames::GetScriptName(fn).c_str(),
					// scriptPieceNum,
					// (piece->original) ? piece->original->name.c_str() : "n/a");
		// }
	// }

	return scriptPieceNum;
}


void CLuaUnitScript::Call(int fn, float arg1)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!HasFunction(fn))
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);

	PushFunction(fn);
	lua_pushnumber(L, arg1);

	RunCallIn(fn, 1, 0);
}


void CLuaUnitScript::Call(int fn, float arg1, float arg2)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!HasFunction(fn))
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	PushFunction(fn);
	lua_pushnumber(L, arg1);
	lua_pushnumber(L, arg2);

	RunCallIn(fn, 2, 0);
}


void CLuaUnitScript::Call(int fn, float arg1, float arg2, float arg3)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!HasFunction(fn))
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	PushFunction(fn);
	lua_pushnumber(L, arg1);
	lua_pushnumber(L, arg2);
	lua_pushnumber(L, arg3);

	RunCallIn(fn, 3, 0);
}


/******************************************************************************/
/******************************************************************************/


void CLuaUnitScript::Create()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// There is no use for Create
	// (Lua code can just call it after Spring.UnitScript.CreateScript(...))
}


void CLuaUnitScript::Killed()
{
	ZoneScoped;
	const int fn = LUAFN_Killed;

	if (!HasFunction(fn)) {
		unit->KilledScriptFinished(unit->delayedWreckLevel);
		return;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	PushFunction(fn);
	lua_pushnumber(L, unit->recentDamage);
	lua_pushnumber(L, unit->maxHealth);

	inKilled = true;

	if (!RunCallIn(fn, 2, 1))
		return;

	// If Killed returns an integer, it signals it hasn't started a thread.
	// In this case the return value is the delayedWreckLevel.
	if (lua_israwnumber(L, -1)) {
		inKilled = false;
		unit->KilledScriptFinished(lua_toint(L, -1));
	}
	else if (!lua_isnoneornil(L, -1)) {
		const std::string& fname = CLuaUnitScriptNames::GetScriptName(fn);

		LOG_L(L_ERROR, "%s: bad return value, expected number or nil", fname.c_str());
		RemoveCallIn(fname);

		// without this we would end up with zombie units
		unit->KilledScriptFinished(unit->delayedWreckLevel);
	}

	lua_pop(L, 1);
}


void CLuaUnitScript::WindChanged(float heading, float speed)
{
	ZoneScoped;
	Call(LUAFN_WindChanged, heading, speed);
}


void CLuaUnitScript::ExtractionRateChanged(float speed)
{
	ZoneScoped;
	Call(LUAFN_ExtractionRateChanged, speed);
}



void CLuaUnitScript::WorldRockUnit(const float3& rockDir) { RockUnit(unit->GetObjectSpaceVec(rockDir)); }
void CLuaUnitScript::RockUnit(const float3& rockDir)
{
	ZoneScoped;
	Call(LUAFN_RockUnit, rockDir.x, rockDir.z);
}

void CLuaUnitScript::WorldHitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage) { HitByWeapon(unit->GetObjectSpaceVec(hitDir), weaponDefId, inoutDamage); }
void CLuaUnitScript::HitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage)
{
	ZoneScoped;

	const int fn = LUAFN_HitByWeapon;

	if (!HasFunction(fn))
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);

	PushFunction(fn);
	lua_pushnumber(L, hitDir.x);
	lua_pushnumber(L, hitDir.z);
	lua_pushnumber(L, weaponDefId);
	lua_pushnumber(L, inoutDamage);

	if (!RunCallIn(fn, 4, 1))
		return;

	if (lua_israwnumber(L, -1)) {
		inoutDamage = lua_tonumber(L, -1);
	}
	else if (!lua_isnoneornil(L, -1)) {
		const std::string& fname = CLuaUnitScriptNames::GetScriptName(fn);

		LOG_L(L_ERROR, "%s: bad return value, expected number or nil", fname.c_str());
		RemoveCallIn(fname);
	}

	lua_pop(L, 1);
}


void CLuaUnitScript::SetSFXOccupy(int curTerrainType)
{
	ZoneScoped;
	const int fn = LUAFN_SetSFXOccupy;

	if (!HasFunction(fn))
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);

	PushFunction(fn);
	lua_pushnumber(L, curTerrainType);

	RunCallIn(fn, 1, 0);
}


void CLuaUnitScript::QueryLandingPads(std::vector<int>& out_pieces)
{
	ZoneScoped;
	const int fn = LUAFN_QueryLandingPads;

	if (!HasFunction(fn))
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);

	PushFunction(fn);

	if (!RunCallIn(fn, 0, 1))
		return;

	if (lua_istable(L, -1)) {
		int n = 1;
		// get the first piece number at t[n=1]
		lua_rawgeti(L, -1, n);

		// t = {[1] = piece_number_1, [2] = piece_number_2, ...}
		while (lua_israwnumber(L, -1)) {
			out_pieces.push_back(lua_toint(L, -1) - 1);
			lua_pop(L, 1);
			lua_rawgeti(L, -1, ++n);
		}

		lua_pop(L, 1);
	} else {
		const std::string& fname = CLuaUnitScriptNames::GetScriptName(fn);

		LOG_L(L_ERROR, "%s: bad return value, expected table", fname.c_str());
		RemoveCallIn(fname);
	}

	lua_pop(L, 1);
}


void CLuaUnitScript::BeginTransport(const CUnit* unit)
{
	ZoneScoped;
	Call(LUAFN_BeginTransport, unit->id);
}


int CLuaUnitScript::QueryTransport(const CUnit* unit)
{
	ZoneScoped;
	return RunQueryCallIn(LUAFN_QueryTransport, unit->id);
}


void CLuaUnitScript::TransportPickup(const CUnit* unit)
{
	ZoneScoped;
	Call(LUAFN_TransportPickup, unit->id);
}


void CLuaUnitScript::TransportDrop(const CUnit* unit, const float3& pos)
{
	ZoneScoped;
	const int fn = LUAFN_TransportDrop;

	if (!HasFunction(fn))
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);

	PushFunction(fn);
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);

	RunCallIn(fn, 4, 0);
}


void CLuaUnitScript::StartBuilding(float heading, float pitch)
{
	ZoneScoped;
	Call(LUAFN_StartBuilding, heading, pitch);
}


int CLuaUnitScript::QueryNanoPiece()
{
	ZoneScoped;
	return RunQueryCallIn(LUAFN_QueryNanoPiece);
}


int CLuaUnitScript::QueryBuildInfo()
{
	ZoneScoped;
	return RunQueryCallIn(LUAFN_QueryBuildInfo);
}


int CLuaUnitScript::QueryWeapon(int weaponNum)
{
	ZoneScoped;
	return RunQueryCallIn(LUAFN_QueryWeapon, weaponNum + LUA_WEAPON_BASE_INDEX);
}


void CLuaUnitScript::AimWeapon(int weaponNum, float heading, float pitch)
{
	ZoneScoped;
	Call(LUAFN_AimWeapon, weaponNum + LUA_WEAPON_BASE_INDEX, heading, pitch);
}


void  CLuaUnitScript::AimShieldWeapon(CPlasmaRepulser* weapon)
{
	ZoneScoped;
	Call(LUAFN_AimShield, weapon->weaponNum + LUA_WEAPON_BASE_INDEX);
}


int CLuaUnitScript::AimFromWeapon(int weaponNum)
{
	ZoneScoped;
	return RunQueryCallIn(LUAFN_AimFromWeapon, weaponNum + LUA_WEAPON_BASE_INDEX);
}


void CLuaUnitScript::Shot(int weaponNum)
{
	ZoneScoped;
	// FIXME: pass projectileID?
	Call(LUAFN_Shot, weaponNum + LUA_WEAPON_BASE_INDEX);
}


bool CLuaUnitScript::BlockShot(int weaponNum, const CUnit* targetUnit, bool userTarget)
{
	ZoneScoped;
	const int fn = LUAFN_BlockShot;

	if (!HasFunction(fn))
		return false;

	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 4);

	PushFunction(fn);
	lua_pushnumber(L, weaponNum + LUA_WEAPON_BASE_INDEX);
	PushUnit(targetUnit);
	lua_pushboolean(L, userTarget);

	if (!RunCallIn(fn, 3, 1))
		return false;

	return PopBoolean(fn, false);
}


float CLuaUnitScript::TargetWeight(int weaponNum, const CUnit* targetUnit)
{
	ZoneScoped;
	const int fn = LUAFN_TargetWeight;

	if (!HasFunction(fn))
		return 1.0f;

	LUA_CALL_IN_CHECK(L, 1.0f);
	lua_checkstack(L, 3);

	PushFunction(fn);
	lua_pushnumber(L, weaponNum + LUA_WEAPON_BASE_INDEX);
	PushUnit(targetUnit);

	if (!RunCallIn(fn, 2, 1))
		return 1.0f;

	return PopNumber(fn, 1.0f);
}


void CLuaUnitScript::AnimFinished(AnimType type, int piece, int axis)
{
	ZoneScoped;
	switch (type) {
	case ATurn:
		Call(LUAFN_TurnFinished, piece + 1, axis + 1); break;
	case AMove:
		Call(LUAFN_MoveFinished, piece + 1, axis + 1); break;
	case AScale:
		Call(LUAFN_ScaleFinished, piece + 1); break;
	default:
		assert(false);
	}
}


void CLuaUnitScript::RawCall(int functionId)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (functionId < 0)
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 1);

	RawPushFunction(functionId);
	RawRunCallIn(functionId, 0, 0);
}


std::string CLuaUnitScript::GetScriptName(int functionId) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// only for error messages, so speed doesn't matter
	const auto pred = [functionId](const decltype(scriptNames)::value_type& p) { return (functionId == p.second); };
	const auto iter = std::find_if(scriptNames.begin(), scriptNames.end(), pred);

	if (iter != scriptNames.end())
		return iter->first;

	return ("<unnamed: " + IntToString(functionId) + ">");
}


bool CLuaUnitScript::RawRunCallIn(int functionId, int inArgs, int outArgs)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* oldActiveUnit = activeUnit;
	CUnitScript* oldActiveScript = activeScript;

	activeUnit = unit;
	activeScript = this;

	std::string err;
	const int error = handle->RunCallInLUS(L, &err, inArgs, outArgs);

	activeUnit = oldActiveUnit;
	activeScript = oldActiveScript;

	if (error == 0)
		return true;

	const std::string& hname = handle->GetName();
	const std::string& fname = GetScriptName(functionId);

	LOG_L(L_ERROR, "[LuaUnitScript::%s][%s::%s] error=%i trace=%s", __func__, hname.c_str(), fname.c_str(), error, err.c_str());
	RemoveCallIn(fname);

	return false;
}


void CLuaUnitScript::Destroy() { ZoneScoped; Call(LUAFN_Destroy); }
void CLuaUnitScript::StartMoving(bool reversing) { ZoneScoped; Call(LUAFN_StartMoving, reversing * 1.0f); }
void CLuaUnitScript::StopMoving() { ZoneScoped; Call(LUAFN_StopMoving); }
void CLuaUnitScript::StartSkidding(const float3& vel) { ZoneScoped; Call(LUAFN_StartSkidding, vel.x, vel.y, vel.z); }
void CLuaUnitScript::StopSkidding() { ZoneScoped; Call(LUAFN_StopSkidding); }
void CLuaUnitScript::ChangeHeading(short deltaHeading) { ZoneScoped; Call(LUAFN_ChangeHeading, deltaHeading * 1.0f); }
void CLuaUnitScript::StartUnload() { ZoneScoped; Call(LUAFN_StartUnload); }
void CLuaUnitScript::EndTransport() { ZoneScoped; Call(LUAFN_EndTransport); }
void CLuaUnitScript::StartBuilding() { ZoneScoped; Call(LUAFN_StartBuilding); }
void CLuaUnitScript::StopBuilding() { ZoneScoped; Call(LUAFN_StopBuilding); }
void CLuaUnitScript::Falling() { ZoneScoped; Call(LUAFN_Falling); }
void CLuaUnitScript::Landed() { ZoneScoped; Call(LUAFN_Landed); }
void CLuaUnitScript::Activate() { ZoneScoped; Call(LUAFN_Activate); }
void CLuaUnitScript::Deactivate() { ZoneScoped; Call(LUAFN_Deactivate); }
void CLuaUnitScript::MoveRate(int curRate) { ZoneScoped; Call(LUAFN_MoveRate, curRate); }
void CLuaUnitScript::FireWeapon(int weaponNum) { ZoneScoped; Call(LUAFN_FireWeapon, weaponNum + LUA_WEAPON_BASE_INDEX); }
void CLuaUnitScript::EndBurst(int weaponNum) { ZoneScoped; Call(LUAFN_EndBurst, weaponNum + LUA_WEAPON_BASE_INDEX); }


/******************************************************************************/
/******************************************************************************/


bool CLuaUnitScript::PushEntries(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	{
		// reset these in case we were reloaded
		activeUnit = nullptr;
		activeScript = nullptr;
	}

	CreateIKMetatables(L);

	lua_pushstring(L, "UnitScript");
	lua_createtable(L, 0, 25);

	REGISTER_LUA_CFUNC(CreateScript);
	REGISTER_LUA_CFUNC(UpdateCallIn);
	REGISTER_LUA_CFUNC(CallAsUnit);

	REGISTER_LUA_CFUNC(GetUnitValue);
	REGISTER_LUA_CFUNC(SetUnitValue);
	REGISTER_LUA_CFUNC(SetPieceVisibility);
	REGISTER_LUA_CFUNC(EmitSfx);
	REGISTER_LUA_CFUNC(AttachUnit);
	REGISTER_LUA_CFUNC(DropUnit);
	REGISTER_LUA_CFUNC(Explode);
	REGISTER_LUA_CFUNC(ShowFlare);

	REGISTER_LUA_CFUNC(Spin);
	REGISTER_LUA_CFUNC(StopSpin);
	REGISTER_LUA_CFUNC(Turn);
	REGISTER_LUA_CFUNC(Move);
	REGISTER_LUA_CFUNC(Scale);
	REGISTER_LUA_CFUNC(MultiSetPieceVisibility);
	REGISTER_LUA_CFUNC(MultiSpin);
	REGISTER_LUA_CFUNC(MultiStopSpin);
	REGISTER_LUA_CFUNC(MultiTurn);
	REGISTER_LUA_CFUNC(MultiMove);
	REGISTER_LUA_CFUNC(MultiExplode);
	REGISTER_LUA_CFUNC(MultiScale);
	REGISTER_LUA_CFUNC(IsInTurn);
	REGISTER_LUA_CFUNC(IsInMove);
	REGISTER_LUA_CFUNC(IsInSpin);
	REGISTER_LUA_CFUNC(IsInScale);
	REGISTER_LUA_CFUNC(WaitForTurn);
	REGISTER_LUA_CFUNC(WaitForMove);
	REGISTER_LUA_CFUNC(WaitForScale);

	REGISTER_LUA_CFUNC(SetDeathScriptFinished);

	REGISTER_LUA_CFUNC(GetPieceTranslation);
	REGISTER_LUA_CFUNC(GetPieceRotation);
	REGISTER_LUA_CFUNC(GetPieceScale);
	REGISTER_LUA_CFUNC(GetPiecePosDir);
	REGISTER_LUA_CFUNC(GetPieceBasePos);
	REGISTER_LUA_CFUNC(GetPieceWorldBasePos);
	REGISTER_LUA_CFUNC(GetPieceBounds);
	REGISTER_LUA_CFUNC(GetPieceParent);

	REGISTER_LUA_CFUNC(GetActiveUnitID);

	REGISTER_LUA_CFUNC(CreateIKSkeleton);

	lua_rawset(L, -3);

	// backwards compatibility
	REGISTER_LUA_CFUNC(GetUnitCOBValue);
	REGISTER_LUA_CFUNC(SetUnitCOBValue);

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Parsing helpers
//

// FIXME: this badly needs a clean up, it's duplicated

static inline CUnit* ParseRawUnit(lua_State* L, const char* caller, int index)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!lua_israwnumber(L, index))
		luaL_error(L, "%s(): Bad unitID", caller);

	CUnit* u = unitHandler.GetUnit(lua_toint(L, index));

	if (u == nullptr)
		luaL_error(L, "%s(): Bad unitID: %d", caller, lua_toint(L, index));

	return u;
}


static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	if (!CanControlUnit(L, unit))
		return nullptr;

	return unit;
}


static inline int ParseAxis(lua_State* L, const char* caller, int index)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!lua_israwnumber(L, index))
		luaL_error(L, "%s(): Bad axis", caller);

	const int axis  = lua_toint(L, index) - 1;

	if ((axis < 0) || (axis > 2))
		luaL_error(L, "%s(): Bad axis", caller);

	return axis;
}


/******************************************************************************/
/******************************************************************************/


int CLuaUnitScript::CreateScript(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	// check table of callIns
	// (we might not get a chance to clean up later on, if something is wrong)
	if (!lua_istable(L, 2))
		luaL_error(L, "%s(): error parsing callIn table", __func__);

	for (lua_pushnil(L); lua_next(L, 2) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2) || !lua_isfunction(L, -1)) {
			luaL_error(L, "%s(): error parsing callIn table", __func__);
		}
	}

	if (unit->script != &CNullUnitScript::value)
		spring::SafeDestruct(unit->script);

	// replace the unit's script (ctor parses callIn table)
	unit->script = CUnitScriptFactory::CreateLuaScript(unit, L);
	return 0;
}


int CLuaUnitScript::UpdateCallIn(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	CLuaUnitScript* script = dynamic_cast<CLuaUnitScript*>(unit->script);

	if (script == nullptr)
		luaL_error(L, "%s(): not a Lua unit script", __func__);

	// we would get confused if our refs aren't together in a single state
	if (L != script->L)
		luaL_error(L, "%s(): incorrect lua_State", __func__);

	if (!lua_israwstring(L, 2) || (!lua_isfunction(L, 3) && !lua_isnoneornil(L, 3)))
		luaL_error(L, "Incorrect arguments to %s()", __func__);

	return script->UpdateCallIn();
}


int CLuaUnitScript::CallAsUnit(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const int funcIndex = 2;

	if (!lua_isfunction(L, funcIndex))
		luaL_error(L, "Incorrect arguments to %s()", __func__);

	CUnit* oldActiveUnit = activeUnit;
	CUnitScript* oldActiveScript = activeScript;

	activeUnit = unit;
	activeScript = unit->script;

	const int error = lua_pcall(L, lua_gettop(L) - funcIndex, LUA_MULTRET, 0);

	activeUnit = oldActiveUnit;
	activeScript = oldActiveScript;

	if (error != 0)
		lua_error(L);

	return (lua_gettop(L) - funcIndex + 1);
}


// moved from LuaSyncedCtrl

int CLuaUnitScript::GetUnitValue(lua_State* L, CUnitScript* script, int arg)
{
	RECOIL_DETAILED_TRACY_ZONE;
	bool splitData = false;
	if (lua_isboolean(L, arg)) {
		splitData = lua_toboolean(L, arg);
		arg++;
	}

	const int val = luaL_checkint(L, arg); arg++;

	int p[4];
	for (int a = 0; a < 4; a++, arg++) {
		if (lua_istable(L, arg)) {
			int x, z;
			lua_rawgeti(L, arg, 1); x = luaL_checkint(L, -1); lua_pop(L, 1);
			lua_rawgeti(L, arg, 2); z = luaL_checkint(L, -1); lua_pop(L, 1);
			p[a] = PACKXZ(x, z);
		}
		else {
			p[a] = (int)luaL_optnumber(L, arg, 0);
		}
	}

	const int result = script->GetUnitVal(val, p[0], p[1], p[2], p[3]);
	if (!splitData) {
		lua_pushnumber(L, result);
		return 1;
	}
	lua_pushnumber(L, UNPACKX(result));
	lua_pushnumber(L, UNPACKZ(result));
	return 2;
}


int CLuaUnitScript::GetUnitCOBValue(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	return GetUnitValue(L, unit->script, 2);
}


int CLuaUnitScript::GetUnitValue(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeScript == nullptr)
		return 0;

	return GetUnitValue(L, activeScript, 1);
}


// moved from LuaSyncedCtrl

int CLuaUnitScript::SetUnitValue(lua_State* L, CUnitScript* script, int arg)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int args = lua_gettop(L) - arg; // number of arguments
	const int val = luaL_checkint(L, arg++);
	int param;
	if (args == 1) {
		param = lua_isboolean(L, arg) ? int(lua_toboolean(L, arg++)) : luaL_checkint(L, arg++);
	}
	else {
		const int x = luaL_checkint(L, arg++);
		const int z = luaL_checkint(L, arg++);
		param = PACKXZ(x, z);
	}
	script->SetUnitVal(val, param);
	return 0;
}


int CLuaUnitScript::SetUnitCOBValue(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	return SetUnitValue(L, unit->script, 2);
}


int CLuaUnitScript::SetUnitValue(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeScript == nullptr)
		return 0;

	return SetUnitValue(L, activeScript, 1);
}


int CLuaUnitScript::SetPieceVisibility(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// void SetVisibility(int piece, bool visible);
	if (activeScript == nullptr)
		return 0;

	// note: for Lua unit scripts it would be confusing if the unit's
	// unit->script->pieces differs from the unit->localModel->pieces.

	const int piece = luaL_checkint(L, 1) - 1;
	const bool visible = lua_toboolean(L, 2);
	activeScript->SetVisibility(piece, visible);
	return 0;
}


int CLuaUnitScript::EmitSfx(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// void EmitSfx(int type, int piece);
	if (activeScript == nullptr)
		return 0;

	// note: the arguments are reversed compared to the C++ (and COB?) function
	const int piece = luaL_checkint(L, 1) - 1;
	const int type = lua_isnumber(L, 2)? luaL_checkint(L, 2): (explGenHandler.LoadCustomGeneratorID(lua_tostring(L, 2)) | SFX_GLOBAL);

	activeScript->EmitSfx(type, piece);
	return 0;
}


int CLuaUnitScript::AttachUnit(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// void AttachUnit(int piece, int unit);
	if (activeScript == nullptr)
		return 0;

	const CUnit* transportee = ParseUnit(L, __func__, 2);

	if (transportee == nullptr)
		return 0;

	activeScript->AttachUnit(luaL_checkint(L, 1) - 1, transportee->id);
	return 0;
}


int CLuaUnitScript::DropUnit(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// void DropUnit(int unit);
	if (activeScript == nullptr)
		return 0;

	const CUnit* transportee = ParseUnit(L, __func__, 1);

	if (transportee == nullptr)
		return 0;

	activeScript->DropUnit(transportee->id);
	return 0;
}


int CLuaUnitScript::Explode(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// void Explode(int piece, int flags);
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;
	const int flags = luaL_checkint(L, 2);
	activeScript->Explode(piece, flags);
	return 0;
}


int CLuaUnitScript::ShowFlare(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// void ShowFlare(int piece);
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;
	activeScript->ShowFlare(piece);
	return 0;
}


int CLuaUnitScript::Spin(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// void Spin(int piece, int axis, int speed, int accel);
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;
	const int axis = ParseAxis(L, __func__, 2);
	const float speed = luaL_checkfloat(L, 3);
	const float accel = luaL_optfloat(L, 4, 0.0f); // accel == 0 -> start at desired speed immediately

	activeScript->Spin(piece, axis, speed, accel);
	return 0;
}


int CLuaUnitScript::StopSpin(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// void StopSpin(int piece, int axis, int decel);
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;
	const int axis = ParseAxis(L, __func__, 2);
	const float decel = luaL_optfloat(L, 3, 0.0f); // decel == 0 -> stop immediately

	activeScript->StopSpin(piece, axis, decel);
	return 0;
}


int CLuaUnitScript::Turn(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// void Turn(int piece, int axis, int speed, int destination);
	// void TurnNow(int piece, int axis, int destination);
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;
	const int axis = ParseAxis(L, __func__, 2);
	const float dest  = luaL_checkfloat(L, 3);
	const float speed = luaL_optfloat(L, 4, 0.0f); // speed == 0 -> TurnNow

	if (speed == 0.0f) {
		activeScript->TurnNow(piece, axis, dest);
	} else {
		activeScript->Turn(piece, axis, speed, dest);
	}

	return 0;
}


int CLuaUnitScript::Move(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// void Move(int piece, int axis, int speed, int destination);
	// void MoveNow(int piece, int axis, int destination);
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;
	const int axis = ParseAxis(L, __func__, 2);
	const float dest  = luaL_checkfloat(L, 3);
	const float speed = luaL_optfloat(L, 4, 0.0f); // speed == 0 -> MoveNow

	if (speed == 0.0f) {
		activeScript->MoveNow(piece, axis, dest);
	} else {
		activeScript->Move(piece, axis, speed, dest);
	}

	return 0;
}

int CLuaUnitScript::Scale(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// void Scale(int speed, int destination);
	// void ScaleNow(int destination);
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;
	const float dest = luaL_checkfloat(L, 2);
	const float speed = luaL_optfloat(L, 3, 0.0f); // speed == 0 -> ScaleNow

	if (speed == 0.0f) {
		activeScript->ScaleNow(piece, dest);
	}
	else {
		activeScript->Scale(piece, speed, dest);
	}

	return 0;
}


// Do not call with a function that returns values to lua
int CLuaUnitScript::MultiExec(lua_State *L, int (*const func)(lua_State*), const int expectedArgs) {
	RECOIL_DETAILED_TRACY_ZONE;

	int numArgs = lua_gettop(L);
	if (numArgs % expectedArgs != 0 && numArgs > 0) {
		luaL_error(L, "%s(): requires a multiple of %d arguments", __func__, expectedArgs);
		return 0;
	}
	while(numArgs > 0) {
		func(L);
		for(int x = expectedArgs; x > 0; x--) {
			lua_replace(L, x);
		}
		numArgs -= expectedArgs;
	}

	return 0;
}

int CLuaUnitScript::MultiSetPieceVisibility(lua_State* L)
{
	return MultiExec(L, &SetPieceVisibility, 2);
}

int CLuaUnitScript::MultiSpin(lua_State* L)
{
	return MultiExec(L, &Spin, 4);
}

int CLuaUnitScript::MultiStopSpin(lua_State* L)
{
	return MultiExec(L, &StopSpin, 3);
}

int CLuaUnitScript::MultiTurn(lua_State* L)
{
	return MultiExec(L, &Turn, 4);
}

int CLuaUnitScript::MultiExplode(lua_State* L)
{
	return MultiExec(L, &Explode, 2);
}

int CLuaUnitScript::MultiMove(lua_State* L)
{
	return MultiExec(L, &Move, 4);
}

int CLuaUnitScript::MultiScale(lua_State* L)
{
	return MultiExec(L, &Scale, 3);
}

int CLuaUnitScript::IsInAnimation(lua_State* L, const char* caller, AnimType type)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;
	const int axis  = ParseAxis(L, caller, 2);

	lua_pushboolean(L, activeScript->IsInAnimation(type, piece, axis));
	return 1;
}


int CLuaUnitScript::IsInTurn(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return IsInAnimation(L, __func__, ATurn);
}


int CLuaUnitScript::IsInMove(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return IsInAnimation(L, __func__, AMove);
}


int CLuaUnitScript::IsInSpin(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return IsInAnimation(L, __func__, ASpin);
}

int CLuaUnitScript::IsInScale(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;

	lua_pushboolean(L, activeScript->IsInAnimation(AScale, piece, -1));
	return 1;
}


int CLuaUnitScript::WaitForAnimation(lua_State* L, const char* caller, AnimType type)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeScript == nullptr)
		return 0;

	CLuaUnitScript* script = dynamic_cast<CLuaUnitScript*>(activeScript);

	if (script == nullptr)
		luaL_error(L, "%s(): not a Lua unit script", caller);

	const int piece = luaL_checkint(L, 1) - 1;
	const int axis  = ParseAxis(L, caller, 2);

	lua_pushboolean(L, script->NeedsWait(type, piece, axis));
	return 1;
}


int CLuaUnitScript::WaitForTurn(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return WaitForAnimation(L, __func__, ATurn);
}


int CLuaUnitScript::WaitForMove(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return WaitForAnimation(L, __func__, AMove);
}

int CLuaUnitScript::WaitForScale(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;

	if (activeScript == nullptr)
		return 0;

	CLuaUnitScript* script = dynamic_cast<CLuaUnitScript*>(activeScript);

	if (script == nullptr)
		luaL_error(L, "%s(): not a Lua unit script", __func__);

	const int piece = luaL_checkint(L, 1) - 1;

	lua_pushboolean(L, script->NeedsWait(AScale, piece, -1));
	return 1;
}


int CLuaUnitScript::SetDeathScriptFinished(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeUnit == nullptr || activeScript == nullptr)
		return 0;

	CLuaUnitScript* script = dynamic_cast<CLuaUnitScript*>(activeScript);

	if (script == nullptr || !script->inKilled)
		luaL_error(L, "%s(): not a Lua unit script or 'Killed' not called", __func__);

	activeUnit->KilledScriptFinished(luaL_optint(L, 1, -1));
	return 0;
}

/******************************************************************************/

int CLuaUnitScript::GetPieceTranslation(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeScript == nullptr)
		return 0;

	LocalModelPiece* piece = ParseLocalModelPiece(L, activeScript, __func__);
	return ToLua(L, piece->GetPosition() - piece->original->offset);
}


int CLuaUnitScript::GetPieceRotation(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeScript == nullptr)
		return 0;

	LocalModelPiece* piece = ParseLocalModelPiece(L, activeScript, __func__);
	return ToLua(L, piece->GetRotation());
}

int CLuaUnitScript::GetPieceScale(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeScript == nullptr)
		return 0;

	LocalModelPiece* piece = ParseLocalModelPiece(L, activeScript, __func__);
	lua_pushnumber(L, piece->GetScaling());
	return 1;
}


int CLuaUnitScript::GetPiecePosDir(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeScript == nullptr)
		return 0;

	LocalModelPiece* piece = ParseLocalModelPiece(L, activeScript, __func__);

	float3 pos;
	float3 dir;

	if (!piece->GetEmitDirPos(pos, dir))
		return 0;

	ToLua(L, pos);
	ToLua(L, dir);
	return 6;
}

int CLuaUnitScript::GetPieceBasePos(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeScript == nullptr || activeUnit == nullptr) return 0;
	LocalModelPiece* piece = ParseLocalModelPiece(L, activeScript, __func__);
	const float3& off = piece->original->offset;
	lua_pushnumber(L, off.x);
	lua_pushnumber(L, off.y);
	lua_pushnumber(L, off.z);
	lua_pushnumber(L, off.Length());
	return 4;
}

int CLuaUnitScript::GetPieceWorldBasePos(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeScript == nullptr || activeUnit == nullptr) return 0;
	LocalModelPiece* piece = ParseLocalModelPiece(L, activeScript, __func__);
	const float3 pos = activeUnit->GetObjectSpacePos(piece->GetAbsolutePos());
	return ToLua(L, pos);
}

int CLuaUnitScript::GetPieceBounds(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeScript == nullptr || activeUnit == nullptr) return 0;
	LocalModelPiece* piece = ParseLocalModelPiece(L, activeScript, __func__);
	const S3DModelPiece* omp = piece->original;
	lua_pushnumber(L, omp->mins.x);
	lua_pushnumber(L, omp->mins.y);
	lua_pushnumber(L, omp->mins.z);
	lua_pushnumber(L, omp->maxs.x);
	lua_pushnumber(L, omp->maxs.y);
	lua_pushnumber(L, omp->maxs.z);
	return 6;
}

int CLuaUnitScript::GetPieceParent(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeScript == nullptr)
		return 0;

	LocalModelPiece* piece = ParseLocalModelPiece(L, activeScript, __func__);

	if (piece->parent == nullptr) {
		lua_pushnil(L);
		return 1;
	}

	const int parentScript = activeScript->ModelToScript(piece->parent->GetLModelPieceIndex());
	lua_pushnumber(L, parentScript + 1);
	return 1;
}

/******************************************************************************/
/******************************************************************************/

int CLuaUnitScript::GetActiveUnitID(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeScript == nullptr)
		return 0;
	if (activeUnit == nullptr)
		return 0;

	lua_pushnumber(L, activeUnit->id);
	return 1;
}

/******************************************************************************/
/******************************************************************************/
//
// IK (Inverse Kinematics) support
//

bool CLuaUnitScript::CreateIKMetatables(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;

	// IKSkeleton metatable
	luaL_newmetatable(L, "IKSkeleton");

	HSTR_PUSH_CFUNC(L, "__gc",        Skeleton_meta_gc);
	HSTR_PUSH_CFUNC(L, "__index",     Skeleton_meta_index);
	LuaPushNamedString(L, "__metatable", "protected metatable");

		LuaPushRawNamedCFunc(L, "CreateChain",              Skeleton_CreateChain);
		LuaPushRawNamedCFunc(L, "SetJointProperties",       Skeleton_SetJointProperties);
		LuaPushRawNamedCFunc(L, "SetBallJointConstraint",    Skeleton_SetBallJointConstraint);
		LuaPushRawNamedCFunc(L, "SetHingeJointConstraint",   Skeleton_SetHingeJointConstraint);
		LuaPushRawNamedCFunc(L, "ClearJointConstraint",      Skeleton_ClearJointConstraint);
		LuaPushRawNamedCFunc(L, "GetJointBasePos",           Skeleton_GetJointBasePos);
		LuaPushRawNamedCFunc(L, "GetJointWorldBasePos",      Skeleton_GetJointWorldBasePos);
		LuaPushRawNamedCFunc(L, "GetJointBounds",            Skeleton_GetJointBounds);
		LuaPushRawNamedCFunc(L, "SolveChain",                Skeleton_SolveChain);

	lua_pop(L, 1);

	// IKChain metatable
	luaL_newmetatable(L, "IKChain");
	HSTR_PUSH_CFUNC(L, "__gc",        Chain_meta_gc);
	HSTR_PUSH_CFUNC(L, "__index",     Chain_meta_index);
	LuaPushNamedString(L, "__metatable", "protected metatable");

	LuaPushRawNamedCFunc(L, "SetGoal",          Chain_SetGoal);
	LuaPushRawNamedCFunc(L, "GetGoal",          Chain_GetGoal);
	LuaPushRawNamedCFunc(L, "SetSolver",        Chain_SetSolver);
	LuaPushRawNamedCFunc(L, "GetBoneLengths",   Chain_GetBoneLengths);
	lua_pop(L, 1);

	// IKSolution metatable
	luaL_newmetatable(L, "IKSolution");
	HSTR_PUSH_CFUNC(L, "__gc",        Solution_meta_gc);
	HSTR_PUSH_CFUNC(L, "__index",     Solution_meta_index);
	LuaPushNamedString(L, "__metatable", "protected metatable");

	LuaPushRawNamedCFunc(L, "Apply",           Solution_Apply);
	lua_pop(L, 1);

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Skeleton metatable
//

int CLuaUnitScript::Skeleton_meta_gc(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (lua_isnil(L, 1))
		return 0;

	auto skel = std::move(*static_cast<std::shared_ptr<IK::Skeleton>*>(luaL_checkudata(L, 1, "IKSkeleton")));
	skel = {};

	return 0;
}


int CLuaUnitScript::Skeleton_meta_index(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	luaL_getmetatable(L, "IKSkeleton");
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (!lua_isnil(L, -1))
		return 1;

	lua_pop(L, 1);

	return 0;
}


/******************************************************************************/
/******************************************************************************/
//
//  Chain metatable
//

int CLuaUnitScript::Chain_meta_gc(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (lua_isnil(L, 1))
		return 0;

	auto chain = std::move(*static_cast<std::shared_ptr<IK::Chain>*>(luaL_checkudata(L, 1, "IKChain")));
	chain = {};

	return 0;
}


int CLuaUnitScript::Chain_meta_index(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	luaL_getmetatable(L, "IKChain");
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (!lua_isnil(L, -1))
		return 1;

	lua_pop(L, 1);

	auto* chain = toChain(L, 1);

	if (lua_israwstring(L, 2)) {
		switch (hashString(lua_tostring(L, 2))) {
			case hashString("rootPiece"): {
				lua_pushnumber(L, chain->rID + 1);
				return 1;
			} break;
			case hashString("effectorPiece"): {
				lua_pushnumber(L, chain->eID + 1);
				return 1;
			} break;
			case hashString("weight"): {
				lua_pushnumber(L, chain->weight);
				return 1;
			} break;
			default: {
			} break;
		}
	}

	return 0;
}


/******************************************************************************/
/******************************************************************************/
//
//  IK static callout
//

/*** Creates an IK skeleton for the currently active unit.
 *
 * The skeleton mirrors the unit's piece hierarchy. Each piece becomes a joint
 * that can have optional constraints (ball-joint or hinge). The skeleton is
 * garbage-collected when the Lua reference is lost.
 *
 * Must be called inside a unit script context (e.g. within CallAsUnit).
 *
 * @function Spring.UnitScript.CreateIKSkeleton
 * @return IKSkeleton? userdata, or nil if no unit is active or the skeleton is too small.
 */
int CLuaUnitScript::CreateIKSkeleton(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (activeUnit == nullptr)
		return 0;

	try {
		auto skeleton = std::make_shared<IK::Skeleton>(*activeUnit);

		auto ud = static_cast<std::shared_ptr<IK::Skeleton>*>(lua_newuserdata(L, sizeof(std::shared_ptr<IK::Skeleton>)));
		new (ud) std::shared_ptr<IK::Skeleton>(std::move(skeleton));

		luaL_getmetatable(L, "IKSkeleton");
		lua_setmetatable(L, -2);
		LOG_L(L_DEBUG, "CreateIKSkeleton: unit=%d joints=%u", activeUnit->id, unsigned(ud->get()->GetJoints().size()));
		return 1;
	} catch (const std::exception& e) {
		LOG_L(L_ERROR, "CreateIKSkeleton: %s", e.what());
		return 0;
	}
}


/******************************************************************************/
/******************************************************************************/
//
//  Skeleton userdata callouts
//

/*** Creates an IK chain from root to effector piece.
 *
 * The chain walks the piece tree from effector up to root via parent links.
 * Both pieces must be on the same ancestor path, otherwise returns nil.
 *
 * @function IKSkeleton:CreateChain
 * @param effectorPiece number 1-based piece index of the end-effector.
 * @param rootPiece number? 1-based piece index of the chain root. Defaults to 1 (base piece).
 * @param weight number? Blend weight for the solution (0..1). Defaults to 1.0.
 * @return IKChain? userdata, or nil if the path from effector to root is invalid.
 */
int CLuaUnitScript::Skeleton_CreateChain(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* skel = toSkeleton(L, 1);

	if (activeScript == nullptr)
		luaL_error(L, "%s(): no active script", __func__);

	const int scriptEffector = luaL_checkint(L, 2) - 1;
	const int scriptRoot = luaL_optint(L, 3, 1) - 1;
	const float weight = luaL_optfloat(L, 4, 1.0f);

	const int effectorID = activeScript->ScriptToModel(scriptEffector);
	const int rootID = activeScript->ScriptToModel(scriptRoot);

	if (effectorID < 0 || rootID < 0) {
		LOG_L(L_WARNING, "Skeleton_CreateChain: bad piece mapping script(eff=%d root=%d) -> model(eff=%d root=%d)",
			scriptEffector, scriptRoot, effectorID, rootID);
		return 0;
	}

	auto chain = skel->CreateChain(static_cast<uint32_t>(effectorID), static_cast<uint32_t>(rootID), weight);
	if (!chain) {
		LOG_L(L_WARNING, "Skeleton_CreateChain: invalid chain model root=%d effector=%d (script root=%d effector=%d)",
			rootID, effectorID, scriptRoot, scriptEffector);
		return 0;
	}

	auto ud = static_cast<std::shared_ptr<IK::Chain>*>(lua_newuserdata(L, sizeof(std::shared_ptr<IK::Chain>)));
	new (ud) std::shared_ptr<IK::Chain>(std::move(chain));

	luaL_getmetatable(L, "IKChain");
	lua_setmetatable(L, -2);
	LOG_L(L_WARNING, "Skeleton_CreateChain: script(eff=%d root=%d) -> model(eff=%d root=%d) weight=%.3f joints=%u",
		scriptEffector, scriptRoot, effectorID, rootID, weight, unsigned(ud->get()->GetJoints().size()));
	return 1;
}


/*** Sets a ball-joint (cone) constraint on a joint.
 *
 * Limits the bone leaving this joint to stay within a cone of the given angle
 * around the specified axis.
 *
 * @function IKSkeleton:SetBallJointConstraint
 * @param piece number 1-based piece index.
 * @param axisX number Cone axis X component.
 * @param axisY number Cone axis Y component.
 * @param axisZ number Cone axis Z component.
 * @param angle number Half-angle of the cone in radians.
 * @return nil
 */
int CLuaUnitScript::Skeleton_SetBallJointConstraint(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* skel = toSkeleton(L, 1);

	if (activeScript == nullptr)
		luaL_error(L, "%s(): no active script", __func__);

	const int scriptPiece = luaL_checkint(L, 2) - 1;
	const int modelPiece = activeScript->ScriptToModel(scriptPiece);
	if (modelPiece < 0)
		luaL_error(L, "%s(): invalid piece %d", __func__, scriptPiece + 1);
	const uint32_t piece = static_cast<uint32_t>(modelPiece);

	IK::BallJointConstraint constraint;
	constraint.coneAxis = float3(
		luaL_checkfloat(L, 3),
		luaL_checkfloat(L, 4),
		luaL_checkfloat(L, 5)
	);
	constraint.coneAngle = luaL_checkfloat(L, 6);

	skel->SetJointConstraint(piece, constraint);
	return 0;
}


/*** Sets a hinge-joint constraint on a joint.
 *
 * Limits the bone leaving this joint to rotate only around the specified axis,
 * within the given angular range.
 *
 * @function IKSkeleton:SetHingeJointConstraint
 * @param piece number 1-based piece index.
 * @param axisX number Hinge axis X component.
 * @param axisY number Hinge axis Y component.
 * @param axisZ number Hinge axis Z component.
 * @param minAngle number Minimum angle in radians.
 * @param maxAngle number Maximum angle in radians.
 * @return nil
 */
int CLuaUnitScript::Skeleton_SetHingeJointConstraint(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* skel = toSkeleton(L, 1);

	if (activeScript == nullptr)
		luaL_error(L, "%s(): no active script", __func__);

	const int scriptPiece = luaL_checkint(L, 2) - 1;
	const int modelPiece = activeScript->ScriptToModel(scriptPiece);
	if (modelPiece < 0)
		luaL_error(L, "%s(): invalid piece %d", __func__, scriptPiece + 1);
	const uint32_t piece = static_cast<uint32_t>(modelPiece);

	IK::HingeJointConstraint constraint;
	constraint.axis = float3(
		luaL_checkfloat(L, 3),
		luaL_checkfloat(L, 4),
		luaL_checkfloat(L, 5)
	);
	constraint.minAngle = luaL_checkfloat(L, 6);
	constraint.maxAngle = luaL_checkfloat(L, 7);

	skel->SetJointConstraint(piece, constraint);
	return 0;
}


/*** Removes any constraint from a joint.
 *
 * @function IKSkeleton:ClearJointConstraint
 * @param piece number 1-based piece index.
 * @return nil
 */
int CLuaUnitScript::Skeleton_ClearJointConstraint(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* skel = toSkeleton(L, 1);

	if (activeScript == nullptr)
		luaL_error(L, "%s(): no active script", __func__);

	const int scriptPiece = luaL_checkint(L, 2) - 1;
	const int modelPiece = activeScript->ScriptToModel(scriptPiece);
	if (modelPiece < 0)
		luaL_error(L, "%s(): invalid piece %d", __func__, scriptPiece + 1);
	const uint32_t piece = static_cast<uint32_t>(modelPiece);

	skel->SetJointConstraint(piece, std::monostate{});
	return 0;
}


/*** Returns the baked offset and length of an IK joint's piece.
 *
 * @function IKSkeleton:GetJointBasePos
 * @param piece number 1-based piece index.
 * @return number offX
 * @return number offY
 * @return number offZ
 * @return number length
 */
int CLuaUnitScript::Skeleton_GetJointBasePos(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* skel = toSkeleton(L, 1);
	if (activeScript == nullptr) luaL_error(L, "%s(): no active script", __func__);
	const int scriptPiece = luaL_checkint(L, 2) - 1;
	const int modelPiece = activeScript->ScriptToModel(scriptPiece);
	if (modelPiece < 0) return 0;
	const auto& joints = skel->GetJoints();
	for (const auto& joint : joints) {
		if (joint.piece->lmodelPieceIndex == modelPiece) {
			const auto& off = joint.piece->original->offset;
			lua_pushnumber(L, off.x);
			lua_pushnumber(L, off.y);
			lua_pushnumber(L, off.z);
			lua_pushnumber(L, off.Length());
			return 4;
		}
	}
	return 0;
}


/*** Returns the world-space position of an IK joint as it currently is.
 *
 * @function IKSkeleton:GetJointWorldBasePos
 * @param piece number 1-based piece index.
 * @return number x
 * @return number y
 * @return number z
 */
int CLuaUnitScript::Skeleton_GetJointWorldBasePos(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* skel = toSkeleton(L, 1);
	if (activeScript == nullptr) luaL_error(L, "%s(): no active script", __func__);
	const int scriptPiece = luaL_checkint(L, 2) - 1;
	const int modelPiece = activeScript->ScriptToModel(scriptPiece);
	if (modelPiece < 0) return 0;
	const auto& joints = skel->GetJoints();
	for (const auto& joint : joints) {
		if (joint.piece->lmodelPieceIndex == modelPiece) {
			return ToLua(L, joint.worldPos);
		}
	}
	return 0;
}


/*** Returns the model-space bounding box for an IK joint's piece.
 *
 * @function IKSkeleton:GetJointBounds
 * @param piece number 1-based piece index.
 * @return number minX
 * @return number minY
 * @return number minZ
 * @return number maxX
 * @return number maxY
 * @return number maxZ
 */
int CLuaUnitScript::Skeleton_GetJointBounds(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* skel = toSkeleton(L, 1);
	if (activeScript == nullptr) luaL_error(L, "%s(): no active script", __func__);
	const int scriptPiece = luaL_checkint(L, 2) - 1;
	const int modelPiece = activeScript->ScriptToModel(scriptPiece);
	if (modelPiece < 0) return 0;
	const auto& joints = skel->GetJoints();
	for (const auto& joint : joints) {
		if (joint.piece->lmodelPieceIndex == modelPiece) {
			const auto* omp = joint.piece->original;
			lua_pushnumber(L, omp->mins.x);
			lua_pushnumber(L, omp->mins.y);
			lua_pushnumber(L, omp->mins.z);
			lua_pushnumber(L, omp->maxs.x);
			lua_pushnumber(L, omp->maxs.y);
			lua_pushnumber(L, omp->maxs.z);
			return 6;
		}
	}
	return 0;
}





/*** Solves a single IK chain.
 *
 * Automatically updates all joint positions before solving.
 * The chain uses its own solver (see IKChain:SetSolver).
 *
 * The returned IKSolution object provides:
 *   status: "found" | "stretching" | "failed"
 *   iterations: number
 *   joints: table of piecewise solutions
 *
 * @function IKSkeleton:SolveChain
 * @param chain IKChain The chain to solve.
 * @param maxIterations number? Maximum solver iterations. Defaults to 10.
 * @param precision number? Convergence threshold (distance). Defaults to 1.0.
 * @return IKSolution The solution object.
 */
int CLuaUnitScript::Skeleton_SolveChain(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* skel = toSkeleton(L, 1);
	auto chain = toChainShared(L, 2);

	const uint32_t maxIter = static_cast<uint32_t>(luaL_optint(L, 3, 10));
	const float precision = luaL_optfloat(L, 4, 1.0f);

	auto solution = skel->SolveChain(chain, maxIter, precision);
	return PushChainSolution(L, solution, chain);
}


/******************************************************************************/
/******************************************************************************/
//
//  Chain userdata callouts
//

/*** Sets the target goal position for the chain effector.
 *
 * The goal is in world-space coordinates.
 *
 * @function IKChain:SetGoal
 * @param x number Goal X in world space.
 * @param y number Goal Y in world space.
 * @param z number Goal Z in world space.
 * @return nil
 */
int CLuaUnitScript::Chain_SetGoal(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* chain = toChain(L, 1);

	const float x = luaL_checkfloat(L, 2);
	const float y = luaL_checkfloat(L, 3);
	const float z = luaL_checkfloat(L, 4);

	chain->SetGoal(float3(x, y, z));
	return 0;
}


/*** Returns the current goal position of the chain effector.
 *
 * @function IKChain:GetGoal
 * @return number x X component.
 * @return number y Y component.
 * @return number z Z component.
 */
int CLuaUnitScript::Chain_GetGoal(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* chain = toChain(L, 1);

	const auto& goal = chain->GetGoal();
	return ToLua(L, goal);
}


/*** Selects the IK solver algorithm for this chain.
 *
 * Each chain can use a different solver. Defaults to "fabrik".
 *
 * @function IKChain:SetSolver
 * @param name string Solver name: "fabrik" or "ccd".
 * @return nil
 */
int CLuaUnitScript::Chain_SetSolver(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* chain = toChain(L, 1);

	const char* name = luaL_checkstring(L, 2);

	switch (hashString(name)) {
		case hashString("fabrik"):
			chain->SetSolver(&IK::GetFABRIKSolver());
			break;
		case hashString("ccd"):
			chain->SetSolver(&IK::GetCCDSolver());
			break;
		default:
			luaL_error(L, "Unknown IK solver: %s (use 'fabrik' or 'ccd')", name);
			break;
	}

	return 0;
}

int CLuaUnitScript::Chain_GetBoneLengths(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* chain = toChain(L, 1);
	const auto& lengths = chain->GetBoneLengths();

	lua_createtable(L, lengths.size(), 0);
	for (size_t i = 0; i < lengths.size(); ++i) {
		lua_pushnumber(L, lengths[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}



int CLuaUnitScript::Skeleton_SetJointProperties(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* skel = toSkeleton(L, 1);

	if (activeScript == nullptr)
		luaL_error(L, "%s(): no active script", __func__);

	const int scriptPiece = luaL_checkint(L, 2) - 1;
	const bool canRotate = lua_toboolean(L, 3);
	const bool canMove = lua_toboolean(L, 4);

	const int modelPiece = activeScript->ScriptToModel(scriptPiece);
	if (modelPiece < 0)
		luaL_error(L, "%s(): invalid piece %d", __func__, scriptPiece + 1);

	skel->SetJointProperties(static_cast<uint32_t>(modelPiece), canRotate, canMove);
	return 0;
}

/******************************************************************************/
/******************************************************************************/
//
//  Solution metatable
//

int CLuaUnitScript::Solution_meta_gc(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* ud = toSolution(L, 1);
	ud->~IKSolution();
	return 0;
}


int CLuaUnitScript::Solution_meta_index(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* ud = toSolution(L, 1);
	const char* key = luaL_checkstring(L, 2);

	switch (hashString(key)) {
		case hashString("status"): {
			switch (ud->sol.solutionKind) {
				case IK::Result::FOUND:      lua_pushstring(L, "found");      break;
				case IK::Result::STRETCHING: lua_pushstring(L, "stretching"); break;
				case IK::Result::FAILED:     lua_pushstring(L, "failed");     break;
				default:                     lua_pushstring(L, "error");      break;
			}
			return 1;
		}
		case hashString("iterations"): {
			lua_pushnumber(L, ud->sol.iterations);
			return 1;
		}
		case hashString("joints"): {
			const size_t n = ud->sol.solution.size();
			lua_createtable(L, n, 0);
			for (size_t i = 0; i < n; ++i) {
				const auto& [pieceIdx, ypr] = ud->sol.solution[i];
				lua_createtable(L, 0, 4);
				lua_pushstring(L, "piece");
				lua_pushnumber(L, pieceIdx + 1);
				lua_rawset(L, -3);
				lua_pushstring(L, "rx"); lua_pushnumber(L, ypr.x); lua_rawset(L, -3);
				lua_pushstring(L, "ry"); lua_pushnumber(L, ypr.y); lua_rawset(L, -3);
				lua_pushstring(L, "rz"); lua_pushnumber(L, ypr.z); lua_rawset(L, -3);
				lua_rawseti(L, -2, i + 1);
			}
			return 1;
		}
		default: break;
	}

	luaL_getmetatable(L, "IKSolution");
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	return 1;
}


int CLuaUnitScript::Solution_Apply(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto* ud = toSolution(L, 1);
	if (ud->chain == nullptr)
		luaL_error(L, "%s(): solution chain is null", __func__);

	auto* skel = const_cast<IK::Skeleton*>(ud->chain->GetSkeleton());
	if (skel == nullptr)
		luaL_error(L, "%s(): solution chain has no skeleton", __func__);

	skel->ApplySolution(*ud->chain, ud->sol);
	return 0;
}
