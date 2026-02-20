/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#ifndef COB_DEFERRED_CALLIN_H
#define COB_DEFERRED_CALLIN_H

#include <string>

#include "Lua/LuaRules.h"

class CUnit;

class CCobDeferredCallin
{
public:
	CCobDeferredCallin(CCobDeferredCallin&& t) { *this = std::move(t); }
	CCobDeferredCallin(const CCobDeferredCallin& t) { *this = t; }

	CCobDeferredCallin(const CUnit* unit, const LuaHashString& hs, const std::vector<int>& dataStack, const int stackStart);

	~CCobDeferredCallin() {};

	CCobDeferredCallin& operator = (CCobDeferredCallin&& t);
	CCobDeferredCallin& operator = (const CCobDeferredCallin& t);

	void Call();
public:
	const CUnit* unit;
	int luaArgs[MAX_LUA_COB_ARGS];
	int argCount;

	std::string funcName;
	uint32_t funcHash;
};

#endif // COB_DEFERRED_CALLIN_H
