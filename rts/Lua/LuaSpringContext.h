/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_SPRING_CONTEXT_H
#define LUA_SPRING_CONTEXT_H

#include "LuaInclude.h"

// ─────────────────────────────────────────────────────────────────────────────
// Engine API context — split-tables-as-primary model
//
// The engine API is partitioned into three disjoint sub-tables of the `Engine`
// namespace, with each binding declaring its bucket explicitly via its
// `@function` table prefix:
//
//   * Engine.Shared   — reads valid in both contexts (e.g. `LuaSyncedRead`)
//   * Engine.Synced   — synced-only control           (e.g. `LuaSyncedCtrl`)
//   * Engine.Unsynced — unsynced-only                 (e.g. `LuaUnsynced{Ctrl,Read}`)
//
// Each handler (CSplitLuaHandle, LuaUI, LuaIntro, LuaMenu, LuaParser) pushes
// entries directly into the correct bucket via
// `AddEntriesToTable(L, "Engine", "<Bucket>", <PushEntries>)`. After all pushes
// are done, `BuildSpringFromSplitTables` merges the per-context-appropriate
// subset of the three buckets into `Spring` as a back-compat view so pre-split
// callers still see `Spring.X`. Engine constants (`Engine.version`, etc.) share
// the same `Engine` table alongside the buckets.
//
// Per-context exposure (matches the function set each handler registers):
//
//   Context::Synced   merges  Engine.Shared + Engine.Synced       into Spring
//   Context::Unsynced merges  Engine.Shared + Engine.Unsynced     into Spring
//   Context::Both     merges  Engine.Shared + Engine.Synced +
//                             Engine.Unsynced                    into Spring
//
// `Both` is for the synced half of `CSyncedLuaHandle`, which registers BOTH
// `LuaSyncedCtrl::PushEntries` AND `LuaUnsyncedCtrl::PushEntries` (synced
// gadgets are allowed to call unsynced control functions). Everywhere else,
// the merge set matches the push set; out-of-context accesses fail at runtime
// with "attempt to index a nil value" — the same error EmmyLua reports
// statically.
//
// On key collisions across split tables, `BuildSpringFromSplitTables` emits a
// warning via `ILog` and keeps the first-inserted value. Collisions indicate
// an engine authoring bug — a function should live in exactly one bucket.
//
// Why split-as-primary instead of alias-to-Spring:
//   * Runtime structure matches annotation structure — `Engine.Synced.X` is
//     only defined if X is actually synced-bucket; out-of-bucket access is a
//     real nil-deref at runtime, not just a static type-check failure.
//   * Future engine additions (new `REGISTER_LUA_CFUNC` in `LuaSyncedCtrl.cpp`)
//     are classified by the `@function Engine.<Bucket>.Name` prefix on their doc
//     block — one source of truth, no hand-mapping, no drift possible.
//
// Cross-references:
//   * type stubs:    rts/Lua/library/Spring.lua
//   * doc generator: doc/site/docgen/generator.rb
//   * BAR codemod:   BAR-Devtools/bar-lua-codemod/src/spring_split.rs
// ─────────────────────────────────────────────────────────────────────────────

namespace LuaSpringContext {
	enum class Context { Synced, Unsynced, Both };

	// Creates a `Spring` table by eagerly merging the appropriate subset of
	// `Engine.Shared` / `Engine.Synced` / `Engine.Unsynced` (populated by prior
	// `AddEntriesToTable` calls). Call AFTER all split-table pushes.
	// Warns on key collisions across the merged tables.
	void BuildSpringFromSplitTables(lua_State* L, Context ctx);
}

#endif // LUA_SPRING_CONTEXT_H
