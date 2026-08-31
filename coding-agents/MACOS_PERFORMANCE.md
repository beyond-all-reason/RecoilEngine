# macOS renderer performance

Reference, not a narrative. Read the summary, then jump to what you need.

Last updated 2026-08-26. Companion to `MACOS_RENDERER_PLAN.md`, which holds the upstreaming decomposition and the raw investigation history.

## Contents

- [Summary](#summary) - the whole picture in one table
- [Current understanding](#current-understanding) - where the frame time goes
- [Settled: do not reopen](#settled-do-not-reopen) - closed questions and why
- [Open work, ranked](#open-work-ranked) - what is actually worth doing
- [Measurement rules](#measurement-rules) - read before running anything
- [Harnesses and traps](#harnesses-and-traps) - how to run a benchmark that means something
- [Evidence](#evidence) - the numbers behind the claims

---

## Summary

| Question | Answer | Confidence |
|---|---|---|
| Where does the frame go? | The game's LuaUI, 74% to 86% | Measured, 2 games |
| Why is LuaUI slow? | Lua drawing is real immediate mode, `glVertex4f` per vertex | Measured, 790,748 verts/frame |
| Is the present path the problem? | No, 3% | Measured twice |
| Is render scaling a way out? | No, flat 38ms penalty for any non-1:1 present | 6 runs |
| Are render pass breaks the problem? | Probably not. Model demoted | `rp` found nothing |
| Is it the engine's fault? | Mostly no. Empty Mod runs at 80 fps | Measured |
| What is the one big engine-side fix? | Make `LuaOpenGL` stop using `glBegin` | Started 2026-08-26 |
| Would a buffered draw hit the same driver defect? | No. `glDrawArrays` is clean over 60,000 pairs | Measured, with a control |

**Headline numbers**, Splinter Faction, 3024x1832, built-in display:

| Configuration | fps | frame |
|---|---|---|
| Empty Mod, same map | 80 | 12.5ms |
| Splinter Faction, LuaUI disabled | 59.6 | 16.8ms |
| Splinter Faction, as shipped | 15.9 | 62.9ms |

---

## Current understanding

- **LuaUI is the frame.** 74% on Splinter Faction, 86% on Metal Factions, measured on a scene with one unit. Two unrelated games with unrelated widget sets agree.
- **The mechanism is immediate mode.** All Lua drawing goes through `glVertex4f` one vertex at a time (`LuaOpenGL.cpp:313`, `myGL.cpp:573`). 790,748 Lua vertices a frame on Metal Factions.
- **The renderer's own floor is fine.** 2.3ms a Mpixel with no game content. The often-quoted 11.3ms a Mpixel is Splinter Faction's scene, not a property of this port.
- **The driver has a real defect on top.** KosmicKrisp mis-renders consecutive immediate-mode batches. The engine works around it with uniform vertex and texture coordinate arity, a per-batch `glFlush`, and display list deferral. That costs 1.87x and is deliberate.
- **The workarounds cannot cover everything.** 47 widgets compile immediate mode into display lists, where `glFlush` is never recorded and nothing at replay can reach the baked batches.

Everything above points at one fix, and the plan reaches it twice independently: `LuaOpenGL` should not use `glBegin` at all.

---

## Settled: do not reopen

Each of these was measured. Reopening one needs new information, not a new idea.

| Topic | Verdict | Evidence |
|---|---|---|
| Present path | 3%, not the problem | `PLAN:1096`, `:1137` |
| Render scaling, `MacRenderScale` | Dead end, flat 38ms non-1:1 penalty | `PLAN:1112`-`:1180` |
| `MacHiDPIRendering = 0` | Same dead end, different route | `PLAN:1112` |
| Raw fill rate | Driver is 1000x faster than we achieve | `PLAN:1098` |
| CPU-side drawing | CPU is idle in a draw-bound frame | `PLAN:1344` |
| Engine feature sweep | 17.0 vs 15.9 fps, inside drift | `PLAN:1330` |
| 60Hz display as a ceiling | It is not | `PLAN:1225` |
| Buffering, 1 vs 2 IOSurfaces | 15.9 vs 16.1 fps | `ab-doublebuf-*.log` |
| `ZINK_DEBUG=rp` renderpass tracking | No signal, p = 0.167 | Below, 2026-08-25 |
| Compiled display lists | Cannot ship, build menu destroyed | `PLAN:1299` |
| `ForceImmediateModeFlush = 0` | Not a lever. It is the price of correct output | Below, 2026-08-25 |
| `normalise` as a default | No. Reintroduces merging on top of the flush | `PLAN:1261` |

### The two that keep getting reopened

**`ForceImmediateModeFlush = 0` is not a performance option.** It is a capability probe override. Setting it to 0 asserts the driver renders batches correctly, which is false on KosmicKrisp. It turns off five things at once:

| Site | Mitigation | Defect it fixes |
|---|---|---|
| `LuaOpenGL.cpp:319` | vertex arity widening | load screen corruption |
| `LuaOpenGL.cpp:273` | texcoord arity widening | resource bar |
| `LuaOpenGL.cpp:295` | multi-texcoord arity widening | same family |
| `myGL.cpp:570` | per-batch `glFlush` | consecutive batches merging |
| `LuaOpenGL.cpp:6217` | display list deferral | flush cannot be recorded into a list |

Worth 1.87x and 1.65 GB. Also reverts every artefact this project has fixed.

**Compiled display lists cannot ship**, tested in isolation with arity and flush still on:

| Configuration | Frames with build menu content destroyed |
|---|---|
| compiled, `-` | 15 of 35, and 5 of 36 |
| compiled, `normalise` | 6 of 36 |
| compiled, `listflush` | 10 of 37 |
| **deferred, what ships** | **0 of 37** |

Only visible while the menu scrolls, because the widget recompiles on each scroll step. A frozen scene reads as clean.

---

## Open work, ranked

### 1. Make `LuaOpenGL` stop using `glBegin`

**Started 2026-08-26.** The plan concludes it twice (`PLAN:621`, `myGL.cpp:567`) and nobody had acted.

Buffer each `gl.BeginEnd` block and replay it as one `glDrawArrays`, fed by `glVertexPointer` and friends the way `rts/Rendering/GL/VertexArray.cpp` already does. Widgets observe no change.

Vertex specification stays fixed-function, which is the point rather than a compromise. It is the only way to preserve `glTexEnv`, `GL_LIGHTING`, fog, and the `gl_Vertex` and `gl_MultiTexCoord0` built-ins that `gl.UseShader` widgets read. A `RenderBuffers` port cannot feed generic attribute data to `gl_MultiTexCoord0`, so it would break existing content, which defeats the reason for doing this at all.

What it buys:

- Every existing widget in every existing game, with no game-side change and no upstreaming to N games
- Retires all three mitigations, plus the probe. No immediate-mode batches means nothing to merge
- Reaches inside display lists, which no flush ever can, unblocking the 47 widgets that compile them
- Attacks 74% to 86% of the frame at its mechanism

Why it is credible: platform-neutral. Immediate mode is slow on every driver, so this is a general Recoil improvement that pays most on macOS, not a platform carve-out. It deletes the macOS delta rather than adding to it.

Scope: `LuaOpenGL.cpp` has six `glBeginBatch` calls and six matching `glEnd`. That is the whole surface to convert. The accumulate-and-replay machinery already exists in `VertexArray.{h,cpp}`.

Risks, after the 2026-08-26 probe:

- **Settled.** `glDrawArrays` does not merge, so the flush goes. See [Evidence](#gldrawarrays-does-not-merge-tested-2026-08-26)
- **Settled.** Fixed-function state and legacy shader built-ins survive, because vertex specification stays fixed-function
- **New constraint.** The vertex pointer size must never vary between draws or the driver corrupts the frame
- Open: `LuaOpenGL` has no "inside `BeginEnd`" state, so nothing stops a widget calling other `gl.*` functions between two `gl.Vertex` calls. Today that is a silently ignored `GL_INVALID_OPERATION`. Buffered, it would take effect
- Open: display list recording. `glDrawArrays` is compiled into a list with its data dereferenced at compile time, which should retire the deferral, but that is reasoned rather than measured
- Weeks, not an afternoon, with regression risk across every widget in every game

### 2. Bisect which widgets cost the time

Downgraded, and possibly out of scope entirely. Widgets are the game developer's domain. Fixes upstream to each game separately and never reach games that stopped being updated, which is the population that most needs them. Item 1 reaches them all without anyone's cooperation.

Leads if pursued: build menu costs about 10 fps when open, deselecting the starting unit gains about 10 (`PLAN:1324`).

### 3. Capture a Metal System Trace

Never done. Instruments or an Xcode GPU capture, sampling the GPU from outside the process.

**This is not the same as the instrumented Mesa branch, and neither substitutes for the other:**

| | `macos-diagnostics` branch | Metal System Trace |
|---|---|---|
| Answers | how many, how big | how long, and where |
| Vantage | inside the driver | outside the process |
| Gives | counts of submits, blits, allocations, command buffers | per-encoder timings, shader cost, stalls |
| Cost | a Mesa rebuild | no rebuild |

The counters found the leak and the batching bug. They cannot say where GPU time goes, which is the open question. A trace is the only thing that can say whether Zink emits bad shaders or stalls somewhere invisible from inside the engine.

### 4. Count render passes for real. The instrument already exists.

The "about 200 passes a frame" figure is `57ms / 0.266ms`, not a count.

**Most of this is already built.** `337c87ae087` on the `macos-diagnostics` branch of `~/dev/mesa` adds `kk_dbg_count_cmdbuf_new(render)` in `kk_cmd_buffer.c`, which counts render against non-render command buffers, plus submit, blit and allocation counters in `zink_batch.c`, `zink_blit.c` and `zink_bo.c`. All environment gated.

**Why it has only ever been run during loading.** That branch is based on a main that carries `c08dba83025`, the Metal 4 encoding commit, so it leaks about 5 GiB a second and dies at roughly 12 seconds. It never reaches a steady-state frame. That is why the only figure we have is 68177 render passes against 33823 compute passes in 12 seconds of loading, which is not a frame rate number and was never meant to be.

**The fix is to move the instrumentation, not to write it.** Rebase `337c87ae087` onto the [MR 43697](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43697) branch, which carries the same Metal 4 encoding but does not leak, and the counters reach gameplay. That turns the single most load-bearing inference in this document into a measurement, and it is a rebase plus a build rather than new work.

### 5. Track upstream Mesa [issue 15998](https://gitlab.freedesktop.org/mesa/mesa/-/work_items/15998)

Pins our Mesa. [MR 43697](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43697) tested 2026-08-25 and it works. Detail in [Evidence](#evidence).

---

## Measurement rules

Read these before running anything. Every one was learned by getting it wrong.

- **Run-to-run variance is 1 to 2%.** An effect below that cannot be confirmed at any sample size worth paying for. Do not chase single digits.
- **Put both sides inside one run** wherever the switch toggles per frame. Between-run drift and focus loss both exceed most effects. `SPRING_DIAG_CELLS` does this and lives on `macos/renderer-diagnostics`, not on `macos/integration`.
- **Absolute figures do not survive a display change.** Empty Mod costs 16.9ms on the external 5120x2880 panel against 12.5ms on the built-in one, same everything else. Re-baseline after plugging in.
- **A frozen scene hides rebuild-triggered artefacts.** Compiled lists first read as clean for exactly this reason.
- **Verify the scene is what you think.** One sweep reported 40.4 fps and a 2.5x win, which was an empty map with a menu over it.
- **Check the sim frame before trusting any figure.** A paused game renders happily at 27.8 fps.
- **Record the Mesa commit beside every number.** Figures without one cost a rebuild to disambiguate.
- **Give a probe a positive control.** A probe that fails to reproduce a known defect reads exactly like a probe that found nothing, and the first version of `batch_merge_probe.c` did.
- **Check which Mesa a standalone probe loaded.** `DYLD_LIBRARY_PATH=/opt/homebrew/lib` beats the path linked into the binary and substitutes Homebrew's Mesa. See `test-scripts/README.md`.

---

## Harnesses and traps

### Splinter Faction, unattended. Use this by default.

```sh
SPRING_FPS_LOG=5 coding-agents/test-scripts/run-capped.sh 120 /path/to/log.log
```

Runs windowed, enforces a memory ceiling, needs no intervention. Every measurement in the plan was taken on it. Add `--config /path/to/copy.cfg` to change settings without touching the shared `springsettings.cfg`, which is a known contamination trap.

### BAR benchmark. Needs a human.

```sh
"$HOME/.spring/engine/macos_arm64/<build>/spring" \
  --isolation-dir "$HOME/dev/spring-testdata" \
  "$HOME/dev/spring-testdata/benchmark_bar.txt"
```

- **It pauses at sim frame 0 or 1** for the scenario briefing, in fullscreen as well as windowed. Log line is `tomjn paused the game` after `Scenario: Spawning on frame, 0, 0`.
- `debugcommands` are keyed to sim frames, so nothing fires while paused, including the benchmark itself at frame 15.
- Left alone it reports about 27.8 fps for a paused game indefinitely.
- Focus is not the cause. Window activation does not fix it. Synthetic keystrokes need an Accessibility grant and `PLAN:1221` says do not revisit that route.
- The camera decides what is drawn, so a run with a different view is a different benchmark.

### Memory

A run peaks near 8 GiB on 16 GB of RAM. A leaking driver reaches 54 GB and freezes the machine past the point where the out-of-memory dialog responds. `run-capped.sh` polls and kills. Note `ps` RSS stays near 1.3 GB even at 29 GB of real footprint, so RSS shows nothing.

---

## Evidence

### `glDrawArrays` does not merge, tested 2026-08-26

The question that gates item 1. If a buffered draw merged the way `glBegin` does, the flush would survive the rewrite and most of the win with it.

`coding-agents/test-scripts/batch_merge_probe.c`, Mesa `56588ef0665`, 2000 LINE_LOOP circles a frame on a 1280x900 pbuffer, 30 frames a case. Geometry copied from `widget_loop_amp.lua`, which is the configuration already measured positive. Ground truth is the driver's own output with a flush between batches.

| Case | Dirty frames | Worst frame |
|---|---|---|
| immediate, flush, the reference | 0 of 30 | 0 |
| arrays, finish, second ground truth | 0 of 30 | 0 |
| **immediate, no separator, positive control** | **27 of 30** | **40 stray** |
| **arrays, no separator** | **0 of 30** | **0** |
| arrays, flush | 0 of 30 | 0 |
| **arrays, varying pointer size** | **30 of 30** | **553,659 stray** |
| immediate, varying arity, flush | 0 of 30 | 0 |

The positive control reproduces the published figure, 27 of 30 frames against 31 of 31, worst 40 stray pixels against a median of 28. So the probe exercises the defect.

**`glDrawArrays` is clean across 60,000 adjacent pairs**, against a defect that fires about once per 2000 pairs under `glBegin`. The flush can go.

**Varying the vertex pointer size between draws corrupts, badly.** Plain `arrays` changes the pointer address every batch and stays clean, so it is the size change alone. That is legal GL and the driver gets it wrong, so a buffered path must pin the pointer to four floats and never vary it. The arity mitigation does not disappear, it changes shape from 790,748 widened vertices a frame to one constant.

The last row is a null result, not a refutation. The arity defect was found on the load screen with textures bound, which this probe does not set up.

A first attempt at this probe drew 4-vertex squares 10 pixels apart and found nothing, including nothing from the positive control. The spacing is what makes a merge visible: the amplifier puts circles 22 pixels apart so the connecting segment cannot be confused with the shapes.

### `ZINK_DEBUG=rp`, tested 2026-08-25, no signal

| Run | Config | fps | Sim frames |
|---|---|---|---|
| A1 | baseline | 15.74 | 3191 |
| A2 | baseline | 15.79 | 3184 |
| B1 | `rp` | 15.23 | 3122 |
| B2 | `rp` | 15.47 | 3135 |

Logs at `build-macos-legacy/rp-sf-*.log`. Flag confirmed parsed via `ZINK_DEBUG=help`.

Every baseline beat every `rp` run, but with n=2 an arm that is 1 of 6 assignments, p = 0.167. The two `rp` runs differ by 1.6% from each other, so variance is 1 to 2%, and a 2.6% gap sits inside it. Not worth more runs: 2.6% either way is not a lever at 15.8 fps. `rp` cannot be tested with interleaved cells because `zink_debug` is read once at screen creation.

What survives: if tracking were worth the 20% to 40% the pass-break model implies, four runs would have shown it.

**Do not repeat `ZINK_DEBUG=rp,rploads,rpstores`.** Those flags do the opposite of what the names suggest. Per `zink_context.c:3194` and `:3551` they turn existing `DONT_CARE` attachments into `CLEAR` to expose code relying on undefined contents. They add work.

### `ForceImmediateModeFlush = 0`, tested 2026-08-25

| Run | Config | `immediate-mode batching` | fps | frame | Peak footprint |
|---|---|---|---|---|---|
| D1 | baseline | 0 | 14.98 | 66.8ms | 8028 MiB |
| E1 | `ForceImmediateModeFlush = 0` | 1 | 27.98 | 35.7ms | 6377 MiB |

Logs at `build-macos-legacy/dl-{D1,E1}.log`. Reproduces `PLAN:1259` independently.

Rendering visibly broken with it on: red triangles, resource bar corruption, misshapen load screen bars.

### The render pass break model, demoted

Probe figures are solid. The model built on them is not.

| | 3024x1832 | 1512x916 |
|---|---|---|
| 64 blended quads, one pass | 3.72ms | 1.56ms |
| 64 blended quads, one pass each | 20.48ms | 4.24ms |
| implied cost of a pass break | 0.266ms | 0.043ms |

0.266ms for a 22MB store plus 22MB reload is about 166 GB/s, this machine's memory bandwidth. Real mechanism on tiled hardware.

Demoted because the count was never measured, `rp` found nothing to coalesce, and the LuaUI bisect explains the frame without it.

### Mesa pin and [issue 15998](https://gitlab.freedesktop.org/mesa/mesa/-/work_items/15998)

Building `~/dev/mesa` at `56588ef0665`, the commit before `c08dba83025 kk: Move to Metal4 command encoding`, which leaks about 5 GiB a second under the engine. Filed as [issue 15998](https://gitlab.freedesktop.org/mesa/mesa/-/work_items/15998), reproducer at `coding-agents/test-scripts/kk_mipmap_leak.c`. Full account at `PLAN:754`.

[MR 43697](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43697) tested 2026-08-25 across five builds:

| Config | Mesa | Reproducer | Engine peak | Result |
|---|---|---|---|---|
| A, the pin | `56588ef0665` | 481mb | 8065 MiB | survives 120s |
| B, control | `73a03c61101` | 2625mb | ceiling at 43s | killed |
| C, [MR 43697](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43697) | `a0c05f0da7f` | 474mb | 12288 MiB | survives 120s |
| D, Zink flag alone | B + `RELEASE_RESOURCES` | 2627mb | ceiling at 39s | killed |
| E, both | C + `RELEASE_RESOURCES` | 474mb | 7049 MiB | survives 120s |

[MR 43697](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43697) fixes it. The Zink flag alone does nothing, because pre-MR KosmicKrisp declares the reset flag `UNUSED`. Together they are worth 5 GB, and that two-line Zink change is worth proposing separately.

If the MR lands, the pin can move, which reopens released Mesa versions.

**Do not adopt Metal 4 for performance until this is fixed.** Any figure quoted for what Metal 4 buys was measured on a leaking driver.

### Comparison points

Direction only. Different content, not just different present paths.

| Stack | Result |
|---|---|
| sebvries, MoltenVK, direct present | 45 fps at 0.5k units, 12 fps at 2k |
| This build, KosmicKrisp, readback | 3.3 fps at 1300 units |
| lucamignatti, Minecraft, direct present | 70 fps, far lighter scene |

The same engine on the same driver runs Empty Mod at 80 fps. Do not read the gap as a renderer gap.

### The BAR benchmark run this document started from

BAR scenario 23, `corak armpw 650 10 2040`, 2000 sim frames, 3024x1832, 8320 units spawned.

| Span | Frames | Mean | p50 | p95 | p99 | Max |
|---|---|---|---|---|---|---|
| Sim | 1809 | 17.47ms | 16.6 | 23.0 | 27.2 | 41.2 |
| Update | 252 | 4.75ms | 4.0 | 15.4 | 20.3 | 35.5 |
| Draw | 252 | 58.58ms | 32.4 | 212 | 824 | 1818 |

3.3 fps overall. Those spans come from BAR's own gadget and do not cover the whole frame, leaving about 114ms a frame unaccounted. The p50 of 32ms against a p99 of 824ms is a tail problem distinct from being uniformly slow, and nobody has chased it.
