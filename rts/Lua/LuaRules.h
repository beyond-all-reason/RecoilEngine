/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_RULES_H
#define LUA_RULES_H

#include <string>
#include <vector>

#include "LuaHandleSynced.h"
#include "System/UnorderedMap.hpp"


class CUnit;
class CFeature;
class CProjectile;
class CWeapon;
struct UnitDef;
struct FeatureDef;
struct Command;
struct BuildInfo;
struct lua_State;


class CLuaRules : public CSplitLuaHandle
{
public:
	static bool CanLoadHandler() { return true; }
	static bool ReloadHandler() { return (FreeHandler(), LoadFreeHandler()); } // NOTE the ','
	static bool LoadFreeHandler(bool dryRun = false) { return (LoadHandler(dryRun) || FreeHandler()); }

	static bool LoadHandler(bool dryRun);
	static bool FreeHandler();

public: // call-ins
	const char* RecvSkirmishAIMessage(int aiID, const char* data, int inSize, size_t* outSize) {
		return syncedLuaHandle.RecvSkirmishAIMessage(aiID, data, inSize, outSize);
	}


private:
	CLuaRules(bool dryRun);
	virtual ~CLuaRules();

protected:
	bool AddSyncedCode(lua_State* L);
	bool AddUnsyncedCode(lua_State* L);

	std::string GetUnsyncedFileName() const;
	std::string GetSyncedFileName() const;
	std::string GetInitFileModes() const;
	int GetInitSelectTeam() const;

protected: // call-outs
	static int PermitHelperAIs(lua_State* L);
};


extern CLuaRules* luaRules;


#endif /* LUA_RULES_H */
