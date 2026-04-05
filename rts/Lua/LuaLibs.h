/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

struct lua_State;

namespace LuaLibs {

	/** Opens the sync-safe subset of base Lua libs. */
	void OpenSynced(lua_State* L, bool registerCreg);

} // namespace LuaLibs
