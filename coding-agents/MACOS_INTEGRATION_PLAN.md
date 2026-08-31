# macOS integration branch

`macos/integration` collects every unmerged macOS change into one branch that a person can check out, build, and run. It exists to inform and to debug. It is not a merge candidate.

Real merging happens through the small single-purpose PRs the branch is composed of. Each of those is either already open or extractable later, which is why every commit here keeps its own identity. Nothing is squashed.

## What the branch is for

Three readers:

- Someone on Apple Silicon who wants a working engine. They build the branch and play.
- A maintainer who wants to see the total cost of macOS support in one diff, rather than reading twenty PRs.
- Whoever picks up the work next, including a future agent, who needs the measurements reproducible rather than described.

## Composition

Cut from current `origin/master`, 57 commits picked plus 2 new ones.

The original commit order is preserved exactly. The groups below are labels for reading the log, not a re-ordering. Shuffling commits into tidier groups would manufacture conflicts that do not exist, and the order the work happened in is itself part of what the branch documents.

### 1. Portability, 16 commits

The set already open upstream: #3022, #3035, #3036, #3038, #3041 to #3046, #3048, #3072, #3073, #3079, plus the two AGENTS and CLAUDE doc commits. Builds headless, dedicated and unitsync on Apple Silicon with Homebrew GCC.

`DemoTool: fix the build after the alternate replay extension change` is dropped. It merged upstream as #3161 and `git cherry` confirms the patch is identical.

### 2. Renderer, 13 commits

EGL context through Mesa, Metal present, framebuffer sizing, mouse input in framebuffer pixels, HiDPI control, resize, borderless fullscreen, and the `--window` flag fix.

### 3. Driver workarounds, 7 commits

The changes that exist because the driver underneath is Zink on KosmicKrisp rather than a mature desktop GL: the `glPolygonMode(GL_LINE)` fallback, skipping `eglTerminate` on exit, immediate-mode batch detection and routing, and Lua display list replay.

### 4. Diagnostics, 21 commits

`FramePhaseDump`, `SPRING_FPS_LOG`, `MacRenderScale` and VSync reaching the presenting layer. This group was much larger and has since been cut back: a switch that only turns a mitigation off so a settled defect can be watched coming back is documentation's job, not the engine's, and one of them was read once per vertex on the Lua hot path. What is left either measures something still open or costs nothing.

### 5. Two new commits

- The `gladstub.cpp` `glTexCoord4fv` stub. Without it the headless build fails, and headless is what CI compiles.
- The debugging tooling: `coding-agents/test-scripts/` and `MACOS_RENDERER_PLAN.md`, as one commit rather than the 41 journal commits that produced them.

## What is dropped

The 41 commits that touch nothing but `coding-agents/`. They are a working journal. Their output survives whole in group 5.

Both tracked `.DS_Store` files, stripped out of the commits that added them. The `.gitignore` rule that stops them coming back is kept, because it is a genuine fix and a candidate for its own PR.

## How it is built

Cherry-pick, one commit at a time, onto a fresh branch from `origin/master`. Not `rebase -i`, because the point is to keep each commit independently extractable and to handle upstream drift one conflict at a time.

For each commit:

1. `git cherry-pick -n <sha>`
2. Restore `coding-agents/` and any `.DS_Store` to their pre-pick state, so those paths only ever change in the group 5 commit
3. `git commit -C <sha>`

A conflict against upstream drift is a question, not a merge. Master has moved 31 commits since the branch's merge base, and the last integration rebase found upstream had reworked several changes rather than taking them as written. The right response to a conflict is to check whether master already covers it and skip if so, not to force both versions to coexist.

## Done means

- `engine-legacy` and `engine-headless` both build from the rebased branch on Apple Silicon with Homebrew GCC
- a real game launches and draws, verified by running it, not by the build exiting zero
- the draft PR is open against `beyond-all-reason/RecoilEngine` master, referencing #936

Building is not enough. If 31 commits of upstream drift broke the renderer, this is where that gets found.

## The PR

Draft. The first line says it is not for merge and why. The body lists the component PRs so a reviewer can see which parts are already under review, references #936 `BAR on Metal`, discloses AI assistance as `AI_POLICY.md` requires, and gives the Homebrew and Mesa build steps.

The obvious objection is that it re-proposes twenty open PRs. Draft status and saying so in the first line is the answer to that.
