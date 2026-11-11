/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

/*** @env synced, unsynced */

#include "LuaLibs.h"

#include "LuaIO.h"
#include "lib/lua/include/LuaInclude.h"

#include <initializer_list>

static void RemoveMethods(lua_State* L, const char *lib, const std::initializer_list <const char *> &methods)
{
	lua_getglobal(L, lib);
	for (const auto &method : methods) {
		lua_pushstring(L, method);
		lua_pushnil(L);
		lua_rawset(L, -3);
	}
	lua_pop(L, 1);
}

namespace LuaLibs {

	void OpenSynced(lua_State* L, bool registerCreg)
	{
		// The standard library both handles keep: base (less what each handle removes after), coroutine, math, string
		// and table. Documented as globals so the library lists them per handle; the types are Lua's own.
		/***
		 * Lua's global table.
		 * @global _G table
		 */
		/***
		 * Lua's version string.
		 * @global _VERSION string
		 */
		/***
		 * Lua's `assert`.
		 * @global assert function
		 */
		/***
		 * Lua's `error`.
		 * @global error function
		 */
		/***
		 * Lua's `getfenv`.
		 * @global getfenv function
		 */
		/***
		 * Lua's `getmetatable`.
		 * @global getmetatable function
		 */
		/***
		 * Lua's `ipairs`.
		 * @global ipairs function
		 */
		/***
		 * Lua's `load`.
		 * @global load function
		 */
		/***
		 * Lua's `loadstring`. Each handle replaces it with its own, which loads into the handle's environment.
		 * @global loadstring function
		 */
		/***
		 * Lua's `next`. The synced handle replaces it with one that walks keys in a fixed order, so synced code stays deterministic.
		 * @global next function
		 */
		/***
		 * Lua's `pairs`. The synced handle replaces it with one that walks keys in a fixed order, so synced code stays deterministic.
		 * @global pairs function
		 */
		/***
		 * Lua's `pcall`.
		 * @global pcall function
		 */
		/***
		 * Lua's `print`.
		 * @global print function
		 */
		/***
		 * Lua's `select`.
		 * @global select function
		 */
		/***
		 * Lua's `setfenv`.
		 * @global setfenv function
		 */
		/***
		 * Lua's `setmetatable`.
		 * @global setmetatable function
		 */
		/***
		 * Lua's `tonumber`.
		 * @global tonumber function
		 */
		/***
		 * Lua's `tostring`.
		 * @global tostring function
		 */
		/***
		 * Lua's `type`.
		 * @global type function
		 */
		/***
		 * Lua's `unpack`.
		 * @global unpack function
		 */
		/***
		 * Lua's `xpcall`.
		 * @global xpcall function
		 */
		/***
		 * Lua's `coroutine` library.
		 * @global coroutine coroutinelib
		 */
		/***
		 * Lua's `math` library.
		 * @global math mathlib
		 */
		/***
		 * Lua's `string` library.
		 * @global string string
		 */
		/***
		 * Lua's `table` library.
		 * @global table tablelib
		 */
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
		/***
		 * Lua's `io` library, less `io.popen`; files go through the engine's VFS rules.
		 * @global io iolib
		 * @env unsynced
		 */
		/***
		 * Lua's `os` library, less `os.exit`, `os.execute`, `os.tmpname` and `os.getenv`.
		 * @global os oslib
		 * @env unsynced
		 */
		/***
		 * Lua's `debug` library.
		 * @global debug debuglib
		 * @env unsynced
		 */
		LUA_OPEN_LIB(L, luaopen_io);
		LUA_OPEN_LIB(L, luaopen_os);
		LUA_OPEN_LIB(L, luaopen_debug);

		lua_set_fopen (L, LuaIO::fopen);
		lua_set_system(L, LuaIO::system);
		lua_set_remove(L, LuaIO::remove);
		lua_set_rename(L, LuaIO::rename);

		RemoveMethods(L, "io",
			{ "popen"
		});
		RemoveMethods(L, "os",
			{ "exit"
			, "execute"
			, "tmpname"
			, "getenv"
		});
	}

} // namespace LuaLibs