# Factory completion callbacks

Regression for [BAR #9966](https://github.com/beyond-all-reason/Beyond-All-Reason/issues/9966):
`UnitFinished` can destroy a factory and its newly finished unit while
`CFactory::UpdateBuild` is still executing. The factory must neither emit a
nano particle using the invalid build nor call `UnitFromFactory` for it.

Requires Python 3.11+, a headless engine, and development data containing
Quicksilver Remake 1.24 and the BAR Rapid package named below. Each run creates
an isolated write directory and a small game overlay; it removes that overlay
afterwards and preserves the logs and `result.json`. The BAR team-death gadget
is unchanged, so these tests do not depend on the Lua workaround.

```sh
python3 test/validation/factory_build_callbacks/run.py \
  build-amd64-linux/spring-headless /tmp/factory-normal \
  --assets /path/to/development/data \
  --base /path/to/engine/base \
  --game 'Beyond All Reason test-31691-a617033' --case normal
```

Repeat with fresh output directories and `--case buildee`, `--case factory`,
`--case both`, and `--case cancel` (remove the active build order during the
callback). The normal case must deliver exactly one `UnitFromFactory`;
the other cases must deliver none and survive ten further frames with
the expected survivors. Run the same cases on an unmodified engine first.

For manual verification, run the generated `input.txt` with a graphical engine
after recreating the overlay (the runner accepts that engine too). The test
spawns a Cortex bot lab at (1800, 1800), orders a fast AK build, and destroys
the selected units directly inside `UnitFinished`. A successful run logs
`[FactoryCallback] PASS` and exits normally.

Implementation and test preparation were assisted by OpenAI Codex.
