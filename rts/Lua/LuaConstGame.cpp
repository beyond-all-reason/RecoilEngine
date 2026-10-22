/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaConstGame.h"

#include "LuaInclude.h"
#include "LuaHandle.h"
#include "LuaUtils.h"
#include "Game/GameSetup.h"
#include "Game/TraceRay.h"
#include "Map/MapDamage.h"
#include "Map/MapInfo.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Rendering/Fonts/glFont.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/ScriptMoveType.h"
#include "Sim/Units/UnitHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileSystem.h"
#include "System/StringUtil.h"
#include "Rendering/Fonts/FontHandler.h"

/******************************************************************************
 * Game constants
 * @see rts/Lua/LuaConstGame.cpp
******************************************************************************/

/***
 * Game specific information
 * @table Game
 */

bool LuaConstGame::PushEntries(lua_State* L)
{
	{
		// game, should perhaps be moved over to ConstEngine
		/*** @field Game.maxTeams integer Maximum number of teams, including the Gaia team. */
		LuaPushNamedNumber(L, "maxTeams"  , MAX_TEAMS  );
		/*** @field Game.maxPlayers integer Maximum number of players. */
		LuaPushNamedNumber(L, "maxPlayers", MAX_PLAYERS);
		/*** @field Game.gameSpeed integer Number of simulation gameframes per second */
		LuaPushNamedNumber(L, "gameSpeed" , GAME_SPEED );
		/*** @field Game.squareSize integer Divide Game.mapSizeX or Game.mapSizeZ by this to get engine's "mapDims" coordinates. The resolution of height, yard and type maps. */
		LuaPushNamedNumber(L, "squareSize", SQUARE_SIZE);
		/*** @field Game.metalMapSquareSize integer The resolution of metalmap (for use in API such as Spring.GetMetalAmount etc.) */
		LuaPushNamedNumber(L, "metalMapSquareSize", METAL_MAP_SQUARE_SIZE);
		/*** @field Game.buildSquareSize integer Size of a build grid square in elmos (`squareSize * buildGridResolution`). */
		LuaPushNamedNumber(L, "buildSquareSize", BUILD_SQUARE_SIZE);
		/*** @field Game.buildGridResolution integer Resolution of the native build placement grid, in heightmap squares. */
		LuaPushNamedNumber(L, "buildGridResolution", BUILD_GRID_RESOLUTION);
		/*** @field Game.footprintScale integer Multiplier for unit, feature and move def footprint sizes, which are expressed in "TA units". */
		LuaPushNamedNumber(L, "footprintScale", SPRING_FOOTPRINT_SCALE);
	}

	if (CGameSetup::ScriptLoaded()) {
		// game-setup
		/*** @field Game.startPosType (0|1|2|3) `0` fixed, `1` random, `2` choose in game, `3` choose before game. */
		LuaPushNamedNumber(L, "startPosType"    , gameSetup->startPosType);
		/*** @field Game.ghostedBuildings boolean */
		LuaPushNamedBool(L,   "ghostedBuildings", gameSetup->ghostedBuildings);
		/*** @field Game.demoPlayName string? Base name of the demo being played, or `nil` if not playing a demo. */
		if (gameSetup->hostDemo)
			LuaPushNamedString(L, "demoPlayName", FileSystem::GetBasename(gameSetup->demoName));
		else
			LuaPushNamedNil(L, "demoPlayName");
	}

	if (unitHandler.MaxUnits() > 0) {
		// simulation; values are meaningless prior to Game::Load
		/*** @field Game.maxUnits integer */
		LuaPushNamedNumber(L, "maxUnits", unitHandler.MaxUnits());

		// NB: not constants
		/*** @field Game.windMin number */
		LuaPushNamedNumber(L, "windMin" , envResHandler.GetMinWindStrength());
		/*** @field Game.windMax number */
		LuaPushNamedNumber(L, "windMax" , envResHandler.GetMaxWindStrength());

		// map-damage; enabled iff !mapInfo->map.notDeformable
		/*** @field Game.mapDamage boolean Whether map damage (terrain deformation) is enabled. */
		LuaPushNamedBool(L, "mapDamage", !mapDamage->Disabled());
	}

	if (readMap != nullptr) {
		// FIXME: make this available in LoadScreen (LuaIntro) already
		// requires pre-parsing the map header and filling in mapDims
		/*** @field Game.mapX integer Map size along the x axis, in map units (512 elmos). */
		LuaPushNamedNumber(L, "mapX"    , mapDims.mapx / 64);
		/*** @field Game.mapY integer Map size along the z axis, in map units (512 elmos). */
		LuaPushNamedNumber(L, "mapY"    , mapDims.mapy / 64);
		/*** @field Game.mapSizeX integer in worldspace/opengl coords. Divide by Game.squareSize to get engine's "mapDims" coordinates */
		LuaPushNamedNumber(L, "mapSizeX", mapDims.mapx * SQUARE_SIZE);
		/*** @field Game.mapSizeZ integer in worldspace/opengl coords. Divide by Game.squareSize to get engine's "mapDims" coordinates */
		LuaPushNamedNumber(L, "mapSizeZ", mapDims.mapy * SQUARE_SIZE);
	}

	if (mapInfo != nullptr) {
		// map-info
		/*** @field Game.mapName string The map's human readable name. */
		LuaPushNamedString(L, "mapName"        ,  mapInfo->map.name);
		/*** @field Game.mapDescription string The map's description. */
		LuaPushNamedString(L, "mapDescription" ,  mapInfo->map.description);
		/*** @field Game.mapHardness number */
		LuaPushNamedNumber(L, "mapHardness"    ,  mapInfo->map.hardness);
		/*** @field Game.extractorRadius number Extraction radius for mines. */
		LuaPushNamedNumber(L, "extractorRadius",  mapInfo->map.extractorRadius);
		/*** @field Game.tidal number */
		LuaPushNamedNumber(L, "tidal"          ,  mapInfo->map.tidalStrength); // NB: not constant
		/*** @field Game.waterDamage number */
		LuaPushNamedNumber(L, "waterDamage"    ,  mapInfo->water.damage);
		/*** @field Game.gravity number Gravity in elmos per second squared. */
		LuaPushNamedNumber(L, "gravity"        , -mapInfo->map.gravity * GAME_SPEED * GAME_SPEED);
	}

	if (!modInfo.filename.empty()) {
		// mod-info; values are meaningless prior to Game::Load
		/*** @field Game.gameName string The game's human readable name, without version. */
		LuaPushNamedString(L, "gameName"     , modInfo.humanName);
		/*** @field Game.gameShortName string The game's short name, without version. */
		LuaPushNamedString(L, "gameShortName", modInfo.shortName);
		/*** @field Game.gameVersion string The game's version. */
		LuaPushNamedString(L, "gameVersion"  , modInfo.version);
		/*** @field Game.gameMutator string */
		LuaPushNamedString(L, "gameMutator"  , modInfo.mutator);
		/*** @field Game.gameDesc string The game's description. */
		LuaPushNamedString(L, "gameDesc"     , modInfo.description);

		/*** @field Game.modName string The game's human readable name, including version. */
		LuaPushNamedString(L, "modName"      , modInfo.humanNameVersioned);
		/*** @field Game.modShortName string Same as `Game.gameShortName`. */
		LuaPushNamedString(L, "modShortName" , modInfo.shortName);
		/*** @field Game.modVersion string Same as `Game.gameVersion`. */
		LuaPushNamedString(L, "modVersion"   , modInfo.version);
		/*** @field Game.modMutator string Same as `Game.gameMutator`. */
		LuaPushNamedString(L, "modMutator"   , modInfo.mutator);
		/*** @field Game.modDesc string Same as `Game.gameDesc`. */
		LuaPushNamedString(L, "modDesc"      , modInfo.description);

		/*** @field Game.constructionDecay boolean Whether constructions without builders decay. */
		LuaPushNamedBool  (L, "constructionDecay"     , modInfo.constructionDecay);
		/*** @field Game.constructionDecayTime integer How long until constructions without builders start decaying. */
		LuaPushNamedNumber(L, "constructionDecayTime" , modInfo.constructionDecayTime);
		/*** @field Game.constructionDecaySpeed number How fast constructions without builders decay. */
		LuaPushNamedNumber(L, "constructionDecaySpeed", modInfo.constructionDecaySpeed);

		/*** @field Game.multiReclaim integer `0` = 1 reclaimer per feature max, otherwise unlimited. */
		LuaPushNamedNumber(L, "multiReclaim"                  , modInfo.multiReclaim);
		/*** @field Game.reclaimMethod integer `0` = gradual reclaim, `1` = all reclaimed at end, otherwise reclaim in `reclaimMethod` chunks. */
		LuaPushNamedNumber(L, "reclaimMethod"                 , modInfo.reclaimMethod);
		/*** @field Game.reclaimUnitMethod integer `0` = revert to wireframe and gradual reclaim, `1` = subtract HP and give full metal at end. */
		LuaPushNamedNumber(L, "reclaimUnitMethod"             , modInfo.reclaimUnitMethod);
		/*** @field Game.reclaimUnitEnergyCostFactor number How much energy reclaiming a unit costs. */
		LuaPushNamedNumber(L, "reclaimUnitEnergyCostFactor"   , modInfo.reclaimUnitCostFactor.energy);
		/*** @field Game.reclaimUnitEfficiency number How much metal reclaiming a unit returns. */
		LuaPushNamedNumber(L, "reclaimUnitEfficiency"         , modInfo.reclaimUnitEfficiency.metal);
		/*** @field Game.reclaimFeatureEnergyCostFactor number How much energy reclaiming a feature costs. */
		LuaPushNamedNumber(L, "reclaimFeatureEnergyCostFactor", modInfo.reclaimFeatureCostFactor.energy);
		/*** @field Game.reclaimUnitDrainHealth boolean Whether wireframe reclaim drains health. */
		LuaPushNamedBool  (L, "reclaimUnitDrainHealth"        , modInfo.reclaimUnitDrainHealth);
		/*** @field Game.reclaimAllowEnemies boolean Whether enemy units can be reclaimed. */
		LuaPushNamedBool  (L, "reclaimAllowEnemies"           , modInfo.reclaimAllowEnemies);
		/*** @field Game.reclaimAllowAllies boolean Whether allied units can be reclaimed. */
		LuaPushNamedBool  (L, "reclaimAllowAllies"            , modInfo.reclaimAllowAllies);
		/*** @field Game.repairEnergyCostFactor number How much energy repair costs. */
		LuaPushNamedNumber(L, "repairEnergyCostFactor"        , modInfo.repairCostFactor.energy);
		/*** @field Game.resurrectEnergyCostFactor number How much energy resurrection costs. */
		LuaPushNamedNumber(L, "resurrectEnergyCostFactor"     , modInfo.resurrectCostFactor.energy);
		/*** @field Game.captureEnergyCostFactor number How much energy capture costs. */
		LuaPushNamedNumber(L, "captureEnergyCostFactor"       , modInfo.captureCostFactor.energy);

		// Despite being bools, these are exposed to Lua as 0/1 for legacy reasons
		/*** @field Game.transportAir integer If `0`, every aircraft gets `cantBeTransported = true` in its unit def. */
		LuaPushNamedNumber(L, "transportAir"   , modInfo.transportAir);
		/*** @field Game.transportShip integer If `0`, every unit using a ship move def gets `cantBeTransported = true` in its unit def. */
		LuaPushNamedNumber(L, "transportShip"  , modInfo.transportShip);
		/*** @field Game.transportHover integer If `0`, every unit using a hovercraft move def gets `cantBeTransported = true` in its unit def. */
		LuaPushNamedNumber(L, "transportHover" , modInfo.transportHover);
		/*** @field Game.transportGround integer If `0`, every unit using a tank or kbot move def gets `cantBeTransported = true` in its unit def. */
		LuaPushNamedNumber(L, "transportGround", modInfo.transportGround);
		/*** @field Game.fireAtKilled integer `1` if units fire at enemies running their `Killed()` script, otherwise `0`. */
		LuaPushNamedNumber(L, "fireAtKilled"   , modInfo.fireAtKilled);
		/*** @field Game.fireAtCrashing integer `1` if units fire at crashing aircraft, otherwise `0`. */
		LuaPushNamedNumber(L, "fireAtCrashing" , modInfo.fireAtCrashing);
		/*** @field Game.requireSonarUnderWater integer `1` if underwater units are not in LOS unless also in sonar, otherwise `0`. */
		LuaPushNamedNumber(L, "requireSonarUnderWater", modInfo.requireSonarUnderWater);

		/*** @field Game.paralyzeOnMaxHealth boolean Whether paralysis depends on max health rather than current health. */
		LuaPushNamedBool  (L, "paralyzeOnMaxHealth", modInfo.paralyzeOnMaxHealth);
		/*** @field Game.paralyzeDeclineRate number Time it takes for paralysis to decay by 100%, in seconds. */
		LuaPushNamedNumber(L, "paralyzeDeclineRate", modInfo.paralyzeDeclineRate);

		/*** @field Game.allowEnginePlayerlist boolean */
		LuaPushNamedBool  (L, "allowEnginePlayerlist", modInfo.allowEnginePlayerlist);
		/*** @field Game.nativeExcessSharing boolean whether the engine handles excess resources overflow */
		LuaPushNamedBool  (L, "nativeExcessSharing", modInfo.nativeExcessSharing);
	}

	if (archiveScanner != nullptr && mapInfo != nullptr) {
		// archive checksums
		sha512::hex_digest mapHexDigest;
		sha512::hex_digest modHexDigest;
		sha512::dump_digest(archiveScanner->GetArchiveCompleteChecksumBytes(mapInfo->map.name), mapHexDigest);
		sha512::dump_digest(archiveScanner->GetArchiveCompleteChecksumBytes(modInfo.filename), modHexDigest);

		/*** @field Game.mapChecksum string SHA-512 checksum of the map archive, as hex. */
		LuaPushNamedString(L, "mapChecksum", mapHexDigest.data());
		/*** @field Game.modChecksum string SHA-512 checksum of the game archive, as hex. */
		LuaPushNamedString(L, "modChecksum", modHexDigest.data());
	}

	{
		// NB: instance is never null, but might not contain any data yet (e.g. in LuaIntro)
		const std::vector<string>& cats = CCategoryHandler::Instance()->GetCategoryNames(~0);

		/***
		 * Example: 
		 * ```lua
		 * {
		 *   ["vtol"]         = 0,  ["special"]      = 1,  ["noweapon"]     = 2,
		 *   ["notair"]       = 3,  ["notsub"]       = 4,  ["all"]          = 5,
		 *   ["weapon"]       = 6,  ["notship"]      = 7,  ["notland"]      = 8,
		 *   ["mobile"]       = 9,  ["kbot"]         = 10, ["antigator"]    = 11,
		 *   ["tank"]         = 12, ["plant"]        = 13, ["ship"]         = 14,
		 *   ["antiemg"]      = 15, ["antilaser"]    = 16, ["antiflame"]    = 17,
		 *   ["underwater"]   = 18, ["hover"]        = 19, ["phib"]         = 20,
		 *   ["constr"]       = 21, ["strategic"]    = 22, ["commander"]    = 23,
		 *   ["paral"]        = 24, ["jam"]          = 25, ["mine"]         = 26,
		 *   ["kamikaze"]     = 27, ["minelayer"]    = 28, ["notstructure"] = 29,
		 *   ["air"]          = 30
		 * }
		 * ```
		 *
		 * @field Game.springCategories table<string, integer>
		 */
		lua_pushliteral(L, "springCategories");
		lua_createtable(L, 0, cats.size());

		for (unsigned int i = 0; i < cats.size(); i++) {
			LuaPushNamedNumber(L, StringToLower(cats[i]), i);
		}

		lua_rawset(L, -3);
	}

	{
		// NB: empty for LuaIntro and LuaParser which also push ConstGame entries
		const std::vector<std::string>& typeList = damageArrayHandler.GetTypeList();

		/***
		 * Example: 
		 * ```lua
		 * {
		 *   [1]  = amphibious,   [2] = anniddm,     [3] = antibomber,
		 *   [4]  = antifighter,  [5] = antiraider,  [6] = atl,
		 *   [7]  = blackhydra,   [8] = bombers,     [9] = commanders,
		 *   [10] = crawlingbombs, ...
		 *
		 *   ["amphibious"]   = 1, ["anniddm"]    = 2, ["antibomber"] = 3
		 *   ["antifighter"]  = 4, ["antiraider"] = 5, ["atl"]        = 6
		 *   ["blackhydra"]   = 7, ["bombers"]    = 8, ["commanders"] = 9
		 *   ["crawlingbombs"]= 10, ...
		 * }
		 * ```
		 *
		 * @field Game.armorTypes table<string|integer, integer|string> (bidirectional)
		 */
		lua_pushliteral(L, "armorTypes");
		lua_createtable(L, typeList.size(), typeList.size());

		for (size_t i = 0; i < typeList.size(); i++) {
			// bidirectional map (k,v) and (v,k)
			lua_pushsstring(L, typeList[i]);
			lua_pushnumber(L, i);
			lua_rawset(L, -3);
			lua_pushsstring(L, typeList[i]);
			lua_rawseti(L, -2, i);
		}

		lua_rawset(L, -3);
	}

	{
		/***
		 * Environmental damage types. {def}IDs of environmental-damage sources.
		 * @table Game.envDamageTypes
		 */
		lua_pushliteral(L, "envDamageTypes");
		lua_createtable(L, 0, 21);
			/*** @field Game.envDamageTypes.Debris integer Piece-projectile explosion. */
			LuaPushNamedNumber(L, "Debris"           , -CSolidObject::DAMAGE_EXPLOSION_DEBRIS   );
			/*** @field Game.envDamageTypes.GroundCollision integer Ground collision. */
			LuaPushNamedNumber(L, "GroundCollision"  , -CSolidObject::DAMAGE_COLLISION_GROUND   );
			/*** @field Game.envDamageTypes.ObjectCollision integer Object collision. */
			LuaPushNamedNumber(L, "ObjectCollision"  , -CSolidObject::DAMAGE_COLLISION_OBJECT   );
			/*** @field Game.envDamageTypes.Fire integer */
			LuaPushNamedNumber(L, "Fire"             , -CSolidObject::DAMAGE_EXTSOURCE_FIRE     );
			/*** @field Game.envDamageTypes.Water integer Lava, acid, etc. */
			LuaPushNamedNumber(L, "Water"            , -CSolidObject::DAMAGE_EXTSOURCE_WATER    );
			/*** @field Game.envDamageTypes.Killed integer */
			LuaPushNamedNumber(L, "Killed"           , -CSolidObject::DAMAGE_EXTSOURCE_KILLED   );
			/*** @field Game.envDamageTypes.Crushed integer */
			LuaPushNamedNumber(L, "Crushed"          , -CSolidObject::DAMAGE_EXTSOURCE_CRUSHED  );
			/*** @field Game.envDamageTypes.AircraftCrashed integer */
			LuaPushNamedNumber(L, "AircraftCrashed"  , -CSolidObject::DAMAGE_AIRCRAFT_CRASHED   );
			/*** @field Game.envDamageTypes.Kamikaze integer */
			LuaPushNamedNumber(L, "Kamikaze"         , -CSolidObject::DAMAGE_KAMIKAZE_ACTIVATED );
			/*** @field Game.envDamageTypes.SelfD integer */
			LuaPushNamedNumber(L, "SelfD"            , -CSolidObject::DAMAGE_SELFD_EXPIRED      );
			/*** @field Game.envDamageTypes.ConstructionDecay integer */
			LuaPushNamedNumber(L, "ConstructionDecay", -CSolidObject::DAMAGE_CONSTRUCTION_DECAY );
			/*** @field Game.envDamageTypes.Reclaimed integer */
			LuaPushNamedNumber(L, "Reclaimed"        , -CSolidObject::DAMAGE_RECLAIMED          );
			/*** @field Game.envDamageTypes.TurnedIntoFeature integer */
			LuaPushNamedNumber(L, "TurnedIntoFeature", -CSolidObject::DAMAGE_TURNED_INTO_FEATURE);
			/*** @field Game.envDamageTypes.TransportKilled integer */
			LuaPushNamedNumber(L, "TransportKilled"  , -CSolidObject::DAMAGE_TRANSPORT_KILLED   );
			/*** @field Game.envDamageTypes.FactoryKilled integer */
			LuaPushNamedNumber(L, "FactoryKilled"    , -CSolidObject::DAMAGE_FACTORY_KILLED     );
			/*** @field Game.envDamageTypes.FactoryCancel integer */
			LuaPushNamedNumber(L, "FactoryCancel"    , -CSolidObject::DAMAGE_FACTORY_CANCEL     );
			/*** @field Game.envDamageTypes.UnitScript integer */
			LuaPushNamedNumber(L, "UnitScript"       , -CSolidObject::DAMAGE_UNIT_SCRIPT        );
			/*** @field Game.envDamageTypes.SetNegativeHealth integer */
			LuaPushNamedNumber(L, "SetNegativeHealth", -CSolidObject::DAMAGE_NEGATIVE_HEALTH    );
			/*** @field Game.envDamageTypes.OutOfBounds integer */
			LuaPushNamedNumber(L, "OutOfBounds"      , -CSolidObject::DAMAGE_KILLED_OOB         );
			/*** @field Game.envDamageTypes.KilledByCheat integer */
			LuaPushNamedNumber(L, "KilledByCheat"    , -CSolidObject::DAMAGE_KILLED_CHEAT       );
			/*** @field Game.envDamageTypes.KilledByLua integer The lowest index. Games can add their own types below it, e.g. `KilledByLua - 1`. */
			LuaPushNamedNumber(L, "KilledByLua"      , -CSolidObject::DAMAGE_KILLED_LUA         );
		lua_rawset(L, -3);
	}
	{
		/***
		 * Weapon avoidance and projectile collision flags.
		 * @table Game.collisionFlags
		 */
		lua_pushliteral(L, "collisionFlags");
		lua_createtable(L, 0, 9);
			/*** @field Game.collisionFlags.noEnemies integer */
			LuaPushNamedNumber(L, "noEnemies"   , Collision::NOENEMIES   );
			/*** @field Game.collisionFlags.noFriendlies integer */
			LuaPushNamedNumber(L, "noFriendlies", Collision::NOFRIENDLIES);
			/*** @field Game.collisionFlags.noFeatures integer */
			LuaPushNamedNumber(L, "noFeatures"  , Collision::NOFEATURES  );
			/*** @field Game.collisionFlags.noNeutrals integer */
			LuaPushNamedNumber(L, "noNeutrals"  , Collision::NONEUTRALS  );
			/*** @field Game.collisionFlags.noFireBases integer Ignored by raytraces; not added to `avoidFlags`. */
			LuaPushNamedNumber(L, "noFireBases" , Collision::NOFIREBASES );
			/*** @field Game.collisionFlags.noNonTargets integer Ignored by raytraces; not added to `avoidFlags`. */
			LuaPushNamedNumber(L, "noNonTargets", Collision::NONONTARGETS);
			/*** @field Game.collisionFlags.noGround integer */
			LuaPushNamedNumber(L, "noGround"    , Collision::NOGROUND    );
			/*** @field Game.collisionFlags.noCloaked integer */
			LuaPushNamedNumber(L, "noCloaked"   , Collision::NOCLOAKED   );
			/*** @field Game.collisionFlags.noUnits integer `noEnemies`, `noFriendlies` and `noNeutrals` combined. */
			LuaPushNamedNumber(L, "noUnits"     , Collision::NOUNITS     );
		lua_rawset(L, -3);
	}
	{
		/***
		 * Move def speed-modifier types.
		 * @table Game.speedModClasses
		 */
		lua_pushliteral(L, "speedModClasses");
		lua_createtable(L, 0, 5);
			/*** @field Game.speedModClasses.Tank integer */
			LuaPushNamedNumber(L, "Tank" , MoveDef::SpeedModClass::Tank );
			/*** @field Game.speedModClasses.KBot integer */
			LuaPushNamedNumber(L, "KBot" , MoveDef::SpeedModClass::KBot );
			/*** @field Game.speedModClasses.Hover integer */
			LuaPushNamedNumber(L, "Hover", MoveDef::SpeedModClass::Hover);
			/*** @field Game.speedModClasses.Boat integer Same as `Ship`. */
			LuaPushNamedNumber(L, "Boat" , MoveDef::SpeedModClass::Ship );
			/*** @field Game.speedModClasses.Ship integer */
			LuaPushNamedNumber(L, "Ship" , MoveDef::SpeedModClass::Ship );
		lua_rawset(L, -3);
	}
	{
		/***
		 * MoveCtrl script-notify types.
		 * @table Game.scriptNotifyTypes
		 */
		lua_pushliteral(L, "scriptNotifyTypes");
		lua_createtable(L, 0, 3);
			/*** @field Game.scriptNotifyTypes.HitNothing integer */
			LuaPushNamedNumber(L, "HitNothing", CScriptMoveType::HitNothing);
			/*** @field Game.scriptNotifyTypes.HitGround integer */
			LuaPushNamedNumber(L, "HitGround" , CScriptMoveType::HitGround );
			/*** @field Game.scriptNotifyTypes.HitLimit integer */
			LuaPushNamedNumber(L, "HitLimit"  , CScriptMoveType::HitLimit  );
		lua_rawset(L, -3);
	}
	{
		// inline color-codes for text fonts
		bool newIndicators = fontHandler.disableOldColorIndicators;

		/***
		 * Table containing keys that represent the color code operations during font rendering.
		 * @field Game.textColorCodes TextColorCode
		 */
		lua_pushliteral(L, "textColorCodes");
		lua_createtable(L, 0, 3);
			LuaPushNamedChar(L, "Color"          , static_cast<char>(newIndicators ? CglFont::ColorCodeIndicator : CglFont::OldColorCodeIndicator)  );
			LuaPushNamedChar(L, "ColorAndOutline", static_cast<char>(newIndicators ? CglFont::ColorCodeIndicatorEx : CglFont::OldColorCodeIndicatorEx));
			LuaPushNamedChar(L, "Reset"          , static_cast<char>(CglFont::ColorResetIndicator) );
		lua_rawset(L, -3);
	}

	return true;
}

