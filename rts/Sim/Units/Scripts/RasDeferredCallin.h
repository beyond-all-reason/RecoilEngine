/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#ifndef RAS_DEFERRED_CALLIN_H
#define RAS_DEFERRED_CALLIN_H

#include <string>

#include "Lua/LuaRules.h"

class CUnit;

class CRasDeferredCallin
{
public:
	CRasDeferredCallin(CRasDeferredCallin&& t) { *this = std::move(t); }
	CRasDeferredCallin(const CRasDeferredCallin& t) { *this = t; }

	CRasDeferredCallin(const CUnit* unit, const LuaHashString& hs, const std::vector<int>& dataStack, const int stackStart);

	~CRasDeferredCallin() {};

	CRasDeferredCallin& operator = (CRasDeferredCallin&& t);
	CRasDeferredCallin& operator = (const CRasDeferredCallin& t);

	void Call();
public:
	const CUnit* unit;
	int luaArgs[MAX_LUA_COB_ARGS];
	int argCount;

	std::string funcName;
	uint32_t funcHash;
};

#endif // RAS_DEFERRED_CALLIN_H
