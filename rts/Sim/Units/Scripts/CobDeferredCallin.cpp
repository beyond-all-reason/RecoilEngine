/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "CobDeferredCallin.h"
#include "CobEngine.h"

#include "System/Misc/TracyDefs.h"

CCobDeferredCallin::CCobDeferredCallin() : funcName("")
{
	memset(&luaArgs[0], 0, MAX_LUA_COB_ARGS * sizeof(luaArgs[0]));
}

CCobDeferredCallin::~CCobDeferredCallin()
{
}

CCobDeferredCallin& CCobDeferredCallin::operator = (CCobDeferredCallin&& t) {
	unit = t.unit;
	argCount = t.argCount;
	funcName = t.funcName;
	funcHash = t.funcHash;
	cobFile = t.cobFile;
	std::memcpy(luaArgs, t.luaArgs, sizeof(luaArgs));
	return *this;
}

CCobDeferredCallin& CCobDeferredCallin::operator = (const CCobDeferredCallin& t) {
	unit = t.unit;
	argCount = t.argCount;
	funcName = t.funcName;
	funcHash = t.funcHash;
	cobFile = t.cobFile;
	std::memcpy(luaArgs, t.luaArgs, sizeof(luaArgs));
	return *this;
}

void CCobDeferredCallin::Call()
{
	// unused since batching it atm
	luaRules->Cob2Lua(LuaHashString(funcName.c_str()), unit, argCount, luaArgs, false);
}
