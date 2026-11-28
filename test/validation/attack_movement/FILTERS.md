# Standalone friendly-filter validation

This fixture only needs the query-only filter commit. It does not register an
AttackCommandMovement callback and does not use orientation/candidate options.
Prepare an isolated directory archive with `prepare.py`, using a BAR checkout,
development maps and `--mode native`. Install `filters.lua` under the overlay's
`luarules/gadgets/test_filters.lua`, then run:

```sh
python3 run.py ENGINE OUTPUT filters.txt
```

Expected: `PASS filters=2`. A mobile and an immobile friendly must block the
current-pose query, disappear with their matching filter, remain with the
opposite filter, and still block the next query without the override.
`GetUnitWeaponTryTarget(..., {avoidFlags=mask})` returns the native boolean,
reason, visible blocker type and ID. Without an options table it retains the
legacy single boolean result. Options are synced-only and replace avoidance
flags for that query; they never modify the weapon's stored flags.

AI assistance: OpenAI Codex implemented this fixture and runner integration.
