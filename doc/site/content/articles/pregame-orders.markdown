+++
title = "Pregame command queues"
+++

Games can opt into LuaUI orders before simulation starts by setting
`system.allowPreGameCommands = true` in `gamedata/modrules.lua`. The default is
`false`. Lua exposes the effective value as `Game.allowPreGameCommands`.

The option enables the normal `Spring.GiveOrder*` family during pregame.
Commands still travel through the network and normal permission checks;
spectator, team control, helper-AI and unit selection restrictions still apply.
Reading the queue immediately after submitting an order may therefore return
its previous state. The option applies to all controllable units, not a
particular unit definition or rules parameter. Games can further restrict
orders with `AllowCommand`.

A synced gadget can create temporary builders before GameStart so ordinary
widgets can use their build options and engine queues. The game remains
responsible for their positions, ownership, faction changes and transfer of
queued commands to the actual starting units. Enabling this option does not
spawn units or execute the simulation before GameStart.

`Spring.CreateUnit` accepts an optional eleventh argument, `skipUnitStats`
(default `false`). When true, the unit is excluded from team unit counters for
production, losses, kills and transfers. Live unit counts and limits still
include it. Lifecycle callins, resource statistics and damage statistics are
unchanged, so game-side statistics may need separate filtering. Destroying such
a unit does not automatically suppress wreckage or game-side death effects;
these remain the game's responsibility.

`Spring.SetUnitCommandDrawStart(unitID, drawStart)` controls the first queue
line, from the unit to its first command. It defaults to true and is local to
the client. Setting it to false retains command icons and all subsequent
connections, allowing a game to show a queue before its start position exists.
