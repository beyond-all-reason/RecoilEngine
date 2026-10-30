# Pregame orders and temporary-unit statistics

Build `engine-legacy` with the supported container build. Run from an isolated
write directory; do not replace an installed engine. These checks need a game
with a synced gadget that creates a controllable builder during Initialize.
For the BAR companion change, this is `game_pregame_queue.lua`.

1. With `system.allowPreGameCommands` absent or false, select the builder and
   call each of the seven `Spring.GiveOrder*` APIs from LuaUI. All must return
   false before GameStart, with no change to the engine queue.
2. Enable the modrule and repeat with distinct shifted Move commands. All must
   return true. Wait for network delivery, then read `GetUnitCommands`: all
   seven commands must appear before GameStart. Also exercise `CMD.INSERT`
   and `CMD.REMOVE`. Verify spectator and foreign-team calls remain rejected.
3. Create a unit using argument 11 of `Spring.CreateUnit` set to true and an
   otherwise identical unit with that argument omitted. Reclaim both with
   `DestroyUnit(id, false, true)`. The team's death count must increase by one.
   Check production and transfer counters similarly, and verify real units
   still consume unit-limit capacity while temporary units exist. Check kills
   with the temporary unit as both victim and attacker.
4. With the BAR companion change, queue Area Mex before starting; change
   faction and verify equivalent build IDs, positions and order. Choose a
   start position, then start with the normal countdown. Verify transfer to
   the actual commander, removal of temporary builders without tombstones,
   and zero engine/game loss statistics for that removal.
5. Repeat with multiple clients, spectators, Random, coop, team changes,
   reconnection and an order arriving at the GameStart boundary.

The local draft was exercised on Linux with BAR: all seven APIs on/off,
Area Mex, faction conversion and a 44-command handoff, zero cleanup losses and
stones, and one normal versus one temporary-unit death. The six BAR mock tests
also cover owner checks, Random translation, coop assignment and team changes.
The full matrix above (especially multiplayer, save/load, kills and transfers)
remains to be completed before removing draft status.

Queue-origin rendering: enqueue two positional commands before choosing a start
position. Set `Spring.SetUnitCommandDrawStart(unitID, false)` and verify the
unit-to-first-command segment is absent, while both icons and their connecting
segment remain. Restore true and check the origin segment returns. Exercise
both native world/minimap drawing and any game-owned minimap renderer. Move
the chosen start position twice and verify the proxy position matches it in
the first observable UI update, without a refresh-timer delay.
