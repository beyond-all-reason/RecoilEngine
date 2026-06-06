/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

struct lua_State;

namespace LuaTracyExtra {
	bool PushEntries(lua_State* L);

	// Registers tracy.GpuProfile, but only when the TracyLuaGpuZones config is
	// enabled — otherwise the function is absent from the Lua environment so it
	// cannot be called. Returns whether it was registered.
	bool PushGpuProfile(lua_State* L);
};
