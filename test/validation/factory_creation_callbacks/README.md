# Factory creation callback lifetime

Regression for an orphan nanoframe when `UnitCreated` kills its factory before
`CFactory::StartBuild` stores the new unit as `curBuild`. The test overlays BAR
in an isolated directory and disables `unit_prevent_lab_hax2.lua`. It does not
change any installed game or engine.

Run with a headless engine, development assets, and engine base archives:

```sh
python3 test/validation/factory_creation_callbacks/run.py \
  build-amd64-linux/spring-headless /tmp/factory-creation-test \
  --assets /path/to/development/data --base build-amd64-linux/base \
  --case lua-created
```

The `lua-created` case destroys the factory inside its buildee's `UnitCreated`
callback. It must leave no live buildee. `alt-created` inserts a different unit
at the head of the factory queue in that callback and verifies that the
cancelled first buildee does not survive. `normal` checks an ordinary build.
Use a fresh output path for each run. The runner removes its game overlay and
keeps logs and `result.json`.
