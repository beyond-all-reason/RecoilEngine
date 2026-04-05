/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "LuaLibs.h"

#include "lib/lua/include/LuaInclude.h"

namespace LuaLibs {

	void OpenSynced(lua_State* L, bool registerCreg)
	{
		if (registerCreg) {
			SPRING_LUA_OPEN_LIB(L, luaopen_base);
			SPRING_LUA_OPEN_LIB(L, luaopen_math);
			SPRING_LUA_OPEN_LIB(L, luaopen_table);
			SPRING_LUA_OPEN_LIB(L, luaopen_string);
		} else {
			LUA_OPEN_LIB(L, luaopen_base);
			LUA_OPEN_LIB(L, luaopen_math);
			LUA_OPEN_LIB(L, luaopen_table);
			LUA_OPEN_LIB(L, luaopen_string);
		}
	}

	void OpenUnsynced(lua_State* L)
	{
		OpenSynced(L, false);
		LUA_OPEN_LIB(L, luaopen_io);
		LUA_OPEN_LIB(L, luaopen_os);
		LUA_OPEN_LIB(L, luaopen_debug);
	}

} // namespace LuaLibs