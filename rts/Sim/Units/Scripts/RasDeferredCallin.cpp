/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */


#include "RasDeferredCallin.h"

#include "RasEngine.h"

#include "Lua/LuaHashString.h"
#include "Lua/LuaUI.h"

#include "System/Misc/TracyDefs.h"


CRasDeferredCallin::CRasDeferredCallin(const CUnit* unit, const LuaHashString& hs, const std::vector<int>& dataStack, const int stackStart)
	: unit(unit), argCount(std::min(stackStart, MAX_LUA_COB_ARGS)), funcName(hs.GetString()), funcHash(hs.GetHash())
{
	const int size = static_cast<int>(dataStack.size());

	const int start = std::max(0, size - stackStart);
	const int end = std::min(size, start + argCount);

	for (int a = 0, i = start; i < end; i++) {
		luaArgs[a++] = dataStack[i];
	}
}

CRasDeferredCallin& CRasDeferredCallin::operator = (CRasDeferredCallin&& t) {
	unit = t.unit;
	argCount = t.argCount;
	funcName = t.funcName;
	funcHash = t.funcHash;
	std::memcpy(luaArgs, t.luaArgs, sizeof(int)*argCount);
	return *this;
}


CRasDeferredCallin& CRasDeferredCallin::operator = (const CRasDeferredCallin& t) {
	unit = t.unit;
	argCount = t.argCount;
	funcName = t.funcName;
	funcHash = t.funcHash;
	std::memcpy(luaArgs, t.luaArgs, sizeof(int)*argCount);
	return *this;
}


void CRasDeferredCallin::Call()
{
	// unused since batching it atm
	luaRules->unsyncedLuaHandle.Cob2Lua(LuaHashString(funcName.c_str()), unit, argCount, luaArgs);
	if (luaUI)
		luaUI->Cob2Lua(LuaHashString(funcName.c_str()), unit, argCount, luaArgs);
}
