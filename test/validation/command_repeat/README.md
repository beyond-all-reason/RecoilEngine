# Command repeat validation

This headless BAR scenario checks command completion and queue contents through
the native command AIs. It takes about ten simulation seconds. OpenAI Codex
authored the fixture and runner.

Use a **disposable copy of BAR**: the runner installs `check.lua` as a gadget and
enables `canrepeat` on `armllt` to exercise the stationary `CCommandAI` path.
Do not pass your normal game checkout or installed game. The other subjects use
unmodified BAR unit definitions. Run outputs and caches go into a fresh isolated
directory, and the map archive is only linked there.

```sh
cp -a --reflink=auto /path/to/Beyond-All-Reason /tmp/repeat-test-game
python3 test/validation/command_repeat/run.py \
  --engine /path/to/engine/spring-headless \
  --game /tmp/repeat-test-game \
  --map /path/to/supreme_isthmus_v2.1.sd7 \
  --output /tmp/repeat-test-fixed
```

The engine directory must contain its matching `base/` archives. To reproduce
the bug on unmodified master, use its binary, a fresh output directory, and add
`--expect-baseline`. Use the same game copy and map for both runs.

Assertions cover:

- Self-attack with Repeat on: stationary, mobile, aircraft and builder AIs each
  finish once and leave an empty queue; baseline repeatedly finishes/requeues.
- A live aircraft target enters the native crashing state. Its attacker finishes
  once; baseline repeats the attack. Holding the target aloft isolates completion
  from target deletion and BAR's death/neutralization logic.
- Lost sensor contact and a transported target remain repeatable.
- Empty-area repair remains repeatable, while persistent Alt-area repair remains
  queued without completion callbacks.
- A self-attack before a valid attack is removed without removing the successor.
- Repeat off still finishes a self-attack exactly once.

Subjects cannot fire during the observation, but their command AIs and movement
remain native. This checks deterministic behavior, not performance or same-frame
queue draining. Inspect `command-repeat.csv` and `console.log` in the output
directory. A graphical engine can also run the generated `startscript.txt` with
the same `--isolation --write-dir` arguments for manual inspection.

## Recorded validation

Tested against engine master `e117c05a78222bf26b4984128c50535af38a28b0`, using
BAR `da09265374ba43e0579a78c500de1c440c7583e1` and Supreme Isthmus v2.1.
All 14 cases passed their baseline and modified-build expectations. Self-attacks
completed 33–34 times on baseline versus once with the fix; the crashing-target
attack completed 14 times versus once. The repeat controls had identical counts
and queue lengths across builds.

Re-run after rebasing onto master `81f2e5509d5e7b7045b7b3c4fe03b0d674a99c51`
(which includes #3348), same BAR commit and map: the modified build passed all
14 cases again. The `crashing` case exercises the aircraft attacker; the
crashing-target exits #3348 added for stationary and ground units pass the same
`dontRepeat` option but are not covered by a dedicated case.

The Linux Podman headless build passed. The general `check` target passed 27/29
tests initially. UnitSync then passed with `SPRING_DATADIR=/build/out` pointing
at the isolated build's base archives. CREG reports the same 37 missing-member
defects on both the unmodified and modified engine.
