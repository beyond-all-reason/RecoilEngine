# Commands-targeting API validation

Build `engine-headless` and `basecontent`, then run:

```sh
python3 test/validation/commands_targeting/run.py \
  build-amd64-linux/spring-headless /path/to/quicksilver_remake_1.24.sd7
```

The runner creates an isolated temporary directory and retains the logs there.
It uses a minimal test game, the supplied development map, and the engine's base
content. It does not use or modify an installed game. Another map can be supplied
with `--map-name 'Map Name'`.

The fixture covers `Spring.GetCommandsTargeting`, `Spring.ClearCommandsTargeting`
and `Spring.ClearTargetingFromUnit`:

- Full-control LuaRules and `UnitScript.CallAsUnit` access for the two clearing
  callouts; rejection without control, with control of the attacker team, and
  with control of the target team. Denied calls must leave targets and queues
  unchanged. Unsynced Lua must not expose the clearing callouts but must expose
  the getter.
- The getter with a single command ID and with a list, front orders only and
  with `queued`, checked against `Spring.GetUnitCommands` for every unit, plus
  restricted reads via `CallAsTeam` that must only report that team's units.
- Active A → B → C attacks for stationary, mobile, and strafing-aircraft command
  AIs, plus a duplicate queued attack on A. B must start after clearing A and
  remain active when A is destroyed. The returned removal count must match the
  number of matching orders.
- Queued-only ATTACK, unit-targeted FIGHT and MANUALFIRE orders; unrelated command
  tags/order and ground attacks must be preserved. A factory's orders for new
  units are cleared while its own build queue is untouched.
- A REPAIR order that only the single-ID call removes; the builder must stop
  repairing like it does after `CMD.REMOVE`, which is run on a second builder
  as the reference.
- Independent per-weapon targets, weapon-only targets and automatic targets
  survive command removal and are dropped by `ClearTargetingFromUnit`; a finished
  attack order releases its own target. Other weapon targets must survive.
- Invalid unit IDs, malformed command lists, empty lists, repeated calls,
  unchanged neutrality, later automatic reacquisition, and subsequent explicit
  attack orders.

Targets are kept alive by blocking shots. MoveCtrl fixes positions except for the
fighter, whose command AI requires its normal movement controller. This does not
test projectile cancellation (the API deliberately leaves existing projectiles
alone) or assess flight paths.
Success requires `COMMANDS_TARGETING PASS`, `COMMANDS_TARGETING UNSYNCED PASS`,
and a successful runner exit. An engine without the callouts must fail with
`missing ... API`.

For manual review, run the same runner with a graphical `spring` build and inspect
the printed checks in the retained `console.log`. For a gameplay check, add the
callouts to an isolated game's synced gadget, queue A → B → C, clear A, and
verify that B is attacked without changing A's neutral state. Do not install the
fixture into a normal game directory.

OpenAI Codex assisted the original implementation, test fixture, and
documentation. Automated simulation checks do not constitute human gameplay
verification.
