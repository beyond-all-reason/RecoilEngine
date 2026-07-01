/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaSpringContext.h"

#include "System/Log/ILog.h"

namespace LuaSpringContext {

namespace {
	// Merge every key from the `Engine.<bucket>` sub-table into global
	// `Spring`. Warn on collisions (key already in `Spring` from an earlier
	// merge). First merge wins; later merges ignore colliding keys to keep
	// the warning actionable rather than masking.
	void MergeIntoSpring(lua_State* L, const char* bucket) {
		lua_getglobal(L, "Engine");
		if (!lua_istable(L, -1)) {
			lua_pop(L, 1);
			return;
		}
		lua_getfield(L, -1, bucket); // Engine[bucket]
		lua_remove(L, -2);           // drop Engine, leave the bucket table
		if (!lua_istable(L, -1)) {
			// Bucket doesn't exist — handler didn't push anything into it for
			// this fenv (e.g. Engine.Synced in unsynced context). Silently
			// skip; caller controls which buckets merge via the Context arg.
			lua_pop(L, 1);
			return;
		}
		lua_getglobal(L, "Spring");
		// Stack: [..., src, spring]

		lua_pushnil(L); // first key
		while (lua_next(L, -3) != 0) {
			// Stack: [..., src, spring, key, val]
			lua_pushvalue(L, -2); // dup key
			lua_rawget(L, -4);    // spring[key]
			// Stack: [..., src, spring, key, val, spring[key]]
			const bool collision = !lua_isnil(L, -1);
			lua_pop(L, 1);
			// Stack: [..., src, spring, key, val]

			if (collision) {
				const char* keyStr = (lua_type(L, -2) == LUA_TSTRING)
					? lua_tostring(L, -2)
					: "<non-string>";
				LOG_L(L_WARNING,
					"[LuaSpringContext] Split-table collision: '%s' present in multiple of "
					"Engine.Shared/Engine.Synced/Engine.Unsynced; keeping first-merged value. "
					"Re-check the @function bucket declaration at the source (Engine.%s defines it).",
					keyStr, bucket);
				lua_pop(L, 1); // pop val; leave key for next lua_next
			} else {
				lua_pushvalue(L, -2); // dup key
				lua_insert(L, -2);    // move key below val
				// Stack: [..., src, spring, key, key, val]
				lua_rawset(L, -4);    // spring[key] = val
				// Stack: [..., src, spring, key]
			}
		}
		// Stack: [..., src, spring]
		lua_pop(L, 2);
	}
}

void BuildSpringFromSplitTables(lua_State* L, Context ctx) {
	// Create empty `Spring` (overwriting anything prior — no handler
	// registers into `Spring` directly under the split-tables-as-primary
	// model).
	lua_newtable(L);
	lua_setglobal(L, "Spring");

	// Always merge Shared first so it takes priority on any incidental
	// collision — a function listed in both Shared and a context-specific
	// bucket is almost certainly meant to be shared.
	MergeIntoSpring(L, "Shared");
	if (ctx == Context::Synced || ctx == Context::Both)
		MergeIntoSpring(L, "Synced");
	if (ctx == Context::Unsynced || ctx == Context::Both)
		MergeIntoSpring(L, "Unsynced");
}

} // namespace LuaSpringContext
