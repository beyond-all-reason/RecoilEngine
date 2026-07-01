---@meta

---Engine API surface, partitioned by Lua context. `Engine.Synced`,
---`Engine.Unsynced`, and `Engine.Shared` hold the context-specific function
---buckets; engine constants (`Engine.version`, etc.) live on the same
---`Engine` table.

---Functions callable from synced contexts only (gadgets). Widgets cannot
---see this table — calling `Engine.Synced.X` from a widget gets `nil` and
---crashes when the line executes.
---@class Engine.Synced
Engine.Synced = {}

---Functions callable from unsynced contexts only (widgets, gadget unsynced
---part). Synced code *can* also call these (e.g. `Spring.PlaySound`) — the
---runtime does not gate that direction. The static catch is on the
---widget→synced direction only.
---@class Engine.Unsynced
Engine.Unsynced = {}

---Functions callable from any context (synced, unsynced, both). Read-only
---utilities, math helpers, and pure functions.
---@class Engine.Shared
Engine.Shared = {}

--- Unified `Spring` table combining all synced and unsynced functions.
---
--- @deprecated Prefer `Engine.Synced`, `Engine.Unsynced`, or `Engine.Shared`
--- — the namespaced forms give accurate context-aware type checking. `Spring`
--- is retained as a runtime alias for backwards compatibility, but its
--- type is the *union* of all three sub-tables, so the static checker
--- can't distinguish "widget calls a synced function" (which crashes at
--- runtime) from a valid call. Migrate call sites to the namespaced form
--- to get the static catch.
---
--- At runtime, `Engine.Synced`, `Engine.Unsynced`, and `Engine.Shared` are
--- aliases for the same underlying `Spring` table. The split is purely a
--- type-system organization driven by lua-doc-extractor; runtime sandboxing
--- happens in the C++ Lua handlers (LuaHandleSynced, LuaUI, LuaParser)
--- which only register the appropriate function subsets per context. Note
--- the asymmetry: widgets cannot see synced functions, but synced code
--- (gadgets) *can* call unsynced functions (e.g. `Spring.PlaySound`) —
--- the runtime doesn't gate that direction.
---
--- See also: rts/Lua/LuaSpringContext.h
---@class Spring : Engine.Shared, Engine.Synced, Engine.Unsynced
Spring = {}
