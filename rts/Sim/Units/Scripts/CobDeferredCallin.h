/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COB_DEFERRED_CALLIN_H
#define COB_DEFERRED_CALLIN_H

#include <string>
#include <array>

#include "CobInstance.h"
#include "Lua/LuaRules.h"

class CCobFile;
class CCobInstance;
class CUnit;

class CCobDeferredCallin
{
public:
	CCobDeferredCallin();
	CCobDeferredCallin(CCobDeferredCallin&& t) { *this = std::move(t); }
	CCobDeferredCallin(const CCobDeferredCallin& t) { *this = t; }

	~CCobDeferredCallin();

	CCobDeferredCallin& operator = (CCobDeferredCallin&& t);
	CCobDeferredCallin& operator = (const CCobDeferredCallin& t);

	void Call();
public: // protected:
	CUnit* unit;
	int luaArgs[MAX_LUA_COB_ARGS] = {0};
	int argCount;
	int scriptIndex;
	CCobFile* cobFile = nullptr;
	std::string funcName;
	uint32_t funcHash;
};

#endif // COB_DEFERRED_CALLIN_H
