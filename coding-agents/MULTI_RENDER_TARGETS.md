# Multi-render-target support: conclusion

Scope: static, read-only audit of RecoilEngine for rendering one running game into several windows. Prototype = two windows, 3D world only (no GUI, LuaUI, minimap or cursor), independent cameras. **Design constraint added by the project owner: the architecture must allow submitting different draw frames at the same time (e.g. VR: two eye views, HMD-driven pacing).**

The detailed catalogue follows this conclusion (IDs such as `DSN-01` refer to it). It is partially verified: the verification run was cut short, so some entries are marked `unverified`.

## What is fundamentally in the way

1. **There is no "view" object.** The frame is one `UpdateUnsynced` -> one `GenerateIBLTextures` -> one `WorldDrawer::Draw` -> one swap, and all view-dependent state hangs off two singletons: the static `cameras[CAMTYPE_COUNT]` pool (`CameraHandler.cpp:94`, `CAM-02`, `CAM-04`) and `globalRendering` (window/viewport/aspect/pixel size, ~400 reader lines, `TGT-01`, `DSN-03`). `CCamera::Update` pulls viewport and aspect from `globalRendering` (`CAM-03`).
2. **Per-frame GPU data is single-view.** `UniformConstants` holds one camera/view/mouse state, updated and bound once per frame (`BUF-01`); matrices reach shaders both via the fixed-function stack and that UBO (`CAM-01`, `GST-04`).
3. **Draw preparation happens in Update, for the player camera only.** Draw flags, shadow/reflection caster sets, grass visibility, ROAM tessellation, projectile draw flags and sort distances are computed once per frame and cached (`OBJ-01`, `OBJ-02`, `PVC-02`, `PVC-03`, `PVC-06`, `WRD-01`, `WRD-02`). Drawing a second view reuses the first view's culling.
4. **View-dependent passes are singletons**: shadow map fitted to the player camera (`ENV-03`), env cubemap/water reflection (`ENV-01`), DepthBufferCopy and other viewport-sized textures (`FBO-01`, `FBO-02`, `ENV-04`).
5. **Context and window plumbing is hard-wired to the primary window**: `MakeCurrentContext`, single `sdlWindow`/`glContext`, window events not scoped by windowID, geometry/dirty flags, vsync, `active` flag (`CTX-01..03`, `TGT-02`, `LOP-04`, `LOP-05`).
6. **The main loop**: `CGame::Draw` calls `UpdateUnsynced` itself, so a second Draw advances time/counters twice (`LOP-01`), and there is no shared-stage/per-view split nor present phase (`LOP-02`, `LOP-03`).
7. **GL state resets assume "the" main target** (viewport restore idioms, `FBO::Unbind`, no scoped state guard: `GST-01`, `GST-03`), and the overlay path after the world pass runs unconditionally (`OVL-01`, `OVL-02`).

## Effect of the "concurrent draw frames / VR" requirement

This is the most important steering input, and it rules out the cheapest prototype shape as the *end state*:

- **Sequential global-state switching does not extend to concurrency.** Anything of the form "set current target, draw, restore" (a swapped `globalRendering->view`, a swapped `camera` pointer, `MakeCurrent` per window in one thread) works for two windows but cannot have two frames in flight. It is acceptable only as a clearly marked intermediate step.
- **Per-view data must live in a self-contained per-view frame object**: camera pose/projection/frustum, viewport, target (FBO / swapchain image), its own UBO slice, its own visibility results, its own fences. Raises severity of `BUF-01`, `BUF-03` (rings/fences assume one upload and one swap per frame), `CAM-01`, `CAM-04`, `CAM-05`, `PVC-02`, `OBJ-01`.
- **Shared per-sim-frame data must be immutable while views are in flight**: model/uniform storage, instance and indirect buffers written during Update need per-frame ring slots or double buffering rather than rewrite-in-place (`BUF-03`, `PVC-07`). Visibility must be computed per view into per-view lists (not into flags on units).
- **Pacing moves out of the engine loop**: VR submission is paced by a compositor, not by `SwapBuffers`/vsync per window (`LOP-05`, `DSN-07`), and draw time feeds sim lag protection (`LOP` cfg items), so frame timing must become per-view/per-submission.
- **Context choice**: with approach A (one context, N windows, sequential `MakeCurrent`) concurrency is impossible. Approach C (each view renders into its own offscreen colour/depth FBO, then a present step) fits VR naturally (swapchain textures per eye) and decouples rendering from window pixel formats and from default-framebuffer assumptions (`FBO::Unbind`). Approach B (shared contexts, one per submitting thread) is the only route to true parallel GL submission in OpenGL but costs the per-context duplication of VAOs (12 owner classes), ~19 persistent FBOs, queries, indexed UBO/SSBO bindings and one-time state (`DSN-02`, `OBJ-*`, `GST-02`, `BUF-02`). Note that OpenGL context threading often gains little on drivers; a single render thread that records views back-to-back into separate targets with per-view data and fences gives most of the benefit for VR (both eyes are submitted within one frame) and is far cheaper.

## Recommendation

1. **Introduce a `RenderView`/frame object now** (DSN-01) that owns camera (not an entry of the static array), viewport, target FBO, per-view UBO data, visibility lists, and a view id; the global `camera`/`globalRendering->view*` become compatibility shims reading the "primary view" and are migrated incrementally (`DSN-03`). Keep the single-window game working after every step.
2. **Render views into per-view offscreen targets (approach C) in the primary context**, then present: window 1 by blit to its backbuffer, window 2 by shared-texture/shared-context blit or `MakeCurrent` + blit. This keeps one context for the prototype (no VAO/FBO duplication) while the data model is already concurrency-ready (swapchain-like targets for VR later). It also removes most `FBO::Unbind()` default-framebuffer assumptions by turning them into "bind the view's target".
3. **Restructure the frame** (`LOP-01..03`): once-per-sim-frame shared stage (UpdateUnsynced, shared passes such as sky/shading textures) then per-view {prepare (cull, flags, shadow/reflection if view-dependent, UBO slice) -> draw}, then present. Move draw-flag/visibility computation out of `Update()`-global state into per-view results.
4. **Prototype scope cuts** (all severity-downgrading): window 2 camera is a separate `CCamera` with a fixed/offset pose (`DSN-05`), swallow input for window 2, skip Lua draw callins and all post-world overlays for secondary views (`OVL-01`, `DSN-04`), shadows/reflections reused from the primary view where visually acceptable and marked as known-wrong, behind an opt-in config flag (`CFG`).
5. **Add guard tooling early**: a scoped GL-state guard and an "assert current view" in the draw path (`GST-03`, diagnostics items), since state leaks will show up as corruption in the second view only.

## Suggested milestone order

1. `RenderView` struct + primary-view shims; no behaviour change (TGT-01, DSN-01, DSN-03, CAM-02).
2. Split `CGame::Draw` into shared stage / per-view stage / present with one view (LOP-01..03, OVL-01).
3. Per-view UBO slice and per-view camera/viewport ownership (BUF-01, CAM-01, CAM-03, CAM-04, GST-01).
4. Per-view target FBOs + present for one window, then second window creation/events/swap (CTX-01..03, LOP-04, LOP-05, DSN-06).
5. Per-view visibility (OBJ-01, PVC-02/03, WRD-01/02, ROAM PVC-04..06), view-dependent env passes (ENV-01..04, FBO-01/02).
6. Hardening: input routing, Lua view API, diagnostics, performance sharing (culling/sort across views).

## Caveats

- The audit is static analysis; nothing was built or run. Counts and line numbers come from analyst agents and may have drifted.
- Verification was partial: refuted/duplicate entries are in the appendix; entries marked `unverified` carry only the analyst's claim. The planned gap-hunting completed only partially and the architecture-option design agents never ran, so the architecture discussion above is my synthesis of the findings (notably `DSN-02`), not an independently scored comparison.
- Severity labels are relative to the 2-window 3D-only prototype, except where the concurrent-draw/VR note above says otherwise.

---

# Multi-render-target support: problem catalogue (raw, partially verified)

Generated from a static read-only multi-agent audit. 18 subsystem analysts raised 407 problems -> 200 after dedupe (incl. 3 from gap critics) -> 195 kept (5 refuted/duplicate). Verification was cut short by a spend limit and a manual stop: 25 problems are UNVERIFIED (analyst claim only).

Counts: blocker 13, risk 43, full 94, polish 45.

| Category | prefix | blocker | risk | full | polish |
|---|---|---|---|---|---|
| Window & GL context lifecycle | CTX | 2 | 2 | 7 | 5 |
| GL object sharing & resource lifetime | OBJ | 1 | 2 | 4 | 1 |
| Global GL state, viewport & framebuffer resets | GST | 1 | 3 | 1 | 11 |
| Per-target state in GlobalRendering | TGT | 1 | 1 | 2 | 0 |
| Cameras & view state | CAM | 3 | 2 | 4 | 4 |
| Per-view caches & visibility | PVC | 0 | 7 | 6 | 0 |
| GPU buffers, UBOs & shader uniforms | BUF | 1 | 2 | 2 | 4 |
| Offscreen / screen-sized render targets | FBO | 0 | 2 | 5 | 1 |
| Shadows, reflections, water & sky | ENV | 0 | 4 | 4 | 4 |
| World drawers | WRD | 0 | 2 | 1 | 0 |
| Lua API & callins | LUA | 0 | 2 | 10 | 1 |
| Input, window events & picking | INP | 0 | 4 | 9 | 1 |
| Main loop, frame pacing & threading | LOP | 2 | 3 | 3 | 3 |
| Overlays, fonts, GUI & capture | OVL | 0 | 2 | 11 | 2 |
| Config, compat, performance, build & test | CFG | 0 | 0 | 8 | 4 |
| Design decisions | DSN | 2 | 5 | 17 | 4 |

## Blockers and risks at a glance

- **DSN-02** [blocker, confirmed, S] GL context architecture decision: one context across N windows, N shared contexts, or offscreen render plus present
- **TGT-01** [blocker, corrected, L] Window, GL context and view geometry are singleton state in CGlobalRendering
- **CTX-02** [blocker, corrected, M] MakeCurrentContext and every bind site hardwired to the primary window
- **LOP-02** [blocker, corrected, M] Main loop has a single Draw and a single Swap; no per-target present phase
- **BUF-01** [blocker, corrected, M] Per-frame UniformConstants UBO holds one target's camera, view and mouse state
- **CAM-03** [blocker, unverified, M] CCamera::Update and CalcPixelDir derive viewport, aspect and LOD scale from globalRendering
- **GST-01** [blocker, unverified, M] Viewport restore idioms assume a single main viewport
- **LOP-01** [blocker, unverified, M] CGame::Draw runs UpdateUnsynced itself, so a second Draw call advances time and counters twice
- **DSN-01** [blocker, corrected, M] Define the per-render-target view context and split the frame into global update vs per-view prepare/draw
- **CAM-02** [blocker, unverified, M] Camera objects are a fixed static array by purpose, so no second player camera can exist
- **OBJ-01** [blocker, corrected, L] Per-object draw flags and transform updates are computed once per frame from the player camera frustum
- **CAM-01** [blocker, confirmed, M] Camera matrices reach shaders through two channels, the fixed-function stack and a once-per-frame player-camera UBO
- **CTX-01** [blocker, unverified, M] Event handlers run GL work on whichever context happens to be current
- **CTX-04** [risk, unverified, S] Window-creation failure paths are fatal process exits
- **LOP-05** [risk, corrected, M] VSync is applied once to whichever drawable is current; two blocking swaps per frame and mixed refresh rates are unhandled
- **GST-02** [risk, corrected, S] [arch B] One-time GL context state initialisation is not replicated to new contexts
- **CTX-03** [risk, corrected, M] SDL window events are not scoped by windowID (close, minimize, resize, move, focus)
- **TGT-02** [risk, corrected, M] Window geometry pipeline and dirty flags are hard-wired to the primary window
- **DSN-06** [risk, corrected, S] Render target ownership, lifetime and creation trigger are undefined
- **INP-02** [risk, corrected, L] Input and OS integration hooks are bound to the primary window
- **FBO-02** [risk, corrected, L] View-size-keyed GPU resources are singletons sized from the primary view
- **BUF-02** [risk, corrected, M] [arch B] Indexed UBO/SSBO bindings are per-context and the ring rebind picks the wrong slice
- **LUA-01** [risk, unverified, M] Lua-created GL objects carry no context identity
- **PVC-06** [risk, corrected, L] ROAM tessellation, upload and LOD follow a single camera
- **DSN-03** [risk, corrected, M] Migration mechanism for ~400 reader lines of per-target globals
- **ENV-02** [risk, corrected, M] Shader constants and uniforms baked from view size at creation or resize
- **PVC-02** [risk, corrected, L] Once-per-frame caches keyed on drawFrame and update-time PLAYER-camera caches
- **LOP-04** [risk, unverified, S] globalRendering->active is app-wide but driven by any window and gates all drawing
- **CAM-04** [risk, corrected, L] Global active-camera pointer (camera macro / CAMTYPE_ACTIVE) is the only view selector
- **OBJ-03** [risk, corrected, M] Unit icon state, feature fade and icon-zoom distance come from the primary camera/controller
- **ENV-03** [risk, corrected, M] Shadow map projection is fitted to the player camera and generated once per frame
- **WRD-02** [risk, corrected, M] Projectile sort distances, billboard fill and fx shader uniforms assume the player camera
- **WRD-01** [risk, corrected, M] Grass block visibility and ground-texture streaming are computed in the Update stage for the player camera
- **ENV-01** [risk, corrected, M] Camera-dependent pre-passes (shadow map, env cubemap, water reflection) are single instances tied to the player camera
- **GST-03** [risk, unverified, S] No scoped guard exists for framebuffer, viewport or matrix state
- **DSN-04** [risk, corrected, S] Per-view firing policy for Lua draw callins
- **LOP-03** [risk, corrected, M] CGame::Draw has no shared-stage/per-view split and relies on deferred unbind
- **FBO-01** [risk, confirmed, M] DepthBufferCopy is a single global depth texture and FBO sized from the primary viewport
- **ENV-04** [risk, corrected, M] Water renderers bake the construction-time window size into screen-copy textures and shader constants
- **OBJ-02** [risk, corrected, M] Shadow-caster and reflection-caster sets are computed in Update() against singleton cameras
- **PVC-05** [risk, unverified, M] ROAM lazy-update heuristics thrash when the active camera alternates
- **PVC-04** [risk, unverified, S] ROAM heightMapChanged latch is consumed by the first Update of any pass
- **GST-04** [risk, corrected, S] Terrain shaders depend on per-target legacy fixed-function state (matrices, fog, clip planes)
- **BUF-03** [risk, unverified, S] Streaming buffers and fences assume one upload and one swap per frame
- **OVL-02** [risk, corrected, M] Overlay-style drawer entry points called from the 3D pass use ambient camera and viewport
- **PVC-03** [risk, corrected, M] Projectile draw flags are culled once per frame against fixed PLAYER/UWREFL/SHADOW cameras
- **CAM-05** [risk, corrected, M] Hard-coded player-camera reads bypass the active-camera indirection
- **PVC-07** [risk, unverified, M] Same-draw-frame dedupe tokens in effect renderers reuse the first target's culling
- **OVL-01** [risk, corrected, S] No 3D-only pass mask: everything after the world pass runs unconditionally
- **PVC-01** [risk, confirmed, M] Engine-owned screen-sized textures exposed to Lua are single instances sized to the primary view
- **LUA-02** [risk, corrected, M] ViewResize and geometry getters describe only the primary window; SDL window events are not routed per window
- **INP-03** [risk, corrected, M] Mouse, button and wheel events are consumed without checking the source window
- **DSN-05** [risk, corrected, S] Prototype needs a decision on how the second window's camera is driven
- **INP-04** [risk, unverified, S] Pointer enter/leave, offscreen flag and implicit capture track a single window
- **INP-01** [risk, unverified, M] Cached cursor ray and press-time camera state are only valid for the camera that produced them
- **DSN-07** [risk, corrected, S] Swap pacing and vsync telemetry are single-stream; a second vsynced swap can silently halve the frame rate

## Subsystem inventories (analyst findings)

### gst
Today the whole renderer treats "the target" as three implicit globals: FBO 0 of the one window's GL context, the viewport rect in globalRendering (viewPos/viewSize), and the global camera. Nothing is passed in. Instead, ~37 call sites reset to those globals after each offscreen pass: FBO::Unbind() always binds 0, globalRendering->LoadViewport() or camera->LoadViewport() reloads the main rect, and CCamera::Update() re-reads window geometry. The only scoped helpers (GL::SubState, GL::TexBind, glPushAttrib) cover state attributes and textures, never FBO, viewport or matrices. Camera data reaches shaders by two channels: the context's fixed-function matrix stack (21 shaders) and a once-per-frame UBO built from the player camera (3 shaders). Per-context state lives in the GL driver (matrix stack, fog, clip control, FBO and VAO bindings). CPU-side caches are few and mostly benign: GL::State::Attributes, FBO::activeFBOs. With one shared context re-currented per window, most driver state just works. What breaks is the hard-coded main-window viewport and size, size-fixed screen resources, the single-camera UBO, and SwapBuffers fusing frame-level work with presenting one window.

*Inventory:* Counts are engine code only (rts/ minus rts/lib).
- glViewport: 38 calls in 21 files. 28 are world/offscreen bake code (DynWater 6, InfoTexture/Modern 6, BumpWater 4, GrassDrawer 3, GlobalRendering 2, others 1 each); 5 in rts/Rml, 3 in Lua, 2 in MiniMap.
- Viewport restores: 19 globalRendering->LoadViewport() calls (plus InitGLState); camera->LoadViewport() at Game.cpp:1524, AdvWater:351, DynWater:486; 5 CCamera::UpdateLoadViewport calls (offscreen set); GeometryBuffer::LoadViewport 2 callers.
- glScissor: 6 calls, all GUI (Lua 1, MiniMap 3, Rml 2). None in the 3D path.
- Framebuffer reset: 20 static FBO::Unbind() calls in 13 files, plus 17 instance Unbind()/UnBind() calls that bind FBO 0. DynWater adds 7 raw glBindFramebufferEXT(0) calls. Raw bind-0 elsewhere: GlobalRendering.cpp:692, FBO.cpp:191/254/283. Total ~45 bind-0 sites in 3D/offscreen code.
- Lua FBO scopes (LuaFBOs 17 calls) save and restore via glGetIntegerv. RenderToTexture restores 0 outside shadow mode.
- No code selects GL_BACK/GL_FRONT; glDrawBuffer/glReadBuffer are only called inside FBO scopes.
- Default-FB reads: glReadPixels at Screenshot.cpp:58, AVIGenerator.cpp:402, LuaOpenGL.cpp:6559/6684. glCopyTexSubImage2D at BumpWater:906/912, RefractWater:65, LuaOpenGL:4455. FBO::Blit(-1) in DepthBufferCopy 3 times.
- glClear on the default FB: WorldDrawer.cpp:309, InitGLState:1727, ClearScreen.
- Fixed-function matrix calls: 635 hits in 46 files, ~318 in 3D/shared code. Clip planes: 79 hits. GL_QUADS/glBegin: 76 hits (compatibility profile).
- Scoped helpers: GL::SubState (27 refs, 19 files), GL::TexBind, glPushAttrib 83 hits. GL_STATE_CHECKER has 0 uses. State::Viewport exists with 0 users.
- Contexts: 1 SDL_CreateWindow, 1 context, 1 SDL_GL_SwapWindow, 0 SDL_GL_SHARE. MakeCurrent callers: LoadLock, LoadScreen, SpringApp:1246, GlobalRendering.
- Window geometry reads: ~321 hits of globalRendering view/win/pixel/aspect in 47 files.
- MatrixStateTracker is per Lua context, not per GL context.

*Open questions:*
- Does SDL2 on the target platforms (Windows WGL, X11 GLX, Wayland EGL) accept SDL_GL_MakeCurrent(window2, ctx) with a context created for window 1 when both use identical SDL_GL attributes? This should be prototyped first, since the single-context strategy depends on it.
- Should each target render directly into its window's default framebuffer or into an offscreen FBO blitted to the window? This affects the FBO::Unbind semantics, MSAA resolve, capture and the sub-rect options.
- Will the prototype windows have equal size and viewPos 0? If yes, the viewport restore and screen-size resource problems drop from blocker to risk. Is user resizing of window 2 required?
- Which Lua callins and gadgets may run in a secondary target? Gadget material shaders (LuaObjectDrawer, LuaMaterial) are part of the 3D scene; LuaUI DrawWorld callins must be suppressed.
- Per-target recompute (2x shadow, cubemap and water reflection cost) or one shared result? Camera-fitted shadows (SHADOWPROMODE_CAM_CENTER) cannot be shared; map-center mode can.
- Is a sub-viewport-in-one-window prototype acceptable as a stepping stone? The dual-screen code already offsets all copies and blits by viewPos, and only the clear needs a scissor.
- Which thread owns which window at load time? CLoadLockMtx re-currents the main window from the load thread. Window 2 should probably be created after load to avoid contention.
- Is the MSAA and depth pixel format required to match across windows? DepthBufferCopy and GeometryBuffer have separate multisample paths keyed off the global msaaLevel and the default-FB depth.
- Should vsync be a single master (window 1) with other targets unthrottled, and how does that interact with DwmFlush and BlockCompositing, which are applied per window?

### cam
Today the camera system is multi-camera only "per purpose", never "per target". CameraHandler.cpp:94 holds six static CCamera objects indexed by type (PLAYER, UWREFL, SHADOW, ENVMAP, VISCUL, plus an ACTIVE slot that only stores which type is current). The `camera` macro (Camera.h:316) resolves through that one process-global pointer at roughly 570 call sites. Sub-passes such as shadow, cubemap and water reflection swap the pointer and restore it afterwards. Every CCamera::Update() recomputes viewport, aspect and LOD scale from globalRendering and writes fixed-function GL matrices. One singleton CCameraHandler (static placement storage, global controllers, global transition state) drives only the PLAYER camera. The Update stage (CGame::UpdateUnsynced) bakes view-dependent state from that single camera: per-object draw flags, unit icon state, feature fade, projectile sort keys, grass visibility, the per-frame UBO, ROAM tessellation and shadow fitting. All of it is shared by every consumer. A second 3D window therefore needs a first-class per-target view context, a per-view prepare stage, a decoupled viewport and a second independent camera. Many caches also need re-keying.

*Inventory:* Counts and locations I verified by reading code:
- 587 textual hits for camera->/GetActiveCamera/GetCamera/CAMTYPE_ in 94 files; 17 are false positives in rts/lib/assimp, so about 570 real hits.
- 6 static CCamera objects (rts/Game/CameraHandler.cpp:94) with 5 real types. The ACTIVE slot is a selector whose camType field stores the current type.
- 8 controllers (FPS, Overhead, Spring, RotOverhead, Free, Overview, Dolly, Dummy) in rts/Game/Camera/. One CCameraHandler lives in static placement storage (CameraHandler.cpp:96-97,112), with a global `camHandler`.
- 6 sites swap the active camera: ShadowHandler.cpp:650, CubeMapHandler.cpp:204, BumpWater.cpp:1026, AdvWater.cpp:339, DynWater.cpp:469, UniformConstants.cpp:186. The last one hard-codes the restore to PLAYER (:195).
- About 17 camera reset sites call camera->Update/LoadViewport/prvCam->Update (e.g. Game.cpp:1430,1524, WorldDrawer.cpp:316, ShadowHandler.cpp:656, DynWater.cpp:485-492). About 16 sites call globalRendering->LoadViewport().
- The only GL entry points in CCamera are LoadMatrices (glMatrixMode/glLoadMatrixf) and LoadViewport (glViewport), plus the per-context glClipControl assumption baked into the projection.
- Hard-coded CAMTYPE_PLAYER consumers: ROAM (RoamMeshDrawer.cpp:219), BasicMeshDrawer.cpp:181, UniformConstants.cpp:167,230, ShadowHandler.cpp:130, ProjectileDrawer.cpp:494,970,1206, GrassDrawer.cpp:518, BeamLaserProjectile.cpp:119, MiniMap.cpp:1449, and the camType loops in UnitDrawerData.cpp:430 and FeatureDrawerData.cpp:121.
- camType-indexed storage: Patch.h:212 [4], Projectile.h:132 sortDist[3], BasicMeshDrawer visUpdateFrames, NanoParticleRenderer gatheredCamType.
- 9 live GridVisibility(nullptr) callers, which read VISCUL: 5 in LuaUnsyncedRead, plus Grass, DebugVisibility, DebugColVol and InMapDrawView.
- drawFrame increments only at Game.cpp:1277 in-game. DrawFlags is uint8_t with 7 of 8 bits used (WorldObject.h:14-23).
- About 15 Lua camera functions have no target parameter. No test references CCamera.
- Per-frame call order: Game.cpp:1376 controller, 1390 sound, 1424 events and grass, 1427-1430 tracker and camera->Update, 1431 shadow, 1433 world Update (draw flags), 1441 UBO, 1519 IBL, 1524 viewport reset, 1526 world Draw.

*Open questions:*
- What does 'independent camera' mean for the prototype: a fixed or scripted pose, follow-primary-with-offset, or a fully user-controlled camera with its own controller (which needs input routing per window)?
- Draw-flag model: is a union of frusta (flags set if any view sees the object) acceptable for the prototype, with per-view re-testing in draw loops, or must we add per-view flags and per-view uniform storage now (widening the 1-byte DrawFlags and the GPU drawFlag)?
- Which view owns audio listener, FPS direct control, AI/selection queries, unit tracking and minimap frustum drawing? (Proposal: primary view only; confirm.)
- Shadows: share one shadow map fitted to the union (or the map) for the prototype, or render a shadow map per view? Same question for water reflection/refraction and the env cubemap.
- Should Lua gadget and widget DrawWorld* callins run for the secondary view in the prototype, and should they see the current view's camera? Prototype scope says no LuaUI, but LuaRules DrawWorld callins may still fire.
- Does view 2 share the player's allyteam/LOS perspective (gu->myAllyTeam, spectatingFullView), or may a view later show another team? Unit visibility flags, icons and NanoParticle LOS use gu->myAllyTeam globally.
- Is a render target always an OS window, or also offscreen textures (affects viewport origin, aspect, FBO binding and camera ownership)? How does this relate to the existing DualScreenMode viewport split?
- Is it acceptable to keep the `camera` macro as 'current view camera' (cheap, 570 call sites untouched, but thread-unsafe) or do we want explicit view/camera passing for the long-term parallel-render goal?
- Should secondary views render at the full draw rate, or at a reduced rate, given the per-view Update-stage cost at the 10k-unit scale target (ENGINE_PERFORMANCE.md)?
- Should the secondary window's camera type reuse CAMTYPE_PLAYER semantics with per-view storage, or introduce new CAMTYPEs? The latter breaks arrays sized by type (Patch.h:212, Projectile.h:132).

### ctx
Today the engine has exactly one SDL window and one GL context, both stored as raw members of the CGlobalRendering singleton (sdlWindow, glContext). The only SDL_CreateWindow call is GlobalRendering.cpp:442 and there are only three SDL_GL_CreateContext calls, all in the same file. Every context-binding site (6 MakeCurrentContext callers), the only SDL_GL_SwapWindow (:703), vsync (VerticalSync.cpp), SDL event handling (SpringApp::MainEventHandler), window geometry (winSize*/viewSize*/pixel*/aspect, read only from sdlWindow) and the config keys (XResolution, WindowPosX, Fullscreen...) assume that one window. GL is touched by two threads only (main and the game-load thread). They share the single context through the CLoadLock mutex hand-off, so no offscreen or secondary context exists anywhere. Nothing else creates a window: the RmlUi SDL backends under rts/lib are samples and are not built. All failure paths during window and context creation are fatal (modal box then process exit), SDL_GL_SHARE_WITH_CURRENT_CONTEXT is never used, and no code filters SDL events by windowID. A prototype therefore needs a RenderWindow record, a bind/present API, a context-topology decision (recommended: one context rebound to N identical-pixel-format windows), window-scoped event handling, and a split of SwapBuffers into per-frame and per-window parts.

*Inventory:* Counts for rts/ excluding rts/lib:
- Window and context calls: 1 SDL_CreateWindow (GlobalRendering.cpp:442); 3 SDL_GL_CreateContext (:474, :490 in the fallback probe loop, :515); 2 SDL_GL_DeleteContext (:501, :629); 1 SDL_GL_SwapWindow (:703); 3 SDL_GL_SetSwapInterval (VerticalSync.cpp:76/81/87); 20 SDL_GL_SetAttribute calls (all in GlobalRendering.cpp). SDL_GL_SHARE_WITH_CURRENT_CONTEXT, SDL_GetWindowID, SDL_GL_GetCurrentContext and any windowID filtering appear nowhere.
- MakeCurrentContext: 1 definition (:612) and 6 call sites. They are GlobalRendering.cpp:606 and :1305, SpringApp.cpp:1246, LoadLock.h:21 (set) and :32 (clear), and LoadScreen.cpp:175. SDL_GL_MakeCurrent is also called directly at :613 and :624.
- SwapBuffers callers: SpringApp.cpp:906, LoadScreen.cpp:326, SplashScreen.cpp:107, LuaOpenGL.cpp:5352.
- Threads touching GL: main plus the game-load thread, handed one context via CLoadLock (14 GetUniqueLock sites, plus Spring.Yield). ThreadPool glThreadSupport is hard-coded false (ThreadPool.cpp:89). CGameLoadThread no longer creates an offscreen context, but GameLoadThread.h:11 and LoadScreen.cpp:125 still say it does.
- Events: SDL_WINDOWEVENT is consumed in SpringApp.cpp:1098-1252, MouseInput.cpp:95-111, RmlUi_Backend.cpp:568 and RmlUi_SystemInterface.cpp:206, none windowID-aware.
- GetWindow()/sdlWindow consumers: 10 sites in 7 files.
- Geometry reads: viewSizeX 62 uses in 27 files, viewSizeY 87/36, viewPosX 33/14, pixelX 79/16, dualScreenMode 22/6.
- LoadViewport: 20 call sites in 15 files. About 41 unbind-to-0 framebuffer sites in 18 files.
- Non-shareable object creation: VAO (VAO.cpp:21, used by 22 files), FBO (5 files), queries (GlobalRendering.cpp:664, LuaOpenGL.cpp:6712).
- Window creators elsewhere: only the RmlUi sample backends under rts/lib, gated by RMLUI_SAMPLES/TESTS (off); the engine uses rts/Rml on the main context.
- Headless stubs return one static pointer for every window and context (sdlstub.c:55-58, :166-169).

*Open questions:*
- Single context rebound to N windows or N shared contexts? This needs an empirical check on target drivers (NVIDIA/AMD/Intel on WGL, Mesa on GLX/Wayland). It also depends on which SDL2 version the Windows and Linux builds link (the Linux docker image uses the distro libsdl2-dev; the Windows version is not pinned in the repo).
- Does the prototype's primary window keep its GUI, LuaUI and RmlUi, with only the second window 3D-only, or are both windows 3D-only? This decides how much overlay and event-scoping work the prototype needs.
- Who creates the second window and when: command-line/config at startup, an engine command, or Lua? Should it exist during loading, or only after CLoadScreen::Kill? Should it survive Reload or be torn down with the game?
- Frame pacing and vsync policy: which window paces the frame? Are the windows on monitors with different refresh rates, and is the sim/draw loop allowed to run at the primary's rate regardless of the secondary?
- What should closing the secondary window do: remove only that target, or quit the game as it does today for any CLOSE event? What if the primary is closed while a secondary exists?
- Is DualScreenMode to be deprecated in favour of a minimap window target, or kept alongside? Is exclusive fullscreen on either window in scope for the first iteration?
- Must the windows match in size, aspect ratio or MSAA level for the first prototype, or must independent sizes work from day one? This affects the geometry, MSAA and size-dependent offscreen resource work.
- Multi-GPU or hybrid-GPU systems: is a secondary window on a monitor driven by a different GPU in scope?

### inp
Input is built around exactly one SDL window and one pointer. SDL delivers one process-wide event queue (InputHandler::PushEvents, SpringApp.cpp:931) to four chained handlers: IMouseInput, SpringApp::MainEventHandler, aGui and RmlGui. Every SDL event carries a windowID, but nothing in rts/ reads it, so all events are treated as primary-window events. Mouse state is one global CMouseHandler (`mouse`) holding lastx/lasty, buttons[], and a cached world ray `mouse->dir` computed from the single active camera (`#define camera` = CCamera::GetActive(), Camera.h:316). Coordinates are raw ints interpreted through globalRendering view fields. Picking is CPU-only (TraceRay in world space), so it is GL-target independent as long as the camera and cursor pixel are consistent. The window-event switch mutates global state (`globalRendering->active`, geometry, grab, GL context), and a close event quits the whole game. dualScreenMode is the only precedent for several views, and it is one window with hand-rolled coordinate remapping. For the prototype, B's events must be filtered by windowID and B's camera driven without the global mouse and controllers. Full interactivity needs per-target pointer, receiver, selection and Lua state.

*Inventory:* - Zero uses of windowID, SDL_GetWindowID, SDL_GetWindowFromID, SDL_GetMouseFocus or SDL_GetKeyboardFocus anywhere under rts/.
- 4 handlers on the global `input` chain: MouseInput.cpp:39, SpringApp.cpp:293, aGui/Gui.cpp:19, RmlUi_Backend.cpp:164. The first returns true to stop the chain; IMouseInput is registered first (CMouseHandler::InitStatic, SpringApp.cpp:291).
- SpringApp::MainEventHandler handles 8 SDL_WINDOWEVENT groups (MOVED, SIZE_CHANGED, MAXIMIZED/RESTORED/SHOWN, MINIMIZED/HIDDEN, FOCUS_GAINED, FOCUS_LOST, DISPLAY_CHANGED(18), CLOSE), SpringApp.cpp:1098-1252. MouseInput.cpp handles ENTER/LEAVE; Rml handles SIZE_CHANGED/LEAVE.
- 3 synthetic SDL event injection sites with partly uninitialised events: SpringApp.cpp:1224-1231, KeyInput.cpp:188-194, CrashHandler.cpp:946-948. debug.emulateMouse* bypasses SDL entirely (LuaDebugExtra.cpp:128-169).
- `globalRendering->active` is read at 7 sites: Game.cpp:1471, MouseInput.cpp:202, MouseHandler.cpp:815, FBO.cpp:38, FreeController.cpp:87, LuaMenuController.cpp:116, and written in SpringApp.cpp:1165/1185.
- mouse->lastx/lasty/dir/GetWorldMapPos/GetCursorCameraDir: 68 occurrences in 22 files. camHandler uses: 80 occurrences in 14 files.
- 15 CInputReceiver subclasses; static receivers deque and a single static activeReceiver (InputReceiver.h:54-62).
- CCamera::CalcPixelDir has 7 external call sites, CalcViewPortCoordinates about 11. Both are called with global view geometry.
- 6 static cameras, one per CAMTYPE (CameraHandler.cpp:94); 1 CCameraHandler with one currCamCtrlNum (CameraHandler.h:148).
- dual-screen fields (viewWindowOffsetY, dualWindowOffsetY, dualViewPosX, dualScreenMode): 62 occurrences in 10 files, MiniMap.cpp alone 21.
- Process-global SDL calls: SDL_SetRelativeMouseMode (MouseHandler.cpp:845/869), SDL_ShowCursor (864/911/947 and HwMouseCursor), SDL_CaptureMouse (RmlUi_SystemInterface.cpp:185/188). Primary-window-only calls: SDL_WarpMouseInWindow(GetWindow()) (MouseInput.cpp:221), SDL_SetWindowGrab(sdlWindow) (GlobalRendering.cpp:1323), WndProc hook on the primary HWND (MouseInput.cpp:153-168).
- No GL picking (no glReadPixels or GL_SELECT picking); TraceRay is world-space and filters by gu->myAllyTeam.
- Lua: about 35 camera refs in LuaUnsyncedRead.cpp and 19 camHandler/camera refs in LuaUnsyncedCtrl.cpp, none with a target argument.
- The one-time per-frame mouse UBO fields (mouseScreenPos, mouseStatus, mouseWorldPos) are set from the global mouse in UniformConstants.cpp:250-293.

*Open questions:*
- Does window A keep its full UI (Rml, minimap, console, cursor) in the prototype while B is 3D-only, or are both windows 3D-only? This decides how much of A's Rml/GUI pollution matters.
- Should B accept any input in v1 (move, resize, close only) or be a pure display? If B accepts none, the router can drop all pointer and keyboard events from B.
- Should keyboard shortcuts typed while B has focus act on the game as usual (global) or be ignored?
- Which window supplies the audio listener (Game.cpp:1390 sound->UpdateListener(camera...)), and which is the 'primary' for quit, config persistence and sound?
- How is B opened and closed: config at startup, an action such as /window2, or Lua? Where do B's size and position persist (only WindowPosX/Y and the windowed resolution of A are saved, GlobalRendering.cpp:1600-1603)? Does closing B hide it, destroy it, or can it be re-opened?
- Which SDL2 version is vendored, and what are the platform defaults for SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE, SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH and SDL_HINT_MOUSE_AUTO_CAPTURE? The comment at SpringApp.cpp:1242 notes the Linux SDL2 is old. Enter/Leave ordering between windows on Windows, X11 and Wayland is unverified.
- On Windows, does B need the WM_SETCURSOR hook (CWin32MouseInput subclasses only A's HWND, MouseInput.cpp:153-168)? What is the Wayland behaviour of the warp workaround (MouseInput.cpp:214-219) with two windows?
- There is no high-DPI handling (no SDL_GL_GetDrawableSize); window size is assumed equal to drawable size. Is that acceptable for B on high-DPI displays?
- Should B share A's player/ally-team view (LOS, spectator) or need a separate viewer? This determines whether TraceRay and drawing need a viewer parameter.
- Does the game (or a widget) rely on the mouse UBO fields (mouseWorldPos, mouseScreenPos) in custom shaders? That determines whether the UBO split is urgent for the prototype.
- Screenshots and video capture (Screenshot.cpp:58, AVIGenerator.cpp:402, Game.cpp:1553-1557) read the primary framebuffer. Which target should they capture?

### tgt
CGlobalRendering is both the process-wide GL capability record and the single, implicit render target. Window, GL context and view geometry live together in it. That geometry is about 33 fields: winPos/Size, viewPos/Size, the dual* viewport, pixelX/Y, aspectRatio, screenView/ProjMatrix, active, sdlWindow and glContext. Roughly 400 reader lines in about 55 files read these fields as "the current view". Only about 65-70 of those lines, in about 17 files, are on the 3D path: camera, UniformConstants, water, decals, depth copy, G-buffer. The rest is GUI, minimap, Lua and input. Geometry is rewritten in place by UpdateGLGeometry on SDL events. About 20 sites call globalRendering->LoadViewport() to mean "restore the main viewport", and SDL events are never dispatched by windowID. SwapBuffers fuses present with ring-buffer and fence bookkeeping, and MakeCurrentContext always binds window 1. Per-frame time globals are advanced once in UpdateUnsynced. DualScreenMode is a hard-wired second viewport inside one window; it should not be reused for N targets, only as precedent for the view-rect concept. The cheapest incremental path is a RenderTarget struct plus an ActivateTarget() that makes one GL context current on that target's window and loads the legacy globals as "current-target registers". The 3D-path consumers that cache or bake view state are then migrated to explicit view parameters.

*Inventory:* CGlobalRendering (rts/Rendering/GlobalRendering.h, 428 lines) has about 110 members:
- about 33 per-window/target fields (screenSize/Pos, winPos/Size, viewPos/Size, viewWindowOffsetY, dual*, winBorder, winChgFrame, gmeChgFrame, screenView/ProjMatrix, pixelX/Y, aspectRatio, active, fullScreen, borderless, dualScreenMode, dualScreenMiniMapOnLeft, sdlWindow, glContext);
- about 9 per-frame timing fields (drawFrame, timeOffset, lastTimeOffset, lastFrameTime, lastFrameStart, lastSwapBuffersEnd, weightedSpeedFactor, FPS, grTime);
- about 10 global draw/debug toggles;
- about 40 per-process GL caps and limits.
All geometry is CR_IGNORED (GlobalRendering.cpp:132-217).

Reader counts (rts, excluding lib):
- globalRendering appears on 840 lines in 137 files.
- viewSizeX/Y: 170 lines in 39 files.
- viewPosX/Y: 69 lines in 17 files.
- win{Size,Pos}: 88 lines in 11 files.
- aspectRatio/pixelX/pixelY: 133 lines in 20 files, mostly GUI.
- dual*/viewWindowOffsetY: about 60 lines, mostly MiniMap.
- active: 8 lines in 7 files.
- screenView/ProjMatrix: only 2 readers (LuaOpenGL.cpp:990-993, UniformConstants.cpp:163-164).

Viewport call sites:
- UpdateGLGeometry has 4 callers; InitGLState has 3.
- globalRendering->LoadViewport has 19 external callers plus 1 internal.
- GeometryBuffer::LoadViewport has 2 callers, camera->LoadViewport 3, UpdateLoadViewport 5.
- glViewport appears about 38 times.

Context and event plumbing:
- SDL_GL_CreateContext is called only in GlobalRendering.cpp:474-515, SDL_GL_MakeCurrent in 2 places, SDL_GL_SwapWindow once.
- SwapBuffers has 4 callers.
- windowID is referenced 0 times in rts.
- There are 13 GetCamera(CAMTYPE_PLAYER) reads in render code.
- About 20 once-per-frame stamps are keyed on drawFrame in about 10 files.
- The UBO is 2 buffers; ModelVertProgGL4.glsl:125 uses cameraViewProj.

*Open questions:*
- Does the first prototype force both windows to the same size and aspect? That would defer the view-size-keyed resource and baked-constant problems and make the register-copy approach enough.
- Which SDL version and platforms must be supported? Is a single GL context made current on two same-pixel-format SDL windows acceptable on WGL, GLX/X11, Wayland/EGL and macOS (not verified)?
- Which target is primary (owns GUI, minimap, input, sound listener, config keys)? What happens when the primary closes before a secondary: quit, or promote?
- What drives window 2's camera in the prototype (free-fly, follow a unit, a Lua-set pose, a console command)? Is a second CCamera and controller instance planned (camera domain)?
- Should window 2 render at full rate every draw frame, or can it be rate-limited or decimated? How should vsync be paced across two monitors with different refresh rates?
- Shadow maps and cubemaps are generated once per frame in GenerateIBLTextures. Shadows are fit to the player camera (default ShadowProjectionMode CAM_CENTER), so window 2 likely gets cropped or wrong shadows. Which domain owns this and is it prototype-blocking?
- Should DrawWorld and other Lua draw callins fire once per target (with a target id) or only for the primary? What is the compatibility policy per coding-agents/BACKWARDS_COMPATIBILITY.md?
- Which default-on features depend on view-sized resources in the target configs (deferred G-buffer, depth-copy consumers such as decals and soft particles, BumpWater)? Should the prototype disable some via config?
- Not verified: whether any thread-pool code reads globalRendering view geometry off the main thread (ProjectileDrawThreadedFill and similar fills did not appear to), whether gu->globalReload/Reload tears down any window state, and the exact per-target use sites in MiniMap/GUI beyond counts.
- HiDPI: CreateSDLWindow does not pass SDL_WINDOW_ALLOW_HIGHDPI, so window size equals drawable size. Confirm this holds for window 2 on mixed-DPI displays.

### wrd2
The unit/feature/model drawers have no concept of a render target. Everything is per-frame global state set up once in CGame::UpdateUnsynced (Game.cpp:1430-1441). CUnitDrawerData::Update and CFeatureDrawerData::Update classify every object against the three cameras PLAYER, UWREFL and SHADOW. The results go into one uint8 drawFlag per object (WorldObject.h:97), together with the icon bit, feature drawAlpha and iconRadius. The same flag gates the transform and uniform SSBO uploads. Draw passes do not cull. They scan every object in ModelRenderContainer bins and test the flag bits, so unitDrawer->Draw is a pure filter over camera-1 results. The default GL4 path also takes its camera matrices from a global UBO filled from CAMTYPE_PLAYER once per frame, not from the active camera. The legacy path (still used every pass for Lua-material objects) takes the view from the GL matrix stack, fog and lights. Running the drawers a second time with another camera would reuse camera-1 visibility, icon state, uploads, UBO matrices and shadow map. The result would be an empty or duplicate view. Camera-independent parts (transform and uniform SSBO data, bins, instance building, Lua bin draining) are sound for sequential passes in one GL context.

*Inventory:* Pass entry points: 11 unitDrawer sites. WorldDrawer.cpp:372,413,453,482; ShadowHandler.cpp:459; IWater.cpp:163,168,206,211; LuaObjectDrawer.cpp:510; Game.cpp:1534. 9 featureDrawer sites. WorldDrawer.cpp:373,414,454; ShadowHandler.cpp:460; IWater.cpp:164,169,207,212; LuaObjectDrawer.cpp:513.

Frame order: camera->Update (1430), shadowHandler.Update (1431), worldDrawer.Update, which runs CUnitDrawer::UpdateStatic and CFeatureDrawer::UpdateStatic (1433), transformsUploader.Update (1434), modelUniformsUploader.Update (1435), UniformConstants::Update (1441). Then CGame::Draw runs UBO Bind (1464), TeamHighlight::Enable (1512), GenerateIBLTextures (1519), FBO::Unbind + camera->LoadViewport (1522-1524), CWorldDrawer::Draw (1526). Opaque drawing is once. Alpha is twice (below and above water). IWater adds a reflection and a refraction pass.

Class counts:
- 2 drawer-data classes
- 2 model-drawer states (GLSL, GL4)
- 3 CModelDrawerHelper subclasses
- 1 S3DModelVAO singleton
- 2 uploaders (TransformsUploader on SSBO binding 0, ModelUniformsUploader on binding 1)

Ambient reads inside the domain files: 39 `camera->` / CCameraHandler / camHandler (UnitDrawer.cpp 17, UnitDrawerData.cpp 10, LuaObjectDrawer.cpp 8, ModelDrawer.h 2, ModelDrawerState.cpp 1, FeatureDrawerData.cpp 1). About 34 `gu->` reads (UnitDrawer.cpp 21, UnitDrawerData.cpp 11). 24 globalRendering reads (UnitDrawer.cpp 12, UnitDrawerData.cpp 3, FeatureDrawerData.cpp 1, ModelDrawerState.cpp 1, LuaObjectDrawer.cpp 1, ModelsDataUploader.cpp 5). 45 fixed-function matrix-stack calls (UnitDrawer.cpp 27, ModelDrawerHelpers.cpp 15, FeatureDrawer.cpp 3).

Camera structure: 6 CAMTYPE slots, one CCamera each (CameraHandler.cpp:94). Draw-flag bits used: 1, 2, 4, 8, 16, 32, 128 (7 of 8). Per-object visibility is evaluated for 3 camera types.

No glViewport or FBO calls in the domain files except the deferred G-buffer path (LuaObjectDrawer.cpp:479-537). No draw-frame dedupe counters exist. Upload dedupe uses sim-frame stamps (lastSyncedFrameUpload, ModelDrawerData.h:165). CheckLegacyDrawing/ForceLegacyPath is dead code. The legacy path is used via ScopedModelDrawerImpl(true,false,false) at ModelDrawer.h:364,393,450.

Scale target is about 10k units (coding-agents/ENGINE_PERFORMANCE.md:7).

*Open questions:*
- Context strategy: one GL context made current on both windows (SDL_GL_MakeCurrent) or a shared-context share group? This decides whether the per-context VAO, SSBO/UBO binding and fixed-function state problem needs any work.
- Should both windows use the same viewer (gu->myAllyTeam, spectatingFullView, selection) in the prototype? The code assumes yes. Is a different viewer per window ever wanted?
- Update cost model for the prototype: a cheap hack (swap window-2 camera into the CAMTYPE_PLAYER slot and re-run Update+Draw per window, 2x Update cost and 2x UBO upload) versus the real per-view classification. Which do you accept for the first milestone?
- Should window 2 have its own shadow map (extra CreateShadows pass), reuse window 1's, or run without shadows?
- Unit icons in window 2: draw all as models, apply a per-view icon decision, or draw icons per view?
- Do the target games (BAR) rely on Lua-material or legacy FFP model draws? That makes the matrix-stack, fog and lights problem prototype-visible (BAR Lua shaders probably also read the engine UBO).
- Frame pacing: both windows rendered inside one CGame::Draw sharing globalRendering->timeOffset (one UpdateUnsynced), or independent cadence per window (which would require per-view Update)?
- Camera model: new CAMTYPE slots per window versus RenderView-owned CCamera objects? Slot counts are baked into arrays elsewhere (Patch.h:212, BasicMeshDrawer).
- Is deferred model rendering (AllowDeferredModelRendering, off by default) in scope, or can it be disabled for multi-view?
- Should the Lua API gain a view parameter or Spring.GetCurrentView() for draw callins and the draw-flag readers, and what is the backward-compatibility policy (coding-agents/BACKWARDS_COMPATIBILITY.md)?
- Windows swap order and vsync: one SwapBuffers per window will block twice per frame if both have vsync on. This lives in the presentation domain, but it interacts with fence creation (PutBufferLocks) in the streaming-buffer problem.

### lua
Lua sees exactly one render target, implicitly. Every Lua draw callin is dispatched from the single-window frame (CGame::Draw and the passes it drives) through CEventHandler's DRAW_CALLIN macros. None takes a target or view parameter. LuaOpenGL turns that implicit target into state. Matrix reset reads the global `camera` (the active-camera slot). Screen callins read globalRendering->screenProj/ViewMatrix. Viewport, scissor and mouse conversions read globalRendering->view{Pos,Size}. gl.RenderToTexture, RawBindFBO(nil) and BlitFBO(nil) treat FBO 0 as the screen. CopyToTexture, ReadPixels and SaveImage read the current drawable in raw window coordinates. Engine resources exposed to Lua are single instances sized to the primary view: the engine UBO (filled from CAMTYPE_PLAYER), the g-buffers and the depth copy. Lua can only query or manipulate the primary window and camera, and cannot create, enumerate or observe other targets. LuaOpenGL's statics (drawMode, resetMatrixFunc, safeMode) are safe if views render strictly sequentially on the GL thread. So a per-view 'current view' context (active camera, viewport fields, UBO contents) swapped in before each view works with most existing Lua code. The prototype blockers are the callin firing policy, per-view camera/UBO/viewport, and engine resets to the primary target around Lua callins.

*Inventory:* Draw callin entry points in rts/System/EventHandler.cpp: 21 DRAW_CALLIN macro callins (703-723). They are Genesis, World, WorldPreUnit, PreDecals, WaterPost, WorldShadow, ShadowPassTransparent, WorldReflection, WorldRefraction, GroundPre/PostForward, GroundPre/Deferred/PostDeferred, Units/FeaturesPostDeferred, ScreenEffects, ScreenPost, Screen, InMiniMap and InMiniMapBackground. There are also 5 DRAW_ENTITY_CALLINs (743-747: Unit, Feature, Shield, Projectile, Material), 7 hand-written ones (1016-1056: WorldPreParticles, Opaque/Alpha Units/Features Lua, ShadowUnits/FeaturesLua) and DrawBuildSquare (1058).

Fire sites:
- Game.cpp:1468, 1536, 1547 (Genesis, ScreenEffects, ScreenPost)
- WorldDrawer.cpp:323, 346, 367, 440
- SMFGroundDrawer.cpp:269, 286, 304, 313, 336, 346
- ShadowHandler.cpp:444, 485
- IWater.cpp:174, 216
- LuaObjectDrawer.cpp:390, 533
- ProjectileDrawer.cpp:945
- UnitDrawer.cpp:296, 1694
- FeatureDrawer.cpp:300
- MiniMap.cpp:1373, 1386
- LuaInputReceiver.cpp:71
- LuaMenuController.cpp:123-126

Camera and target dependence:
- `camera` is a macro (Camera.h:316) over a static cameras[6] array with exactly one CAMTYPE_PLAYER (CameraHandler.cpp:94).
- LuaOpenGL.cpp has about 12 globalRendering references and 7 `camera->` or CCameraHandler uses.
- LuaOpenGL.cpp has 55 FFP matrix/viewport/scissor/attrib calls.
- LuaUnsyncedRead.cpp has about 59 camera, camHandler and viewSize/viewPos lines. They include 13 `camera->` reads: InView x6, CalcViewPortCoordinates x3 and GetPos/Dir/Rot/FOV/Vectors.
- Window-management Lua functions (SetWindowGeometry, SetWindowMinimized, SetWindowMaximized, SetWMCaption, GetWindowGeometry, GetWindowDisplayMode) all act on the single globalRendering window.

Context-bound Lua GL objects: lazily created VAO (LuaVAOImpl.cpp:233), FBOs (LuaFBOs.cpp:502, LuaTextures.cpp:117), occlusion queries (LuaOpenGL.cpp:6711), plus one-time glEnable in LuaOpenGL::Init (258).

Engine UBO (UniformMatricesBuffer/UniformParamsBuffer): bindings 0/1, updated once per frame (Game.cpp:1441) from CAMTYPE_PLAYER. It is exposed to Lua via gl.GetEngineUniformBufferDef (LuaShaders.cpp:1359). The engine's own GL4 model shader computes gl_Position from it (ModelVertProgGL4.glsl:125, 133).

The repo's cont/ Lua has 14 files that touch view or camera queries. No target-identity API exists anywhere.

*Open questions:*
- Which GL context model will the prototype use: one context made current on each SDL window (same pixel format), or one shared context per window? This decides whether Lua FBO/VAO/query/UBO-binding state is valid in window 2.
- Does the primary window keep its full GUI in the prototype, or must both windows be 3D only? The user's wording ('no menu/GUI/LuaUI') suggests neither window shows GUI, which changes how the primary window's Lua screen callins (DrawScreen*) must behave.
- Should LuaRules/LuaGaia gadget world effects (DrawWorld, DrawWorldPreUnit from gadgets) count as 'pure 3D' and render in window 2, or only engine-drawn 3D plus Lua materials and map shaders?
- How is camera 2 driven in the prototype: fixed offset/overview camera from engine config or a console command, or a new Lua API (Spring.SetViewCameraState)? This sets the minimum Lua surface.
- One shadow map fitted to the union of both frusta, or per-view shadow passes? This determines whether DrawWorldShadow stays once per frame or becomes per view.
- Will the two windows have different sizes, MSAA settings or refresh rates? Different sizes require per-view g-buffers/depth copy and Lua size caching; different refresh rates affect Lua Update cadence and gl.SwapBuffers (LuaMenu only).
- Which BAR widgets and gadgets rely on DrawGenesis (once per frame vs per view), on gl.CopyToTexture of the screen, and on Spring.GetDrawFrame? BAR's source is not in this repo, so a survey of its callin usage is needed to validate the proposed per-view policy.
- Should a view be addressable from Lua by a stable integer viewID (0 = primary), and may Lua create views itself or only the engine/config?
- When any window sets globalRendering->active=false (minimize/hide), should the whole draw loop stop, or only that view? CGame::Draw currently returns before the world pass and after DrawGenesis.

### buf
GPU-side per-frame data is built for exactly one view. Two engine UBOs (binding 0 matrices, binding 1 params) are filled once per draw frame at the end of CGame::UpdateUnsynced from the global CAMTYPE_PLAYER camera, globalRendering viewport/screen matrices, the single shadow fit and the single mouse. They are bound once in CGame::Draw. Only the GL4 model and shadow-gen shaders read them. Terrain, sky, water, grass, particles and RenderBuffer shaders still read the compat matrix stack plus per-draw uniforms.

Object SSBOs (transforms, model uniforms) hold data gated by single-camera drawFlags. Stream-buffer rings and fences advance once per frame inside CGlobalRendering::SwapBuffers, which is welded to the one window's SDL_GL_SwapWindow. There is one GL context, never shared, so VAOs, indexed UBO/SSBO bindings and ARB/fixed-function state are all implicitly single-context. Several CPU-built caches (ROAM mesh, nano particles, grass, projectile reuse, sort distances) are keyed by drawFrame/camType, not by view. UBO N-updates per frame are semantically safe because BufferSubData is ordered, but nothing models a 'current view'.

*Inventory:* UBOs: 2 (UniformMatricesBuffer = 26 mat4; UniformParamsBuffer ~32.3 KiB, teamColor[2048] = 32 KiB), both SB_BUFFERSUBDATA, numElems=1 (UniformConstants.cpp:128-139), created once in PostInit (GlobalRendering.cpp:662), Update() once (Game.cpp:1441), Bind() once (Game.cpp:1464). UniformConstants.h:124 BUFFERING=3 is unused. Engine C++ indexed bindings: 4 (UBO 0,1; SSBO 0,1) via UniformConstants.cpp:335-336 and ModelsDataUploader.cpp:56,90,109,157. Engine GLSL reading the UBO: 3 files (ModelVertProgGL4, ModelFragProgGL4, ShadowGenVertProgGL4), each with a hand-copied block. 26 GLSL files use gl_ModelView*/gl_Projection*/clip built-ins (70 hits), including SMFVertProg, ModernSkyVS, GrassVertProg, ProjFXVertProg, BumpWaterVS. StreamBuffer impls: 6 types; users: UniformConstants(2), ModelsDataUploader(2: transforms = persistent coherent x3, modelUniforms = SUBDATA x1), 15 typed RenderBuffers (RenderBuffers.cpp:12-28, default SUBDATA, cleared only in SwapBuffers). Only callers of RenderBuffer::SwapRenderBuffers and IStreamBufferConcept::PutBufferLocks: GlobalRendering.cpp:688-689. VAO owners: 15 RenderBuffers, S3DModelVAO, ROAM Patch (2 per patch per grid), SkyBox, DebugCubeMapTexture, ModernInfoTexture, ModernSky, GroundDecalHandler, NanoParticleRenderer, Lua VAOs. Contexts: one SDL_GL_CreateContext, no SDL_GL_SHARE_WITH_CURRENT_CONTEXT. drawFrame-keyed caches: ~9 sites. Per-draw camera uniforms set in C++ at ~12 sites. Resolution baked: BumpWater 3 #defines; GroundDecalHandler screenSizeInverse via ViewResize (only 2 C++ ViewResize listeners in Rendering). ROAM defaults ON (SMFGroundDrawer.cpp:53-57). haveGL4 = ARB_multi_draw_indirect (GlobalRendering.cpp:877).

*Open questions:*
- Single context with SDL_GL_MakeCurrent(window_i) versus a second shared context: is the pixel-format/drawable compatibility needed for option A acceptable on the target platforms (WGL/GLX/Wayland), and does the CLoadLock context hand-off tolerate a non-primary drawable?
- Can the prototype first validate UBO/SSBO/stream/ROAM behaviour with two viewports inside the one existing window (dualScreenMode already offsets by viewPosX) before introducing a second SDL window?
- Visibility model: is a union-of-frusta approach for drawFlags and transform upload acceptable when the two cameras look at distant areas (upload volume), or is per-view drawFlag storage required?
- Which of the five global cameras (PLAYER, UWREFL, SHADOW, ENVMAP, VISCUL) become per view? UBO per-view fill needs to know how the reflection camera and shadow fit map to each view.
- Should window 2 have shadows and water reflection/refraction in the prototype, or are they disabled there to avoid duplicating the IBL/reflection passes and the shadow-fit decision?
- Is it acceptable to append members to the UBO blocks (e.g. view index), or must the layout stay bit-identical for Lua widgets (BACKWARDS_COMPATIBILITY)?
- Vsync/swap interval: SDL_GL_SetSwapInterval applies per current drawable; must secondary windows use interval 0 and a single EndFrame so fences/ring depth remain once per frame?
- Is ROAM=0 an acceptable prototype configuration, or must ROAM be made multi-camera for the first milestone?
- Should the prototype require the GL4 model path (haveGL4 / ARB_multi_draw_indirect) and ignore the legacy GLSL model fallback that reads the compat matrix stack?

### wrd1
The terrain/map/decal stack is single-viewer in three ways. (1) The drawers are process singletons (readMap->GetGroundDrawer(), grassDrawer, groundDecals) and CBaseGroundDrawer::Draw(DrawPass) takes no target or camera; the viewer is implied by the global active-camera slot (the `camera` macro) and by globalRendering view size. (2) Terrain is already drawn from several cameras per frame (Normal, Shadow, WaterReflection, WaterRefraction, env-cubemap faces), and both mesh drawers cope by keying visibility on a camera-type slot plus globalRendering->drawFrame. But everything treats the PLAYER slot as 'the' viewer: ROAM tessellates around GetCamera(PLAYER) and keeps one tessellation and one index buffer per patch, grass caches its visible lists in Update from one camera, and texture streaming picks mips from the active camera at Update time. (3) Helpers assume the primary window: G-buffer, DepthBufferCopy and the decal screenSizeInverse uniform come from globalRendering->viewSize, and about 30 call sites reset FBO and viewport to the 'main' state. Map-global resources (heightmap, normal, shading and info textures, decal instance buffer, Basic mesh buffers) are view-independent and shareable. The default mesh drawer (ROAM=1) is the hardest piece. CBasicMeshDrawer (ROAM=0) is nearly stateless and is a viable prototype fallback.

*Inventory:* Ground Draw callers (5): rts/Rendering/WorldDrawer.cpp:342 (Normal), rts/Rendering/Env/IWater.cpp:158 and :201 (WaterReflection, WaterRefraction), rts/Rendering/Env/CubeMapHandler.cpp:232 (TerrainReflection, env faces), rts/Rendering/ShadowHandler.cpp:479 (DrawShadowPass). DrawPass has 7 values (rts/Map/MapDrawPassTypes.h:5-13).

Camera accessors: 16 (camera-> / GetActiveCamera / GetCamera) in rts/Map across 5 files, 9 in GrassDrawer.cpp, 0 in the decal sources (decals rely on the GL matrix stack plus gl_ModelViewProjectionMatrix). globalRendering view/drawFrame uses in rts/Map: 19 across 6 files.

GL state resets (FBO::Unbind / globalRendering->LoadViewport / glViewport): rts/Map 4 (SMFGroundDrawer.cpp:252 and :296, SMFReadMap.cpp:755-756), rts/Rendering/Map/InfoTexture 21 occurrences in 6 files, GrassDrawer::CreateFarTex 5 (:897, :906, :954, :973, :979).

View-dependent caches:
- CRoamMeshDrawer: 2 patch sets, each with tessellation tree, index/border VBO, 2 VAOs and 2 float[4096] variance trees per patch.
- ROAM also has lastCamPos/lastCamDir/lastGroundDetail/patchVisFlags/numPatchesLeftVisibility per set, one heightMapChanged latch, static forceNextTesselation[2] and static pools[2] (2^21 TriTreeNodes x 48 B, about 96 MiB each on 64-bit).
- Patch::lastDrawFrames[4], MeshVisPatch::visUpdateFrames[4], CBasicMeshDrawer::drawPassLOD.
- Grass: static blockDrawer (3 vectors), oldCamPos/oldCamDir, per-block CVertexArray, farnearVA.
- SMFGroundTextures mip state (streaming only).
- Singletons sized to the primary view: GeometryBuffer (7 attachments) and DepthBufferCopy (2 FBO/texture pairs).

Config defaults: ROAM=1, AllowDeferredMapRendering=false, AllowCombinedMapRendering=false, SMFTextureStreaming=false, Shadows=2 (terrain shadow pass skipped), GrassDetail=7, GroundDecals=true, HighQualityDecals=false, MSAALevel=0.

CBaseGroundDrawer has no per-target API. drawFrame is incremented once per CGame::UpdateUnsynced (Game.cpp:1277). One SDL window and one glContext exist (GlobalRendering.cpp:346, :613).

*Open questions:*
- GL context model: one shared context with SDL_GL_MakeCurrent per window (VAOs, FBOs and display lists stay valid) or one context per window with sharing? This decides how much of the VAO/FBO/display-list inventory must be made per-context.
- Camera model for window B: a new camera-type slot (grows Patch/BasicMeshDrawer visibility arrays, the camera pool and VISCUL) or swapping the PLAYER slot's contents per target (visibility stamps then accumulate within a frame)?
- Is drawFrame allowed to increment per target draw, or must it stay once per CGame::UpdateUnsynced? It is the cheapest isolation for visibility stamps but other consumers (grass GC, GL timer queries, texture draw frames) rely on its current meaning.
- Must the two prototype windows have identical size, viewport origin, pixel format and MSAA? If yes, DepthBufferCopy, decal screenSizeInverse and the G-buffer need no changes for the prototype.
- ROAM strategy for the prototype: run with ROAM=0 (Basic mesh, nearly stateless) or implement per-target ROAM sets (about 96 MiB trinode pool plus about 195 KB x 2 per patch of duplicated vertex data per additional set)? Is losing ROAM's detail acceptable for the first test?
- Shadows: one shadow map fit to the PLAYER camera, a union frustum, or per-target shadow generation? Terrain, decals and grass all sample it, and the default Shadows=2 skips terrain shadow casting.
- Should both windows share the same viewer (gu->myAllyTeam, LOS, info-texture mode)? This governs decal visMult, info overlays and whether those become per-target state.
- Which DrawGround* Lua callins and which Lua map-shader or G-buffer consumers must run for the secondary window, and does Lua need a target id?
- Frame structure: is map-global work (UpdateDraw, ground Update, info textures, shading texture, IBL, shadows) strictly once per frame before the per-target loop? That would let the many FBO::Unbind/LoadViewport resets in rts/Rendering/Map stay unchanged for the prototype.
- Do AllowDeferredMapRendering or AllowCombinedMapRendering need to work in the prototype? Defaults are off, but BAR-style configs may enable them.

### env
The view-dependent environment passes assume exactly one window, one GL context, one PLAYER camera and one globalRendering viewport. Pass classes. Per-frame-global (view-independent, run once): sun/sky/water/sunlight param edges, specular cubemap, sky texture (a no-op in all 3 skies), SMF shading/normals texture, BumpWater dynwave + coast textures (Update), AdvWater bump animation, and the shadow-map resource itself. Per-view: shadow-frustum fit (player camera only), sky->Draw plus fog, water reflection/refraction render-to-texture and screen copies, water Draw, DrawBelowWaterOverlay, UBO camera/reflection/fog, DynWater's camera-centred simulation. The env reflection cubemap is camera-anchored but low frequency. Today's order: Update() does camera->Update, shadowHandler.Update, WorldDrawer::Update (draw flags per camera slot, water sim), UBO build. Draw() does CreateShadows, one cubemap face, shading texture, then CGame restores FBO/viewport, then terrain, sky, units, alpha with UpdateWater mid-pass. The required N-view order is: CPU-update all view cameras and shadow fit, then one shared GL stage, then per view {begin target, per-view UBO, water RTT prepass, opaque, alpha/water, 3D-only overlays}, then swap each window. Nested passes currently restore a hardcoded default FBO, viewport and camera.

*Inventory:* Singletons: shadowHandler (ShadowHandler.cpp:37; one smOpaqFBO plus depth/color textures; 59 accessor refs in 13 files; 101 'shadowHandler.' uses in 23 files). cubeMapHandler (3 cube textures + 1 FBO, 128px faces, 1 face/frame, 6- or 12-frame cycle; 14 uses in 6 files). ISky::sky (3 impls: NullSky/SkyBox/ModernSky; Update and UpdateSkyTexture are no-ops in all; Draw reads the active camera and loads FFP matrices). IWater::water (5 impls; default config Water=1 = AdvWater; BumpWater defaults reflection=1 skip-terrain, refraction=1 screencopy, depth copy on). sunLighting and waterRendering are globals. Cameras: static CCamera cameras[6] (CameraHandler.cpp:94); the `camera` macro is the active slot; 45 refs to SHADOW/UWREFL/ENVMAP/VISCUL in 20 files; CCamera::Update/CalcPixelDir read globalRendering view fields. Env FBO objects: 1 shadow, 1 cubemap, 2 AdvWater, 4 BumpWater, 3 DynWater (reflect, refract, frameBuffer), 1 SMF shading. VAOs in ModernSky, SkyBox, DebugCubeMap. 16 FBO-unbind sites and 8 viewport-restore sites in env/shadow/shading files. 24 GLSL files use gl_ModelView/Projection/Fog built-ins (ModernSkyVS.glsl:55 reads gl_ModelViewMatrixInverse); ModelVertProgGL4.glsl reads cameraViewProj/reflectionViewProj/fogParams from the UBO. Update-stage env work is in CGame::UpdateUnsynced (Game.cpp:1430-1441). Draw-stage: WorldDrawer::GenerateIBLTextures (WorldDrawer.cpp:249-285), Draw (302-331), UpdateWater at 437, overlay 497-570. Single window and context: GlobalRendering.cpp:346/474/613. Water is recreated on any resize: Game.cpp:1115-1118.

*Open questions:*
- GL context model: one context with N SDL windows via SDL_GL_MakeCurrent(window_i, ctx), or one context per window with share lists? This decides whether every env FBO/VAO must be duplicated and whether cross-context sync is needed.
- Shadows for the first prototype: disable (Shadows=-1), SHADOWPROMODE_MAP_CENTER (view-independent), union fit, or per-view shadow maps? Is losing/lowering shadows in window 2 acceptable?
- Must the two windows have identical size and aspect? Identical sizes defuse most resolution-bound water problems, but only while globalRendering view fields are swapped per view.
- Which water renderer must the prototype support? Default config is AdvWater (Water=1). BumpWater (screencopy + baked constants) is the harder case, and DynWater is camera-centred and could simply be disabled in multi-view.
- Should each view have its own fog/far-plane (view-range depends on camera height) and its own reflection camera, or may secondary views reuse the primary's environment parameters?
- Should DrawWorldShadow/Reflection/Refraction Lua callins fire per view, once per frame, or only for the primary view?
- WorldDrawer.cpp:273-276 only calls cubeMapHandler.UpdateSpecularTexture when the sun direction changed, and each call refreshes one row of 128*3 (CubeMapHandler.cpp:254-273). Is the incremental per-frame update intended? If so the gating looks like a pre-existing bug, which affects classifying it as frame-global.
- When one window is minimized or occluded (globalRendering->active early-out at Game.cpp:1471-1479), should the shared stage still run for the visible one, and how should the edge flags and cube-face cycle behave?
- Where should the env cubemap and DynWater simulation be anchored (primary view, map centre, per-view)?

### wrd3
The projectile, particle and world-overlay subsystem is written for one view. Alpha particles and projectile models are drawn with modern render buffers (VA_TYPE_PROJ) and compat-profile shaders. Billboards are rebuilt on every Draw() from the global active camera (the `camera` macro, Camera.h:316). Culling and sorting are not per draw. CProjectileDrawer::UpdateDrawFlags runs once per frame from CWorldDrawer::Update and flags every projectile against three global cameras (PLAYER, UWREFL, SHADOW). Per-projectile state (drawFlag, sortDist[3]) is sized for exactly those views. Ground flashes and flying pieces cull at draw time, which is the correct pattern for N targets. Several caches are keyed by globalRendering->drawFrame (alpha-range reuse, GL4 nano enemy gather, shield visibility), which identifies neither target nor camera. Fog, soften and LOD code reads the CAMTYPE_PLAYER slot directly. Overlays (selection, command lines, cursor icons, map marks, debug drawers, Lua world callins) sit between 3D passes inside CWorldDrawer::Draw. Only DrawMiscObjects() can be skipped cleanly, and the rest need a layer mask. Particle VAOs and the perlin FBO are per-context objects. The only code that resets to "the" main target is UpdatePerlin (FBO unbind and globalRendering->LoadViewport).

*Inventory:* Entry points: CProjectileDrawer::UpdateDrawFlags (WorldDrawer.cpp:227), UpdateTextures (230), DrawGroundFlashes (350), DrawOpaque (378), DrawAlpha twice (419, 459). IWater adds DrawOpaque x2 (IWater.cpp:165, 208) and DrawAlpha x2 (170, 213). ShadowHandler calls DrawShadowOpaque/Transparent (456, 484). MiniMap calls DrawProjectilesMiniMap (MiniMap.cpp:2006). NanoParticles::Draw is called 4 times (WorldDrawer 420/460, IWater 171/214).

`camera->` occurrences: 64 in 14 files under Sim/Projectiles (1 spawn-time, in ExplosionGenerator.cpp:453) and 51 in 15 files under Env/Particles (3 in ProjectileDrawer.cpp). Direct CAMTYPE_PLAYER readers: ProjectileDrawer.cpp:494, 970, 1206 and BeamLaserProjectile.cpp:119.

Camera model: 6 static camera slots (CameraHandler.cpp:94). sortDist is std::array<float,3>. DrawFlags uses 7 of 8 bits (bit 64 free). `drawFrame` is incremented once per frame (Game.cpp:1277) and used as a cache key in 3 places in my scope: ProjectileDrawer.cpp:889/954, ShieldSegmentProjectile.cpp:154, NanoParticleRenderer.cpp:668.

Immediate-mode GL: none in particle or projectile code. Immediate or client-array users are the overlays and debug drawers (HUDDrawer 39 matches, HAPFSPathDrawer 20, CommandDrawer 9, LineDrawer 6, QTPFSPathDrawer 5, WorldDrawer DrawBelowWaterOverlay, GuiHandler ~28 glBegin). Particle code issues one tracer draw directly (TracerProjectile.cpp:73-84).

Overlay call sites in CWorldDrawer: 4 in DrawOpaqueObjects (366, 383-385), 5 Lua callin sites (323, 346, 367, 440, plus 945 inside ProjectileDrawer), and 7 in DrawMiscObjects (471-491). lineDrawer has about 119 call sites in 9 files. Its only draw consumers are WorldDrawer:484 and MiniMap:2026.

Per-context GL objects: TypedRenderBuffer VAOs (VAO.cpp:21), NanoParticles instanceBuffer.vao, perlinFB, depthBufferCopy FBOs. NanoParticlesGL4 defaults to false (NanoParticleConfig.cpp:13-14), and SoftParticles defaults to 1 (ProjectileDrawer.cpp:48).

*Open questions:*
- Context model: one GL context made current on N drawables, or one context per window with shared lists? This decides how much particle code changes (VAOs, FBO, texture visibility).
- How is the second camera represented: state copied into the PLAYER slot per target (cheapest, keeps all 111 camera-> sites working) or separate CCamera instances with a new camType or ViewId? Which of these breaks the fixed-size camType-indexed arrays (Projectile::sortDist[3], BasicMeshDrawer visUpdateFrames, Patch lastDrawFrames)?
- Is globalRendering->drawFrame still a per-frame counter, or should it become per-target-pass? Three particle caches are keyed on it.
- Do both windows share the same viewer identity (gu->myAllyTeam, spectatingFullView)? Projectile LOS flags, nano ally visibility and CanDrawProjectile all read gu directly.
- Shadows: one shadow map fitted to the union of the two views, per-target maps, or none in the second window? Projectile shadow flags depend on it.
- Should the second window render water reflections and refractions? That doubles the reflection and refraction passes, including both projectile sub-passes.
- Are unit icons and radar blips (DrawUnitIcons in DrawMiscObjects, DrawUnitIconsScreen in the screen pass) part of the 3D-only view? Dropping DrawMiscObjects also drops them.
- Are Lua world callins (DrawWorld, DrawWorldPreParticles, DrawProjectile, DrawShield) suppressed in window 2 for the prototype, or fired with a target id?
- Soft particles are on by default and need a per-target depth copy. Accept disabling them in the second window for the prototype, or require equal-size windows?
- Should the Lua projectile visibility APIs (GetVisibleProjectiles, GetRenderProjectiles) report the primary view only, or the union of views?
- Is the opt-in GL4 nano renderer (NanoParticlesGL4=false by default) in scope for the prototype?

### fbo
Offscreen and window-sized render targets are process-wide singletons that implicitly describe "the" window. There is one SDL window and one GL context (GlobalRendering.cpp:474,587). There is no engine scene-colour FBO and no post-processing: the world is drawn straight into the default framebuffer. Its RGBA8/D24/S8/MSAA pixel format comes from a per-window fallback chain. Window-sized helpers (DepthBufferCopy, the two GeometryBuffers, BumpWater/RefractWater screen copies, GroundDecals' screenSizeInverse) are sized from globalRendering->viewSize at construction or resize and reused by every pass. Map-sized FBOs (infotex, shading, shadow, cubemap, water) are not window-sized, but they share the same epilogue: bind FBO 0 and reload the primary viewport (about 24 viewport resets and about 35 FBO::Unbind sites). Resize handling is one ViewResize event plus polling, tied to the primary window, and it rebuilds water wholesale. For two windows the key choices are the context model (FBOs are not shareable between contexts), the render-target model, and turning depth copy, G-buffers and water screen copies into per-target resources that are created and sized at target bind.

*Inventory:* - FBO instances: about 20 declarations in 13 files, 57 files and 236 hits mention FBO.
  - GeometryBuffer.buffer: two global instances (SMFGroundDrawer::geomBuffer, LuaObjectDrawer::geomBuffer).
  - DepthBufferCopy: depthFBOs[2] (non-MS and MS).
  - ShadowHandler::smOpaqFBO.
  - AdvWater: reflect+bump. This is the default water, Water=1.
  - BumpWater: reflect, refract, coast, dynWaves.
  - DynWater: reflect, refract, plus 7 raw glBindFramebufferEXT(...,0).
  - CubeMapHandler::reflectionCubeFBO.
  - ModernInfoTexture::fbo in 8 subclasses.
  - MiniMap: fbo + fboResolve.
  - GrassDrawer (2), TextureRenderAtlas, ProjectileDrawer::perlinFB, SkyBox (temporary).
  - LuaFBOs/LuaTextures (user created).
- About 35 FBO::Unbind-style sites in 21 files, and about 24 viewport-reset calls in 18 files (globalRendering->LoadViewport, GeometryBuffer::LoadViewport, camera->LoadViewport).
- globalRendering view/win fields: 265 hits in 42 files. Window-sized texture or constant creation in Rendering/: DepthBufferCopy, GeometryBuffer, BumpWater, RefractWater, DynWater, GroundDecalHandler, UniformConstants, CubeMapHandler.
- Resize triggers: only DepthBufferCopy and GroundDecalHandler implement ViewResize. CGame::ResizeEvent kills and recreates water. GeometryBuffer polls once per frame. SDL window events are never filtered by windowID.
- Window pixel format: RGBA8, depth 24 (fallback 32, 16), stencil 8, MSAA fallback chain. No sRGB and no float framebuffer anywhere.
- Memory per 1080p target: the default config needs about 8 MB (depth copy only). Deferred G-buffers add about 100-108 MB. BumpWater screen copy+depth+RBO adds about 25 MB. Shadow map (shared) is about 33 MB at 2048². MSAA multiplies the MS copies.
- Deferred rendering is off by default (AllowDeferredMapRendering and AllowDeferredModelRendering are false) but Lua can toggle it at runtime. MSAALevel defaults to 0.

*Open questions:*
- Must the two prototype windows have identical size and pixel format, and may they be user-resized? Identical and fixed size would let the depth-copy, decal-uniform, G-buffer and water size problems be deferred.
- Single shared GL context re-bound with SDL_GL_MakeCurrent per target, or one context per window with share lists and per-context FBO twins? Needs an empirical test on Windows (WGL), X11 (GLX) and Wayland (EGL) that a context created on window 1 can be made current on a second window created with identical attributes.
- Should Lua DrawWorld*/DrawGround*/Draw*PostDeferred callins fire for secondary targets at all in the prototype, or only for the primary one? This decides whether G-buffers and Lua resources need per-target handling right away.
- Which water renderer, MSAALevel, deferred settings and HighQualityDecals must the prototype support? Defaults (AdvWater, MSAA 0, deferred off) avoid most size-baking problems, but games typically enable bump water and deferred rendering.
- Shadows: fit to the primary camera, force MAP_CENTER mode, fit to the union of target frusta, or render a per-target shadow map (about 33 MB each at 2048²)?
- Presentation: two SDL_GL_SwapWindow calls per frame with vsync enabled will block twice per frame. SwapBuffers also swaps RenderBuffers and puts stream-buffer locks (GlobalRendering.cpp:688-689), which must happen once per frame. What is the swap-interval policy per window?
- FBO::Blit from the window depth buffer into a DepthBufferCopy texture whose depth format comes from supportDepthBufferBitDepth (possibly 32 bit) may rely on driver leniency when the window has 24 bits. This was not verified on other drivers.
- Should info textures (LOS/radar/path) become per-target when windows view different ally teams or spectator modes?
- Does CLoadLock/MakeCurrentContext (pinned to the main window) get taken at runtime while window 2 is current, for example during lazy model loads? If so it would detach the context from window 2.
- Is fullscreen context loss/reinit (FBO::GLContextLost/Reinit) relevant for secondary windows, and which window's events should drive it?

### lop
Today the main loop is strictly single-target. SpringApp::Run loops Update(): configHandler/UpdateWindow, activeController->Update(), then one activeController->Draw() under CLoadLock, then exactly one globalRendering->SwapBuffers(). CGlobalRendering owns a single SDL_Window and a single GL context, and every window, swap, vsync, make-current, timer-query and frame-pacing path assumes them. CGame::Draw is monolithic: it runs UpdateUnsynced (time, drawFrame, camera, per-object draw flags, UBO), then shadow and cubemap pre-passes, then FBO::Unbind plus viewport reset, then the 3D world, then GUI, with one pair of GL timestamps. SwapBuffers also does once-per-frame buffer-ring advance, fence placement, FrameMark and Lua timing events, so it is both "present this window" and "end of frame". No worker thread touches GL (glThreadSupport is hard-false), and there is no engine frame limiter, only vsync. The natural structure is: update once, draw each view into its own current window, present each, then end the frame once. That needs a target registry, a split of Draw and SwapBuffers, per-view re-derivation of everything the Update phase currently bakes from the main camera, and per-window event, vsync and active handling. The loading screen and shutdown can stay single-window if the secondary window exists only while CGame is the active controller.

*Inventory:* - Single-window ownership: sdlWindow/glContext are one pair of members (rts/Rendering/GlobalRendering.h:384-385), created once (GlobalRendering.cpp:398 CreateSDLWindow, :462 CreateGLContext). SDL headers are 2.28.1 (vclibs64/include/SDL2); SDL source is not in the repo.
- Loop: SpringApp::Run (SpringApp.cpp:929-942) -> Update (881-908). Four live SwapBuffers call sites: SpringApp.cpp:906, LoadScreen.cpp:326, SplashScreen.cpp:107, LuaOpenGL.cpp:5352.
- Six MakeCurrentContext call sites, all binding sdlWindow: GlobalRendering.cpp:606 and :1305, SpringApp.cpp:1246, LoadScreen.cpp:175, LoadLock.h:21 and :32.
- CGame::Draw (Game.cpp:1448-1572) embeds UpdateUnsynced (1226-1445). There is one GL timestamp pair (idx 0 and 7 of 16 queries), one FrameMark (GlobalRendering.cpp:713), and zero TracyGpu uses outside rts/lib.
- drawFrame: 53 occurrences in 20 engine files. FBO::Unbind, glBindFramebuffer(..,0) and LoadViewport: about 69 sites in 26 files. Sim/Projectiles reads `camera->` about 64 times in 14 files.
- SpringApp::MainEventHandler handles 8 SDL window events (:1100-1251). There are zero windowID references in non-lib rts.
- Frame globals in CGlobalRendering: timeOffset, lastFrameTime, lastFrameStart, FPS, drawFrame, lastSwapBuffersEnd, active (GlobalRendering.h:130-151, 279).
- Defaults: VSync=-1 adaptive (VerticalSync.cpp:19-26), DWMFlush=0, ForceSwapBuffers=1, LoadingMT=0. There is no frame limiter (zero grep hits); the only pacing is vsync plus a 10 ms sleep when inactive (Game.cpp:1472).
- Threads: glThreadSupport is hard-false (ThreadPool.cpp:89). About 10 for_mt/for_mt_chunk draw-prep sites are CPU-only. The stream-buffer ring depth is 3 (StreamBuffer.h:83).

*Open questions:*
- Context topology: is a single shared GL context across both windows (SDL_GL_MakeCurrent per target, identical pixel format) acceptable? It serializes all rendering on the main thread and forgoes independent per-window refresh rates.
- Verify on the target SDL (vclibs64 2.28.1) and drivers: SDL_GL_SwapWindow on a non-current window, and whether the WGL/GLX swap interval is per window or per context. SDL source is not in the repo, so these are unconfirmed.
- What should vsync do in the prototype: primary vsynced and secondary at interval 0, both vsynced, or neither with a software limiter? Are windows on different monitors or refresh rates in scope?
- Visibility model for the shared Update phase: union of all views' frusta flagged once and re-culled per view, or per-view flag storage? This decides how invasive the unit, feature, projectile and grass changes are.
- Shadows for the prototype: a shadow pass per view (N times the cost) or one map fitted to the union frustum?
- Who drives window 2's camera in the prototype with no input or GUI (a fixed camera, a Lua-set camera state, a scripted orbit)? That decides how much of the primary-target problem must be solved now.
- Should Lua world-draw callins (gadget DrawWorld etc.) run for the secondary view, or should the secondary be strictly engine 3D?
- What should the secondary window show when CGame is not the active controller (loading, menu, reload)? Close it, clear it, or mirror it? The prototype assumes it only exists during gameplay.
- Closing the secondary window: hide it, destroy it, or quit the game? Does it need to be recreatable at runtime?
- Platform scope for the prototype: Windows only, or Linux (X11 XInitThreads path) too? Does the headless/dedicated build need link parity?
- Is LoadingMT=1 (experimental) required to keep working while a secondary target exists, or can it be asserted off?

### obj
Today every GL resource lives in exactly one SDL_GLContext bound to one SDL_Window (GlobalRendering.h:384-385). Nothing in the engine has context identity and nothing is shared between contexts. About 12 classes own VAOs that are generated lazily in whichever context is current. About 19 persistent FBOs exist. UBO/SSBO binding points are set once or per update. Context state (clip control, depth, MSAA, fog, debug callback) is initialised once. All of this works only because there is one context. Textures, buffers, shaders, renderbuffers, sync objects and display lists are shareable. VAOs, FBOs, query objects, indexed buffer bindings and fixed-function and client state are not. Recommendation for the 2-window 3D-only prototype is architecture A: one context, SDL_GL_MakeCurrent per window. It needs no duplication of non-shared objects, and the engine already hands its single context between threads in CLoadLock. The remaining blockers are pixel-format parity, current-drawable management, per-target UBO content and screen-sized scratch resources. Architecture B (shared contexts) adds duplication of VAOs, FBOs and bindings, plus cross-context synchronisation of in-place streamed buffers (XL). Architecture C (offscreen then blit) is as safe as A but collides with FBO::Unbind() semantics. Problems tagged [arch B] only apply if shared contexts are chosen.

*Inventory:* Context/window: 1 SDL_Window + 1 SDL_GLContext (GlobalRendering.h:384-385). SDL_GL_CreateContext only at GlobalRendering.cpp:474/515. No SDL_GL_SHARE_WITH_CURRENT_CONTEXT anywhere. Make-current only through MakeCurrentContext (GlobalRendering.cpp:612), called from LoadLock.h:21/32, LoadScreen.cpp:175, SpringApp.cpp:1246, GlobalRendering.cpp:606/1305. One SDL_GL_SwapWindow (GlobalRendering.cpp:703). Default profile is compatibility (ForceCoreContext=0, :61).

Non-shared objects:
- VAO: single glGenVertexArrays funnel (VAO.cpp:21), generated lazily on first Bind. 12 owner classes: TypedRenderBuffer (15 global buffers RenderBuffers.cpp:13-27, plus font, water, projectile and BasicMeshDrawer instances), S3DModelVAO singleton, ROAM Patch (2 per patch, mapx/128 patches per axis), GroundDecalHandler, ModernSky, SkyBox, DebugCubeMapTexture, ModernInfoTexture, NanoParticles::Renderer, GL::Shapes (lazy vector), LuaVAOImpl, RmlUi CompiledGeometryData.
- FBO: 7 creation lines (FBO.cpp:250, myGL.cpp:466-467, DynWater.cpp:239, LuaFBOs.cpp:502, LuaTextures.cpp:117, RmlUi:553). 18 FBO-typed members plus 1 raw. About 10 transient scope FBOs. About 20 FBO::Unbind() call sites.
- Queries: 16 timer queries (GlobalRendering.cpp:664) plus Lua occlusion queries (LuaOpenGL.cpp:6712).
- Transform feedback, program pipelines, samplers, texture buffers: none.
- ARB assembly programs (env params are per-context): legacy water and myGL.cpp only.

Shared objects: about 58 glGenTextures sites in 30 files. Buffers via VBO::Generate and IStreamBufferConcept (StreamBuffer.cpp:83/95). Shaders (Shader.cpp:200/460). RBOs. GLsync via PutBufferLocks. 4 display-list sites (shared): GuiHandler, LuaOpenGL, glFontRenderer, GrassDrawer.

Indexed bindings: UBO 0/1 (UniformConstants.cpp:335-336), SSBO 0/1 (ModelsDataUploader.cpp:109,157). Persistent mapping is used only by TransformsUploader (SB_PERSISTENTMAP, 3 coherent slices); all other stream buffers use explicit SB_BUFFERSUBDATA. One-time context state: InitGLState, ToggleGLDebugOutput, LightHandler::Init, SetupFog. Teardown order is explicit: SpringApp::Kill, then PreKill, then ~CGlobalRendering, which destroys the window before the context.

*Open questions:*
- Is the long-term goal render-thread parallelism per window? That needs architecture B; sequential rendering is fully served by A.
- Will the two windows live on different monitors or GPUs, such as a hybrid laptop? A and B both require the same adapter and compatible pixel format.
- Must window 2 be allowed to differ in size, aspect or MSAA from window 1? Differing sizes force per-target scratch resources; differing MSAA breaks DepthBufferCopy blits.
- Should Lua draw callins (LuaRules/LuaGaia DrawWorld, which prototype 3D-only still runs for window 1) run for window 2? This sets how much Lua-created GL object handling is needed.
- Are secondary windows created and destroyed at runtime, or fixed at startup? This decides how much teardown machinery is needed.
- Does SDL_CreateWindow of a second SDL_WINDOW_OPENGL window while the primary context is current alter SDL's current-window tracking on the target platforms (Win32, X11, Wayland/EGL)? Needs an empirical check.
- In a compatibility-profile context on NVIDIA/AMD/Intel, does glBindFramebuffer/glBindVertexArray on a name generated in another context error or silently create an object? This determines how loud B's failures would be. Needs an empirical check.
- How common is PreloadModels=false in BAR deployments? It makes S3DModelVAO::UploadVBOs run at draw time and so recreate its VAO during gameplay.
- Is ForceCoreContext=1 used anywhere? Core profile removes the VAO 0/client-state fallbacks the legacy paths rely on.

### ovl
The overlay, font, GUI-hook and capture code is entirely single-target. After CWorldDrawer::Draw, CGame::Draw (Game.cpp:1530-1548) runs ten screen-space steps unconditionally on whatever context, framebuffer and viewport is current. There is no 3D-only mode: hideInterface gates only two of those steps. These steps read globalRendering->viewSize/pixelX/pixelY, the single global `camera`, one mouse, and a process-wide CInputReceiver deque. Several overlays are inside the world pass itself (DrawMiscObjects: command lines, cursor icons, selection box, DrawMapStuff, map labels, world icons) and drain per-frame queues. Per-frame GPU work (minimap->Update into its own FBO, RmlGui, DrawGenesis, IBL) sits in CGame::Draw before the target is bound, and FBO::Unbind plus camera->LoadViewport is the only target selection. Unit icon state is cached from the primary camera. Fonts depend on implicit per-context GL state (fixed-function MVP, VAOs, a consumed upload flag). Screenshot, AVI capture and RmlUi read or write framebuffer 0 of the current context. Nothing in rts/ filters SDL events by window id. For the 3D-only prototype, most of this must be skipped for secondary targets; what must stay once per frame is minimap Update, RmlGui::Update, DrawGenesis, queue draining and capture.

*Inventory:* Screen pass: 10 unconditional steps at Game.cpp:1534-1547 (screen unit icons, DrawScreenEffects, hudDrawer, debugDrawerAI, DrawInputReceivers, DrawInputText, DrawInterfaceWidgets, RmlGui::RenderFrame, mouse->DrawCursor, DrawScreenPost). hideInterface gates only Game.cpp:1580 and 1607, plus WorldDrawer.cpp:490 and the icon code.

Once-per-frame items inside CGame::Draw: RmlGui::Update (1454), UniformConstants Bind (1464), DrawGenesis (1468), TeamHighlight bracket (1512/1560), minimap->Update (1514), GenerateIBLTextures (1519), FBO::Unbind plus LoadViewport (1523-1524), video capture (1553-1557), timing and GL timestamps (1459/1562-1567).

World-pass overlays: WorldDrawer::DrawMiscObjects has 8 draws (WorldDrawer.cpp:471-492), plus selectedUnitsHandler.Draw (366) and 4 Lua callins (323, 346, 367, 440). Queue drains: lineDrawer (LineDrawer.cpp:91-92), cursorIcons (CursorIcons.cpp:66), map labels (InMapDrawView.cpp:242).

15 CInputReceiver subclasses sit in one static deque (InputReceiver.h:54-57). 296 uses of globalRendering view/pixel/win size in 47 files; MiniMap.cpp has 44 and ProfileDrawer.cpp 48.

Fonts: 2 global fonts, 3 renderer variants. Only pixelX/Y is read (glFont.cpp:864-865, 1058-1059). Matrices are implicit via gl_ModelViewProjectionMatrix, and CglFont::viewMatrix/projMatrix are unused (glFont.cpp:191-192).

Capture: Screenshot.cpp:58 and AVIGenerator.cpp:402 (Windows-only), plus Lua readbacks at LuaOpenGL.cpp:6559 and 6684. Zero windowID uses outside lib; one SDL_WINDOWEVENT handler (SpringApp.cpp:1098). RmlUi caches window size at init (RmlUi_Backend.cpp:150-157). dualScreenMode appears at 6+ overlay sites. Target resets live at Game.cpp:1523-1524 and MiniMap.cpp:1403/1816.

*Open questions:*
- GL architecture: shared contexts per window, or one context with secondary targets rendered to offscreen FBOs and blitted? This decides whether fonts, minimap, RmlUi and the RenderBuffer VAOs hit cross-context problems at all, and how screenshots, video and glReadPixels can address a target.
- Should secondary targets run LuaUI/gadget DrawWorld* callins (widget health bars and similar)? If yes, with which camera, and how do widgets learn the target id?
- Is DrawScreenEffects a 3D post-process hook (bloom/tonemapping widgets) that must run per target, or a UI hook that stays primary-only? Same question for DrawScreenPost.
- Which target owns the mouse, keyboard focus, GUI, minimap and software cursor? Is the primary always the UI window, and do secondary windows ever receive input (picking, camera control)?
- Should hideInterface become per-target? What is the default overlay set for new targets?
- Is the first-person controllee hidden in all targets or only the owner? Same for per-player or per-allyteam perspective (gu->myAllyTeam, spectatingFullView feed icons and LOS) once targets show different players.
- What are the semantics of closing, minimizing or hiding a secondary window (quit, hide, destroy), and should hidden targets skip rendering?
- How should screenshot, /createvideo and gl.ReadPixels pick a target (focused window, explicit id, primary only)? Is video capture with multiple windows supported at all?
- Should DualScreenMode remain supported alongside multi-target, or be re-expressed as two viewport targets in one window?
- Do map labels, marks and other in-world text (InMapDrawView) belong to the 3D target or are they UI that secondary targets omit?
- Vsync and pacing: one swap interval per window would halve the frame rate with two vsynced windows. Does the primary's swap pace the loop and secondary windows swap without vsync?
- When should secondary windows be created (config, command, Lua API), and what do they show before the game has loaded?

### cfg
Configuration, build, test and frame-loop plumbing are single-window by construction. CGlobalRendering owns exactly one sdlWindow/glContext (created together, GlobalRendering.cpp:518-609) plus one set of window/view globals. About 15 window-related CONFIG keys, their NotifyOnChange hooks, SaveWindowPosAndSize persistence and the SDL window-event handler (SpringApp.cpp:1095-1253) all address that one window and never read windowID. End-of-frame work (stream-buffer ring swap, fences, FrameMark, timing) is fused into SwapBuffers; vsync/DWM settings are applied once to one drawable; Draw time feeds the multiplayer lag protector. Legacy and headless builds compile the same Rendering sources, headless via SDL/GL stubs that lack window-id APIs. CI never creates a GL context, so rendering has no automated coverage. DualScreenMode (one window, two viewports) is the only existing multi-output precedent. Cheapest safe prototype: an opt-in int config defaulting to 0 (headless and safemode forced 0), one GL context made current to N windows with identical pixel format, secondary targets drawn before the main world pass with a lean present, window events filtered by windowID, and visibility computed as a union frustum.

*Inventory:* Window/context sites (rts/Rendering/GlobalRendering.cpp): 1 SDL_CreateWindow (:442), 3 SDL_GL_CreateContext (:474,:490,:515), 1 SDL_GL_SwapWindow (:703), 2 SDL_GL_MakeCurrent (:613,:624). No SDL_GL_SHARE_WITH_CURRENT_CONTEXT, SDL_GetWindowID or event.window.windowID anywhere in rts outside rts/lib. 8 GetWindow() callers outside GlobalRendering (AVIGenerator, LuaUnsyncedRead, MouseInput x2, WindowManagerHelper, HwMouseCursor x4). MakeCurrentContext callers: GlobalRendering:606,:1305; SpringApp:1246; LoadLock.h:21,:32; LoadScreen:175.
Config: about 15 window/GL keys (Fullscreen default true, WindowBorderless, XResolution[Windowed], YResolution[Windowed], WindowPosX/Y, DualScreenMode[MiniMapOnLeft], MSAALevel, VSync, DWMFlush, BlockCompositing, MinimizeOnFocusLoss, ForceSwapBuffers, DebugGL*); 12 keys in GlobalRendering NotifyOnChange (:358-371); undeclared keys throw (ConfigHandler.cpp:289).
Counts: 305 refs to globalRendering view/win/aspect/pixel/screen-matrix fields in 47 files; 39 glViewport in 21 files; 65 FBO::Unbind/bind-0/LoadViewport hits in 28 files; 8 VAO/FBO container creation sites.
Events: 11 SDL_WINDOWEVENT cases in SpringApp::MainEventHandler, 2 in MouseInput, 2 in RmlUi; none check windowID.
Builds: engine-legacy, engine-headless (-DHEADLESS plus headlessStubs), engine-dedicated (no rendering); explicit source list rts/Rendering/CMakeLists.txt. CI: amd64-linux, arm64-linux, amd64-windows (docker, gcc-13/mingw); Linux image uses Ubuntu 18.04 libsdl2-dev (SDL 2.0.8); user also builds MSVC. SDL2 only.
Tests: 22 Catch2 sources in test/engine (System/Sim only), none touch Rendering/GL; tests link explicit .cpp subsets; testCreg runs spring-headless --test-creg; validation/synctest run headless only.

*Open questions:*
- Single shared GL context with MakeCurrent per window (recommended) or one context per window with sharing? Is a render thread per window a future goal, since that is the only reason to pay for option B?
- Should Lua DrawWorld-style callins (DrawPreDecals, DrawWorldPreUnit, DrawWorld, DrawWaterPost, DrawGenesis) run for secondary targets? They query the primary camera via Spring.GetCamera*, so output would be wrong or doubled.
- How does window 2 get its independent camera in a config-only prototype: fixed offset from the main camera, a second camera controller, or a Lua-driven state? This decides how much camera work is needed before the flag is useful.
- Is the prototype limited to spectator/replay/local games (to sidestep lag-protector CPU reporting), or must it be safe in live multiplayer?
- Must the secondary window be allowed a different size or aspect from the primary, or can the prototype require identical sizes to avoid reallocating viewport-sized resources (DepthBufferCopy, water screen textures, depth copy) and to keep the shadow/env sharing simple?
- Should globalRendering->viewSizeX/winSizeX etc. become 'current target' state swapped around each target draw (smallest change, 305 references untouched) or stay primary-only while target-aware accessors are added?
- Which config surface is wanted long-term: declared CONFIG keys, a startscript/ClientSetup tag, a CLI flag, or a Lua API (Spring.CreateRenderWindow/CloseRenderWindow)? Only config is needed for the prototype; Lua is additive.
- Is macOS or Wayland support required? macOS is not in the CI matrix, and Wayland cannot honour window position or pointer warping, which affects persistence and input design.
- Should the second window default to vsync off with possible tearing, or must it be tear-free (which implies paced presents and a different frame-pacing design)?
- Are shadows, env cubemap and water reflection allowed to be shared from the primary camera in the prototype (visibly wrong outside the primary frustum), or must each target compute its own?

### diag
Diagnostics and verification are built around exactly one window, one GL context and one frame stream. GL debug output, the error queue, debug groups, GL timer queries, Tracy FrameMark, the profiler timers (including one that feeds the server lag-protector), screenshots and capture-tool detection are all process-global and assume the single sdlWindow/glContext pair in CGlobalRendering. The debug callback is installed once on the context current in PostInit. Errors are drained only inside the primary SwapBuffers, and DebugGL is off by default, so GL errors are silent unless a developer turns them on. The one state-leak detector (GL_STATE_CHECKER) has zero call sites and is context-agnostic. At least four different "default GL state" definitions exist. Per-frame caches keyed on drawFrame and camType would silently serve window-1 results to window 2. No GL-level automated verification exists: tests are pure logic, CI runs only spring-headless with all GL stubbed, and the stubs return the same pointer for every window and context. Nothing can answer "which target is current" or "did this pass leave state clean". The main work is a RenderTargetScope guard with runtime-switchable validators, per-target attribution of logs, errors, groups and stats, and a verification ladder from headless unit tests up to a software-GL smoke test.

*Inventory:* GL debug facilities:
- rts/Rendering/GL/glDebugGroup.{hpp,cpp}: SCOPED_GL_DEBUGGROUP, 25 use sites (WorldDrawer.cpp 16, Game.cpp 4, IWater.cpp 2, RoamMeshDrawer.cpp 2, GlobalRendering.cpp 1). Gated only by GLAD_GL_KHR_debug, and heap-allocates per scope.
- rts/Rendering/GL/glStateDebug.{h,cpp}: GL_STATE_CHECKER has 0 call sites and compiles only with -DDEBUG_GLSTATE (rts/CMakeLists.txt:32-34).
- Debug callback at GlobalRendering.cpp:1950-1982, installed only by ToggleGLDebugOutput (PostInit:660 and /debuggl). DebugGL defaults to false. The context always requests SDL_GL_CONTEXT_DEBUG_FLAG (:555).
- No glObjectLabel in engine C++, only Lua gl.ObjectLabel (LuaOpenGL.cpp:478).

Error and sync calls (29 hits in 10 files):
- 8 glClearErrors call sites plus its definition (myGL.cpp:733).
- Ad hoc glGetError in VBO, LuaTextures, LuaFBOs and RmlUi.
- glFinish x3 (StreamBuffer.h Kill paths) plus Lua gl.Finish.
- glFlush x7 (DynWater.cpp) plus Lua gl.Flush.

Tracy and timers:
- 1 FrameMark (GlobalRendering.cpp:713), 1 named frame set "SimFrame" (Game.cpp:1705/1823), 0 Tracy GPU zones.
- GL timer queries: glTimerQueries[16], issued at Game.cpp:1459/1567, read only at ProfileDrawer.cpp:502.

Single-target reset and assumption counts:
- ~23 textual FBO::Unbind() occurrences in 14 files.
- 39 glViewport occurrences in 21 files.
- 19 globalRendering->LoadViewport() sites outside GlobalRendering.cpp, ~9 camera LoadViewport/UpdateLoadViewport sites, 2 GeometryBuffer::LoadViewport sites.
- 296 reads of globalRendering view/win/aspect/pixel fields in 47 files.
- The `camera` macro (Camera.h:316) has ~505 uses in 85 files.
- 109 GL::State/SubState/glPushAttrib matches in 44 files.
- 0 uses of SDL windowID anywhere in rts/.
- 0 uses of SDL_GL_SHARE_WITH_CURRENT_CONTEXT.

Test and CI infrastructure:
- test/CMakeLists.txt registers ~25 add_spring_test targets. None link Rendering, and only SerializeLuaState links headlessStubs.
- CI: .github/workflows/synctest.yml:186 runs spring-headless. docker-build-v2 has no Mesa or Xvfb.
- headlessStubs: gladstub.cpp makes every GL call a no-op. sdlstub.c returns one static address for every SDL_CreateWindow and SDL_GL_CreateContext. There are no stubs for SDL_GetWindowID or SDL_GL_GetCurrentContext.

Misc:
- Watchdog HangTimeout=60s (Watchdog.cpp:24).
- Default build is RELWITHDEBINFO with -DNDEBUG, so assert() is compiled out.

*Open questions:*
- Which context topology is intended for the prototype: one GL context made current on N drawables (SDL_GL_MakeCurrent(window2, glContext)) or one shared context per window? This choice sets which validators are mandatory.
- Is it acceptable to turn GL debug output (errors, undefined behaviour, high severity only) on by default in dev and CI builds, given the always-requested debug context? Should validators be controlled by a runtime config var and console command so they work in RELWITHDEBINFO/-DNDEBUG builds, where assert() is compiled out?
- Which GPU vendors and drivers must be validated (NVIDIA, AMD, Intel, Mesa)? Cross-context visibility, query-object sharing and fence-flush semantics are partly vendor-dependent and need verifying against the GL spec and real drivers. Glad loads function pointers once (wglGetProcAddress); pointers could in principle differ per context or pixel format.
- Is a Linux CI job with Mesa llvmpipe and Xvfb acceptable to add to docker-build-v2 for a two-window smoke and screenshot comparison, or must verification stay headless-plus-manual (AGENTS.md says manual testing is required for non-Lua changes)?
- Should DbgTimingInfo, the TIMING_SWAP event and other Lua-visible diagnostics gain a target argument or stay primary-only? See coding-agents/BACKWARDS_COMPATIBILITY.md before changing a public callin signature.
- Should the 'Draw' special timer and lag-protector CPU feed cover only the primary target? Is a local-only second window expected in multiplayer sessions, where sim speed could be affected?
- Should the headless/synctest build exercise the multi-target code path with N stub targets to prove the sim hash is unaffected, or is the headless build allowed to collapse to one target?
- Is adding the RenderDoc in-app API header as an optional dependency acceptable, or should capture-boundary control be limited to a deterministic single-target dev mode?
- WorldDrawer::DrawMiscObjects (WorldDrawer.cpp:466-493) and eventHandler.DrawWorld (:323) draw overlays and Lua world callins inside the 3D pass (selection box, command queues, line drawer, cursor icons, map marks). Should the 3D-only prototype assert that none run for the secondary target? That belongs to the overlay domain but affects what the validators treat as a violation.

### sync
Sim and unsynced draw state is already cleanly shareable in one respect: one UpdateUnsynced per frame computes a single interpolation clock (globalRendering->timeOffset, drawFrame) and per-object drawPos that both windows can reuse. Everything else in the update-to-draw handoff assumes exactly one viewer and one camera. CGame::Draw runs UpdateUnsynced and then the draw, and UpdateUnsynced bakes camera-dependent results into global or per-object state. That includes unit/feature/projectile draw flags and icon state, a GL4 camera UBO, shadow matrices, grass and ROAM visibility, and the sound listener. All of it reads the static CAMTYPE_PLAYER camera. About ten draw-time caches dedupe on globalRendering->drawFrame, so a second target drawn in the same frame silently reuses window 1's culling, or is suppressed. Viewer identity (gu->myAllyTeam, spectatingFullView) is baked into persistent state in 462 references across 85 files, but the prototype can leave it identical for both windows. The frame also ends with one global SwapBuffers, one active flag, and process-global draw-mode and pass state.

*Inventory:* Frame flow: CGame::UpdateUnsynced (Game.cpp:1226-1445) is called from CGame::Draw (1448-1451). It runs camHandler->UpdateController, camera->Update, shadowHandler.Update, worldDrawer.Update, infoTextureHandler->Update, sound->UpdateListener(camera) and UniformConstants::Update, all once per frame. Draw then runs GenerateIBLTextures, FBO::Unbind plus camera->LoadViewport, worldDrawer.Draw, the screen/GUI passes and TeamHighlight.

Camera set: 6 file-static CCamera objects (CameraHandler.cpp:94). 26 CAMTYPE_PLAYER references in 14 files, 13 of them hard-coded GetCamera(PLAYER) reads in 8 files. 7 SetActiveCamera sub-pass sites.

drawFrame: 53 references in 20 files, 4 increment sites. About 10 draw-time dedupe/token sites: Patch, BasicMeshDrawer (6 reads), Grass (4), ProjectileDrawer (2), NanoParticleRenderer (2), ShieldSegmentProjectile (2), SMFGroundTextures (2) and LuaMaterial visFrame. GL timer queries use drawFrame&1.

Draw flags: one uint8 drawFlag plus previousDrawFlag per unit, feature and projectile (WorldObject.h:97-98). They are computed in three places: UnitDrawerData.cpp:418, FeatureDrawerData.cpp:114 and ProjectileDrawer.cpp:478. Projectiles also carry sortDist[3].

Viewer state: gu->myAllyTeam/myTeam/spectating* has 462 references in 85 files under rts. 90 of those are in 22 files under Rendering, and 2 are in Map.

GameDrawMode: 13 SetDrawMode call sites, 4 engine readers and 5 Lua callins that pass it.

Lua draw callins: about 28 eventHandler.Draw* dispatch sites in the world path.

Edge-triggered dirty flags consumed in GenerateIBLTextures: 4.

Sim-to-camera coupling: only 2 sites. GroundMoveType.cpp:3331 writes camera, ExplosionGenerator.cpp:453 reads it.

Defaults that matter: ROAM=1, Shadows=2, Water=REFLECTIVE, deferred model and map rendering off, UnitIconsAsUI=false.

*Open questions:*
- Will the second window use its own GL context (shared with the first) or one context switched with SDL_GL_MakeCurrent? FBOs, VAOs, queries and the update-phase GL work (info textures, geometry buffers) are context-local, so this decides several pvc and gst problems.
- Union culling (cheap, over-draws) or per-target visibility sets for the first prototype? What N and how far apart will the cameras be? That determines how much per-target CPU prepass cost is acceptable at ~10k units.
- Should shadows and the environment cubemap be enabled in the secondary window in the prototype (and fit to which camera), or disabled there?
- Should any Lua draw callins (gadget DrawWorld, per-object DrawUnit) fire for the secondary target, and does Lua need a target id from day one?
- Will a viewer other than gu's (different team or spectator mode per window) ever be required? If so, the unsynced heightmap, LOS/info textures and nano buffers need a per-viewer design now.
- Which window's camera drives audio and FPS/unit-tracking control, and how is input routed to the secondary window's camera controller?
- How should frame pacing and vsync behave for two windows on one render thread (swap interval on the secondary, the primary's swap as the time base for the camera tween)?
- What does the secondary window show during load, menu, skip or a hidden/minimised state, and may the game keep running (and keep updating) while only the secondary window is hidden?
- Is a stable drawFlag/previousDrawFlag Lua API required to stay on the primary camera's semantics, or may it change to the union?


# Detailed catalogue

## Window & GL context lifecycle

### CTX-01 - Event handlers run GL work on whichever context happens to be current
**Severity:** blocker - **Effort:** M - **Status:** unverified - **Depends on:** Window lifecycle events are applied to the primary window regardless of which window sent them

Window-event handlers and the start of each frame do GL work (InitGLState, water FBO rebuild, FBO reinit, texture/font/uniform updates) assuming the single main context is current. With two contexts the last-drawn target's context stays current into the next frame's event pump and UpdateUnsynced.

*Why it breaks:* InputHandler::PushEvents (SpringApp.cpp:931) runs before Update (940) with no MakeCurrent. CGlobalRendering::MakeCurrentContext only knows one window/context (GlobalRendering.cpp:612-614). In the event path: SIZE_CHANGED calls InitGLState (glClear, LoadViewport, state resets only on the current context, GlobalRendering.cpp:1701-1734), CGame::ResizeEvent rebuilds the water FBOs (Game.cpp:1115-1118), SHOWN/HIDDEN call FBO::GLContextReinit/Lost, and DISPLAY_CHANGED calls MakeCurrentContext(false). Then CGame::UpdateUnsynced (called from Draw, Game.cpp:1451) does GL work: CNamedTextures::Update (1381), infoTextureHandler->Update (1388), CFontTexture::Update (1415), worldDrawer.Update and the uploaders (1433-1435), UniformConstants::Update (1441). If the previous frame ended with context B current, FBOs and VAOs (non-shared container objects) get created or updated in B's context and are invalid when A uses them, giving GL_INVALID_OPERATION or incomplete framebuffers. If A is drawn last this is hidden, but B's own events would still run InitGLState on A's context.

*Evidence:*
- `rts/System/SpringApp.cpp:931` PushEvents at loop top; no context guarantee before event handlers or Update
- `rts/System/SpringApp.cpp:1144` UpdateGLConfigs/UpdateGLGeometry/InitGLState inside the event handler
- `rts/System/SpringApp.cpp:1174` FBO::GLContextReinit in SHOWN handler (1194 GLContextLost in HIDDEN)
- `rts/System/SpringApp.cpp:1246` DISPLAY_CHANGED re-makes primary context current
- `rts/Rendering/GlobalRendering.cpp:612` MakeCurrentContext is single sdlWindow/glContext
- `rts/Game/Game.cpp:1381` UpdateUnsynced GL work: CNamedTextures::Update 1381, infoTexture 1388, font 1415, uploaders 1433-1435, UBO 1441

*Direction:* Define the invariant that the primary context is current from the top of the main loop (before PushEvents) until a per-target draw scope begins. Do MakeCurrent(target) only inside an RAII RenderTargetScope that restores the primary on exit. Route B's events through the router so they never run GL code. Decide with the ctx domain which objects are shared between contexts; FBOs and VAOs are not.

### CTX-02 - MakeCurrentContext and every bind site hardwired to the primary window
**Severity:** blocker - **Effort:** M - **Status:** corrected - **Depends on:** Primary-only window and context handles in CGlobalRendering; DSN-02; GL context strategy: one context over several drawables vs context per window; TGT-01; Present path hard-wired to the single sdlWindow and assumes it is current

All context binding is MakeCurrentContext(bool clear), which always uses (sdlWindow, glContext). There is no way to make a specific window the drawable, and failures are ignored.

*Why it breaks:* Steady state. In the running game the context is bound to the primary window once and never rebound. The only binds are CreateWindowAndContext (:606) and CLoadScreen::Kill (LoadScreen.cpp:175). UpdateWindow (:1305) only rebinds when winChgFrame == drawFrame, i.e. after a window-config change (:1281, :1298-1299). The DISPLAY_CHANGED handler (SpringApp.cpp:1243-1247) is also event-driven. The "primary is current" invariant therefore holds because nothing ever changes the binding, not because it is re-established each frame.

CLoadLock is not a per-frame rebind. Its lock()/unlock() bind and clear only while the 'real' mutex is selected. CLoadLockImpl defaults needThreadSafety=false (LoadLock.h:45-47). It is switched on only for multi-threaded loading (LoadScreen.cpp:122) and off again by Kill (LoadScreen.cpp:173). GetUniqueLock picks sync[needThreadSafety] (WrappedSync.h:25), which is a noop_mutex when false (WrappedSync.h:13-16). So the `CLoadLock::GetUniqueLock()` around Draw at SpringApp.cpp:901 does no MakeCurrent(false)/MakeCurrent(true) pair after load. The lock/unlock binds matter only for the loading hand-off between the load thread and the main thread. This also means a second window must not be drawn during multi-threaded loading, since the load thread owns the context then.

Consequence for the prototype. A window-2 pass has to call SDL_GL_MakeCurrent(win2, ctx) before its GL calls and before its SDL_GL_SwapWindow. It must then restore the primary, or else SwapBuffers' unconditional SDL_GL_SwapWindow(sdlWindow) at :703 runs against a window that is not current. SDL_video.h:2035 says the context must be created with a compatible window, so window 2 must be created with identical GL attributes. Failures are silent because every return value is dropped.

I recall that SDL2 rejects a swap on a window that is not current ("window has not been made current"), but that is not verifiable in this tree, which ships only SDL headers. Treat it as an assumption.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:612` MakeCurrentContext(clear) takes no window; SDL_GL_MakeCurrent(sdlWindow, ctx|null)
- `rts/Rendering/GlobalRendering.cpp:1305` UpdateWindow -> MakeCurrentContext(false) after config change
- `rts/System/SpringApp.cpp:1246` DISPLAY_CHANGED -> MakeCurrentContext(false)
- `rts/System/LoadLock.h:21` lock() binds, unlock() clears (:32), both on sdlWindow
- `rts/Game/LoadScreen.cpp:175` CLoadScreen::Kill re-binds the main window
- `rts/Game/LoadScreen.cpp:172` after load: thread-safety off, MakeCurrentContext(false) forever (to 176)
- `rts/Rendering/GlobalRendering.cpp:703` SDL_GL_SwapWindow(sdlWindow), the only present call
- `rts/Rendering/GlobalRendering.cpp:617` DestroyWindowAndContext handles exactly one window (to 636)
- `rts/System/SpringApp.cpp:901` CLoadLock held across Draw+Swap
- `rts/Rendering/GlobalRendering.h:384` one sdlWindow and one glContext member
- `rts/System/SpringApp.cpp:906` single SwapBuffers per loop iteration
- `vclibs64/include/SDL2/SDL_video.h:2150` SDL_GL_SwapWindow(window) docs; MakeCurrent requires a compatible window (:2035)
- `rts/Rendering/GlobalRendering.cpp:606` initial bind in CreateWindowAndContext

*Direction:* Add a small CGlobalRendering::BindTarget(SDL_Window*) that calls SDL_GL_MakeCurrent(win, glContext) and logs a failure. Call it explicitly for each target pass in the frame loop: bind primary, draw and swap primary, then bind window 2, draw and swap window 2. Always bind the primary at the start of SpringApp::Update (or after the last swap) so Lua, unsynced updates, UpdateWindow and the DISPLAY_CHANGED handler keep their existing 'primary is current between frames' behaviour. Leave CLoadLock, CLoadScreen::Kill, CreateWindowAndContext, UpdateWindow and SpringApp:1246 untouched, and create window 2 only after CLoadScreen::Kill. Skip an RAII ScopedTarget for the prototype; add it later if nested targets (capture, minimap) need it. Make SDL_GL_SwapWindow take the window as a parameter, and keep the once-per-frame work in CGlobalRendering::SwapBuffers (RenderBuffer::SwapRenderBuffers, IStreamBufferConcept::PutBufferLocks, glClearErrors) outside the per-window swap. Apply the swap interval to window 2 after binding it, since SDL_GL_SetSwapInterval acts on the current drawable.

> verifier A (corrected): The core claim holds in the current tree. MakeCurrentContext(bool clear) at GlobalRendering.cpp:612-614 is `SDL_GL_MakeCurrent(sdlWindow, clear ? nullptr : glContext)` with the return value discarded, and DestroyWindowAndContext (:624) also drops it. The only call sites are the ones listed: :606, :1305, SpringApp.cpp:1246, LoadLock.h:21/:32 and LoadScreen.cpp:175. The header has exactly one `SDL_Window* sdlWindow` and one `SDL_GLContext glContext` (GlobalRendering.h:384-385). SwapBuffers calls `

> verifier B (corrected): The core need is real and unavoidable. MakeCurrentContext (GlobalRendering.cpp:612-614) only binds (sdlWindow, glContext), and SwapBuffers (:703) only swaps sdlWindow. SDL 2.28 (vclibs64/include/SDL2/SDL_version.h:61) rejects SDL_GL_SwapWindow(w) unless w is the current window, and the return is ignored, so without a window-2 bind it is never presented. Window 2's default framebuffer is likewise empty unless it is current. Every design needs it, including 'render offscreen then blit', because th

### CTX-03 - SDL window events are not scoped by windowID (close, minimize, resize, move, focus)
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** Primary-only window and context handles in CGlobalRendering; Present path hard-wired to the single sdlWindow and assumes it is current; Window and GL context are CGlobalRendering singletons with no per-target MakeCurrent; TGT-01; Secondary window creation path and pixel-format parity

SpringApp::MainEventHandler treats every SDL_WINDOWEVENT as an event of the single main window. The first secondary window already breaks quit, visibility and resize behaviour.

*Why it breaks:* All SDL_WINDOWEVENTs reach the same handlers regardless of event.window.windowID. They are pumped by InputHandler::PushEvents (InputHandler.cpp:26-29), called from the main loop (SpringApp.cpp:931) and from spring::UnfreezeSpring during loading (UnfreezeSpring.h:18). The existing handler chain is MouseInput (registered via CMouseHandler::InitStatic at SpringApp.cpp:291, MouseHandler.cpp:132, MouseInput.cpp:39), then MainEventHandler (:293), then aGui and RmlUi. None of them check the window. Consequences for a secondary window:

1. Close. Closing window 2 sets gu->globalQuit through RequestQuit (SpringApp.cpp:1249-1251, :1092). This is not a crash and may even be acceptable for a prototype.
2. Hide/minimize/show. Minimizing or hiding ANY window sets the global active=false (:1185), and Game.cpp:1471-1479 then skips drawing for ALL windows (10 ms sleep, early return for up to 30 s). Other consumers of the flag also freeze: FBO::IsReady (FBO.cpp:38, used at TextureRenderAtlas.cpp:359), CFreeController::Update (FreeController.cpp:87) zeroing velocity, MouseHandler.cpp:815 and IMouseInput::SetPos (MouseInput.cpp:202), and LuaMenuController.cpp:116. This only happens on a user minimize or hide. SHOWN/RESTORED/MAXIMIZED sets active=true (:1165) and is harmless in windowed mode.
3. Resize. SIZE_CHANGED from window 2 runs the full primary resize chain, which re-reads the PRIMARY's size (GlobalRendering.cpp:1571-1572), so it is idempotent for primary geometry. It still calls InitGLState, which does glClear on whatever drawable is current plus LoadViewport (GlobalRendering.cpp:1726-1729), and ActiveController::ResizeEvent. That kills and recreates the water (Game.cpp:1116-1118). Window creation emits one such event. Dragging window-2's border emits a stream of them, each recreating the water, which is a severe slowdown. Window-2's own size is never learned, so its viewport would go stale on resize. This part is the per-target geometry problem, not this routing problem.
4. Rml. RmlUi sets its render viewport (RmlUi_Backend.cpp:568-576) and context->SetDimensions (RmlUi_SystemInterface.cpp:208-211) to window-2's size, and ProcessMouseLeave on LEAVE (:212). This is only visible if the primary keeps its GUI.
5. Focus. Moving focus between the windows emits FOCUS_LOST for the old one. That runs ReleaseAllKeys, pushes fake mouse-ups, toggles MMB scroll, and ungrabs input (SpringApp.cpp:1204-1240) on every window switch.
6. Enter/leave. ENTER/LEAVE from any window calls mouse->WindowEnter/WindowLeave and, on LEAVE, resets mousepos to the PRIMARY view centre (MouseInput.cpp:95-110, MouseHandler.cpp:229-244).
7. Display change. DISPLAY_CHANGED from any window re-binds the primary window's GL context (SpringApp.cpp:1243-1247, GlobalRendering.cpp:612-614).

Corrections to the claim:
- MOVED does not run ResizeEvent. It only calls SaveWindowPosAndSize, which writes the primary's unchanged values to config (SpringApp.cpp:1111, GlobalRendering.cpp:1583-1604). The UpdateGL chain runs only when numDisplays > 1 && dualScreenMode (:1113-1122).
- FBO::GLContextLost does not walk "every FBO". It is fullscreen-only (:1192) and only touches FBOs with reloadOnAltTab (FBO.cpp:176), which are perlinFB (ProjectileDrawer.cpp:382), coastFBO (BumpWater.cpp:300) and dynWavesFBO (BumpWater.cpp:399).

*Evidence:*
- `rts/System/SpringApp.cpp:1249` SDL_WINDOWEVENT_CLOSE calls RequestQuit()
- `rts/System/SpringApp.cpp:1185` HIDDEN/MINIMIZED sets the global active flag
- `rts/System/SpringApp.cpp:1135` SIZE_CHANGED: UpdateGLGeometry/InitGLState/UpdateInterfaceGeometry/ResizeEvent with no window check
- `rts/Game/Game.cpp:1116` CGame::ResizeEvent recreates water on every resize
- `rts/Game/Game.cpp:1471` inactive state stalls the whole draw
- `rts/Rendering/GL/FBO.cpp:167` GLContextLost walks all FBOs when a window is hidden in fullscreen
- `rts/System/SpringApp.cpp:1098` SDL_WINDOWEVENT switch never reads event.window.windowID
- `rts/Game/Game.cpp:1105` ResizeEvent recreates water, updates minimap geometry, fires ViewResize
- `rts/Rml/Backends/RmlUi_Backend.cpp:568` SIZE_CHANGED sets render_interface viewport and winX/winY from any window
- `rts/Rml/Backends/RmlUi_SystemInterface.cpp:208` context->SetDimensions(any window's size); LEAVE -> ProcessMouseLeave (212)
- `rts/Rendering/GlobalRendering.cpp:1571` geometry read from sdlWindow only
- `rts/System/SpringApp.cpp:291` IMouseInput is registered here (via CMouseHandler::InitStatic) before MainEventHandler (293), so a router must be added before this line to run first
- `rts/System/SpringApp.cpp:1179` HIDDEN/MINIMIZED sets globalRendering->active=false (SHOWN/RESTORED sets true at 1160-1165)
- `rts/System/SpringApp.cpp:1204` FOCUS_LOST releases keys/mouse/grab (1204-1241)
- `rts/Game/Game.cpp:1115` ResizeEvent kills and recreates water (1115-1119)
- `rts/Rendering/GlobalRendering.cpp:1600` SaveWindowPosAndSize writes fixed WindowPosX/XResolutionWindowed keys
- `rts/System/Input/MouseInput.cpp:95` WINDOWEVENT_ENTER/LEAVE unfiltered (95-101)
- `rts/Rendering/GlobalRendering.cpp:573` MinimizeOnFocusLoss hint is global
- `rts/System/SpringApp.cpp:1076` RequestQuit sets gu->globalQuit
- `rts/Rendering/GlobalRendering.cpp:1726` InitGLState glClear on current drawable
- `rts/Rendering/GlobalRendering.cpp:1701` InitGLState does glClear (1727) and LoadViewport (1729)
- `rts/System/SpringApp.cpp:1095` MainEventHandler SDL_WINDOWEVENT switch with no windowID (1098-1252)
- `rts/System/Input/MouseInput.cpp:65` HandleSDLMouseEvent ignores window (65-113)
- `rts/System/Input/InputHandler.cpp:26` SDL_PollEvent loop dispatches globally
- `rts/System/Input/MouseInput.cpp:69` mousepos = event.motion.x/y regardless of which window
- `rts/System/Input/MouseInput.cpp:101` WINDOWEVENT_LEAVE recenters to the primary view center

*Direction:* Add one windowID router at the single dispatch point, with no changes to the individual handlers. The router could be a first-registered handler, or a filter in InputHandler::PushEvent. Resolve event.window.windowID (and motion/button/wheel/key/text windowID) to a render target via SDL_GetWindowFromID and look it up in the target registry.

- Primary-window events go to the existing logic unchanged.
- Secondary-window events go to a per-target handler: update that target's size, visibility and focus; on CLOSE, destroy or hide only that target (reserve RequestQuit for the primary or last window); do not run ResizeEvent, water recreation or the primary UpdateGL chain.
- Replace globalRendering->active with per-target visibility plus an any-visible aggregate for the Game.cpp:1471 sleep path.
- Consume secondary events (return true) so they do not reach MouseInput or Rml.
- Treat events with windowID 0 or an unknown windowID as global/primary. SDL_PushEvent'ed synthetic events never set windowID, and they are the stack-uninitialized events at KeyInput.cpp:188-194 (key-ups) and SpringApp.cpp:1224-1231 (mouse-ups). Dropping them would leave keys and buttons stuck after FOCUS_LOST.
- Make the router safe when no secondary window exists and before the game is loaded, since events are also pumped mid-load.

Defer secondary-window mouse and keyboard focus and picking to the input-routing and per-target geometry problems. Mark this as "risk" for the 2-window prototype rather than "blocker". Effort: S for routing and filtering, M including per-target visibility and close semantics.

> verifier B (corrected): The mechanism is real and the line evidence checks out. The only handler registered for window events as a whole, SpringApp::MainEventHandler (SpringApp.cpp:1098-1252), never looks at event.window.windowID. A grep of rts/ finds no use of windowID or SDL_GetWindowID, and exactly one SDL_CreateWindow (GlobalRendering.cpp:442). CLOSE calls RequestQuit (1249-1251). MINIMIZED/HIDDEN sets active=false (1185), and CGame::Draw then sleeps and returns early (Game.cpp:1471-1479). SIZE_CHANGED runs the pri

> verifier A (corrected): The core mechanism holds in the current tree. No windowID, SDL_GetWindowID or SDL_GetWindowFromID reference exists anywhere in rts/ outside lib/. SpringApp::MainEventHandler (SpringApp.cpp:1098-1252), IMouseInput::HandleSDLMouseEvent (MouseInput.cpp:95-111) and the RmlUi handlers (RmlUi_Backend.cpp:568, RmlUi_SystemInterface.cpp:206-215) all treat every SDL_WINDOWEVENT as belonging to the one primary window. Verified in the code: CLOSE leads to RequestQuit (:1249-1251, :1076-1093). MINIMIZED/HID

### CTX-04 - Window-creation failure paths are fatal process exits
**Severity:** risk - **Effort:** S - **Status:** unverified - **Depends on:** none

Every error in the window/context creation helpers ends in handleerror, which shows a modal box and exits the process. Reusing these helpers for a secondary window would kill the whole game if that window cannot be created.

*Why it breaks:* CreateSDLWindow, CreateGLContext and CreateWindowAndContext call handleerror(...) (:453, :470, :506, :526, :602). handleerror is a macro for ErrorMessageBox (errorhandler.h:23), which on the main thread shows a message box and then always calls ExitSpringProcess (errorhandler.cpp:85-98). Nothing returns an error to the caller. A failed second window (unsupported display, driver limit, pixel-format mismatch) should log, report to the caller and keep one window.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:451` could not create SDL-window -> handleerror
- `rts/Rendering/GlobalRendering.cpp:505` context creation failure -> handleerror
- `rts/System/Platform/errorhandler.h:23` handleerror(o,m,c,f) is ErrorMessageBox
- `rts/System/Platform/errorhandler.cpp:98` ExitSpringProcess always reached
- `rts/System/Threading/ThreadPool.cpp:390` precedent: a non-fatal opengl_error catch used for the old shared-context path

*Direction:* Give the secondary creation path a bool/optional or error-string result. Use LOG_L(L_ERROR) instead of handleerror, destroy any half-created window and leave the target table unchanged. Keep the fatal path only for the primary.

### CTX-05 - [arch B] Per-context VAO duplication across 12 VAO-owning classes
**Severity:** full - **Effort:** L - **Status:** corrected - **Depends on:** DSN-02; Single shared GL context versus one context per window; Context topology decision: one context rebound to N windows vs N shared contexts

VAOs are not shared between contexts. Every VAO owner generates its VAO lazily in the context current at first use, configures it once, and later only binds it. A second context would bind names it never generated. Not needed under A or C.

*Why it breaks:* Same core mechanism, with three amendments. (1) VBO growth replaces the buffer name. VBO::Resize builds a new VBO and swaps it in via `*this = std::move(vbo)` (VBO.cpp:206-249), and Patch::UploadStreamDrawData does `vbo = VBO{...}` (Patch.cpp:211). The VAOs are rebuilt only in the context that did the growth (S3DModelVAO reinitVAO at 3DModelVAO.cpp:173-200, Patch.cpp:226/:233, GroundDecalHandler.cpp:763). Under B, every other context's per-context VAO also goes stale whenever the shared buffer is reallocated. VAO setup must therefore be re-run on both first bind in a context and on a change of the underlying buffer name. (2) The listed evidence mixes in items that are not per-context VAO problems. GrassDrawer.cpp:253 `glGenLists` is a display list, and display lists are shared across a share group. FBO (FBO.cpp:250, ProjectileDrawer.h:160/perlinFB, DynWater.cpp:239, LuaFBOs.cpp:502, GeometryBuffer.h:60) and query objects (GlobalRendering.cpp:664) are non-shared container objects of the same kind, but a different class from VAOs, so they should be tracked in a separate "non-shared container objects" problem or the title widened. (3) "Compat profile keeps VAO 0 usable" is not a mitigation. ForceCoreContext defaults to 0 (GlobalRendering.cpp:61), but every owner binds its non-zero VAO unconditionally and draws with buffer-offset pointers (RenderBuffers.h:908). With a failed bind the draw would run on VAO 0 with no element buffer, which at best draws nothing and at worst reads the offset as a client pointer. The four empty-VAO owners (SkyBox.cpp:256, ModernSky.cpp:69, DebugCubeMapTexture.cpp:112, ModernInfoTexture.cpp:30) hold a stale non-zero id after first bind, so they suffer the same problem trivially, and their VAOs have no attribute state to rebuild.

*Evidence:*
- `rts/Rendering/GL/VAO.cpp:15` Generate() caches one id from the current context
- `rts/Rendering/GL/RenderBuffers.h:986` CondInit/InitVAO: VAO configured once at first upload (to 1048); asserts :877,905,936
- `rts/Rendering/GL/RenderBuffers.cpp:13` 15 global typed render buffers, each with its own VAO (to 27)
- `rts/Rendering/Models/3DModelVAO.cpp:148` CreateVAO builds the model VAO once (hpp:141 holds it)
- `rts/Rendering/Models/IModelParser.cpp:470` runtime S3DModelVAO::UploadVBOs from model load (to 471)
- `rts/Map/SMF/ROAM/Patch.cpp:226` InitMainVAO only when VBO recreated (Patch.h:218-219 two VAOs per patch)
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:763` VAO attribute setup only on instVBO growth (to 775)
- `rts/Rendering/GL/glExtra.cpp:377` lazy wire-box shape creation (CreateGLObjects :436-463)
- `rts/Lua/LuaVAOImpl.cpp:224` Lua VAOs recreated only when VBO ids change
- `rts/Rml/Backends/RmlUi_Renderer_GL3_Recoil.cpp:891` CompileGeometry creates persistent VAOs
- `rts/Rendering/GlobalRendering.cpp:554` compatibility profile by default
- `rts/Rendering/GL/RenderBuffers.h:778` one VAO per typed RenderBuffer; InitVAO at 1018-1048
- `rts/Map/SMF/ROAM/Patch.h:218` mainVAO/borderVAO per patch per set
- `rts/Rendering/Env/NanoParticles/NanoParticleRenderer.h:83` another VAO owner; also SkyBox.h:40, ModernSky.h:29, GroundDecalHandler.h:104, ModernInfoTexture.h:29, DebugCubeMapTexture.h:27
- `rts/Rendering/GL/RenderBuffers.h:1018` InitVAO binds a VAO created in the creating context
- `rts/Rendering/GL/VAO.cpp:21` glGenVertexArrays single creation site; VAOs not shared
- `rts/Rendering/Env/NanoParticles/NanoParticleRenderer.h:81` InstanceBuffer holds a VAO and VBO
- `rts/Rendering/Env/Particles/ProjectileDrawer.h:160` perlinFB is an FBO container object
- `rts/Rendering/GL/FBO.cpp:250` glGenFramebuffersEXT; also DynWater.cpp:239, LuaFBOs.cpp:502
- `rts/Rendering/GlobalRendering.cpp:664` timer query objects generated once in PostInit
- `rts/Rendering/WorldDrawer.cpp:104` world drawers are created under CLoadLock in the primary context
- `rts/Map/SMF/Basic/BasicMeshDrawer.cpp:43` render buffers (VAO-backed) created once
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:829` single decal VAO
- `rts/Rendering/GL/GeometryBuffer.h:60` G-buffer FBO
- `rts/Rendering/Env/GrassDrawer.cpp:253` glGenLists display list
- `rts/Rendering/GlobalRendering.cpp:346` single glContext member

*Direction:* Conditional on architecture B only. Under A or C, mark it not applicable and drop it from the prototype blocker list. It should stay a dependent child of the context-topology decision (dsn). If B is chosen: replace the raw `VAO` member with a per-context handle, for example a small map keyed by context id (not an id-as-sentinel check), plus a "buffer generation" stamp compared against the owning VBO/StreamBuffer. Re-run the attribute setup lambda when either the context-local VAO is missing or the stamp changed. Route the setup through one helper so the 12 owners (RenderBuffer, S3DModelVAO, Patch x2, GroundDecalHandler, NanoParticles::Renderer, ModernSky, SkyBox, DebugCubeMapTexture, ModernInfoTexture, GL::Shapes, LuaVAOImpl, plus RmlUi if GUI is ever drawn per target) do not each reinvent it. Since the 2-window prototype is 3D-only, RmlUi can be deferred, and LuaVAOImpl matters only for gadget DrawWorld callins. Remove the display-list evidence, and fold FBO and query objects into a separate sibling problem.

> verifier B (corrected): The mechanism is correct, but the blocker severity does not hold for the prototype. VAOs are container objects and are not shared between GL contexts, even shared ones. Code confirms this. VAO::Generate (VAO.cpp:15-22) calls glGenVertexArrays in the current context and GetId() generates lazily (VAO.h:21). TypedRenderBuffer::CondInit uses vao.GetIdRaw()>0 as its 'initialised' sentinel and configures attributes exactly once (RenderBuffers.h:986-1048). S3DModelVAO::UploadVBOs recreates its VAO only

> verifier A (corrected): Core mechanism verified in the current tree. VAO::GetId() lazily calls glGenVertexArrays in whichever context is current (VAO.h:21, VAO.cpp:15-22, Bind at :33-37), and no other file in rts/ outside rts/lib calls glGenVertexArrays or glBindVertexArray, so every owner goes through this one lazy path. Owners checked, all as claimed. RenderBuffer::InitStatic makes exactly 15 global typed buffers (RenderBuffers.cpp:12-28), each with one VAO built once in CondInit/InitVAO (RenderBuffers.h:987-1048), a

### CTX-06 - Context loss and display-change recovery cover only one window and one context
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** none

The alt-tab and display-change paths re-current only the main window and rely on process-global FBO bookkeeping lists.

*Why it breaks:* SpringApp::MainEventHandler (SpringApp.cpp:1095-1252) switches on event.window.event and never reads event.window.windowID. A grep for windowID and SDL_GetWindowID across rts/ (excluding lib) finds nothing. So every SDL_WINDOWEVENT from a second window is applied to main-window global state. (1) RECOIL_SDL_WINDOWEVENT_DISPLAY_CHANGED (1243-1247) calls globalRendering->MakeCurrentContext(false), which is SDL_GL_MakeCurrent(sdlWindow, glContext) (GlobalRendering.cpp:612-614), for the main window only. It is harmless if each target's draw re-currents its own window, but it is wrong if draw assumes the last-current window stays current. (2) SDL_WINDOWEVENT_MINIMIZED/HIDDEN (1179-1198) sets the single global globalRendering->active=false (1185) and calls sound->Iconified(true). The only writers of active are SpringApp.cpp:1165 and 1185. CGame::Draw then does spring_sleep(10ms) and skips the frame (Game.cpp:1471-1479), so hiding or minimizing window 2 would freeze rendering of window 1 too. (3) SDL_WINDOWEVENT_SIZE_CHANGED (1135-1157) re-reads sdlWindow geometry into the single globalRendering and calls activeController->ResizeEvent(), so a resize of window 2 would re-init the main window's viewport state. (4) SDL_WINDOWEVENT_CLOSE (1249-1251) calls RequestQuit(), so closing window 2 quits the whole game. Context-loss recovery proper (FBO::GLContextLost/Reinit at 1172-1195, gated on fullScreen) is a tiny, per-process, single-context path that only needs rework if the design chooses one GL context per window. Design decision (dsn): single shared context re-currented per target (the FBO lists stay valid) versus one context per window (needs per-context FBO and VAO duplication and per-context loss tracking).

*Evidence:*
- `rts/Rendering/GL/FBO.cpp:21` static activeFBOs / fboTexData
- `rts/Rendering/GL/FBO.cpp:167` GLContextLost
- `rts/System/SpringApp.cpp:1243` DISPLAY_CHANGED re-currents main window only

*Direction:* Reframe this as window-event routing by SDL windowID plus a design decision on context topology. Route SDL_WINDOWEVENT by windowID to a per-target state (active, size, display), so only the main window's events touch globalRendering. Make window 2 close just destroy that target, not RequestQuit. Keep FBO::GLContextLost/Reinit global if one shared context is chosen, and make them per-context only if one context per window is chosen. The prototype can defer the display-change and context-loss handling itself. The windowID routing for CLOSE/MINIMIZED/SIZE_CHANGED should be treated as a separate, higher-priority input/window-management problem, since it can stall or quit the main window.

> verifier A (corrected): The cited lines are real (FBO.cpp:21-22 statics, FBO.cpp:167 GLContextLost, SpringApp.cpp:1243-1247 DISPLAY_CHANGED -> MakeCurrentContext(false) -> SDL_GL_MakeCurrent(sdlWindow, glContext) at GlobalRendering.cpp:612-614). But the headline mechanism, that the process-global FBO lists are what breaks, does not hold for the likely design. There is exactly one GL context today (SDL_GL_CreateContext only at GlobalRendering.cpp:474/490/515, no share-with-current attribute anywhere). SDL can drive N wi

### CTX-07 - GL timer queries use a single ring, non-shared objects and an unbounded spin
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** Context topology (one context with N drawables vs one per window) must be decided before validation can be specified; SwapBuffers fuses frame-boundary buffer bookkeeping with presenting one window

The frame GPU timer is one pair of timestamp slots per draw frame, created on the primary context. A second target would overwrite the slot or issue it on a context that does not own the query, and the reader busy-waits without a bound.

*Why it breaks:* What the code does today (single window): CGame::Draw first calls UpdateUnsynced, which bumps globalRendering->drawFrame once per draw frame (Game.cpp:1277, drawFrame starts at 1, GlobalRendering.cpp:234). Draw then stamps FRAME_REF (idx 0) at the top (Game.cpp:1459), runs the whole frame including worldDrawer.Draw (:1526) and the GUI, and stamps FRAME_END (idx 7) at the bottom (:1567). Both stamps go to slot-set (drawFrame&1). ProfileDrawer::DrawScreen, called from inside the same Draw, reads the other slot-set, which is last frame's finished pair (ProfileDrawer.cpp:502, GlobalRendering.cpp:733). The double-buffering is therefore per draw frame, not per target.

Corrections to the claim:
1. Overwrite is design-dependent. If the multi-target loop sits around worldDrawer.Draw inside one CGame::Draw (one UpdateUnsynced, one drawFrame), the two stamps bracket all N targets once. Nothing is overwritten and the readout becomes a total-frame GPU time, which is already coherent. Overwrite or misattribution only arises if CGame::Draw (or the stamp pair) is invoked once per target. In that case UpdateUnsynced also runs per target and drawFrame++ (:1277) flips the parity per target. With 2 targets, target A would always stamp one slot-set and target B the other, and CalcGLDeltaTime would then read the OTHER target's finished pair, so the profiler would show window B's GPU time labelled as A's. That is a different failure from "lose the first target's timing".
2. The spin on a never-issued query is pre-existing and single-window, not multi-target-specific. LoadScreen.cpp:313, SelectMenu.cpp:188 and LuaMenuController.cpp:120 all bump drawFrame without calling SetGLTimeStamp, and nothing else issues stamps (grep for glQueryCounter finds only :725). So the first CalcGLDeltaTime after enabling /debug may read generated-but-never-issued names. Whether those report AVAILABLE or raise INVALID_OPERATION is driver-dependent, and an error leaves res==0 forever. Multi-target only adds a second route to the same hang: calling CalcGLDeltaTime on a context that did not generate the ids (query objects are not shared across share groups, which I recall from the GL spec's shared-objects appendix but could not verify offline), or reading a slot the other target never issued.
3. Query/context affinity only matters for a context-per-window topology. With one GL context made current against N windows (SDL_GL_MakeCurrent(window, ctx), the pattern at GlobalRendering.cpp:613), the 16 query names stay valid. The claim's own dependsOn on the context-topology decision is the right gate.
4. The Game.cpp:1458 debug-group and GlobalRendering.cpp:713 FrameMark citations are separate concerns (debug labels, and Tracy frame boundaries doubling if SwapBuffers is called per window). They are not part of the timer-query mechanism and belong in their own problems.

Prototype impact: none expected. ProfileDrawer is only instantiated by SetEnabled(true) (ProfileDrawer.cpp:58-71, /debug) and draws in DrawScreen (:560), which is GUI-pass and skipped by the 3D-only windows. The unbounded spin is therefore reachable in the prototype only with /debug on plus a non-owning context or a never-issued slot.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:725` timestamp query indexed by drawFrame parity
- `rts/Rendering/GlobalRendering.cpp:745` unbounded spin on GL_QUERY_RESULT_AVAILABLE (745-747)
- `rts/Game/Game.cpp:1459` frame ref timestamp at start of Draw; end at 1567
- `rts/Game/UI/ProfileDrawer.cpp:502` sole consumer of CalcGLDeltaTime
- `rts/Rendering/GlobalRendering.cpp:720` SetGLTimeStamp/CalcGLDeltaTime double-buffered by drawFrame (720-750)
- `rts/Rendering/GlobalRendering.cpp:664` queries created once in PostInit
- `rts/Rendering/GlobalRendering.cpp:713` FrameMark, timing and lastSwapBuffersEnd inside SwapBuffers
- `rts/Game/Game.cpp:1458` SCOPED_GL_DEBUGGROUP("Draw") with no per-view naming

*Direction:* Keep the classification as full (not a prototype blocker) and rewrite the problem around the decision on where the stamp pair lives. Option A: keep the current pair around the whole CGame::Draw and document it as total-frame GPU time. This needs no change and no per-target query. Option B: add per-target rings indexed by (targetId, drawFrame&1), issued inside the per-target loop around worldDrawer.Draw, with the ring owned by whichever context renders that target. Under either option, bound the CalcGLDeltaTime wait by iteration count or time, and bail out (returning 0 with a one-time log) on a GL error or a never-issued slot. The never-issued case can be tracked cheaply with a per-slot "issued" flag. Optionally export a per-target GPU time as a Tracy plot. Do not tie this to the prototype milestone. The hang guard (bounded wait) is worth doing early because it is a trivial S-effort fix of a latent single-window hazard, and the per-target ring is only needed once the profiler overlay is shown per target. Effort: S for the bounded wait, M for per-target rings.

> verifier A (corrected): The code facts check out. glTimerQueries is 8*2 = 16 names, generated once in PostInit on the primary context (GlobalRendering.cpp:664, context made current at :606). SetGLTimeStamp picks its slot by drawFrame&1 only (:725). CalcGLDeltaTime spins on GL_QUERY_RESULT_AVAILABLE with no bound (:745-747). The only caller is ProfileDrawer.cpp:502, and the only stamp sites are Game.cpp:1459 and :1567. No Tracy GPU zones exist in the tree. The "second target overwrites the same slot" mechanism is not tr

### CTX-08 - Loading screen, splash, LuaMenu and reload draw only to the main window via separate swap sites
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** MakeCurrentContext(false) always re-targets the main window

CLoadScreen draws to the default framebuffer of the single window, either through SpringApp::Update's swap or its own SwapBuffers inside SetLoadMessage. LoadingMT=1 hands the context between threads on sdlWindow.

*Why it breaks:* Pre-game controllers (SplashScreen, LoadScreen, LuaMenu, PreGame) are single-target by construction and each presents through its own SwapBuffers. They stay correct as long as no secondary window exists while they run. The real obligations are narrower. (1) Reload (SpringApp.cpp:787) must destroy the secondary window and its GL resources before ~CGame and before LuaOpenGL::Free/Init (817-818), or window 2 shows stale frames and holds dangling GL objects. (2) If LoadingMT=1 is kept, the loader thread binds a hidden-window context (comment at LoadScreen.cpp:125). So window 2 creation must wait until after IsDoneLoading (282-285), or LoadingMT must be disabled. Both are lifecycle rules, not blockers.

*Evidence:*
- `rts/Game/LoadScreen.cpp:325` non-MT path swaps directly (:326); SetLoadMessage -> Update+Draw (355-356)
- `rts/Game/LoadScreen.cpp:46` LoadingMT default 0, experimental (46-49); MT setup :121-136
- `rts/Game/LoadScreen.cpp:282` IsDoneLoading -> DeleteInstance: point at which the game becomes active
- `rts/System/SplashScreen.cpp:107` SwapBuffers in splash loop; events discarded (:111-113)
- `rts/Menu/LuaMenuController.cpp:119` menu draws into the main window only (drawFrame++ :120)
- `rts/System/SpringApp.cpp:787` Reload deletes game; also LuaOpenGL::Free/Init (817-818)
- `rts/Rendering/GlobalRendering.cpp:1288` QueuedFunction drained each loop for non-main SDL calls

*Direction:* Document the policy "secondary windows exist only while CGame is the active controller". Create them after CLoadScreen::DeleteInstance/FinishedLoading, and destroy them in the Reload and exit-to-menu paths before SafeDelete(game). Leave the splash, load screen and menu single-target. Treat LoadingMT as incompatible with secondary targets, but this matters little because window 2 does not yet exist while loading runs.

> verifier B (corrected): The mechanism is real. LoadScreen.cpp:325-326 swaps directly when not MT loading. SetLoadMessage (:352-356) calls Update and Draw from inside game->Load, and LoadingMT is an opt-in experimental setting. But none of this can break the prototype if window 2 is created only after the loader hands over to CGame, which is the claim's own stated policy. The pre-game controllers (LoadScreen, splash, LuaMenu) then never see a second window, so there is nothing to corrupt. "risk" overstates it: this is a

> verifier A (corrected): The code facts check out: LoadingMT defaults to 0 and is experimental (LoadScreen.cpp:46-49, 102-105). Non-MT SetLoadMessage calls Update() then Draw() (355-356), and Draw swaps directly (325-326). The splash loop swaps at SplashScreen.cpp:107 and discards all events at 111-112. Reload does SafeDelete(game) at SpringApp.cpp:787. LoadScreen's destructor makes game the active controller (77-79). None of this endangers the prototype, though. These pre-game paths only ever touch the single main wind

### CTX-09 - Model VAO, SSBO/UBO binding points and fixed-function state are per-context GL state
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** none

The model render path owns a VAO (S3DModelVAO::vao), indexed SSBO/UBO bindings (transform SSBO 0, uniforms SSBO 1, UBOs 0/1) and GL fixed-function state that are not shared across GL contexts. A second window with its own shared context would draw nothing or raise GL errors.

*Why it breaks:* Per-context GL objects and state are tied to the single glContext created in CreateWindowAndContext (GlobalRendering.cpp:587). This covers the model VAO (3DModelVAO.cpp:148-167), the SSBO/UBO indexed bindings (ModelsDataUploader.cpp:109, UniformConstants.cpp:335-336), and the persistent per-context state set once in InitGLState: glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE), glEnable(GL_DEPTH_TEST), glDepthFunc(GL_LEQUAL), seamless cubemaps, and multisample (GlobalRendering.cpp:1710-1722). The GL debug callback at :2002 is registered per context as well. A second window with its own (shared or unshared) context would see none of this: textures, buffers and shaders are shared through a share group, but the VAO, indexed bindings and the clip-control/depth state are not. glClipControl matters most, because the default [-1,1] depth range would give wrong depth output for the engine's [0,1] projections. This only applies if more than one GL context is created. With a single context, SDL_GL_MakeCurrent(window2, glContext) before each window's draw keeps all of it intact. The real work for that path is that CGlobalRendering::MakeCurrentContext (GlobalRendering.cpp:612-614) and SwapBuffers (SDL_GL_SwapWindow at :703, glBindFramebuffer(GL_READ_FRAMEBUFFER,0) at :692) are hard-wired to the single sdlWindow member. They need a window parameter, and window 2 needs the same GL attributes and pixel format (including MSAA) as window 1. This design decision should be catalogued with the context-strategy decision, not as a standalone blocker.

*Evidence:*
- `rts/Rendering/Models/3DModelVAO.hpp:122` static singleton instance; VAO member at 141
- `rts/Rendering/Models/3DModelVAO.cpp:148` CreateVAO builds the single VAO in the creating context
- `rts/Rendering/ModelsDataUploader.cpp:109` BindBufferRange to fixed SSBO binding points, performed once in Update
- `rts/Rendering/UniformConstants.cpp:335` UBO binding points set once per frame
- `rts/Rendering/Common/ModelDrawerState.cpp:46` glPushAttrib / FFP state used in model state setup
- `rts/Rendering/GlobalRendering.cpp:474` single SDL_GL_CreateContext

*Direction:* Downgrade to a design decision (dsn), non-blocking for the prototype, and merge it with the context-strategy entry. Prototype path (S): create window 2 with the same SDL_GL attributes as window 1 and keep the single glContext. Make MakeCurrentContext and SwapBuffers take a target window, and call SDL_GL_MakeCurrent(targetWindow, glContext) before each target's draw. Model VAO, SSBO/UBO bindings and FFP/InitGLState state then need no change. The cross-thread MakeCurrent handoff in LoadLock.h:21,32 shows this pattern is already used for one context. Only if shared contexts are later chosen (for example for parallel per-window render threads) is the L-cost work needed: per-context VAO re-creation (extend the reinitVAO check at 3DModelVAO.cpp:173), re-binding SSBO/UBO per context, re-running InitGLState and the debug callback per context, and glFlush/fence sync for the persistently mapped streambuffers.

> verifier B (corrected): The facts are right but the severity is not. Every cited per-context object does exist: the S3DModelVAO singleton (3DModelVAO.cpp:148-167, CreateVAO also reruns from UploadVBOs at :199-200), indexed SSBO binds in ModelsDataUploader.cpp:109 and :157, the UBO binds in UniformConstants.cpp:335-336, and the glPushAttrib FFP state in ModelDrawerState.cpp:46/74. These break only if the second window gets its own GL context in a share group. The claim concedes this itself ("With one context made curren

> verifier A (corrected): The GL facts hold in the current tree. S3DModelVAO is a singleton that owns one VAO (3DModelVAO.hpp:122,141). CreateVAO builds it once (3DModelVAO.cpp:148-167), and UploadVBOs only re-creates it when vao.GetIdRaw()==0 or a buffer grew (:173,199-200). The transform and uniforms SSBOs are bound to fixed indices by BindBufferRange at update time (ModelsDataUploader.cpp:109, binding idx 0 at ModelsDataUploader.h:52). The UBOs are bound in UniformConstants::Bind (UniformConstants.cpp:335-336), called

### CTX-10 - Startup, reload and shutdown ordering for secondary windows and their GL resources
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** DSN-06; DSN-02; CTX-08

The window and context are created once at startup and destroyed only in ~CGlobalRendering, after the game and GL singletons are gone. Secondary windows need their own place in startup, SpringApp::Reload and Kill ordering.

*Why it breaks:* Window and context lifetime is hard-wired to a single sdlWindow/glContext pair. It is created in CreateWindowAndContext (GlobalRendering.cpp:518-609, called from SpringApp::InitWindow :427-443), bound by MakeCurrentContext (:612-614, always the primary), and destroyed in DestroyWindowAndContext (:617-636) from ~CGlobalRendering (:375-387) after the game, fonts and PreKill are gone (SpringApp.cpp:1025-1057). Reload (:758-876) keeps the window and context while it deletes the game, NamedTextures and LuaOpenGL, and leaves fonts alone. Secondary windows therefore need three things. (a) Their own creation and teardown slot, with any per-target GL objects released while a context is current, before the context is deleted and before KillSDL. (b) A rule for their lifetime relative to game objects: any per-target draw hook must be gated on game != nullptr && game->IsDoneLoading(), because SpringApp::Update (:881-908) runs Draw and SwapBuffers every frame, including during menu, load and Reload, when the game is null or the load thread owns the context. (c) Per-window event routing. The event handler never reads event.window.windowID (no hits outside rts/lib). SDL_WINDOWEVENT_CLOSE at :1249-1251 calls RequestQuit() unconditionally, so closing a secondary window quits the whole game. SIZE_CHANGED, MOVED, HIDDEN and SHOWN (:1100-1198) also all mutate the primary's globalRendering state, e.g. HIDDEN sets globalRendering->active=false. Events can be pumped from the load thread (comment at :1081), but SDL window create and destroy must happen on the main thread, so a close during load has to be deferred rather than handled inline.

*Evidence:*
- `rts/System/SpringApp.cpp:263` window created before PostInit
- `rts/System/SpringApp.cpp:758` Reload never touches window or context
- `rts/System/SpringApp.cpp:1057` CGlobalRendering::KillStatic after game/fonts/camera handler
- `rts/Rendering/GlobalRendering.cpp:389` PreKill: UniformConstants/RenderBuffer/shapes/ShaderHandler
- `rts/Rendering/GlobalRendering.cpp:624` DestroyWindowAndContext: MakeCurrent(nullptr), DestroyWindow, then DeleteContext
- `rts/Rendering/GlobalRendering.cpp:375` ~CGlobalRendering: delete queries, DestroyWindowAndContext (:617-636)
- `rts/Rendering/GlobalRendering.cpp:617` one window destroyed, then context deleted (to 636)
- `rts/System/SpringApp.cpp:1025` SafeDelete(game) in Kill; context still current
- `rts/System/SpringApp.cpp:814` reload re-inits NamedTextures and LuaOpenGL (to 818)
- `rts/System/SpringApp.cpp:1172` window events not filtered per window (hide/show 1160-1198)
- `rts/Game/LoadScreen.cpp:157` CLoadScreen::Kill joins load thread and restores the main context (to 177)
- `rts/Rendering/WorldDrawer.cpp:175` CWorldDrawer::Kill order (to 204)
- `rts/System/SpringApp.cpp:1012` SaveWindowPosAndSize only for the primary
- `rts/Game/Game.cpp:1007` KillRendering -> worldDrawer.Kill, uploaders

*Direction:* Keep the direction, with these changes. Give secondary targets process lifetime and create and destroy them next to the primary, inside CreateWindowAndContext and DestroyWindowAndContext. That avoids touching Reload, and it keeps the HEADLESS order intact. Free per-target GL objects while their context is current, before SDL_GL_DeleteContext and before ~CGlobalRendering's KillSDL. Gate the secondary draw and swap on the game being alive and loaded. Route SDL window events by windowID, with a CLOSE on a secondary window closing only that target (deferred to the main thread). After destroying a window, explicitly rebind the primary (sdlWindow, glContext) instead of relying on SDL. Drop the claim that SDL_DestroyWindow leaves the context bound.

> verifier B (corrected): The mechanism is accurate: the evidence at SpringApp.cpp:263-274, 758-818 and 1012-1057 and GlobalRendering.cpp:375-395 and 617-636 all checks out. Reload never calls DestroyWindowAndContext. That function is called only from ~CGlobalRendering (:385). Kill runs game, fonts, camera handler, then KillStatic -> PreKill -> ~CGlobalRendering. The severity of "risk" is too high for the 2-window, 3D-only prototype, for three reasons.

1. Reload is out of prototype scope. Reload keeps the primary window

> verifier A (corrected): The ordering facts are accurate and verified in the current tree. Startup (SpringApp.cpp:263-274) creates the single window and context before PostInit/UpdateGLGeometry/InitGLState. Reload (:758-876) never touches the window or context. Kill (:1025-1057) deletes game, fonts, NamedTextures and camera handler, then CGlobalRendering::KillStatic (GlobalRendering.cpp:225) runs PreKill (:389-395: UniformConstants, RenderBuffer, shapes, ShaderHandler) and then ~CGlobalRendering. The destructor (:375-38

### CTX-11 - Window creation, event bursts and GL calls race with the load-thread context hand-off
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** Window lifecycle events are applied to the primary window regardless of which window sent them

During multi-threaded loading the main thread owns the GL context only while it holds CLoadLock, yet window attribute changes and window-event handlers run outside the lock. Window creation or destruction in that period can run GL or MakeCurrent on a context another thread owns.

*Why it breaks:* Hazard window is only the MT-loading phase (CLoadLock thread safety on, LoadScreen.cpp:121-173). In that phase, a window or context created, destroyed or MakeCurrent'ed on the main thread without holding CLoadLock can fail, or silently skip context sharing, because the primary context is current on the load thread. Window events for any window can also be dispatched on the load thread, because SetLoadMessage -> UnfreezeSpring -> input.PushEvents() (UnfreezeSpring.h:18, LoadScreen.cpp:338) runs SDL_PollEvent and MainEventHandler there. The SIZE_CHANGED handler then calls InitGLState/glClear/glViewport on a thread that has no GL context current. The hazard disappears after CLoadScreen::Kill, when the lock becomes a noop_mutex and the main thread owns the context permanently, and also before CLoadScreen exists. Existing hazards that are not specific to multi-window: UpdateWindow's MakeCurrentContext(false) on a config change (GlobalRendering.cpp:1305) runs before the lock; the QueuedFunction vector is unsynchronised.

*Evidence:*
- `rts/System/SpringApp.cpp:887` UpdateWindow (MakeCurrent) runs before the load lock at :901
- `rts/System/SpringApp.cpp:931` input.PushEvents outside any lock
- `rts/System/LoadLock.h:14` lock/unlock acquire and clear the primary GL context per thread
- `rts/Rendering/GlobalRendering.cpp:1204` comment: SDL calls deadlock off the main thread, hence QueuedFunction
- `rts/System/QueueToMain.h:14` static functions vector, no synchronisation
- `rts/System/Misc/UnfreezeSpring.h:18` input.PushEvents from the load thread (LoadScreen.cpp:290, 338)
- `rts/Rendering/GlobalRendering.cpp:1320` SDL calls from non-main threads are queued (main-thread-only constraint)
- `rts/System/SpringApp.cpp:1046` Kill order: CCameraHandler, CMouseHandler, CGlobalRendering (1057)
- `rts/System/SpringApp.cpp:758` Reload tears down game state while the window survives
- `rts/System/SpringApp.cpp:427` InitWindow creates the single window and context
- `rts/Game/LoadScreen.cpp:121` CglFont and CLoadLock thread-safety enabled when mtLoading
- `rts/Game/LoadScreen.cpp:175` after join: MakeCurrentContext(false) 'set last time and forever'
- `rts/Game/LoadScreen.cpp:318` LoadScreen::Draw renders luaIntro to the primary window only
- `rts/Rendering/GlobalRendering.cpp:612` MakeCurrentContext uses the single sdlWindow/glContext pair
- `rts/System/SpringApp.cpp:901` CLoadLock::GetUniqueLock around Draw

*Direction:* Prototype: create the secondary window and its context on the main thread after loading completes (after CLoadScreen::DeleteInstance/FinishedLoading, where the main thread owns the primary context). Also acceptable is creation in Init, before any load thread exists. Destroy secondary windows before Reload (SpringApp::Reload) and Kill. No lock work is needed.

Hardening for the full product:
- Make UnfreezeSpring (rts/System/Misc/UnfreezeSpring.h) call input.PushEvents() only when Threading::IsMainThread(), and otherwise just Watchdog::ClearTimer. In MT loading the main Run loop already pumps events (SpringApp.cpp:931), so the load-thread pump is redundant. In single-threaded loading the load runs on main, so behaviour is unchanged. This fixes the real off-thread event and GL dispatch.
- Defer GL work out of window-event handlers to the draw phase via dirty flags, as winChgFrame already does.
- Add a mutex to QueuedFunction::functions (and swap the vector out under lock in UpdateWindow) if more off-main enqueuers are added.
- If windows must ever be created during loading, take CLoadLock on the main thread around creation so SDL_GL_SHARE_WITH_CURRENT_CONTEXT sees the primary context.

> verifier B (corrected): The race is real but it is a sequencing hazard that the prototype avoids by timing alone, and the main cause of it already exists for the single window today. It is not a risk for the 2-window, 3D-only prototype. Verified in code:

(1) The load-thread hand-off exists only while CLoadLock thread safety is on. LoadScreen.cpp:121-122 turns it on when mtLoading. LoadScreen.cpp:173-175 turns it off and does the final MakeCurrentContext(false) in Kill. WrappedSync.h:21 then selects a noop_mutex. After

> verifier A (corrected): The code-level mechanism is real, but only in the experimental LoadingMT=1 mode. In the default configuration it never fires, so it cannot block or glitch the 2-window prototype. It is also a pre-existing per-process threading hazard, not something multi-window creates. I confirmed these points. SpringApp::Update calls UpdateWindow at :887 and takes CLoadLock at :901. SpringApp::Run calls input.PushEvents unlocked at :931. The SIZE_CHANGED handler runs InitGLState at :1146 and SDL_WINDOWEVENT_SH

### CTX-12 - Capability and limit queries are per-process, taken from the primary context
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** none

GL version, extensions, limits and feature flags are queried once at PostInit and stored globally. glad function pointers are loaded once. A second context on another GPU or with different attributes could differ.

*Why it breaks:* PostInit runs QueryVersionInfo, CheckGLExtensions, SetGLSupportFlags and QueryGLMaxVals once (:647-669). Results fill glExtensions, globalRenderingInfo and the support*/maxTex* fields. gladLoadGL() runs once after the first context (:590), and GLX::Load once (:591). On hybrid-GPU or multi-adapter systems, a window on a monitor driven by another GPU may get a context with a different renderer or limits while the engine keeps using the first set. The context version is re-read per CheckGLContextVersion only at creation.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:647` capability discovery once in PostInit
- `rts/Rendering/GlobalRendering.cpp:590` gladLoadGL once, process-global function pointers
- `rts/Rendering/GlobalRendering.cpp:757` CheckGLExtensions fills one process-wide set
- `rts/Rendering/GlobalRendering.cpp:936` QueryGLMaxVals once

*Direction:* On creating a secondary window, query GL_RENDERER and GL_VERSION and warn or reject if they differ from the primary. Keep a single global capability set otherwise.

> verifier A (corrected): The code facts check out in the current tree, but the stated hybrid-GPU scenario is weak and the proposed direction is mostly moot. PostInit (GlobalRendering.cpp:647-669) calls QueryVersionInfo, CheckGLExtensions, SetGLSupportFlags and QueryGLMaxVals exactly once. SpringApp.cpp:271 is the only caller, and I found no other callers. gladLoadGL() (:590) and GLX::Load() (:591) also run once, right after the single glContext = CreateGLContext() at :587. CheckGLContextVersion (:593, defined :1868) is 

### CTX-13 - GL debug groups can straddle context switches; captures carry no target names or object labels
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** Context topology (one context with N drawables vs one per window) must be decided before validation can be specified

SCOPED_GL_DEBUGGROUP pushes and pops on whichever context is current. Game::Draw wraps the entire draw in one group, so a per-target context switch inside it would unbalance both contexts. Groups and objects also have no target identity in RenderDoc or NSight.

*Why it breaks:* Debug groups are push/pop on the current GL context's debug-group stack, so they must open and close within one make-current span. This matters only if the design uses a separate context per window and switches contexts inside the Game.cpp:1458 "Draw" scope. If one context with multiple drawables is used, nothing is unbalanced. Static group names and the absence of engine-side glObjectLabel affect only RenderDoc/NSight readability. The heap allocation per scope (glDebugGroup.cpp:15-21) is minor.

*Evidence:*
- `rts/Rendering/GL/glDebugGroup.cpp:7` push/pop on current context, gated by KHR_debug only
- `rts/Game/Game.cpp:1458` 'Draw' group wraps the whole draw, would span a target loop
- `rts/Rendering/WorldDrawer.cpp:305` static group names, no target id
- `rts/Lua/LuaOpenGL.cpp:478` only gl.ObjectLabel; no engine-side labels

*Direction:* Do not treat this as a prototype blocker. When the per-target loop is written, put a per-target "Draw[target]" group inside each make-current span and close the outer "Draw" group before or after the loop. Optionally make the macro allocation-free and add label helpers later.

> verifier B (corrected): The mechanism is real. DebugGroupImpl pushes in its constructor and pops in its destructor on whatever context is current (glDebugGroup.cpp:7-13), and Game.cpp:1458 opens "Draw" for the whole function. It only misbehaves if a per-target make-current happens inside that scope while the targets use separate GL contexts. The consequence is limited to debug state. The other context gets a GL_STACK_UNDERFLOW error and the first context leaks one open group per frame. After about 64 frames that contex

> verifier A (corrected): The code facts hold, but the severity is overstated. glDebugGroup.cpp:7-13 pushes and pops on the current context. The macro (glDebugGroup.hpp:25) heap-allocates through GetScoped, gated only by GLAD_GL_KHR_debug (cpp:17). Game.cpp:1458 opens "Draw" at function scope. There are 38 references across 10 files, though not all are SCOPED_GL_DEBUGGROUP uses, and WorldDrawer.cpp has 16. The only glObjectLabel in rts outside lib is the Lua binding. However, the unbalanced push/pop happens only if conte

### CTX-14 - GL error-queue polling is per-context, off by default and tied to the primary swap
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** Context topology (one context with N drawables vs one per window) must be decided before validation can be specified

glGetError state is per-context. The engine drains it only in the primary SwapBuffers, and only when a debug flag is on. Ad hoc clear-then-check sites would misattribute errors that piled up in the other context.

*Why it breaks:* glGetError state is per-context. The primary SwapBuffers drains it only when a debug flag is on, apart from the loading-screen loop at SpringApp.cpp:373, which always drains. With N contexts, errors raised in the secondary context are never read or drained by that path. The ad hoc sites (VBO.cpp:229-281 and 392-410, LuaTextures.cpp:58, LuaFBOs.cpp:841, myGL.cpp:583) clear and then check on whichever context is current. So an error left in the queue by an earlier pass is discarded or blamed on the wrong operation. This only affects diagnostic accuracy, not rendering. The assert at LuaFBOs.cpp:953 drains the queue only in assert builds, so debug and release builds differ in diagnostic behaviour.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:680` drain only when clearErrors||glDebugErrors, in the primary swap path
- `rts/System/SpringApp.cpp:906` SwapBuffers(swap,false): no drain by default
- `rts/Rendering/GL/VBO.cpp:229` ad hoc clear-then-check around a resize
- `rts/Lua/LuaFBOs.cpp:953` assert with side effect on error queue

*Direction:* Do not block the prototype on this. When the context topology is settled, add a per-target drain at the end of the target's frame when validation is enabled. Remove the side-effecting assert at LuaFBOs.cpp:953.

> verifier B (corrected): The mechanism is real. GlobalRendering.cpp:680-681 drains the error queue only when clearErrors or glDebugErrors is set, and LuaFBOs.cpp:953 has a side-effecting assert. It cannot make the 2-window 3D-only prototype crash, raise GL errors or show wrong output. The ad hoc sites do clear-then-check, and the clear is what makes their check correct. An error left over from an earlier pass is discarded and cannot cause a false failure. The only effect is a log message that may blame the wrong operati

> verifier A (corrected): The mechanism is real, but it only affects diagnostics, so it cannot be a "risk" for the prototype. SwapBuffers drains errors only when `clearErrors || glDebugErrors` (GlobalRendering.cpp:680-681). `glDebugErrors` defaults to false (GlobalRendering.cpp:306) and is toggled only by /debugglerrors (UnsyncedGameCommands.cpp:1668). The main loop passes `clearErrors=false` (SpringApp.cpp:906). The ad hoc clear-then-check sites exist at VBO.cpp:229/271 and 392/400, LuaFBOs.cpp:841, LuaTextures.cpp:58 a

### CTX-15 - No context-ownership validation on CPU-side GL handle wrappers
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** none

VAO, FBO and Lua handles are bare integers with no owner context, and initialisation is inferred from 'id != 0'. Cross-context or cross-target misuse is silent.

*Why it breaks:* The 'id != 0 means initialised' sentinel (TypedRenderBuffer::CondInit RenderBuffers.h:989, S3DModelVAO::UploadVBOs 3DModelVAO.cpp:173, NanoParticles Renderer :418/:460, FBO::IsValid FBO.cpp:305) is per-process, not per-context. VAO and FBO names are generated lazily in whichever GL context is current first (VAO.cpp:21, FBO.cpp:250) and are never shared between contexts. Under a multi-context design, a second context finds id!=0, skips creating its own VAO/FBO, and binds the first context's name, which may alias an unrelated object. This is silent by default because DebugGL is off (GlobalRendering.cpp:46) and glGetError is rarely polled. Under a single-context, multi-window design (SDL_GL_MakeCurrent of the same glContext) the problem does not arise. Treat this as a debug/detection aid layered on top of the separate 'VAO/FBO are non-shareable container objects' functional problem, not a problem in its own right.

*Evidence:*
- `rts/Rendering/GL/VAO.h:21` GetId()/Generate() with no owner (mutable id :31)
- `rts/Rendering/GL/FBO.h:45` bare GLuint fboId
- `rts/Rendering/GL/RenderBuffers.h:989` VAO id used as 'initialised' sentinel
- `rts/Rendering/Models/3DModelVAO.cpp:173` reinitVAO = (vao id == 0)
- `rts/Rendering/Env/NanoParticles/NanoParticleRenderer.cpp:418` same sentinel pattern (also :460)

*Direction:* Make it conditional on choosing a multi-context architecture. If so, keep per-context VAO/FBO state (a per-target handle table or a context-id tag on VAO/FBO) instead of one id plus an id-as-sentinel. For cheap debug detection, add assert(glIsVertexArray(id)) in VAO::Bind and assert(glIsFramebuffer(fboId)) in FBO::Bind, since both queries are context-local. Do not just swap the sentinel for a bool.

> verifier A (corrected): Every cited line checks out: VAO::id is a mutable bare uint32_t with no owner (VAO.h:21-31), lazily glGenVertexArrays'd in whichever context is current on first Bind (VAO.cpp:15-22,33-37). FBO::fboId is a bare GLuint (FBO.h:45), generated in the current context in FBO::Init (FBO.cpp:250), and IsValid is just fboId != 0 && valid (FBO.cpp:302-306). The id-as-initialised sentinel exists in TypedRenderBuffer::CondInit (RenderBuffers.h:989), S3DModelVAO::UploadVBOs (3DModelVAO.cpp:173) and NanoPartic

### CTX-16 - Second window must replay the negotiated pixel format and GL attributes
**Severity:** polish - **Effort:** M - **Status:** corrected - **Depends on:** Context topology decision: one context rebound to N windows vs N shared contexts; GL context strategy: one context over several drawables vs context per window; GL context topology decision: one shared context across all windows versus one context per window; GL context and drawable topology is hardwired to one window

CreateSDLWindow negotiates MSAA and depth bits by trial and error and leaves the winning values only as sticky process-global SDL_GL attributes. A second window created later is not guaranteed to get the same pixel format.

*Why it breaks:* Replay of the pixel format is automatic in the current code. The winning MSAA/depth attributes persist as sticky SDL globals, and nothing after :449 touches them. The context-fallback loop only changes version/profile attributes, which do not affect WGL/GLX pixel-format selection. The real residual points are (a) MakeCurrentContext at :613 ignores the SDL_GL_MakeCurrent return, so any window-2 incompatibility would be silent, and (b) the winning {msaa, depth} is only logged, not stored. msaaLevel stays at the requested value, and it is later zeroed at :1721. A secondary-window path that re-ran CreateSDLWindow() using the member msaaLevel could therefore request a different format from the primary.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:432` aaLvls x zbBits fallback loop; winner kept only in locals (to 449)
- `rts/Rendering/GlobalRendering.cpp:447` chosen aa/depth only logged, not stored
- `rts/Rendering/GlobalRendering.cpp:483` CreateGLContext fallback loop changes SDL_GL_CONTEXT_* attributes (to 515)
- `rts/Rendering/GlobalRendering.cpp:540` initial colour/depth/stencil/doublebuffer attributes
- `rts/Rendering/GlobalRendering.cpp:595` SDL_GL_GetAttribute reads requested, not actual, attributes
- `rts/Rendering/DepthBufferCopy.cpp:140` glTexImage2DMultisample with msaaLevel
- `rts/Rendering/DepthBufferCopy.cpp:84` FBO::Blit from default FB into MS texture
- `rts/Rendering/GL/FBO.cpp:358` samples queried but never compared
- `rts/Rendering/GlobalRendering.cpp:914` depth bit depth probed once for the primary context
- `rts/Rendering/GlobalRendering.cpp:518` CreateWindowAndContext does SDL_Init, attributes, window, context, gladLoadGL, GLX::Load (518-609)
- `rts/Rendering/GlobalRendering.cpp:398` CreateSDLWindow with aa/zb fallback loops, log text says 'main window'
- `rts/Rendering/GlobalRendering.cpp:1721` msaaLevel mutated after window creation
- `rts/Rendering/GlobalRendering.cpp:1204` comment: SDL calls deadlock off the main thread during MT loading
- `rts/System/SpringApp.cpp:427` InitWindow is single-window
- `rts/Rendering/GlobalRendering.cpp:402` aaLvls/zbBits fallback arrays are local
- `rts/Rendering/GlobalRendering.cpp:561` msaaLevel parity fix-up
- `vclibs64/include/SDL2/SDL_video.h:2035` 'The context must have been created with a compatible window'
- `rts/Rendering/GlobalRendering.cpp:1206` QueuedFunction pattern for SDL calls from non-main threads
- `rts/Rendering/GlobalRendering.cpp:436` SDL_GL_SetAttribute inside the retry loops
- `rts/Rendering/GlobalRendering.cpp:613` SDL_GL_MakeCurrent return value ignored

*Direction:* Downgrade from blocker to polish and fold it into the context-topology entry. When creating window 2, create it with the sticky attributes untouched, or store the winning {msaa, depth} once after the primary succeeds and have CreateSDLWindow skip negotiation for secondaries. Check the SDL_GL_MakeCurrent return value in MakeCurrentContext and log SDL_GetError(). Optionally read back the actual samples and depth bits of window 2 and compare them with the primary.

> verifier B (corrected): The mechanism is mostly wrong for this codebase, and the failure it predicts is not a prototype blocker. (1) The winning pixel-format attributes ARE preserved: SDL_GL_SetAttribute state is process-global and sticky, and every SDL_GL_SetAttribute call in rts/ (excluding rts/lib) is in GlobalRendering.cpp (grep: 436-440, 484-488, 510-512, 540-558). The RGBA/depth/stencil/double-buffer attributes are set once at 540-546. MSAA and depth are only touched inside the CreateSDLWindow loop at 436-440, an

> verifier A (corrected): The central claim does not hold in the current tree, so it is not a blocker. The winning MSAA/depth combination is not lost. The SDL_GL_* pixel-format attributes are sticky process-global state, and after the negotiation loop they sit at exactly the winning values.

1. The loop at GlobalRendering.cpp:432-449 sets MULTISAMPLEBUFFERS/SAMPLES and DEPTH_SIZE, then calls SDL_CreateWindow. Both loop conditions include `newWindow == nullptr`, so the first success ends the loop with the attributes still

## GL object sharing & resource lifetime

### OBJ-01 - Per-object draw flags and transform updates are computed once per frame from the player camera frustum
**Severity:** blocker - **Effort:** L - **Status:** corrected - **Depends on:** CAM-02; RenderView definition: per-target parameters consumed by model drawers; No per-view identity to key per-frame caches, flags and tokens on; LOP-01

Units, features and projectiles get their draw flags (opaque, alpha, reflection, refraction, shadow) in the Update stage by testing the PLAYER, UWREFL and SHADOW cameras. Draw passes filter by these flags. Objects outside camera 1's frustum are not drawn in window 2.

*Why it breaks:* What the claim got right: PLAYER-camera InView at UnitDrawerData.cpp:452 and FeatureDrawerData.cpp:139, computed once per frame, gates both the pass flags and the transform upload. The prototype also hits these sub-mechanisms, which the claim missed or under-specified. (a) Icon state precedes the flag pass. UpdateUnitIconState (UnitDrawerData.cpp:330-356) sets isIcon from camera 1 only: DrawAsIconByDistance(camera->GetPos()) at :350, camera->InView at :352, and sqCamDistToGroundForIcons from camera->GetPos() at :257-264. UpdateObjectDrawFlags then skips icon units entirely (:443). A unit that is far from camera 1 but close to camera 2 therefore gets no flags and no model in window 2, even with a union of InView. The default iconLength threshold is 200^2*750 (UnitDrawerData.h:128), roughly 5.5k elmos. (b) The SHADOW and UWREFL bits come from a single camera each, derived from the single player camera. ShadowHandler::Update (ShadowHandler.cpp:128-135) builds the shadow camera from CAMTYPE_PLAYER. UniformConstants::Update (UniformConstants.cpp:186-189) refreshes the UWREFL camera from the active camera, and it runs at Game.cpp:1441, after worldDrawer.Update, so the reflection flags already lag a frame. A union of PLAYER InView does not make the SHOPAQ, SHTRAN or REFLEC flags valid for window 2's shadow map or reflection texture. Those need a per-view shadow camera and reflection camera, or must be disabled for the secondary view. (c) The feature drawAlpha fade (FeatureDrawerData.cpp:145-179) and the projectile SetSortDist(CAMTYPE_PLAYER) (ProjectileDrawer.cpp:515) are single per-object values computed against camera 1. Window 2 gets wrong fade, and wrong alpha-particle sort order (a visual glitch, not missing objects). (d) The flag pass runs under for_mt (UnitDrawerData.cpp:244, FeatureDrawerData.cpp:94, ProjectileDrawer.cpp:498), so any per-view camera list must be read-only and thread-safe. (e) The flags are exposed to Lua (GetUnitDrawFlag and GetRenderUnitsDrawFlagChanged, LuaUnsyncedRead.cpp:2329-2365), so widening them to a union or per-view value changes widget-visible semantics, which is a full-product compatibility question.

*Evidence:*
- `rts/Rendering/Units/UnitDrawerData.cpp:430` per-camType InView loop writing SO_* flags (452-469)
- `rts/Rendering/Features/FeatureDrawerData.cpp:121` same camera loop; drawAlpha from player cam distance at 145-176; transform gate at 207
- `rts/Rendering/Common/ModelDrawerData.h:225` UpdateCommon: UpdateObjectDrawFlags then transform update only if drawFlag > NODRAW and < DRICON (:227); UpdateObjectUniforms uploads drawFlag (:207)
- `rts/Rendering/Units/UnitDrawer.cpp:196` draw selection by drawFlag/HasDrawFlag (also :229-244)
- `rts/Sim/Objects/WorldObject.h:14` DrawFlags enum with bits 1,2,4,8,16,32,128; drawFlag is a single uint8 at line 97
- `rts/Rendering/Units/UnitDrawerData.cpp:244` flag update runs in for_mt workers (feature equivalent FeatureDrawerData.cpp:94)
- `rts/Rendering/Units/UnitDrawer.cpp:187` ShouldDrawOpaqueUnit / ShouldDrawAlphaUnit (223) / ShouldDrawUnitShadow (256) test only drawFlag bits
- `rts/Rendering/Units/UnitDrawer.cpp:1950` GL4 opaque loop iterates every bin object and filters with ShouldDrawOpaqueUnit
- `rts/Rendering/WorldDrawer.cpp:225` CUnitDrawer/CFeatureDrawer::UpdateStatic and projectileDrawer->UpdateDrawFlags (:227)
- `rts/Rendering/Models/ModelRenderContainer.h:64` container holds all objects per texture bin; no per-camera lists exist
- `rts/Sim/Objects/WorldObject.h:97` uint8_t drawFlag and previousDrawFlag stored per object (97-98)
- `rts/Rendering/Common/ModelDrawerData.h:224` UpdateCommon: previousDrawFlag=drawFlag then UpdateObjectDrawFlags
- `coding-agents/ENGINE_PERFORMANCE.md:7` ~10k units scale target makes the per-camera prepass cost matter
- `rts/Rendering/Common/ModelDrawerData.h:207` drawFlag uploaded to GPU uniforms
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:494` UpdateDrawFlags uses camPlayer/camUWRefl/camShadow InView (514-537)
- `rts/Rendering/Units/UnitDrawer.cpp:190` pass predicates consume the cached per-object flags (also 202-244)
- `rts/Rendering/Units/UnitDrawerData.cpp:461` SetDrawFlag(SO_OPAQUE_FLAG) stored on the unit
- `rts/Rendering/Common/ModelDrawerData.h:227` transform refreshed only if drawFlag in visible range
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:515` SetSortDist per camera type
- `rts/Game/Game.cpp:1433` worldDrawer.Update(newSimFrame) in the Update phase
- `rts/Rendering/Env/GrassDrawer.cpp:518` Update caches visible grass from CAMTYPE_PLAYER (:521-546)
- `rts/Rendering/Units/UnitDrawerData.cpp:217` icon size from global viewSize; camera-dependent icon state (:256-264, :350-352)

*Direction:* Prototype, effort S for PLAYER bits only, M if shadows and reflections must also be correct in window 2. (1) Introduce a read-only list of active view cameras and update all of them before worldDrawer.Update (Game.cpp:1430-1433). Replace the single PLAYER InView in the unit, feature and projectile flag passes with InAnyView, and use the minimum camera distance for the feature fade and for DrawAsIconByDistance. The union also drives the transform gating at ModelDrawerData.h:227 and FeatureDrawerData.cpp:207, so transformsUploader.Update (Game.cpp:1434) uploads every object any view can see. The OPAQUE, ALPHAF and REFRAC bits are otherwise view-independent (IsAlpha, IsInWater), so no bit widening is needed. A cheaper variant is to skip InView for PLAYER entirely while more than one view is active, accepting vertex-level overdraw, because the GL4 loops already submit whole bins. (2) Make icon state view-safe. For the prototype either force isIcon=false (raise UnitIconDist or skip UpdateUnitIconState when N>1), or set isIcon only if the unit is an icon in all views. (3) Shadow and reflection bits: either disable shadows and reflections for the prototype (shadow generation bit off, basic water), or compute one shadow camera and one reflection camera per view and OR the SHOPAQ, SHTRAN and REFLEC bits over them, regenerating the shadow map and reflection texture per view. (4) Make the stronger per-view filter cheap and optional. Do the per-view pass flags as a second O(N) pass just before each view's draw, recomputing drawFlag from only that view's cameras after the union transform upload. Alternatively test the active camera's InView inside the ShouldDraw* predicates. Both keep the uint8_t flags. (5) Full solution, not prototype: per-view visibility bins or flag words, widening DrawFlags and the GPU drawFlag, and defined Lua semantics for the DrawFlag API. Budget the cost for 10k objects x N views per ENGINE_PERFORMANCE.md:7.

> verifier A (corrected): The core mechanism holds in the current tree. UpdateObjectDrawFlags is invoked from UpdateCommon, called from CUnitDrawerData::Update and CFeatureDrawerData::Update (for_mt_chunk, UnitDrawerData.cpp:244, FeatureDrawerData.cpp:94). It loops camType PLAYER..<ENVMAP and tests CCameraHandler::GetCamera(camType)->InView(drawMidPos, radius) (UnitDrawerData.cpp:430-452, FeatureDrawerData.cpp:121-139). CCameraHandler::GetCamera returns a slot of the static CCamera cameras[CAMTYPE_COUNT] array (CameraHan

> verifier B (corrected): The core mechanism is real, and without a fix window 2 shows wrong output when its camera looks somewhere camera 1 does not. CUnitDrawerData::UpdateObjectDrawFlags (UnitDrawerData.cpp:430-487) and CFeatureDrawerData::UpdateObjectDrawFlags (FeatureDrawerData.cpp:121-205) set drawFlag only if CCameraHandler::GetCamera(camType)->InView(...) passes. CModelDrawerDataBase::UpdateCommon runs once per frame, via CGame::UpdateUnsynced -> worldDrawer.Update (Game.cpp:1433 -> WorldDrawer.cpp:225-227). The 

### OBJ-02 - Shadow-caster and reflection-caster sets are computed in Update() against singleton cameras
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** Shadow map is fitted to the single player-camera frustum

Which objects enter the shadow pass and the reflection pass is decided per object in Update() with three fixed cameras (PLAYER, UWREFL, SHADOW) and stored as draw flags.

*Why it breaks:* Visibility is decided once per frame in Update(), not per render pass or per view. UpdateObjectDrawFlags for units, features and projectiles reads three global cameras (PLAYER, UWREFL, SHADOW). It stores the result as a single uint8_t drawFlag on each object, plus per-camera-type sort distances on projectiles. The draw passes only filter on those flags (UnitDrawer.cpp:187-274, FeatureDrawer.cpp:61-120). Anything outside window 1's PLAYER frustum is therefore missing from window 2's main pass, which is a prototype blocker. Anything outside window 1's reflection frustum is missing from window 2's reflection pass, and anything outside window 1's shadow fit is missing from window 2's shadow pass. The UWREFL camera is also one frame behind when the flags are computed (UniformConstants.cpp:189 runs after WorldDrawer::Update), but SHADOW is current. The flag word is also published to the GPU as ObjectUniforms.drawFlag (ModelDrawerData.h:207), exposed to Lua through previousDrawFlag/drawFlag (LuaUnsyncedRead.cpp:2365), and used for transform-update gating (ModelDrawerData.h:227, FeatureDrawerData.cpp:207). All of these assume one view.

*Evidence:*
- `rts/Rendering/Units/UnitDrawerData.cpp:430` loop over camera types; flags per type at 455-481
- `rts/Rendering/Features/FeatureDrawerData.cpp:122` UWREFL skip + per-type flags (182)
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:494` camPlayer/camUWRefl/camShadow flagging and sort distances (530-548)
- `rts/Rendering/WorldDrawer.cpp:225` UpdateStatic calls that compute the flags in Update()
- `rts/Game/Game.cpp:1433` worldDrawer.Update runs before UniformConstants::Update at 1441

*Direction:* Do not widen the draw-flag bitmask for the prototype, and do not re-run flagging per view after the uploaders have run.

Not viable:
- drawFlag is a uint8_t with only bit 64 free (WorldObject.h:14-23, 97-98). It is also exposed to Lua (LuaUnsyncedRead.cpp:1380-1387, plus the GetRenderUnitsDrawFlagChanged and previousDrawFlag APIs) and uploaded as a GPU uniform (ModelsMemStorageDefs.h:14). Widening it carries API and shader compatibility risk.
- Re-running per-view flagging after the uploads is unsafe. The flag gates the transform upload (ModelDrawerData.h:221-228 calls UpdateObjectTrasform only when drawFlag is in 1..127, and FeatureDrawerData.cpp:207 does the same for features). transformsUploader.Update() and modelUniformsUploader.Update() run once per frame at Game.cpp:1434-1435 and cycle a 3-deep buffer ring (ModelsDataUploader.cpp:35, 109-110). An object visible only in window 2 would otherwise be drawn with stale or missing transforms.

Recommended prototype fix (about S to M):
- Keep a list of per-view cameras: PLAYER_i, and a UWREFL_i computed from each view's PLAYER camera before worldDrawer.Update. Compute UWREFL_i explicitly there instead of relying on UniformConstants::UpdateMatricesImpl.
- In the three UpdateObjectDrawFlags/UpdateDrawFlags paths, set a flag bit if the object is in view of any view's camera for that slot (union culling). That keeps transform upload correct, and each window just overdraws a few objects that GPU clipping discards.
- Take the minimum camera distance across views for the feature drawAlpha fade.
- Compute SHADOW flags against whatever shadow camera policy the shadow-map problem picks (union fit, or per-view shadow generation).
- Defer true per-view precision (draw-time InView with the pass camera, per-view sort distances) to post-prototype. Alpha sort order in window 2 is a known glitch until then.
- Smallest possible prototype: run with Shadows=0 and a water type that cannot draw reflection (BasicWater), which skips the SHADOW and UWREFL flags entirely and reduces this problem to the main-pass flags.

> verifier A (corrected): The mechanism holds against the current tree. UnitDrawerData::UpdateObjectDrawFlags (rts/Rendering/Units/UnitDrawerData.cpp:418-488) loops camType PLAYER..<ENVMAP, fetches the singleton camera with CCameraHandler::GetCamera(camType) (line 437) and tests cam->InView (452). It then writes SO_OPAQUE/SO_ALPHAF/SO_REFRAC (PLAYER), SO_REFLEC (UWREFL) and SO_SHOPAQ/SO_SHTRAN (SHADOW) into the single uint8_t drawFlag (lines 455-481). The feature version is the same (rts/Rendering/Features/FeatureDrawerD

> verifier B (corrected): The mechanism is real and I verified it in the code, but the claim overstates its independence and the fix direction is not viable as written. The prototype would not crash, raise GL errors or show empty output because of this alone, so it is a risk and not a blocker.

Verified:
- UnitDrawerData.cpp:430-487, FeatureDrawerData.cpp:121-205 and ProjectileDrawer.cpp:494-548 each loop over PLAYER, UWREFL and SHADOW, test InView against the single global camera slot, and store the result in one per-ob

### OBJ-03 - Unit icon state, feature fade and icon-zoom distance come from the primary camera/controller
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** OBJ-01; RenderView definition: per-target parameters consumed by model drawers; Draw-flag bitmask encodes visibility for a single player camera; Per-object draw flags are one bitmask computed against the global camera set

Whether a unit is drawn as an icon, how far features fade, and the icon-zoom distance use the active camera and the current controller. Those results are stored per object, so they cannot differ between windows.

*Why it breaks:* The distance-based state is evaluated once per frame against the PLAYER camera (the `camera` macro, i.e. the active camera) and stored on the object, so window 2 cannot get its own result. (1) Units: UpdateUnitIconState sets SO_DRICON_FLAG when the unit is farther than iconLength*sqIconDistMult from camera->GetPos() (or, via useDistToGroundForIcons, when camera-to-average-ground height is large) AND camera->InView(...) (UnitDrawerData.cpp:350-352, 490-500; default UnitIconDist 200 gives iconLength = 200*200*750, about 5477 elmos). Icon units are skipped by the model passes (UnitDrawer.cpp:199, 232). The world icon pass has no frustum cull, so a unit that is far from cam1 but close to cam2 is drawn as an icon in both windows. The InView(camera) test at :352 is a second, separate cull from the PLAYER-camera loop in UpdateObjectDrawFlags (:437-453). A unit outside cam1's frustum is never marked as an icon and gets no PLAYER flags, so it is not drawn at all in window 2. A fix for the draw-flag union problem therefore has to change :352 as well. (2) Transform side effect: UpdateCommon only calls UpdateObjectTrasform when drawFlag is greater than SO_NODRAW_FLAG and less than SO_DRICON_FLAG (ModelDrawerData.h:227; DRICON=128 is the top bit, WorldObject.h:22). An object iconified by cam1 therefore gets no transform update that frame, so a model-draw from window 2 would show a stale transform. (3) Features: in the PLAYER branch, camDist from cam1 beyond FeatureDrawDistance (default 6000) sets drawAlpha=0 with no draw flag, and between 4500 and 6000 it sets SO_ALPHAF_FLAG. Features near cam2 but more than 6000 from cam1 vanish. Features outside cam1's frustum are skipped even earlier (FeatureDrawerData.cpp:139) and keep a stale drawAlpha. drawAlpha is also read by the ground-decal fade (GroundDecalHandler.cpp:1588) and by the Lua material draw (LuaObjectDrawer.cpp:465). (4) Decals: decal visibility is keyed on drawFlag == SO_DRICON_FLAG (GroundDecalHandler.cpp:1564), so one unit-icon decision hides its decal in every window. (5) The controller-based part is narrower than the claim says. camHandler->GetCurrentController() feeds iconZoomDist and the useDistToGroundForIcons switch (UnitDrawerData.cpp:224-230, 256). iconZoomDist is consumed only on the UnitIconsAsUI path (:401, UnitDrawer.cpp:835-839). On the default world-icon path only the useDistToGroundForIcons switch matters. (6) iconRadius is written at draw time (UnitDrawer.cpp:770, 788) and read by picking (TraceRay.cpp:460), so the last window drawn wins. This is not needed for the prototype because it has no input.

*Evidence:*
- `rts/Rendering/Units/UnitDrawerData.cpp:339` UpdateUnitIconState: DrawAsIconByDistance(unit, (unit->pos - camera->GetPos()).SqLength()) and camera->InView (:350-352)
- `rts/Rendering/Units/UnitDrawerData.cpp:224` iconZoomDist computed from camHandler current controller ray
- `rts/Rendering/Units/UnitDrawer.cpp:199` ShouldDrawOpaqueUnit returns false when GetIsIcon (also 232 alpha)
- `rts/Rendering/Features/FeatureDrawerData.cpp:145` camDist from player camera; drawAlpha written at 150,156,162,172,176
- `rts/Rendering/Units/UnitDrawerData.cpp:330` UpdateUnitIconState: distance + InView -> SetIsIcon (330-356)
- `rts/Rendering/Units/UnitDrawerData.cpp:443` if GetIsIcon() continue: model flags skipped for icons
- `rts/Rendering/Units/UnitDrawerData.cpp:217` iconSizeBase from globalRendering->viewSizeX/Y
- `rts/Rendering/Units/UnitDrawer.cpp:770` unit->iconRadius written during draw (also 788); camera used at 690-700
- `rts/Game/TraceRay.cpp:460` picking uses GetIsIcon()
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:1564` decal visMult uses unit icon flag and feature drawAlpha (1564-1588)
- `rts/Sim/Units/Unit.h:257` GetIsIcon / SetIsIcon live in the shared drawFlag; iconRadius and currentIconIndex at 539-542
- `rts/Rendering/Units/UnitDrawerData.cpp:358` screen-icon variant uses camera->CalcViewPortCoordinates, writes unit->iconRadius (358-402)
- `rts/Rendering/Features/FeatureDrawerData.cpp:161` drawAlpha fade from distance to PLAYER cam (145-176)
- `rts/Game/Game.cpp:1533` UseScreenIcons icons are drawn in the screen pass
- `rts/Rendering/Units/UnitDrawerData.cpp:256` sqCamDistToGroundForIcons from camera->GetPos()
- `rts/Rendering/Units/UnitDrawerData.cpp:350` DrawAsIconByDistance and camera->InView decide SetIsIcon (350-354)
- `rts/Rendering/Units/UnitDrawerData.cpp:430` UpdateObjectDrawFlags culls with CCameraHandler::GetCamera(camType) once per frame
- `rts/Rendering/Units/UnitDrawer.cpp:754` world icon pass filters by unit->GetIsIcon()
- `rts/Rendering/Features/FeatureDrawerData.cpp:183` reflection and shadow classification depends on drawAlpha from the player branch
- `rts/Sim/Features/Feature.h:124` drawAlpha is a single unsynced float on the feature
- `rts/Rendering/LuaObjectDrawer.cpp:465` Lua material draw uses feat->drawAlpha

*Direction:* Downgrade to risk. For the prototype, treat icon and feature-fade policy as primary-view-only and document the limitation. A cheap mitigation is to set UnitIconDist very large so units draw as models, and FeatureDrawDistance very large. Note that UnitIconDist=0 makes every unit an icon, because iconLength becomes 0 and sqDist > 0 is always true. Fix the draw-flag frustum union first, and extend the InView test at UnitDrawerData.cpp:352 to the union at the same time, otherwise icons and models still vanish for cam2. Full solution: keep per-object state as the nearest-view or most-detailed result across active views (model if any view is close, icon only if all views want an icon), compute the feature fade alpha per view (shader uniform or per-view draw loop) with drawAlpha as the maximum across views, and make the iconified-unit transform update conditional on any view needing the model. Screen-icon sizing, iconZoomDist and iconRadius picking move to per-view state in the full product, with GUI per target. Effort M.

> verifier B (corrected): The mechanism is real and I verified it in code, but it is not a standalone blocker. It is mostly a facet of the draw-flag-from-one-camera problem already listed in dependsOn, and it has a trivial config workaround.

Verified:
- CWorldDrawer::Update (WorldDrawer.cpp:225-227) runs CUnitDrawer::UpdateStatic and CFeatureDrawer::UpdateStatic once per frame, before the draw passes.
- That reaches CUnitDrawerData::Update (UnitDrawerData.cpp:214-266). UpdateUnitIconState (:339-355) stores isIcon in the

> verifier A (corrected): The core mechanism holds in the current tree. UpdateUnitIconState (UnitDrawerData.cpp:330-356) sets the shared SO_DRICON_FLAG from camera->GetPos() and camera->InView at :350-352. `camera` is the CCamera::GetActive() macro (Camera.h:316), which is the PLAYER camera during CGame::UpdateUnsynced, because worldDrawer.Update() runs at Game.cpp:1433 and calls CUnitDrawer::UpdateStatic() at WorldDrawer.cpp:225. ShouldDrawOpaqueUnit and ShouldDrawAlphaUnit drop icon units at UnitDrawer.cpp:199 and :232

### OBJ-04 - Cross-context object ownership and visibility are unvalidated
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** Context topology (one context with N drawables vs one per window) must be decided before validation can be specified

If the second target uses a separate shared context, context-local objects (VAOs, FBOs, queries) are invalid there and shared textures written by the first context need flush and fence handoff. The engine has neither a handoff nor a validator that would show the failure.

*Why it breaks:* If a second shared GL context is created, textures and buffers are shared (shadow map, cubemaps, info textures, atlases, VBOs). VAOs, FBOs and query objects are not shared. The engine's VAO.cpp:21, FBO.cpp:250, DynWater.cpp:239, myGL.cpp:466-467 and GlobalRendering.cpp:664 each create one in ctx1 only. Using them in ctx2 gives GL_INVALID_OPERATION or an empty draw, and the RenderBuffers.h asserts (877, 905, 936) only check id > 0. The fix is per-context object ownership, meaning a VAO, FBO or query set per context, not just validation. The cross-context fence handoff (glFenceSync/glWaitSync) is only needed if the contexts run on different threads. For sequential rendering on one thread, glFlush before switching contexts is enough. A single context with N drawables via SDL_GL_MakeCurrent avoids the whole problem, so treat this as a consequence of the topology decision, not an independent blocker.

*Evidence:*
- `rts/Rendering/GL/VAO.cpp:21` VAOs are context-local objects
- `rts/Rendering/GL/RenderBuffers.h:877` assert only checks id > 0, not validity in the current context
- `rts/Rendering/GL/FBO.cpp:250` FBOs are context-local objects
- `rts/Rendering/GL/StreamBuffer.cpp:89` precedent for assert(glIsBuffer(id)) validators
- `rts/Rendering/Env/DynWater.cpp:609` glFlush used for single-context pacing, not cross-context handoff

*Direction:* Resolve the context topology first. Prefer one context with N drawables (SDL_GL_MakeCurrent per window), which makes this item moot. If separate contexts are required, add per-context VAO, FBO and query creation (lazy per-context caches keyed by context), and add glFlush or fences only if rendering is multi-threaded. Treat glIsVertexArray and glIsFramebuffer validation as a debug-only aid.

> verifier B (corrected): The failure mechanism is real GL behaviour, but it only exists under one topology (separate shared contexts, one per window). The engine today creates exactly one context. GlobalRendering.cpp:474-515 calls SDL_GL_CreateContext once, and SDL_GL_MakeCurrent appears only at lines 613 and 624. It never uses SDL_GL_SHARE_WITH_CURRENT_CONTEXT. The simplest prototype path is one GL context made current against N windows with SDL_GL_MakeCurrent(win2, sameCtx). In that case VAOs, FBOs and queries stay va

> verifier A (corrected): The code facts check out. VAO.cpp:21, FBO.cpp:250, DynWater.cpp:239, myGL.cpp:466-467 and GlobalRendering.cpp:664 each generate a context-local object. The RenderBuffers.h asserts at 877, 905 and 936 only test vao.GetIdRaw() > 0. glFlush appears only in DynWater (7 sites, lines 609 to 1221) and LuaOpenGL.cpp:6438, and glFinish only in StreamBuffer.h (344, 432, 521) and LuaOpenGL.cpp:6449. There are no fences. I did not verify LuaOpenGL.cpp:6712 as a glGenQueries site: a grep across rts/Rendering

### OBJ-05 - Font rendering depends on implicit per-context GL state
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** none

The font renderers use the fixed-function modelview-projection matrix, TypedRenderBuffers with a single VAO id, a GL display list (no-shader variant), and a 'needsTextureUpload' flag consumed by whichever context draws first. None of this is target-aware.

*Why it breaks:* Font drawing is not target-aware in three ways. (1) The shader renderer uses gl_ModelViewProjectionMatrix (glFontRenderer.cpp:30, 90). That is correct as long as each context's matrix stack is set before text is drawn. (2) VAO::Generate (VAO.cpp:15-22) stores one id lazily, and VAOs are never shared between GL contexts. Under shared contexts, a TypedRenderBuffer first bound in context A has a VAO id that is invalid or wrong in context B. (3) The upload path is CFontTexture::UploadGlyphAtlasTextureImpl (CFontTexture.cpp:1529-1555). It does glBindTexture then glTexImage2D on glyphAtlasTextureID, then clears needsTextureUpload. With shared contexts the texture object is shared, so the first context to draw uploads it. The other context needs a flush or fence before it samples the new contents. Without sharing, the texture does not exist in the second context at all. The display list (glFontRenderer.cpp:280-282, 360-362) is only the fallback no-shader renderer. It is compiled empty at construction and re-specified as a glScalef on each upload, and display lists are shared across shared contexts, so it is the least important of these. The text that reaches the world pass is gated behind CWorldDrawer::Draw (WorldDrawer.cpp:491 inMapDrawerView->Draw, InMapDrawView.cpp:238-241). It also appears in Lua world callins and debug drawers, and all of it is overlay content. A 3D-only prototype can skip it.

*Evidence:*
- `rts/Rendering/Fonts/glFontRenderer.cpp:30` vertex shader uses gl_ModelViewProjectionMatrix (also line 90)
- `rts/Rendering/Fonts/glFontRenderer.cpp:130` per-renderer TypedRenderBuffer (primary/outline) created in the creating context
- `rts/Rendering/GL/RenderBuffers.h:1018` InitVAO binds a VAO stored in a single member id
- `rts/Rendering/GL/VAO.cpp:15` Generate() lazily creates one id with glGenVertexArrays; not per-context
- `rts/Rendering/Fonts/glFontRenderer.cpp:280` display list textureSpaceMatrix created in the constructor (no-shader variant)
- `rts/Rendering/Fonts/glFont.cpp:673` End() calls HandleTextureUpdate (GL upload) then draws; DrawBuffered (694) does the same
- `rts/Rendering/Fonts/CFontTexture.cpp:1529` UploadGlyphAtlasTextureImpl does glTexImage2D and clears needsTextureUpload
- `rts/Game/Game.cpp:1415` CFontTexture::Update() uploads atlases from UpdateUnsynced in the primary context
- `rts/Rendering/InMapDrawView.cpp:238` font->glWorldPrint + DrawWorldBuffered from the world pass
- `rts/Lua/LuaFonts.cpp:457` Lua DrawWorldBuffered from world callins

*Direction:* Keep this as a full-product item. For the prototype, skip text drawing on secondary targets (for example, by not calling inMapDrawerView->Draw and not running Lua world callins there). Once the context-sharing model is decided, give render buffers per-context VAOs, keep atlas uploads in one designated context, and add a fence or glFlush after the upload.

> verifier B (corrected): The mechanism is real but it is not a prototype risk. In WorldDrawer.cpp:490-492, the only default world-pass text caller (inMapDrawerView->Draw, map labels) is gated by drawMapMarks and !hideInterface. It sits in the misc/overlay block next to mouse->DrawSelectionBox, guihandler->DrawMapStuff, cursorIcons and lineDrawer, all of which the 3D-only prototype must skip for the secondary view anyway. The other text callers are Lua world callins and the HAPFS debug path drawer (HAPFSPathDrawer.cpp:46

> verifier A (corrected): The mechanisms are real: the shader reads gl_ModelViewProjectionMatrix, VAOs are lazily generated with a single id, and needsTextureUpload is a single flag. Three parts of the claim are weaker than stated. First, the display list is a nearly empty, trivial case. Second, the "consumed by the wrong context" hazard only exists under one particular sharing model. Third, the world-pass text is an overlay (map labels), so the 2-window 3D-only prototype can skip it. That puts this at full, not risk.

### OBJ-06 - Per-object draw flags, icon state and visibility queries are single-view
**Severity:** full - **Effort:** L - **Status:** corrected - **Depends on:** Per-object draw flags are one bitmask computed against the global camera set; PVC-10

The Lua contract GetRenderUnits(drawMask), GetRenderUnitsDrawFlagChanged, GetUnitDrawFlag, IsUnitIcon and the SSBO ModelUniformData.drawFlag expose one drawFlag per object, computed once per frame from the player/reflection/shadow cameras. With N views the flag has no defined meaning.

*Why it breaks:* All the headline mechanics hold, with these corrections and additions.

(1) IsUnitIcon is not 'relative to the active view's camera'. unit->GetIsIcon() is bit 128 of drawFlag (Unit.h:257), stamped once per frame in CUnitDrawerData::Update. That Update uses camera->GetPos() and camera->InView (UnitDrawerData.cpp:257, 350-352). It also uses camHandler->GetCurrentController() for iconZoomDist (224-230) and globalRendering->viewSizeX/Y for iconSizeBase (217). In the screen-icon variant it uses camera->right and CalcViewPortCoordinates and caches unit->iconRadius (385-391). So it is a cached per-frame value from one camera and one viewport. Only GetVisibleUnits and IsUnitInView query the active camera at call time (LuaUnsyncedRead.cpp:2148, 1883, 1892). Documenting it as 'relative to the active view' would be wrong.

(2) Icon and model are mutually exclusive in the encoding. UpdateObjectDrawFlags does 'if (u->GetIsIcon()) continue;' (UnitDrawerData.cpp:443), so an icon unit gets only bit 128 and no model bits. A plain OR-union over views therefore cannot express 'icon in view A, model in view B'. The engine also depends on the exclusive encoding: ModelDrawerData.h:227 updates the transform only when drawFlag < SO_DRICON_FLAG, so 128|OPAQUE=129 would skip the matrix upload; GroundDecalHandler.cpp:1564 tests GetDrawFlag() == SO_DRICON_FLAG (iconOnly); LuaUnsyncedRead.cpp:2219 does f->drawFlag == SO_DRICON_FLAG. Icon state must move to per-view storage; a union cannot cover it.

(3) The same transform gate is an extra consequence in the engine-side dependency. UpdateCommon only calls UpdateObjectTrasform when drawFlag is in (0,128) or alwaysUpdateMat is set (ModelDrawerData.h:227; FeatureDrawerData.cpp:207). A unit that is visible only in window 2 has flag 0 and also keeps a stale shared transform in the SSBO, not just a skipped draw.

(4) Features carry another per-view value outside drawFlag. drawAlpha is one float, computed from player-camera distance (FeatureDrawerData.cpp:145-179) and then used by the REFL and SHADOW branches (183, 194).

(5) The SSBO exposure is slightly different from stated. The CR metadata registers composite1, not drawFlag (ModelsMemStorageDefs.cpp:15). So SetGLSLDefinition generates 'uint composite1', and Lua shaders must mask the low byte to see drawFlag (ModelsMemStorageDefs.h:11-17). I found no engine shader in cont/ that reads it (the grep for drawFlag and composite1 returned nothing), so the SSBO flag is a Lua-contract concern, not an engine-shader blocker.

(6) The change-tracking point is a design caution, not current behaviour. previousDrawFlag is rewritten once per Update, so churn would only appear if flags were recomputed per view (ModelDrawerData.h:224). Projectiles have their own parallel pair: drawFlag, previousDrawFlag and sortDist[type] (ProjectileDrawer.cpp:504-538).

*Evidence:*
- `rts/Lua/LuaUnsyncedRead.cpp:2319` GetRenderObjects / DrawFlagChanged (2319-2393) and GetRenderUnits (2431) read single drawFlag/previousDrawFlag
- `rts/Rendering/Units/UnitDrawerData.cpp:430` drawFlag computed from PLAYER/UWREFL/SHADOW cameras only (418-487); icon state from camera pos at 257 and 350-352
- `rts/Rendering/Units/UnitDrawer.cpp:196` drawFlag==0 => unit skipped by opaque (196) and alpha (229) passes
- `rts/Rendering/Models/ModelsMemStorageDefs.h:14` drawFlag uploaded to GPU in ModelUniformData, visible to Lua shaders via GetEngineModelUniformDataDef
- `rts/Lua/LuaUnsyncedRead.cpp:1905` IsUnitIcon returns unit->GetIsIcon(), a single per-unit state
- `rts/Lua/LuaUnsyncedRead.cpp:2354` GetRenderObjectsDrawFlagChanged compares previousDrawFlag (2335-2379)
- `rts/Lua/LuaUnsyncedRead.cpp:2499` ClearUnitsPreviousDrawFlag (2499-2513)
- `rts/Sim/Objects/WorldObject.h:98` previousDrawFlag member
- `coding-agents/BACKWARDS_COMPATIBILITY.md:3` compat is a weighed constraint
- `rts/Lua/LuaUnsyncedRead.cpp:2293` InView on the active camera at call time
- `rts/Lua/LuaUnsyncedRead.cpp:2329` drawFlag mask exposed to Lua

*Direction:* Fold the engine half into the existing per-object bitmask problem and keep this entry for the Lua and ABI contract only, at severity full.

Engine side (needed for the prototype): compute the flags once per frame in UpdateObjectDrawFlags as a union over all active views' PLAYER cameras, plus the shared UWREFL and SHADOW cameras. Icon state follows the same rule: a unit is isIcon only if every view classifies it as an icon, and it gets model bits if any view wants a model. Then both views draw the union set and GPU clipping discards the rest. This has no per-view storage cost and leaves previousDrawFlag change tracking untouched, because Update still runs once per frame. Per-view CPU culling (for example a per-object uint8 viewMask, or a per-view InView test at draw time) is an optimisation to add later.

Lua side (after the prototype): document that drawFlag, GetRenderUnits, GetRenderUnitsDrawFlagChanged and previousDrawFlag mean 'union over all views'. Document that IsUnitIcon, IsUnitInView, GetVisibleUnits and ParseDrawUnit are relative to the active view at call time (the global `camera` pointer, which the draw loop must switch per view). Add GetRenderUnits(drawMask, viewID) or a viewMask in the SSBO only once a view-identity API exists.

Effort: S for the union flags (once the camera set exists), M for per-view masks and a Lua API.

> verifier A (corrected): The core mechanism is real in the current tree. CUnitDrawerData::UpdateObjectDrawFlags (UnitDrawerData.cpp:418-488) loops camType from PLAYER(0) to below ENVMAP(3) and writes one per-object byte, WorldObject::drawFlag (WorldObject.h:97). CModelDrawerDataBase::UpdateCommon copies it into previousDrawFlag once per Update (ModelDrawerData.h:224-225). The same byte is uploaded as the low byte of ModelUniformData.composite1 (ModelDrawerData.h:207). CUnitDrawer::ShouldDrawOpaqueUnit and ShouldDrawAlph

> verifier B (corrected): The mechanism is real, but the claim is over-scoped as a 'risk' for the 2-window, 3D-only prototype.

What is real:
- UnitDrawerData.cpp:418-488 builds one drawFlag per unit from CAMTYPE_PLAYER..<ENVMAP (the loop at :430; enum at Camera.h:16-19). Each view-gating `continue` (noDraw, isIcon, InVoid, LOS, `cam->InView`) leaves the flag at 0.
- Icon state is decided once per frame from the global `camera`: UpdateUnitIconState at :350-352 and :257, UpdateUnitIconStateScreen at :386-389. The result i

### OBJ-07 - ROAM process-wide static pools and flags block per-target mesh instances
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** none

Trinode pools, pool-size counters and the forced-retessellation flags are statics, so a second CRoamMeshDrawer would stomp the first one's memory and state.

*Why it breaks:* Pools, counters and force flags are process-wide statics. `pools` is a file-static std::array indexed only by shadowPass (Patch.cpp:31), and Patch::Tessellate rebinds curTriPool from it on every call (Patch.cpp:603). CTriNodePool::CUR_POOL_SIZE and MAX_POOL_SIZE are `static inline` (Patch.h:91-92). CRoamMeshDrawer::forceNextTesselation is a class-static (RoamMeshDrawer.h:89, RoamMeshDrawer.cpp:45), and ForceNextTesselation() is a static function. It is called from the constructor (RoamMeshDrawer.cpp:62) and from Lua (LuaUnsyncedCtrl.cpp:4397), where the roamMeshDrawer pointer is only used as an existence check. The ctor calls InitPools for both sets (RoamMeshDrawer.cpp:54-56), which does pools[i].Reset() (nextTriNodeIdx = 0) and tris.resize() (Patch.cpp:39-40, :84). At the same size no reallocation occurs, so a second instance's allocations overwrite the nodes the first instance's patches point to, and its forced Reset() zeroes the shared allocation index. Patches keep raw TriTreeNode pointers into the pool (Patch.h:48-53, Patch.cpp:99-100). The first instance's next incremental (non-forced) Tessellate or GenerateIndices would then walk overwritten trees. One drawer's Update() clears the shared forceNextTesselation[shadowPass] at RoamMeshDrawer.cpp:448, so an instance can miss a Lua- or ctor-forced retessellation. ResetAll(shadowPass) grows only its own pool (Patch.cpp:63). CUR_POOL_SIZE is a single shared counter, so one set's growth skews the other set's next doubling. All of this only bites if per-target instances are chosen. Fix: move pools, counters and force flags into per-mesh-set instance members, which also costs 2 sets x 96 MiB per extra target (about 192 MiB), and share the identical per-patch vertex VBO across patches and instances.

*Evidence:*
- `rts/Map/SMF/ROAM/Patch.cpp:31` static pools array shared by all CRoamMeshDrawer instances
- `rts/Map/SMF/ROAM/Patch.cpp:33` InitPools resets/resizes shared pools and mutates static CUR/MAX_POOL_SIZE
- `rts/Map/SMF/ROAM/Patch.h:89` NEW_POOL_SIZE=1<<21 nodes; CUR/MAX_POOL_SIZE are static inline
- `rts/Map/SMF/ROAM/RoamMeshDrawer.h:89` static forceNextTesselation[MESH_COUNT]
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:54` constructor calls InitPools for both sets
- `rts/Map/SMF/ROAM/Patch.cpp:603` Tessellate binds curTriPool = CTriNodePool::GetPool(shadowPass)
- `rts/Map/SMF/SMFGroundDrawer.cpp:163` exactly one meshDrawer per ground drawer (SwitchMeshDrawer)

*Direction:* Do not treat this as a free-standing blocker. Gate it on the design decision 'one ROAM mesh per target' versus 'one shared ROAM mesh' versus 'ROAM=0 for the prototype'. If per-target ROAM is chosen, make CTriNodePool, CUR/MAX_POOL_SIZE and forceNextTesselation per-instance and size the pools per target. A 192 MiB pool cost per target suggests starting smaller, since pool growth already exists (ResetAll). Also share the constant per-patch vertex data. If the prototype uses ROAM=0 (BasicMeshDrawer) or a shared instance, this item drops to full.

> verifier B (corrected): The static-state mechanism is real but is not a prototype blocker. All of the code facts check out: Patch.cpp:31 holds one static pools array indexed by shadowPass; InitPools (Patch.cpp:33-51) resets and resizes pools[shadowPass] and writes the static CUR_POOL_SIZE/MAX_POOL_SIZE (Patch.h:91-92); the CRoamMeshDrawer constructor calls InitPools for both sets (RoamMeshDrawer.cpp:54-56); forceNextTesselation is a class-static (RoamMeshDrawer.h:89, .cpp:45). It also holds that a second live CRoamMesh

> verifier A (corrected): The core mechanism holds: pools, the forceNextTesselation flags and the CUR/MAX_POOL_SIZE counters are all process-wide statics, so two live CRoamMeshDrawer instances would corrupt each other. Three parts of the claim are wrong or overstated. (1) 'Blocker' is too strong. The current tree never has two instances; SwitchMeshDrawer deletes the old one before building the new one (SMFGroundDrawer.cpp:175, :187), and the ground drawer is a single object (SMFReadMap.cpp:1029). The prototype only hits 

### OBJ-08 - Model drawers rely on process-wide statics and strictly sequential non-nested passes
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** none

The active drawer and state, GL4 mode caches, Lua material bins, pass flags and function-local static buffers are globals. They are only safe if views are drawn one after another and never nested or threaded.

*Why it breaks:* The model drawers hold process-wide mutable state: the static modelDrawer, modelDrawerState, mtModelDrawer and implStack (ModelDrawer.h:164-180); the cached scm, ssm, smm and modelShader on the singleton IModelDrawerState (ModelDrawerState.hpp:132-137); LuaObjectDrawer's inDrawPass, inAlphaBin, binObjTeam and the shared luaMatHandler bins (LuaObjectDrawer.h:97-113, .cpp:332-369); and function-local static scratch. Drawer passes must therefore not re-enter each other or run on other threads. That is already true today: the engine runs shadow, reflection, refraction and main passes strictly one after another, and nests whole water sub-views at pass boundaries (WorldDrawer.cpp:437 -> IWater.cpp:139-223). Rendering two windows by running the full CWorldDrawer::Draw once per window keeps the invariant with no change. Almost all the scratch state is cleared at entry or re-asserted at pass Enable, so it does not leak between views. The invariant is pass-level non-reentrancy, not view-level non-nesting, and it only needs to be documented and asserted. The one state that survives between sequential views and keys off the global frame counter is ProjectileDrawer's alphaRange cache (ProjectileDrawer.cpp:889, 954), which needs per-view invalidation. That belongs with the view-dependent caches problem.

*Evidence:*
- `rts/Rendering/Common/ModelDrawer.h:152` Push/Pop swap static drawer/state/mt flag
- `rts/Rendering/LuaObjectDrawer.h:97` static inDrawPass, binObjTeam, deferred flags
- `rts/Rendering/Units/UnitDrawer.cpp:2053` function-local static std::map buckets reused across calls
- `rts/Rendering/Common/ModelDrawerState.hpp:132` cached scm/ssm/smm modes on the singleton state
- `rts/Rendering/Env/Particles/ProjectileDrawer.h:182` member scratch vectors shared by all passes
- `rts/Sim/Projectiles/ExpGenSpawnable.cpp:143` thread_local fill target, single primary RB
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:1052` unsortedParticles reused across shadow and alpha passes
- `rts/Lua/LuaOpenGL.h:143` static color/drawMode/prevDrawMode/safeMode/resetMatrixFunc (143-152)
- `rts/Lua/LuaOpenGL.cpp:606` assert(drawMode == DRAW_NONE) in EnableCommon
- `rts/Lua/LuaOpenGL.cpp:6762` static bool running in RunQuery
- `rts/Lua/LuaContextData.h:118` glMatrixTracker is per Lua state, push/popped per callin (LuaHandle.cpp:390-408)

*Direction:* Keep severity polish. Reword the contract as 'no drawer pass may start while another is active; whole views may only be interleaved at pass boundaries, as water already does'. Enforce it with an assert at pass entry, for example inDrawPass or a pass-depth counter in Setup*Drawing. Do not claim sequential is sufficient: fold the drawFrame-keyed alphaRange reuse (and any other drawFrame-keyed cache) into the per-view cache invalidation problem, for example by keying on a view id plus frame. Revisit threading or true nesting only if per-view threads are wanted later.

> verifier A (corrected): The facts and all line citations check out: the drawer statics and Push/Pop stack, the cached scm/smm/ssm, the LuaObjectDrawer statics, the function-local statics, and the LuaOpenGL statics. But the framing is partly wrong, and the evidence list mixes in items that are self-resetting scratch or mitigations. (1) The engine already nests views at pass boundaries. CWorldDrawer::DrawAlphaObjects calls water->UpdateWater(game) between the below-water and above-water alpha halves (WorldDrawer.cpp:437)

## Global GL state, viewport & framebuffer resets

### GST-01 - Viewport restore idioms assume a single main viewport
**Severity:** blocker - **Effort:** M - **Status:** unverified - **Depends on:** DSN-03; CAM-03; CAM-04; GST-03; Decision: render straight into each window's default framebuffer or into offscreen per-target FBOs; FBO wrapper cannot save and restore bindings and keeps process-wide static state

After offscreen passes, 19 external sites call globalRendering->LoadViewport() to restore 'the' viewport, 2 call GeometryBuffer::LoadViewport(), 3 call camera->LoadViewport() and 5 use CCamera::UpdateLoadViewport. There are about 38 glViewport sites in total.

*Why it breaks:* Correct only if the registers always describe the target currently being drawn. A pass executed outside an ActivateTarget scope (info-texture/atlas/perlin updates run in UpdateUnsynced) restores the primary viewport, and any in-draw pass that runs after a stray switch leaves the rest of window 2's rendering in window 1's rect. The two idioms also disagree: GeometryBuffer::LoadViewport uses (0,0,viewSize) whereas globalRendering uses (viewPos,viewSize).

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:2018` CGlobalRendering::LoadViewport uses viewPos/viewSize of the primary window
- `rts/Rendering/GL/GeometryBuffer.cpp:213` GeometryBuffer::LoadViewport = glViewport(0,0,viewSizeX,viewSizeY); GetWantedSize also uses viewSize (208-210)
- `rts/Map/SMF/SMFGroundDrawer.cpp:296` globalRendering->LoadViewport() right before DrawGroundPostDeferred (304)
- `rts/Rendering/LuaObjectDrawer.cpp:536` LoadViewport() after the *PostDeferred callin fired at 533; g-buffer LoadViewport at 495
- `rts/Rendering/Env/BumpWater.cpp:986` restore sites at 572, 804, 883, 986; Grass 973; DynWater 519; Projectile 1359
- `rts/Map/SMF/SMFReadMap.cpp:756` shading texture pass restores primary viewport
- `rts/Game/Game.cpp:1524` camera->LoadViewport() after FBO::Unbind(), the 'restore default target' idiom
- `rts/Rendering/ShadowHandler.cpp:656` prvCam->Update() restores the camera after the shadow pass; same pattern at BumpWater.cpp:1038, AdvWater.cpp:350-351, DynWater.cpp:485-492
- `rts/Game/Camera.cpp:159` Update() ends in LoadMatrices() (GL FFP write, Camera.cpp:289-297)
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:1359` one of ~16 globalRendering->LoadViewport() reset sites (also GrassDrawer.cpp:973, LuaObjectDrawer.cpp:536, BumpWater.cpp:572/804/883/986, SMFGroundDrawer.cpp:296, SMFReadMap.cpp:756, DynWater.cpp:519)
- `rts/Rendering/Env/BumpWater.cpp:571` water code in the same passes also calls FBO::Unbind + globalRendering->LoadViewport (571-572, 804, 883, 986)
- `rts/Rendering/Env/DynWater.cpp:519` globalRendering->LoadViewport() after refraction (422 Unbind, 486 prvCam viewport)
- `rts/Rendering/GL/GeometryBuffer.cpp:216` GeometryBuffer::LoadViewport uses globalRendering->viewSize
- `rts/Game/Camera.cpp:153` camera viewport taken from globalRendering in CCamera::Update (UpdateViewPort at 155)
- `rts/Rendering/Env/BumpWater.cpp:572` UpdateWater ends with FBO::Unbind + globalRendering->LoadViewport (also 804, 883, 986)
- `rts/Rendering/Env/AdvWater.cpp:351` prvCam->LoadViewport(); FBO::Unbind() at 353
- `rts/Rendering/Map/InfoTexture/Modern/ModernInfoTexture.cpp:34` RunFullScreenPass: FBO::Unbind + globalRendering->LoadViewport (also Los.cpp:94, Combiner.cpp:144)
- `rts/Rendering/Env/GrassDrawer.cpp:973` grass billboard FBO pass restores primary viewport (FBO::Unbind at 979)
- `rts/Rendering/Textures/TextureRenderAtlas.cpp:528` fbo.DetachAll, FBO::Unbind, globalRendering->LoadViewport (530)
- `rts/Rendering/GL/FBO.cpp:335` Unbind() = glBindFramebufferEXT(0)
- `rts/Map/SMF/SMFReadMap.cpp:755` shadingFBO unbind + globalRendering viewport
- `rts/Game/Game.cpp:1522` FBO::Unbind(); camera->LoadViewport() immediately before worldDrawer.Draw()
- `rts/Lua/LuaOpenGL.cpp:528` ResetGLState: 'FIXME glViewport depends on the mode'; EnableCommon/DisableCommon push/pop GL_VIEWPORT_BIT only (609, 625)

*Direction:* Replace them with RenderTarget::LoadViewport() on the current target (the one-line mechanical change). Longer term, use a scoped viewport/scissor guard in GL::State instead of 'restore to main'. Unify the GeometryBuffer variant.

### GST-02 - [arch B] One-time GL context state initialisation is not replicated to new contexts
**Severity:** risk - **Effort:** S - **Status:** corrected - **Depends on:** DSN-02; Context topology decision: one context rebound to N windows vs N shared contexts; Context topology (one context with N drawables vs one per window) must be decided before validation can be specified

Clip control, depth settings, seamless cubemaps, multisample, fixed-function fog and lights and the debug callback are applied once to the single context. A new context has none of them.

*Why it breaks:* Contingent on N-context topology. In a fresh context, glClipControl, TEXTURE_CUBE_MAP_SEAMLESS and the FFP fog state are at GL defaults. The first is a precision loss that breaks depth-reconstruction passes (decals, soft particles, water depth). The second is a cubemap seam glitch. The third turns the terrain black on the first frame, because SMF terrain reads gl_Fog before any SetupFog in the frame. LightHandler::Init and the debug callback do not matter for the prototype.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:1701` InitGLState is the single place for default state
- `rts/Rendering/GlobalRendering.cpp:660` only startup call, executed once in PostInit
- `rts/Rendering/GL/LightHandler.cpp:16` one-time FFP light init (to 37)
- `rts/Rendering/Env/ISky.cpp:49` SetupFog uses global camera far plane (to 63)
- `rts/System/LoadLock.h:21` re-applies ToggleMultisampling after MakeCurrent (:22)
- `rts/System/SpringApp.cpp:1144` size-change path re-runs InitGLState (to 1147)
- `rts/Rendering/Shaders/Shader.cpp:440` ARB env params set through glProgramEnvParameter4fARB (context state)
- `rts/Rendering/GlobalRendering.cpp:1715` glClipControl only inside InitGLState
- `rts/Rendering/GlobalRendering.cpp:2002` debug callback installed on current context only
- `rts/Game/Game.cpp:1464` UniformConstants::Bind once per Draw on the current context
- `rts/Rendering/UniformConstants.cpp:335` BindBufferRange state is per-context
- `rts/System/SpringApp.cpp:274` single startup call, current context only
- `rts/Game/CameraHandler.cpp:106` clip-control matrix set from globalRendering->supportClipSpaceControl at InitStatic
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:419` shader DEPTH_CLIP01 flag assumes clip-control state
- `rts/Game/Camera.cpp:604` projectionMatrix = clipControlMatrix * PerspProj(...) (ortho at :619)
- `rts/Rendering/GlobalRendering.cpp:1714` glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE) only in InitGLState of the primary context
- `rts/Rendering/GlobalRendering.cpp:1985` ToggleGLDebugOutput registers callback on current context only
- `rts/Game/UnsyncedGameCommands.cpp:1657` /debuggl re-registers on whatever context is current
- `rts/Rendering/GlobalRendering.cpp:46` DebugGL default false
- `rts/Rendering/GlobalRendering.cpp:555` debug context flag always requested

*Direction:* Resolve the topology first and make arch A the prototype default: one SDL_GLContext, SDL_GL_MakeCurrent(window_i, ctx) per target, and SDL_GL_SwapWindow only for the current window (SDL requires the window to be current). Window 2 must be created with identical GL attributes so its pixel format is compatible. Under A, add a cheap post-switch hook that calls ToggleMultisampling (precedent e3213aa7a7) and set viewport, camera and fog per target; there is no ApplyContextDefaults.

Under B, write a small ApplyContextDefaults() containing only clip control, the seamless-cubemap enable, the depth state, the optional MSAA toggle and, if DebugGL is on, the debug callback. Do not call InitGLState wholesale; split out the pieces that touch global msaaLevel, glClear, LoadViewport and LogDisplayMode(sdlWindow). Skip LightHandler::Init entirely. B has much bigger blockers (non-shared FBOs and VAOs, UniformConstants UBO binding points via Game.cpp:1464 and UniformConstants.cpp:335).

In both topologies, call SetupFog with the target's own camera at the start of each target's world draw, before the terrain.

> verifier A (corrected): The core is real under arch B (N separate GL contexts). glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE) is issued exactly once, in InitGLState (GlobalRendering.cpp:1714-1715). InitGLState is called only from SpringApp.cpp:274 (startup), :1119 (dual-screen move) and :1146 (size change), all on the single context. CCameraHandler::InitStatic bakes CMatrix44f::ClipControl(supportClipSpaceControl) into every camera (CameraHandler.cpp:106), and Camera.cpp:604/619 multiplies it into the projection. The sh

> verifier B (corrected): Partly real, but only under the claimed [arch B] (N shared contexts), and mis-graded as a blocker. Under arch A (one SDL_GLContext re-bound to each window with SDL_GL_MakeCurrent) there is no new context, so nothing needs replicating. The engine already re-binds its single context between threads (LoadLock.h:21, GlobalRendering.cpp:612-613). The claim itself concedes this ("Under A, context state is shared"). Even under B the fix is tiny (S), and the prototype shows glitches rather than crashing

### GST-03 - No scoped guard exists for framebuffer, viewport or matrix state
**Severity:** risk - **Effort:** S - **Status:** unverified - **Depends on:** none

The engine has scoped guards for state attributes (GL::SubState), texture binds (GL::TexBind) and glPushAttrib, but nothing that saves and restores FBO binding, viewport, or matrix stacks. Call sites hand-write the restore.

*Why it breaks:* GL::SubState has a Viewport attribute at State.h:138 but no code uses it. glPushAttrib(GL_VIEWPORT_BIT) is used in a few places (Lua, SkyBox, Lua RenderToTexture). The FFP attrib stack is deprecated and its viewport bit saves only the rect. Every offscreen pass writes its own FBO::Unbind() and LoadViewport() pair. A missed or misordered restore is the typical state leak that two targets will expose.

*Evidence:*
- `rts/Rendering/GL/State.h:138` Viewport state attribute defined but unused
- `rts/Rendering/GL/SubState.h:55` SubState is the only scoped state guard
- `rts/Rendering/GL/TexBind.h:13` scoped texture bind
- `rts/Rendering/Env/SkyBox.cpp:99` glPushAttrib(GL_VIEWPORT_BIT) then manual FBO::Unbind at 153
- `rts/Rendering/GL/FBO.cpp:322` no RAII for FBO binding
- `rts/Rendering/GL/FBO.cpp:312` Bind/Unbind semantics: Unbind is static, binds 0, with explanatory comment (322-336)
- `rts/Rendering/GL/FBO.cpp:338` Blit saves GL_FRAMEBUFFER_BINDING and restores it (350-369); fromID<0 means 'current'
- `rts/Rendering/GL/FBO.h:200` static registries activeFBOs/fboTexData (200-204)
- `rts/System/SpringApp.cpp:1192` GLContextLost on any HIDDEN in fullscreen (1172-1175 for Reinit)
- `rts/Rendering/GL/FBO.cpp:36` IsReady = globalRendering->active
- `rts/Lua/LuaFBOs.cpp:499` existing manual save/restore of the current FBO (499-529)

*Direction:* Add ScopedFramebuffer, ScopedViewport (using State::Viewport) and ScopedMatrices guards, or a single ScopedTarget; migrate offscreen passes incrementally.

### GST-04 - Terrain shaders depend on per-target legacy fixed-function state (matrices, fog, clip planes)
**Severity:** risk - **Effort:** S - **Status:** corrected - **Depends on:** none

SMF terrain, border and decal shaders read gl_ModelViewProjectionMatrix, gl_ClipVertex and gl_Fog, so each target's camera matrices and fog parameters must be in the GL fixed-function state right before the ground draw. Fog is only refreshed later in the frame.

*Why it breaks:* Only the fog lag is real. Terrain shaders (SMFVertProg.glsl:52-53) read gl_Fog.end/scale, set only by ISky::SetupFog() with the active camera's far plane (ISky.cpp:60-61). Nothing calls SetupFog before the ground draw at WorldDrawer.cpp:342, so terrain uses fog left by the previous call, which in a two-window setup is the other window's camera. Matrices are not at risk, because camera->Update() (WorldDrawer.cpp:316 -> Camera.cpp:159/289-297) reloads projection and MV, and SMFRenderStateGLSL::Enable re-derives MV from the active camera (SMFRenderState.cpp:250-251). Separately, GL4 units and projectiles take fog from the UBO keyed to CAMTYPE_PLAYER (UniformConstants.cpp:230-233, ProjectileDrawer.cpp:982/1215), so a fog fix that touches only SetupFog leaves them mismatched.

*Evidence:*
- `cont/base/springcontent/shaders/GLSL/SMFVertProg.glsl:47` gl_ModelViewProjectionMatrix/gl_ClipVertex/gl_Fog in terrain vertex shader
- `rts/Map/SMF/SMFRenderState.cpp:250` Enable loads MV from the active camera, assumes projection loaded
- `rts/Rendering/Env/ISky.cpp:60` glFog start/end from active camera far plane
- `rts/Game/Camera.cpp:352` far plane computed per camera position/direction
- `rts/Rendering/WorldDrawer.cpp:342` terrain drawn before sky (which calls SetupFog)

*Direction:* Make fog a per-target value with a single source. At the start of each target's world draw, after activating that target's camera and calling camera->Update(), call ISky::GetSky()->SetupFog() before DrawOpaqueObjects. That is a one-line fix, effort S. Also derive UniformConstants' fogParams (UniformConstants.cpp:230-233) and the ProjectileDrawer fxShader fogParams uniforms (ProjectileDrawer.cpp:982, 1215) from the target's active camera instead of CAMTYPE_PLAYER, either by re-uploading per target or by a per-target camera UBO. Treat this as part of the general per-target UBO/camera-state problem. Moving the SMF/grass/water shaders off gl_Fog onto the UBO is a legitimate long-term cleanup but not needed for the prototype.

> verifier B (corrected): The mechanism is real but narrower than claimed. Terrain fog (gl_Fog.end/scale in SMFVertProg.glsl:52-53, gl_Fog.color in SMFFragProg.glsl:447) is read from GL fixed-function fog state. That state is only written by ISky::SetupFog() (ISky.cpp:60-61), using camera->GetFarPlaneDist(). The ground draw is the first thing in the frame (WorldDrawer.cpp:342) and nothing before it calls SetupFog. The remaining callers run later: grass GrassDrawer.cpp:679, sky ModernSky.cpp:111 / SkyBox.cpp:281, legacy m

> verifier A (corrected): The core mechanism holds. SMFVertProg.glsl:53 reads gl_Fog.end and gl_Fog.scale, and nothing in the terrain path calls SetupFog before the ground draw. CWorldDrawer::Draw calls camera->Update() at :316 and gd->Draw at :342, and the first same-frame SetupFog is sky->Draw at :363. The glFogf values come from ISky::SetupFog (ISky.cpp:60-61) using the global active camera's far plane. WorldDrawer.cpp:330 only calls glDisable(GL_FOG), so the start/end values persist into the next target's terrain pas

### GST-05 - No single GL baseline-state contract; the only checker is dead code and context-agnostic
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** Context topology (one context with N drawables vs one per window) must be decided before validation can be specified

At least four different 'default GL state' definitions exist and nothing compares a target's actual state against any of them. The sole state checker has zero call sites.

*Why it breaks:* The state checker is compile-time only (-DDEBUG_GLSTATE) and has no call sites. It tracks lastSet/errorsSet in process-global tables and relies on macro-wrapping glEnable and similar calls. It therefore cannot validate per-target or per-context state at pass boundaries. The differing defaults reflect different pass roles: world pass entry, Lua GUI reset, and init. They are not competing contracts. A per-target baseline table would help find leaks between targets, but it is debug infrastructure. It is not a prerequisite for the prototype.

*Evidence:*
- `rts/Rendering/GL/glStateDebug.cpp:169` VerifyState baseline, an unused fourth definition
- `rts/Rendering/GL/glStateDebug.h:15` GL_STATE_CHECKER macro, zero call sites in rts/
- `rts/Lua/LuaOpenGL.cpp:502` ResetGLState: conflicting depth/blend defaults
- `rts/Rendering/WorldDrawer.cpp:311` WorldDrawer::Draw entry assumptions
- `rts/CMakeLists.txt:32` DEBUG_GLSTATE is compile-time only, default FALSE

*Direction:* Reclassify as a debugging-aid item (full or polish). Define the world-pass entry and exit baseline once, and use it to drive an optional runtime-toggled validator. Do not unify it with LuaOpenGL::ResetGLState, which is intentionally different.

> verifier B (corrected): The facts hold. Within rts/, GL_STATE_CHECKER has no call site: the only hits are its macro definitions in glStateDebug.h and the VerifyState implementation. The checker is also compile-time only, behind the DEBUG_GLSTATE option (default FALSE) at rts/CMakeLists.txt:32. The conflicting defaults in InitGLState, WorldDrawer::Draw and LuaOpenGL::ResetGLState are plausible, but I did not re-read those sites. This is a debugging and hygiene gap, not a prototype blocker. The 2-window 3D-only prototype

> verifier A (corrected): The cited facts hold in the current tree. Grep finds GL_STATE_CHECKER only in its definition (glStateDebug.h:15), with no call sites in rts/. DEBUG_GLSTATE defaults to FALSE (CMakeLists.txt:32). VerifyState expects depth mask true, depth func LEQUAL, no scissor, stencil or clip planes, and white current colour (glStateDebug.cpp:169-205). WorldDrawer::Draw sets depth mask true, depth test on and blend off (311-314). LuaOpenGL::ResetGLState sets depth test off, depth mask false and blend on (504-5

### GST-06 - Dynamic model lights live in fixed-function glLight state, initialised once and uploaded per model pass
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** none

GL::LightHandler stores dynamic lights in the GL compatibility-profile light slots (glLightfv) rather than in a UBO. Init runs once on the context current at startup, and Update re-uploads from the model-state setup of every pass.

*Why it breaks:* Only one LightHandler exists: the static ModelDrawer::lightHandler (ModelDrawer.h:53), initialised once at ModelDrawer.cpp:20. The ground drawer's GetLightHandler returns nullptr (BaseGroundDrawer.h:50-51) and SMFGroundDrawer does not override it. Update (LightHandler.cpp:105-226) uploads the FFP light slots from the model-state setup (ModelDrawerState.cpp:210). Each pass re-uploads, and the frameNum guard makes the time advance idempotent per sim frame, so the multi-view re-upload is harmless in a single shared context. A dead light is zeroed one call late (line 193 then line 133). In an arch-B per-window context the FFP light slots would be uninitialised (spot cutoff and colours at GL defaults), unless Init is replayed per context. Dynamic lights are also not in a UBO, and the LOS filter uses the global gu->myAllyTeam.

*Evidence:*
- `rts/Rendering/GL/LightHandler.cpp:16` Init sets FFP light slots once on the current context
- `rts/Rendering/GL/LightHandler.cpp:142` decay/time advance keyed on gs->frameNum, first caller per sim frame wins
- `rts/Rendering/GL/LightHandler.cpp:202` LOS filter uses gu->myAllyTeam
- `rts/Rendering/Common/ModelDrawerState.cpp:210` Update is called from the per-pass model state setup
- `rts/Rendering/Common/ModelDrawer.h:53` single static lightHandler for all model drawers

*Direction:* Fold this into the arch-B one-time context state replay: re-run LightHandler::Init's slot defaults on each new context, or move lights to a UBO. Drop the claim about a second ground-drawer handler.

> verifier A (corrected): The core mechanism holds, but the claim that a second ground-drawer LightHandler instance exists is false. Init (LightHandler.cpp:16-42) sets the FFP light slots once at startup through ModelDrawer.cpp:20. Update uploads with glLightfv (lines 135-138, 199-224) and is called only from ModelDrawerState.cpp:210. The time advance is guarded by gs->frameNum (line 142) and the LOS filter reads gu->myAllyTeam (line 202). The ground drawer has no LightHandler: BaseGroundDrawer.h:50-51 returns nullptr, a

### GST-07 - Fixed-function fog is derived from the active camera far plane
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** none

Fog is FFP glFog state computed from the active camera's far plane and also mirrored into the UBO from the player camera only.

*Why it breaks:* Fog reaches shaders by three routes:
(a) FFP gl_Fog state, used by SMF terrain, grass, BumpWater and legacy model and projectile shaders. It is set by ISky::SetupFog from the ACTIVE camera and is stale for terrain, because terrain draws before the first SetupFog of the view (ProjectileDrawer.cpp:823, after terrain at WorldDrawer.cpp:342).
(b) The UBO fogParams and cameraViewInv, both player-only (UniformConstants.cpp:167-177, :230-233), used by the ModelVertProgGL4 unit and feature path.
(c) The particle shader uniforms camPos and fogParams, taken from camPlayer (ProjectileDrawer.cpp:980-982, :1215).
Only (a) is a fog-specific problem. (b) and (c) are instances of the 'player camera hardcoded' problem.

*Evidence:*
- `rts/Rendering/Env/ISky.cpp:60` fog start/end from camera->GetFarPlaneDist()
- `rts/Rendering/WorldDrawer.cpp:155` one-time SetupFog at init; glDisable(GL_FOG) at 330
- `rts/Rendering/Env/ModernSky.cpp:111` SetupFog after sky Draw, skipped by early returns at 46-50
- `rts/Game/Camera.cpp:352` near/far from camera position and direction
- `rts/Rendering/UniformConstants.cpp:230` fogParams from player far plane

*Direction:* Do not add a camera parameter to SetupFog. The per-view begin step must already make the view camera active, because every `camera` macro use depends on it, so SetupFog() then resolves to the right camera.
1. In CWorldDrawer::Draw (WorldDrawer.cpp), call `ISky::GetSky()->SetupFog()` right after `camera->Update()` at line 316 and before DrawOpaqueObjects. This also fixes the existing stale-fog-on-terrain behaviour, and the camera's far plane is already valid at that point because CCamera::Update calls UpdateViewRange (Camera.cpp:148-149). Effort S.
2. Make fogParams a field of the per-view UBO (or per-view UBO range), computed from the view camera. This comes with the per-view UBO problem, not as a fog task.
3. Replace `camPlayer` with the active or view camera in ProjectileDrawer.cpp:970-982 and :1215.
4. If window B gets its own GL context, ensure its first frame runs step 1 before terrain; no other per-context fog initialisation is needed.

> verifier A (corrected): The core is real: ISky::SetupFog (ISky.cpp:60-61) multiplies camera->GetFarPlaneDist() by fogStart/fogEnd, and `camera` is the active-camera macro (Camera.h:316). The far plane depends on camera pos and forward (Camera.cpp:338-353), and all callers are as listed. UniformConstants.cpp:230-233 uses camPlayer only. The mechanism needs correcting. (1) The terrain pass is the main victim. SMFVertProg.glsl:52-53 computes fogFactor from gl_Fog.end and gl_Fog.scale, and SMFFragProg.glsl:447 mixes gl_Fog

> verifier B (corrected): The FFP fog mechanism is real but weaker than claimed, and it is nearly free to fix.

What is real:
- ISky::SetupFog (ISky.cpp:49-63) writes GL_FOG_START/END from `camera->GetFarPlaneDist()`, and `camera` is just `CCamera::GetActive()` (Camera.h:316).
- The far plane really is per-camera: Camera.cpp:338-353 derives it from camera height and forward direction, and fogStart/fogEnd default to 0.1/1.0 (MapInfo.cpp:138-139).
- Terrain does read the FFP fog state: SMFVertProg.glsl:53 uses gl_Fog.end a

### GST-08 - Fixed-function matrix stack and default-GL-state assumptions in projectile passes
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** About 110 billboard/LOD sites read the global active camera at draw time

The FX shaders read gl_ModelViewMatrix and gl_ProjectionMatrix, which exist only as per-context fixed-function state loaded by camera->Update(). Several passes also hard-reset GL state to assumed defaults on exit.

*Why it breaks:* Reduce this to a design note, not a defect. Each CWorldDrawer::Draw already re-establishes the GL baseline (WorldDrawer.cpp:311-314) and reloads the fixed-function matrices (camera->Update at :316) in whatever context is current, so the per-context matrix state and the hard-coded state resets in DrawGroundFlashes/UpdatePerlin need no separate fix. Camera.cpp:160-161 deliberately leaves LoadViewport to the caller. What genuinely remains is narrower. (a) camera->Update() reads aspect, viewSize and viewPos from the process-wide globalRendering (Camera.cpp:144-155), so the target's size must be swapped in before Draw. That belongs to the existing globalRendering/viewport problem. (b) The fixed-function built-ins are pervasive: 42 occurrences in 21 shader files under cont/base/springcontent/shaders/GLSL, not just ProjFX. A second GL context therefore has to be created as a compatibility-profile context. That context-creation requirement is better tracked on its own than under "projectile passes". (c) The one projectile-specific multi-camera issue not in the claim is that the FX shader's fog uniforms (camPos, fogParams) are taken from CCameraHandler::GetCamera(CAMTYPE_PLAYER), not from the active target camera (ProjectileDrawer.cpp:970, 980-982, 1206, 1213-1215). If window 2 uses a camera object other than the PLAYER camera, its particles and ground flashes are fogged relative to window 1's eye position. This is a visual glitch, so it would be a risk-level item on its own if it is not already listed.

*Evidence:*
- `cont/base/springcontent/shaders/GLSL/ProjFXVertProg.glsl:62` gl_ModelViewMatrix/gl_ProjectionMatrix used
- `rts/Game/Camera.cpp:144` matrices and viewport derived from globalRendering
- `rts/Rendering/WorldDrawer.cpp:316` camera->Update() at the start of every world draw

*Direction:* Do not carry this as a standalone matrix-stack/state-reset problem. Fold the viewSize/aspect swap into the globalRendering-per-target problem and the compat-profile context requirement into the GL context-creation problem. Add a separate, narrow item for the fog/camPos uniforms in ProjectileDrawer::DrawAlpha and DrawGroundFlashes: take them from the active target camera instead of CAMTYPE_PLAYER. No BeginTarget "reset the GL baseline" step is needed for these passes, since CWorldDrawer::Draw already does it.

> verifier B (corrected): Both halves of the claim are already covered by the existing flow, or are not on the prototype's per-target path. This is not a standalone risk.

(1) Matrices. The claim is correct that the FX shaders read gl_ModelViewMatrix and gl_ProjectionMatrix. ProjFXVertProg.glsl:62-63 uses both, and ProjFXFragProg.glsl:25 and :34 use the projection matrix. But CWorldDrawer::Draw already calls camera->Update() at WorldDrawer.cpp:316, immediately before DrawOpaqueObjects and DrawAlphaObjects. CCamera::Updat

> verifier A (corrected): The cited code facts are accurate, but they do not support the "risk" severity and the "hard-reset" half is harmless. Accurate: ProjFXVertProg.glsl:62-63 and ProjFXFragProg.glsl:25-34 read gl_ModelViewMatrix/gl_ProjectionMatrix; Camera.cpp:144-155 derives aspect, viewSize and viewPos from globalRendering; Camera.cpp:159 and 289-297 end Update in LoadMatrices; ProjectileDrawer.cpp:1260-1263 and 1361-1364 hold the hard-coded resets. Weak points: (1) The matrix reload is already done on every world

### GST-09 - GL debug output, error queue, debug groups and timer queries are per-context but driven globally
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** Context topology decision: one context rebound to N windows vs N shared contexts

The debug-message callback, glGetError queue, push/pop debug groups and timer-query objects are per-context state managed from one global place. Under the shared-context option they become incorrect, and /debuggl affects only the current context.

*Why it breaks:* Debug callback, glGetError queue, debug-group stack and query objects are per-GL-context. The engine sets them up once, for the single context created at GlobalRendering.cpp:587. ToggleGLDebugOutput is called only from PostInit (:660) and from /debuggl, and there is no hook on context creation. A second shared context created later would therefore never get the debug callback, even with DebugGL=1 in config. /debuggl would still reach only whichever context is current on the calling thread. glTimerQueries (:664) belong to the first context and are used only for the main-pass FRAME_REF/FRAME_END stamps in CGame::Draw (Game.cpp:1459, :1567). Their only reader is the ProfileDrawer overlay (:502). CalcGLDeltaTime's unbounded `while(!res)` loop (:745-747) would hang if called with a context current that does not own the queries (INVALID_OPERATION leaves res at 0). The "Draw" debug group (Game.cpp:1458) wraps worldDrawer.Draw (Game.cpp:1526), so a context switch inside that scope that is not switched back before the scope ends would pop on the wrong context. That gives GL_STACK_UNDERFLOW there and a leaked push on the original context. Under the single-context option (one context, N windows via SDL_GL_MakeCurrent) none of this applies, and the prototype needs no change. The existing CLoadLock (LoadLock.h:21-32) already moves the single context between threads, which is the precedent for how it is handled.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:2002` debug callback per current context
- `rts/Rendering/GlobalRendering.cpp:680` glClearErrors of the current context in SwapBuffers
- `rts/Rendering/GL/glDebugGroup.cpp:5` push/pop group in ctor/dtor
- `rts/Rendering/GlobalRendering.cpp:745` while(!res) spin on query availability (to 750)
- `rts/Rendering/GlobalRendering.cpp:664` glGenQueries for 16 timer queries (NUM_OPENGL_TIMER_QUERIES=8 x2)
- `rts/Game/UI/ProfileDrawer.cpp:502` CalcGLDeltaTime consumer
- `rts/Rendering/GlobalRendering.cpp:1985` debug callback set on current context only (to 2016)
- `rts/Game/Game.cpp:1458` debug group spans the whole Draw

*Direction:* Make no change for the single-context option. For shared contexts, call a per-context debug-setup helper (the glDebugMessageCallback block from ToggleGLDebugOutput) right after each context is created and again from /debuggl for every live context. Keep CGame::Draw and the SwapBuffers debug groups balanced by switching contexts only at group boundaries. Keep the profiling timer queries and the ProfileDrawer readback on the main-target context. If per-target GPU timing is wanted later, give each target its own query set and bound the busy-wait. Drop the debug-flag-always-set point from this problem.

> verifier A (corrected): The code facts check out, but the problem only exists under the N-shared-contexts topology, and it is a diagnostics-tooling concern rather than a product need. It does not reach the 2-window 3D-only prototype. I checked each cited line: ToggleGLDebugOutput installs the callback on the current context only (GlobalRendering.cpp:2001-2003, called from PostInit :660 and /debuggl UnsyncedGameCommands.cpp:1657). SwapBuffers calls glClearErrors (:680-681). SCOPED_GL_DEBUGGROUP is a plain push/pop (glDe

### GST-10 - GL::State attribute cache is process-global and the GL state checker is unused
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** Context topology (one context with N drawables vs one per window) must be decided before validation can be specified

GL::State::Attributes is a single global tuple, and the debug-only CGLStateChecker (which verifies defaults on scope entry and exit) is defined but never used.

*Why it breaks:* Verified. (1) rts/Rendering/GL/State.cpp:3 defines one process-global GL::State::Attributes tuple (declared extern at State.h:191-251). It is not thread_local and not per-context. The doc at State.h:13-20 and the StateAttribute code at State.h:41-54 confirm it reports "known" only while a SubState is alive. The outermost pop restores a (Unknown, value fetched from GL) pair, which sets the slot back to Unknown. A SubState constructed on an Unknown slot always re-queries GL via FetchEffectualStateAttribValues (State.h:45). SubState.h:21-25 saves and sets in its constructor, and its destructor (SubState.h:44-46) calls pop() and writes to whatever context is current. (2) Every one of the 26 GL::SubState uses in rts (outside rts/lib) is a function-local `auto state = ...` inside a single draw or update routine. Examples are UnitDrawer.cpp:797 and :994, GroundDecalHandler.cpp:793, SMFReadMap.cpp:714 and the ProjectileDrawer and InfoTexture sites. None spans a frame, a pass loop or a target switch, so a target switch between draw calls leaves the cache empty. The cache is therefore correct even with separate contexts unless someone later holds a SubState across a MakeCurrent. The claim's sentence "With separate contexts it would be wrong" is too strong. Only a SubState that lives across a context switch would be wrong. It is also irrelevant to a single context with two drawables, because GL state is per-context. The cache covers little: the Viewport, Scissor and ScissorTest attributes exist at State.h:138 and :145-146, but no call site passes them to SubState (grep of the SubState argument lists finds none), and nothing tracks FBO binding. (3) GL_STATE_CHECKER has zero users. A grep of the repo finds the macro only at glStateDebug.h:7 and :15, and it is compiled in only if the option DEBUG_GLSTATE (rts/CMakeLists.txt:32-34, default FALSE) is on. VerifyState (glStateDebug.cpp:169-206) checks only default values. Those include SCISSOR_TEST, CLIP_PLANE0-5, blend factors, colour and depth masks, polygon mode and offsets, line width and current colour, and it does not check GL_VIEWPORT or GL_FRAMEBUFFER_BINDING. It also relies on compatibility-profile enums (GL_LIGHTING, GL_ALPHA_TEST, GL_CURRENT_COLOR, GL_CLIP_PLANE*). Minor limits: it logs once per (area+id+pname) through the static errorsSet (glStateDebug.cpp:19 and :24), so a repeat leak on target 2 under the same scope name would not be reported again, and its bookkeeping is main-thread only (IsMainThread guards). BeginTarget/EndTarget do not exist in the tree. They are the analyst's proposed API. The correct classification is tooling polish or an optional debug aid, not a prototype risk.

*Evidence:*
- `rts/Rendering/GL/State.cpp:3` single global Attributes instance
- `rts/Rendering/GL/State.h:12` documents that untracked GL calls must not collide with SubState
- `rts/Rendering/GL/glStateDebug.h:15` GL_STATE_CHECKER macro, zero users in rts
- `rts/Rendering/GL/glStateDebug.cpp:169` VerifyState checks defaults only, no FBO/viewport
- `rts/Rendering/GL/SubState.h:44` destructor restores into whichever context is current

*Direction:* Downgrade to polish, or fold into a debug-tooling item rather than a standalone risk. No code change is needed for the 2-window prototype. Add a rule to the context-topology design decision: no GL::SubState may be alive across a target switch or MakeCurrent. If separate contexts are chosen, the global Attributes tuple would need to be per-context or thread_local. Optionally, add a cheap debug assertion at target begin/end that checks FBO binding, viewport, scissor test, clip distances and blend state. Reuse CGLStateChecker rather than wiring it as-is, since it lacks FBO and viewport checks and depends on the DEBUG_GLSTATE build flag.

> verifier B (corrected): The facts hold but the problem does not: the attribute cache is a process-global tuple (rts/Rendering/GL/State.cpp:3) and GL_STATE_CHECKER has zero users, but the claim itself concedes the cache is safe across target switches. I checked every GL::SubState site (about 27 across 19 files, grep'd above). All are function-scope locals, such as `auto state = GL::SubState(...)` at UnitDrawer.cpp:797, ProjectileDrawer.cpp:812, GroundDecalHandler.cpp:793 and SMFReadMap.cpp:714. None is a member or stati

> verifier A (corrected): The facts hold in the current tree. The cited global tuple, the unused checker and its coverage gaps are all real. The "risk" severity does not hold. By the claim's own mechanism the cache is safe for the 2-window prototype, and the checker is optional debug tooling. The sentence "With separate contexts it would be wrong" also overstates the hazard.

### GST-11 - Global pass-mode state and hardcoded restore values
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** none

Pass identity (draw mode, in-shadow, reflection/refraction flags, active camera type) is process-global state mutated by nested passes with hardcoded restore values.

*Why it breaks:* Pass identity (game draw mode, CShadowHandler::inShadowPass, IWater::drawReflection/drawRefraction, active-camera slot) is process-global, and passes set it with hardcoded values (SetDrawMode(gameNormalDraw)) instead of a scoped save/restore. For strictly sequential, non-nested passes this is harmless, and today it is never wrong. Camera passes already save and restore the previous active type via GetSetActiveCamera(...)/SetActiveCamera(prvCam->GetCamType()), and the `camera` macro is CCamera::GetActive() (Camera.h:316), so the existing active-camera indirection lets a view's camera be selected by pointing the ACTIVE slot at it. Three concrete consequences for multi-target work. (a) The prototype must set gameNormalDraw at the start of each view's draw. CGame::Draw sets gameNotDrawing at Game.cpp:1559. If a second view's world draw ran after that, ModernSky.cpp:72 would pick skyShaders[1], the SIMPLIFIED_RENDERING variant (ModernSky.cpp:26), because mode != gameNormalDraw. SMFGroundTextures.cpp:641 would skip SetDrawFrame, and ModelDrawerState.cpp:317/345 would fall to the default NORMAL_CAMERA branch. This is S effort and is a pass-entry convention, not a redesign. (b) Hardcoded restores break only under re-entrancy: a view or Lua render-to-texture driven from inside another pass such as a DrawWorld callin, or a future nested pass inside reflection. That is a full-product hardening concern. (c) UniformConstants.cpp:195 is harmless in its current position, but the UBO it fills is built from the PLAYER camera (UniformConstants.cpp:167). That belongs to the global-camera-UBO problem, not this one.

*Evidence:*
- `rts/Game/Game.h:34` GameDrawMode enum; accessors at 105-106
- `rts/Rendering/Env/CubeMapHandler.cpp:238` cubemap pass resets to normal (227)
- `rts/Rendering/UniformConstants.cpp:195` SetActiveCamera(CAMTYPE_PLAYER) hardcoded
- `rts/Rendering/Common/ModelDrawer.h:324` assert on active camera type
- `rts/Game/Game.h:152` GameDrawMode gameDrawMode plain member; Set/Get at 105-106
- `rts/Rendering/Env/IWater.cpp:180` reset to gameNormalDraw (also 222); drawReflection flag 144/177
- `rts/Rendering/LuaObjectDrawer.cpp:522` deferred pass resets to normal (set at 494)
- `rts/Rendering/WorldDrawer.cpp:258` shadow pass resets to normal (256)
- `rts/Rendering/Common/ModelDrawer.h:180` static implStack with Push/Pop (152-160)
- `rts/Map/SMF/SMFGroundTextures.cpp:641` draw-mode check in BindSquareTexture

*Direction:* Downgrade from risk to full. Reframe from "any non-PLAYER view is lost after the first nested pass", which is false, to "pass state is global with hardcoded restores; safe only while passes are sequential and every view starts in gameNormalDraw". For the prototype, require only that each view's draw entry resets draw mode to gameNormalDraw and selects its camera through the ACTIVE slot. For the full product, replace hardcoded restores with scoped RAII guards (a PassContext, or save/restore of draw mode, inShadowPass and the water flags), so views can be driven re-entrantly. Make Lua DrawWorld callins aware of which view they run in.

> verifier B (corrected): The facts are real (global gameDrawMode with hardcoded gameNormalDraw resets, one hardcoded PLAYER reset). The conclusion is wrong, and the proposed fix is not needed for the 2-window, 3D-only prototype. The root cause is not one-way resets but the single-slot, process-global design, and for the prototype that is harmless.

1. Draw mode. Every setter of a non-normal mode is entered from gameNormalDraw, so restoring to gameNormalDraw equals the previous value at every current call site. CGame::Dr

> verifier A (corrected): The core observation holds: draw mode, inShadowPass and the water reflection/refraction flags are process-global, and the pass code restores them to hardcoded values rather than the previous ones (WorldDrawer.cpp:256-258, CubeMapHandler.cpp:227/238, IWater.cpp:141/180/185/222, ShadowHandler.cpp:438/490, IWater.cpp:144/177). The claimed failure does not hold for a 2-window, sequential, 3D-only prototype, so "risk" is too high. (1) Nested passes do not lose a non-PLAYER active camera. Every pass t

### GST-12 - InitGLState re-runs on every window event and mutates global msaaLevel from whichever drawable is current
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** none

Resize/move handlers call InitGLState, which multiplies the global msaaLevel by a probe of the current drawable's sample count. With a second window of a different pixel format current, the global MSAA level can be zeroed permanently.

*Why it breaks:* msaaLevel is a process-global derived value that is re-derived by InitGLState from the currently bound drawable on every SIZE_CHANGED event, and on MOVED when dual-screen mode is on. It is only ever lowered to 0, never raised. It becomes 0 only if that event runs while a zero-sample drawable (or a non-MSAA FBO) is bound and the main window had MSAA. Two windows with the same pixel format leave it unchanged, as does MSAALevel=0. The risk is therefore latent and applies only to a design with per-window contexts and differing MSAA negotiation. A related, more fundamental gap is that CreateSDLWindow never writes the negotiated sample count back into msaaLevel (GlobalRendering.cpp:402-447). msaaLevel keeps the configured value, so a 4x request that fell back to 2x still sizes the MSAA FBOs and textures (GeometryBuffer.cpp:144/151, DepthBufferCopy.cpp:140) with 4. Only the all-or-nothing outcome is corrected, on the first InitGLState.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:1721` msaaLevel *= CheckGLMultiSampling()
- `rts/Rendering/GlobalRendering.cpp:1862` samples probed from the current framebuffer
- `rts/System/SpringApp.cpp:1146` InitGLState on every SIZE_CHANGED
- `rts/Rendering/DepthBufferCopy.cpp:140` msaaLevel used for texture sample count and size

*Direction:* Prototype: do nothing for msaaLevel beyond window-ID scoping in MainEventHandler (SDL_WINDOWEVENT must compare event.window.windowID against SDL_GetWindowID(primary) before running UpdateGLGeometry, InitGLState, UpdateInterfaceGeometry and ResizeEvent), and create window 2 with the same SDL_GL_* attributes as window 1. Cheap hygiene, which can be done any time (S): resolve msaaLevel exactly once, right after window and context creation, and make InitGLState only re-apply state to the current target. Best source is SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES) after CreateSDLWindow, or a single CheckGLMultiSampling probe at the end of CreateWindowAndContext. Taking the negotiated value from SDL also fixes the missing write-back of the negotiated level in the CreateSDLWindow fallback loop. Add per-window sample metadata only if the product later allows windows with different pixel formats or separate non-shared contexts.

> verifier A (corrected): The code facts are real: InitGLState does `msaaLevel *= CheckGLMultiSampling()` (GlobalRendering.cpp:1721). CheckGLMultiSampling probes GL_SAMPLE_BUFFERS and GL_SAMPLES from whatever framebuffer is currently bound (:1862-1865). InitGLState is re-run from the SIZE_CHANGED handler (SpringApp.cpp:1146) and from MOVED in dual-screen mode (:1113-1119), and nothing in rts/ filters events by SDL windowID (grep for windowID/GetWindowID found no hits outside rts/lib). The severity claim does not hold. Th

> verifier B (corrected): The mechanism is real but the trigger does not occur in the 2-window, 3D-only prototype, so "risk" is too high. Confirmed in code: GlobalRendering.cpp:1721 does `msaaLevel *= CheckGLMultiSampling()`, and CheckGLMultiSampling (1852-1866) reads GL_SAMPLE_BUFFERS and GL_SAMPLES from whatever draw framebuffer is bound. Because it is multiplicative, a single bad probe zeroes msaaLevel for the rest of the process. InitGLState is called from SpringApp.cpp:274 (init), :1119 (MOVED, dual-screen only) and

### GST-13 - Legacy fixed-function path (used for Lua-material objects) carries view state in GL matrix stack, fog and lights
**Severity:** polish - **Effort:** M - **Status:** corrected - **Depends on:** Camera slots are one per CAMTYPE and drawers use the ambient active camera

Every opaque, alpha and shadow pass switches to the legacy GLSL drawer to draw Lua-material objects. That path takes its view from the GL matrix stack, fog and lights, which only match the camera if each view loads them first.

*Why it breaks:* The legacy/fixed-function Lua-material path does depend on implicit GL state: the projection stack holding P with V multiplied in by `PushTransform`, `gl_Fog`, and the viewport. In the current tree that state is re-derived from the ACTIVE camera every time a view's `CWorldDrawer::Draw` runs (`camera->Update()` -> `LoadMatrices` at WorldDrawer.cpp:316 and Camera.cpp:159; `SetupFog` in `DrawImpl<legacy>`, ModelDrawer.h:321), and nested passes restore it through `prvCam->Update()` (BumpWater.cpp:1036-1038, AdvWater.cpp:349-351). The coupling is therefore not a new blocker. It is an invariant the per-view driver must preserve: make the view's camera the active camera and call `Draw` (or an equivalent that calls `camera->Update()`) with that GL context current.

Real residuals, narrower than the claim:
1. FFP state is per GL context. If the second window gets its own context, the one-time fog and alpha-func setup at WorldDrawer.cpp:155 and any glEnable defaults only apply to the first context. The per-frame `SetupFog` in `DrawImpl<legacy>` covers fog. The wider per-context init belongs with the context-state problem.
2. `DrawImpl<false>` (the GL4 path) skips `SetupFog`, so a Lua material that reads `gl_Fog` there sees whichever fog the last sky, grass or water draw set. This only matters if the second camera's far plane differs.
3. Matrix and aspect inputs come from `globalRendering->viewSize*` and `aspectRatio` (Camera.cpp:144-155). That is the per-window size problem, not this one.

*Evidence:*
- `rts/Rendering/Common/ModelDrawer.h:364` ScopedModelDrawerImpl(true,false,false) forces the legacy drawer for Lua-material objects (also 393, 450)
- `rts/Rendering/Common/ModelDrawerHelpers.cpp:83` PushTransform multiplies camera view into current GL_PROJECTION
- `rts/Rendering/Common/ModelDrawerState.cpp:195` legacy Enable calls PushTransform(camera) and updates lights
- `rts/Game/Camera.cpp:289` LoadMatrices loads projection/view into the FFP stack
- `rts/Rendering/LuaObjectDrawer.cpp:128` Setup/ResetOpaqueUnitDrawState route Lua material draws through the legacy drawer

*Direction:* Keep this as an invariant note in the render-view design: the per-view begin step must (a) make the view camera the active camera, (b) call `camera->Update()` (or `LoadMatrices` plus `LoadViewport`) with that window's GL context current, and (c) not skip `SetupFog`. No separate fix is needed for the prototype beyond that. Long term, porting Lua-material draws to the GL4 path (UBO matrices) removes the dependency on the stack and `gl_Fog`. Drop "lights" from the list of view state; they are world-space.

> verifier B (corrected): The matrix-stack dependency is real, but it is not a separate problem and does not block the prototype. It is a consequence of the "ambient active camera" and "globalRendering viewSize" problems. The existing code already reloads the stack for the active camera, so a per-view run of the normal draw sequence gets the legacy path right.

What holds up:
- DrawOpaquePassImpl, DrawAlphaPassImpl and DrawShadowPassImpl each open a ScopedModelDrawerImpl(true,false,false) and send Lua-material objects th

> verifier A (corrected): The code facts are accurate, but the claimed failure does not happen in the current tree. The legacy path's view state is re-established every time CWorldDrawer::Draw runs, so a per-view Draw with the right camera active needs no extra work. Verified in code:

- Lua-material passes force the legacy drawer. `ScopedModelDrawerImpl(true,false,false)` sits at ModelDrawer.h:364, 393 and 450.
- That path reaches `CModelDrawerStateGLSL::Enable`, which calls `PushTransform(camera)` (ModelDrawerState.cpp

### GST-14 - Overlay passes leave fixed-function state that later phases silently depend on
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** Draw path has no split between once-per-frame work and per-target work; GST-05

The end of the frame resets some state by hand (0..1 ortho, blend on, depth test enabled, TEXTURE_2D). The next frame's passes silently rely on that steady state, and the world pass re-initialises only part of it.

*Why it breaks:* The world pass calls ResetMVPMatrices itself at WorldDrawer.cpp:553, in the underwater-water-colour quad. That leaves a 0..1 ortho projection, a modelview identity, blend on and depth test off. It also leaves glColor4f(0,0.2,0.8,0.333) and GL_TEXTURE_2D disabled (lines 556-557) at the end of that function. A second world pass starting in the same context would inherit these from the first. That holds only if the camera is below y=0 (the early return at line 505), and only if later draw code reads the fixed-function current colour or the TEXTURE_2D enable. The Draw() prologue resets only depth and blend (lines 311-314), not colour or texture state. Per-target begin-pass state initialisation (matrices, viewport, colour, texture enables) is a cheap hygiene fix. It is not a prototype blocker.

*Evidence:*
- `rts/Rendering/WorldDrawer.cpp:287` ResetMVPMatrices leaves depth test off and blend on
- `rts/Game/Game.cpp:1550` ad hoc end-of-frame state patch
- `rts/Game/Game.cpp:1578` glEnable(GL_TEXTURE_2D) around DrawInputReceivers (also 1601)
- `rts/Lua/LuaOpenGL.cpp:996` RevertScreenMatrices leaves 0..1 ortho
- `rts/Rendering/WorldDrawer.cpp:308` Draw() resets only clear color, depth and blend, not matrices/viewport/texture units
- `rts/Rendering/WorldDrawer.cpp:557` leaves current colour tinted and GL_TEXTURE_2D off
- `rts/Rendering/ShadowHandler.cpp:629` shadow FBO and viewport intentionally left for the caller to restore
- `rts/Game/Game.cpp:1521` only restore of FBO and viewport after IBL passes

*Direction:* Fold this into the baseline GL state contract and per-target begin-pass items. Add glColor4f(1,1,1,1), GL_TEXTURE_2D and matrix initialisation to the per-target pass preamble. Size: S.

> verifier B (corrected): The cited code is real. ResetMVPMatrices (WorldDrawer.cpp:287-298) leaves a 0..1 ortho with blend on and depth off. Game.cpp:1550-1551 re-enables depth and loads identity. DrawInputReceivers toggles GL_TEXTURE_2D at 1578 and 1601. WorldDrawer::Draw (308-314) resets only clear colour, depth, depth mask and blend. Game.cpp:1521-1524 is the only FBO and viewport restore after the IBL passes. But the severity is overstated for the 2-window, 3D-only prototype, for three reasons. (1) The prototype has

> verifier A (corrected): The cited code exists and matches: ResetMVPMatrices at WorldDrawer.cpp:287-298 (0..1 ortho, blend on, depth off), Game.cpp:1550-1551 (glEnable(GL_DEPTH_TEST), glLoadIdentity), and the GL_TEXTURE_2D enables at Game.cpp:1578 and 1601. WorldDrawer::Draw (lines 308-314) resets only clear colour, depth and blend. No code was found that makes the world pass depend on the leftover state, so "risk" is not supported. The claim is conditional ("if a secondary world pass is inserted...") and has no concret

### GST-15 - TeamHighlight mutates CTeam::color for a single frame bracket
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** none

CTeamHighlight::Enable/Disable modify CTeam::color in place around Draw. A second Enable without Disable would capture highlighted colors as the originals and make the shift permanent.

*Why it breaks:* CTeamHighlight keeps process-global state: it mutates CTeam::color in place and records the originals in the file-static oldColors (TeamHighlight.cpp:22,29,38,44,56). The mutation is lossless only if every Enable is followed by a Disable before the next Enable. Today this holds, since CGame::Draw makes one pair (Game.cpp:1512 and 1560) with no return between them. A per-window loop is safe if it either keeps the pair around the whole loop or makes a complete Enable..Disable pair per window. It breaks only on a nested or repeated Enable without Disable, because Enable clears oldColors and re-saves the already-shifted colors, so the shift becomes permanent. This is an ordering invariant, not a per-target correctness problem, and nothing in the prototype requires changing it. The pulse is also computed from one currentTimePreDraw (Game.cpp:1512), so two per-window pairs could differ by a few ms of phase, which is cosmetic. Live-read versus snapshot: the GL4 model path (ModelDrawerState.cpp:293-305 sets only teamColorAlpha; ModelVertProgGL4.glsl:364 reads teamColor[paletteIndex] from the UBO) uses the pre-Enable snapshot and never sees the pulse. The GLSL model path (ModelDrawerState.cpp:185 via ModelDrawerHelpers.cpp:110) and the unit icon and ghost paths (UnitDrawer.cpp:766, 1394, 2303) read CTeam::color at draw time and do see it. A separate pre-existing hazard, unrelated to multi-target: LuaUnsyncedCtrl.cpp:2991-2994 (SetTeamColor) called during Draw callins between Enable and Disable is overwritten by Disable's restore.

*Evidence:*
- `rts/Rendering/TeamHighlight.cpp:24` Enable: oldColors.clear then rewrites t->color (24-47)
- `rts/Game/Game.cpp:1512` Enable at 1512, Disable at 1560
- `rts/Rendering/UniformConstants.cpp:295` UBO team colors captured before the highlight bracket
- `rts/Rendering/TeamHighlight.cpp:29` Enable clears and re-saves old colours
- `rts/Game/Game.cpp:1560` Disable at frame end
- `rts/Rendering/TeamHighlight.cpp:49` Disable restores (49-60)

*Direction:* When the multi-view draw loop is written, either keep the single Enable/Disable bracket (Game.cpp:1512 and 1560) around all views, or make a complete Enable..Disable pair per view. Never nest or repeat Enable without Disable. A hardening option is to make Enable return early when oldColors is non-empty, or to turn the shift into a per-draw uniform instead of mutating CTeam::color. This is optional and can be deferred; it does not block the prototype.

> verifier A (corrected): The cited code is accurate. Enable (TeamHighlight.cpp:24-47) clears oldColors, saves the current color and adds a pulse offset. Disable (49-60) restores from oldColors. There is exactly one bracket, at Game.cpp:1512 and 1560, with no early return between them. The UBO team colors are copied in UniformConstants.cpp:295-301, which runs from UpdateUnsynced (Game.cpp:1441) before Enable. The defect only appears if someone writes a bracket that nests or double-enables, such as Enable, Enable, Disable

### GST-16 - WorldDrawer::Draw clears the entire bound framebuffer with no scissor discipline for sub-rect targets
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** none

glClear ignores the viewport, so a target that shares a drawable with another (dual-screen style sub-rect) would wipe its neighbour. The 3D path uses no scissor anywhere.

*Why it breaks:* WorldDrawer::Draw (WorldDrawer.cpp:307-309) sets the clear color from the sky fog color and runs glClear(COLOR|DEPTH|STENCIL) with no GL_SCISSOR_TEST. glClear ignores glViewport. CGame::Draw (Game.cpp:1522-1526) does FBO::Unbind() and camera->LoadViewport() before calling worldDrawer.Draw(). In dual-screen mode the camera viewport is the sub-rect viewPos/viewSize (Camera.cpp:155, :302), but the clear still wipes the whole window. Today this is benign only because the neighbouring region is the minimap. It is drawn after the world clear in the same frame, and CMiniMap::LoadDualViewport (MiniMap.cpp:324-332) re-clears its own region under its own scissor. So dual-screen does not exercise "wiping a neighbour's finished output". It exercises "clear everything, then paint the neighbour afterwards". A multi-target design where target B's world pass runs after target A's full frame on the same drawable would wipe A. This cannot happen with one drawable per OS window. A fix would wrap the clear in glEnable(GL_SCISSOR_TEST), glScissor(target rect), glClear and glDisable. It must restore scissor-off afterwards, because CGLStateChecker::VerifyState expects GL_SCISSOR_TEST to be GL_FALSE in 3D areas (glStateDebug.cpp:176). The only alternative is per-target FBOs.

*Evidence:*
- `rts/Rendering/WorldDrawer.cpp:309` unscissored glClear of the whole bound FB
- `rts/Rendering/GlobalRendering.cpp:1433` dual-screen sub-viewport geometry via viewPos offsets
- `rts/Game/UI/MiniMap.cpp:325` GUI-only scissor use

*Direction:* Document that each render target owns a full drawable. That covers the 2-window prototype and any one-window-per-target design, so no code change is needed. Only if sub-rect targets, such as split-screen inside one window, are explicitly added to scope, scissor the clear to the target rect and restore GL_SCISSOR_TEST off afterwards. Alternatively, render each sub-rect target to its own FBO and composite.

> verifier A (corrected): The code facts are accurate: WorldDrawer.cpp:309 does an unscissored glClear of color, depth and stencil on whatever FB is bound. A grep of rts (excluding rts/lib) finds glScissor/GL_SCISSOR_TEST only in MiniMap, Rml and Lua code, and no 3D pass uses it. For two OS windows each target has its own default framebuffer, so the clear is harmless, as the claim says. The problem only applies to a hypothetical sub-rectangle target that shares a drawable with another. The goal is multiple windows and th

## Per-target state in GlobalRendering

### TGT-01 - Window, GL context and view geometry are singleton state in CGlobalRendering
**Severity:** blocker - **Effort:** L - **Status:** corrected - **Depends on:** none

There is no render-target object. One SDL_Window, one GL context and one set of view/window geometry fields are members of the process-wide globalRendering, and a 3D frame is drawn with exactly one FBO::Unbind, camera->LoadViewport and WorldDrawer::Draw.

*Why it breaks:* A second window has nowhere to store its size, position, aspect, pixel size, screen matrices, dirty flag or active flag. UpdateGLGeometry/ReadWindowPosAndSize write these globals in place from sdlWindow, so any second-window update would overwrite window 1's geometry that the rest of the frame reads. A RenderTarget struct is the unit all other problems hang off.

*Evidence:*
- `rts/Rendering/GlobalRendering.h:156` screenSize, winPos/Size, viewPos/Size, dual*, winBorder, winChgFrame, matrices, pixelX/Y, aspectRatio are plain members (156-216)
- `rts/Rendering/GlobalRendering.h:384` single SDL_Window* sdlWindow and SDL_GLContext glContext fields
- `rts/Rendering/GlobalRendering.cpp:132` all geometry fields are CR_IGNORED (132-217), so they can move out of the class without save-game impact
- `rts/Rendering/GlobalRendering.cpp:1689` UpdateGLGeometry mutates globals in place
- `rts/Game/Game.cpp:1521` one Unbind + camera->LoadViewport + worldDrawer.Draw per frame
- `rts/Rendering/GlobalRendering.h:45` CreateWindowAndContext/CreateSDLWindow/CreateGLContext/DestroyWindowAndContext have no target parameter
- `rts/Rendering/GlobalRendering.cpp:442` only SDL_CreateWindow call site
- `rts/Rendering/GlobalRendering.cpp:570` result assigned straight to the member sdlWindow
- `rts/Rendering/GlobalRendering.cpp:587` single context created from sdlWindow
- `rts/Game/UI/HwMouseCursor.cpp:221` example external GetWindow() consumer (also :596, :703, :721)
- `rts/Rendering/GlobalRendering.cpp:398` CreateSDLWindow builds one window; attribute search loop 432-449
- `rts/Rendering/GlobalRendering.cpp:612` MakeCurrentContext always uses sdlWindow/glContext
- `rts/System/LoadLock.h:21` load lock rebinds the primary window
- `rts/Rendering/GlobalRendering.cpp:345` sdlWindow{nullptr}, glContext{nullptr} are singletons
- `rts/Rendering/GlobalRendering.cpp:703` only SDL_GL_SwapWindow call
- `rts/Rendering/GL/VAO.cpp:21` VAO creation, non-shared container object
- `rts/Rendering/GL/FBO.cpp:250` FBO creation, non-shared container object
- `rts/Rendering/GlobalRendering.cpp:664` timer queries created on the single context
- `rts/System/SpringApp.cpp:1246` DISPLAY_CHANGED handler re-makes primary current

*Direction:* Keep the existing globalRendering geometry fields as a mirror of the active target, so the roughly 300 readers need no change. Add a small RenderTarget {SDL_Window*, winSize, viewSize, pixelX/Y, aspect, camera ref}. Add ActivateTarget(t), which does SDL_GL_MakeCurrent(t.window, glContext), copies t's geometry into the globalRendering fields, and leaves the FBO and viewport reload to the caller. Add an RAII scope that restores the primary target. Add a per-target SwapBuffers(t) (SDL_GL_SwapWindow(t.window)); window 2 should use swap interval 0 or be swapped last, to avoid double vsync stalls.

For the prototype, create window 2 once with the same pixel format as window 1, pin the same depth/MSAA attributes, and give it the same size as window 1 so the screen-sized resources (GeometryBuffer, DepthBufferCopy, BumpWater/RefractWater screen textures) need no per-target handling yet. Defer per-target screen matrices, border and dual-screen fields, dirty stamps, fullscreen and config persistence to later.

Fold in a minimal SDL window-event router for the same milestone. SpringApp::MainEventHandler (SpringApp.cpp:1095-1252) never checks event.window.windowID, and no windowID or SDL_GetWindowFromID is used anywhere in rts outside lib. As written, closing window 2 calls RequestQuit() (:1249-1251). Minimizing or hiding it sets the global globalRendering->active=false (:1179-1185), which makes CGame::Draw return early and stops the primary window too (Game.cpp:1471-1479). Resize and move on window 2 re-run window 1's UpdateGLGeometry, InitGLState and ResizeEvent (:1135-1157).

> verifier A (confirmed): Verified in the current tree. CGlobalRendering is the process-wide singleton and holds the geometry directly: screenSize/winPos/winSize/viewPos/viewSize/viewWindowOffsetY/dual*/winBorder/winChgFrame/screenView/ProjMatrix/pixelX/Y/aspectRatio (GlobalRendering.h:156-216, all CR_IGNORED at GlobalRendering.cpp:132-217). It also holds exactly one `SDL_Window* sdlWindow` and one `SDL_GLContext glContext` (GlobalRendering.h:384-385, initialised to nullptr at GlobalRendering.cpp:345-346). The window and

> verifier B (corrected): The mechanism is real and I confirmed it in code. CreateSDLWindow has the only SDL_CreateWindow (GlobalRendering.cpp:442) and returns into the member sdlWindow (:570). The only GL context is created from that window (:474, :587), and MakeCurrentContext (:612) and SDL_GL_SwapWindow (:703) are hard-wired to the same two members. ReadWindowPosAndSize (:1571-1572) and UpdateGLGeometry (:1689-1696) read sdlWindow and write the singleton winSize, viewSize, pixelX/Y, aspect and screen matrices in place

### TGT-02 - Window geometry pipeline and dirty flags are hard-wired to the primary window
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** TGT-01; Primary-only window and context handles in CGlobalRendering

UpdateGLGeometry (ReadWindowPosAndSize, UpdateViewPortGeometry, UpdatePixelGeometry, UpdateScreenMatrices) reads only sdlWindow; the resize/move/config dirty state is two global frame stamps, and InitGLState and SetWindowAttributes act on the primary window.

*Why it breaks:* Window-2 geometry is simply never read. The geometry chain (GlobalRendering.cpp:1689-1699) is hard-wired to sdlWindow, but the config and dirty-stamp machinery is window-1-only by construction and is inert for window 2. The actual hazards are three things.

1. The global SDL event handler (SpringApp.cpp:1095-1263) does not check windowID. Every window-2 event runs primary-window logic: water rebuild, active=false freezing both views, quit on close, key release on focus loss.
2. Per-target size has no source. The camera, uniforms and size-dependent buffers all read globalRendering->viewSize*, which is the dependency problem.
3. InitGLState's once-per-context state (GlobalRendering.cpp:1705-1724) needs re-running only if window 2 gets its own GL context. With one shared context and MakeCurrent per window it does not.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:1546` ReadWindowPosAndSize uses SDL_GetWindowSize/Position(sdlWindow) (1571-1572)
- `rts/Rendering/GlobalRendering.cpp:1259` ConfigNotify sets winChgFrame/gmeChgFrame and may call UpdateGLGeometry (1272-1281)
- `rts/Rendering/GlobalRendering.cpp:1285` UpdateWindow: stamp compare against drawFrame then SetWindowAttributes + MakeCurrent (1295-1305)
- `rts/Rendering/GlobalRendering.cpp:1726` InitGLState glClear + LoadViewport on the current drawable
- `rts/System/SpringApp.cpp:1135` SIZE_CHANGED runs SaveWindowPosAndSize, UpdateGL*, InitGLState, UpdateInterfaceGeometry, ResizeEvent (1135-1157)
- `rts/Rendering/GlobalRendering.cpp:1689` UpdateGLGeometry chain reads only the primary window
- `rts/Rendering/GlobalRendering.cpp:1571` SDL_GetWindowSize(sdlWindow)
- `rts/Rendering/GlobalRendering.cpp:1615` screen matrices derived from the primary window's desktop position
- `rts/Rendering/GlobalRendering.cpp:2018` LoadViewport uses global viewPos/viewSize (20 callers)
- `rts/Rendering/UniformConstants.cpp:222` viewGeometry uniform from global view size
- `rts/Rendering/DepthBufferCopy.cpp:140` size-dependent resource allocated from global view size

*Direction:* Do not refactor UpdateGLGeometry to take a RenderTarget& for the prototype. There are about 218 viewSize/viewPos reads across 39 files, so a per-target signature changes nothing until those readers move too, which belongs in the singleton-geometry problem.

Prototype-sized plan, effort S:
1. Add a small per-target ViewGeometry struct filled by a RefreshFromWindow(SDL_Window*) helper. It uses SDL_GL_GetDrawableSize and recomputes pixelX/Y and aspectRatio. Poll it at the start of each target's draw instead of driving it from SIZE_CHANGED.
2. In MainEventHandler, filter on event.window.windowID. Primary-window events keep the current path. Secondary-window events are routed to the target or ignored. Do not let a secondary window's HIDDEN/MINIMIZED flip globalRendering->active or trigger ResizeEvent and the water rebuild. Treat secondary CLOSE explicitly (close that target, or quit) rather than falling into RequestQuit by accident.
3. Keep ConfigNotify, winChgFrame/gmeChgFrame, SaveWindowPosAndSize, SetWindowAttributes and dual-screen mode primary-only. Document dual-screen as unsupported while a secondary target exists.
4. Run InitGLState once per GL context, not per target. Skip it entirely if the prototype shares one context across both windows.
5. Keep drawFrame a per-frame counter, not per target.

Full per-target dirty flags and config keys (window-2 position, size and fullscreen persistence) are a later 'full' item.

> verifier A (corrected): The core facts hold: the geometry chain reads only the primary window, and the SDL window-event handler never filters by window. The mid-frame claim and the "dirty state is two stamps" framing are wrong, and the blocker rating is too high.

Confirmed in the current tree:
- ReadWindowPosAndSize uses SDL_GetWindowSize and SDL_GetWindowPosition on sdlWindow (GlobalRendering.cpp:1571-1572).
- UpdateGLGeometry is ReadWindowPosAndSize, UpdateViewPortGeometry, UpdatePixelGeometry, UpdateScreenMatrices 

> verifier B (corrected): The facts are accurate: ReadWindowPosAndSize reads only sdlWindow (GlobalRendering.cpp:1571-1572), winChgFrame/gmeChgFrame are single stamps (1277/1281/1295-1298), ConfigNotify calls UpdateGLGeometry directly (1274), InitGLState clears and calls LoadViewport (1726-1729), and SaveWindowPosAndSize writes window-1 keys (1600-1603). But "blocker" overstates it for the 2-window, 3D-only prototype, for three reasons.

1. The part that does break the prototype is the per-target viewSize/viewPos, pixel 

### TGT-03 - Screen-copy refraction and depth copy read the current read framebuffer
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** none

BumpWater and RefractWater copy the already-rendered scene with glCopyTexSubImage2D from whatever framebuffer is currently bound for reading.

*Why it breaks:* BumpWater, RefractWater and DepthBufferCopy allocate screen-sized scratch textures once, from the primary view's viewSize at init. BumpWater also bakes the inverse screen size into the shader defines. None of them is resized per view or on ViewResize. For a second view whose size differs, glCopyTexSubImage2D / FBO::Blit use a rect larger or smaller than the texture, which gives GL_INVALID_VALUE (bigger) or a wrong UV scale (smaller). The read source itself is fine as long as the view's framebuffer is bound for read and draw with a single-sample depth buffer.

*Evidence:*
- `rts/Rendering/Env/BumpWater.cpp:906` colour copy; depth copy at 912
- `rts/Rendering/Env/RefractWater.cpp:65` glCopyTexSubImage2D from current read FB
- `rts/Rendering/WorldDrawer.cpp:343` depthBufferCopy->MakeDepthBufferCopy is a separate existing copy of the same buffer

*Direction:* Do not re-route the read source. Keep binding the view target before WorldDrawer::Draw (default FB, or a single-sample FBO with colour and depth). Fix the sizing instead. Either (a) require equal window sizes, or run Water=1 or skip water on the secondary, for the prototype; or (b) key BumpWater/RefractWater/DepthBufferCopy scratch textures per view (or reallocate on a view-size change), and pass the screen-inverse sizes as per-draw uniforms instead of compile-time defines. Do NOT share BumpWater's depth copy with DepthBufferCopy. DepthBufferCopy::MakeDepthBufferCopy is called once, right after gd->Draw (WorldDrawer.cpp:343), so it holds terrain-only depth. BumpWater's depth is copied at water->Draw() (WorldDrawer.cpp:439), after opaque units, features and projectiles. Merging them would change the water depth and shoreline effect. Re-running MakeDepthBufferCopy at water time would instead change the soft-particle and decal depth the projectile and decal passes sample (ProjectileDrawer.cpp:967,1203; GroundDecalHandler.cpp:502). If MSAA is ever supported, the existing pattern is to blit the multisampled source into a single-sample FBO and copy from that. Treat MSAA as a pre-existing bug, not a multi-target blocker.

> verifier A (corrected): The cited lines are real (BumpWater.cpp:906/912 and RefractWater.cpp:65 call glCopyTexSubImage2D with no explicit read-FBO binding). But the claimed mechanism is only a hypothetical about a future FBO-based design. In the current tree the world is drawn into the default framebuffer: Game.cpp:1522-1526 calls FBO::Unbind() then camera->LoadViewport() then worldDrawer.Draw(). The water draw at WorldDrawer.cpp:439 therefore reads the current context's own back buffer, and each window's own default f

> verifier B (corrected): The stated mechanism is not a defect for the prototype. glCopyTexSubImage2D in BumpWater.cpp:906/912 and RefractWater.cpp:65 reads the currently bound read framebuffer. In the simplest design that is correct without changes. Make window N current, bind FB 0 (or a per-view FBO via glBindFramebuffer(GL_FRAMEBUFFER), which sets read and draw together), then run WorldDrawer::Draw. The copy then reads that view's colour and depth. The depth-bits worry does not hold: GlobalRendering.cpp:544 sets SDL_G

### TGT-04 - Screen-space matrices and desktop-relative geometry are per window
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** TGT-01

screenView/ProjMatrix are derived from the window's desktop position and its display's size (a pixel-coordinate frustum), and screenSize/Pos come from the display containing sdlWindow.

*Why it breaks:* Verified at the cited lines.

1. `CGlobalRendering::UpdateScreenMatrices` (GlobalRendering.cpp:1615-1647) builds a pixel-coordinate perspective frustum from `screenSizeX/Y`, `winPosY`, `winSizeY`, `viewPosX/Y` and `winPosX`. `ReadWindowPosAndSize` (1546-1565) fills `screenSizeX/Y/PosX/PosY` via `GetDisplayBounds(screenSize)`. With no index argument that resolves to `GetCurrentDisplayIndex()`, which is `SDL_GetWindowDisplayIndex(sdlWindow)` (1389-1397). `winPosX/Y` come from `SDL_GetWindowPosition(sdlWindow)` (1572). So these values differ per window even at equal size.

2. Consumers:
   - `UniformConstants::UpdateMatricesImpl` (UniformConstants.cpp:163-165) copies `screenViewMatrix` and `screenProjMatrix` into the single global UBO and computes `screenViewProj`. `UpdateMatrices` (308-316) is called once per frame through `UniformConstants::Update` at Game.cpp:1441. The UBO is declared in ModelVertProgGL4.glsl and ShadowGenVertProgGL4.glsl (lines 17-19), but no engine shader reads `screenView`, `screenProj` or `screenViewProj`. Only game-side Lua shaders could.
   - `LuaOpenGL::SetupScreenMatrices` (LuaOpenGL.cpp:985-994) loads them into the fixed-function matrix stacks. It is reached through `EnableDrawScreenCommon` (832-841) and `ResetScreenMatrices` (1082-1089), i.e. the `DrawScreen`, `DrawScreenEffects` and `DrawScreenPost` Lua callins. These are GUI and are not part of the 3D-only prototype. (The claim's "gl.LoadIdentity-style screen projection" is vague.)

3. Wrong as stated: `numDisplays` is not per window. It is the system-wide `SDL_GetNumVideoDisplays()` (GlobalRendering.cpp:1227), refreshed in `SetWindowAttributes`. It should stay in a global display/platform struct, not on RenderTarget. Refresh rate is only read live per window in `LuaUnsyncedRead::GetWindowDisplayMode` (962-966, main window only) and in the `LogDisplayMode` log line (1155-1167). The engine does not use `refresh_rate` for pacing. `Platform.hz` comes from the system-wide `availableVideoModes` list (LuaConstPlatform.cpp:87).

4. Extra mechanism: recomputation is tied to singleton events. `UpdateScreenMatrices` runs only inside `UpdateGLGeometry` (GlobalRendering.cpp:1696). That is called from SIZE_CHANGED (SpringApp.cpp:1145), from MOVED only when `numDisplays > 1 && dualScreenMode` (1113-1118), and from `ConfigNotify` (1274). A plain MOVED calls `SaveWindowPosAndSize`, which re-reads `winPos` and `screenSize` (SpringApp.cpp:451) but does not rebuild the matrices, so they go stale after a move. `MainEventHandler` does not check `event.window.windowID`. Per-target geometry therefore needs per-window event routing, not just per-target storage.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:1615` UpdateScreenMatrices from single-window geometry (1620-1646)
- `rts/Rendering/UniformConstants.cpp:163` copied into the UBO
- `rts/Lua/LuaOpenGL.cpp:990` loaded for Lua screen-space drawing
- `rts/Rendering/GlobalRendering.cpp:1389` GetCurrentDisplayIndex/GetDisplayBounds use sdlWindow (1389-1404)

*Direction:* Keep the plan to put `screenView/ProjMatrix`, `screenSize/Pos` and the display index on RenderTarget, recomputed in the target's `UpdateGeometry` and driven by SDL window events routed by `windowID`. Leave `numDisplays` and the video-mode list in the global platform info. Per-target display mode or refresh rate is an optional Lua-query concern. The per-target UBO upload only matters once the GUI/Lua DrawScreen path is brought to extra targets; for the 3D-only prototype the screen fields can be left stale or global without visible effect.

> verifier A (corrected): The core mechanism holds and the prototype classification (full, not blocker) is right. The screen matrices are derived from the display containing sdlWindow plus the window's desktop position, so they differ per window. In the current tree the only consumers are the UBO fields, which no engine shader reads, and the Lua DrawScreen* callins, which are GUI. Two details in the claim are wrong: numDisplays is not per window, and refresh rate has no engine consumer. The Lua entry point is also named 

## Cameras & view state

### CAM-01 - Camera matrices reach shaders through two channels, the fixed-function stack and a once-per-frame player-camera UBO
**Severity:** blocker - **Effort:** M - **Status:** confirmed - **Depends on:** CCamera::Update derives viewport, aspect and pixel scale from globalRendering and writes the GL matrix stack

21 shader files read gl_ModelViewProjectionMatrix and friends (context matrix stack), while 3 GL4 shaders read a UBO built once per frame from CAMTYPE_PLAYER. A second target would see one camera through one channel and the other through the other.

*Why it breaks:* GL4 unit/feature rendering is the default path. It reads cameraViewProj, reflectionViewProj and cameraViewInv from the UBO, which is filled once per frame in UpdateUnsynced from the PLAYER camera slot. The terrain/sky/water/projectile path reads the GL matrix stack, which each target reloads via CCamera::Update/LoadMatrices. With two targets, window 2's terrain and the GL4 units disagree on the camera, so units are misplaced or invisible.\n\nTwo side effects matter for any per-target refresh:\n1. UpdateMatricesImpl calls reflCam->CopyStateReflect(prvCam) (UniformConstants.cpp:186-189). That runs Camera::Update(false,true,false) (Camera.cpp:138), which ends in LoadMatrices() (Camera.cpp:159) and so overwrites GL_PROJECTION and GL_MODELVIEW with the reflection camera's matrices. It also hard-resets the active camera to PLAYER (:195). Calling it mid-Draw clobbers the stack and the active camera, so the camera must be loaded again afterwards.\n2. CCamera::Update derives aspect and viewport from globalRendering (Camera.cpp:144-155), so the target's view size must be current before the UBO refresh.

*Evidence:*
- `rts/Rendering/UniformConstants.cpp:167` UBO camera matrices hard-wired to CAMTYPE_PLAYER
- `rts/Rendering/UniformConstants.cpp:222` viewGeometry from globalRendering
- `rts/Rendering/UniformConstants.cpp:230` fog params from player far plane
- `rts/Game/Game.cpp:1441` UBO updated once per frame in UpdateUnsynced; bound at 1464
- `rts/Game/Camera.cpp:289` CCamera::LoadMatrices() writes GL_PROJECTION/GL_MODELVIEW from the camera
- `cont/base/springcontent/shaders/GLSL/ModelVertProgGL4.glsl:1` GL4 path reads UBO cameraViewProj; 21 other shaders use gl_ModelViewProjectionMatrix
- `rts/Rendering/GL/RenderBuffers.inl:15` gl_Position = gl_ModelViewProjectionMatrix * ... in the generated RenderBuffer vertex shader
- `cont/base/springcontent/shaders/GLSL/SMFVertProg.glsl:47` terrain MVP from compat stack; ModernSkyVS.glsl:55,63 sky likewise
- `rts/Map/SMF/SMFRenderState.cpp:249` Enable reloads modelview from camera->GetViewMatrix(); cameraPos uniform at 294
- `rts/Rendering/Env/GrassDrawer.cpp:363` camPos/camDir/camUp/camRight set per EnableShader from global camera; similar at BumpWater.cpp:932, ProjectileDrawer.cpp:980/1213, NanoParticleRenderer.cpp:715

*Direction:* Do not parameterise UpdateMatricesImpl by camera. Many other call sites hard-code CAMTYPE_PLAYER: ShadowHandler.cpp:130, GrassDrawer.cpp:518/616, ProjectileDrawer.cpp:494/970/1206, UnitDrawerData.cpp:430-457, FeatureDrawerData.cpp:121-144, BasicMeshDrawer.cpp:181, RoamMeshDrawer.cpp:219. A UBO that follows a different camera than those would be inconsistent. Instead, have BeginTarget load the target's camera state into the PLAYER slot and set the target's view size and viewport. The UBO code can then stay as is.\n\nSplit UniformConstants::Update() into two parts:\n1. A per-frame part (UpdateParams: time, mouse ray trace, 2048-entry team colours, sun) that stays in UpdateUnsynced. Do not call it per target, because it runs a GuiTraceRay each time.\n2. A per-target part (UpdateMatrices, plus fogParams.xy and viewGeometry written as a sub-range or a tiny separate UBO) called inside BeginTarget.\n\nThe per-target sequence must be: set globalRendering view size, camera->Update, shadowHandler.Update (the UBO copies the shadow matrices, so it must come after), UpdateMatrices (which clobbers the GL matrix stack), Bind, then camera->LoadMatrices and LoadViewport.\n\nThis is cheap because the buffer is SB_BUFFERSUBDATA: Map/Unmap issue an immediate glBufferSubData (StreamBuffer.h:240-251) and Bind is just glBindBufferRange, so repeated updates in one frame are safe. If targets run on different GL contexts, UBO binding points are per-context, so Bind() must be repeated per context.\n\nNote the shadow matrices in the UBO must match whichever shadow map the target samples. That is a separate shadow problem, but the UBO refresh is where it surfaces.

> verifier A (confirmed): Verified in the current tree. UniformConstants::UpdateMatricesImpl (UniformConstants.cpp:161-211) hard-wires camera view/proj/viewProj and inverses to CCameraHandler::GetCamera(CAMTYPE_PLAYER) at line 167. UpdateParamsImpl takes viewGeometry from globalRendering (222) and fogParams plus the mouse trace from camPlayer (230-290). UniformConstants::Update() (UniformConstants.h:107-110) calls UpdateMatrices and UpdateParams. Its only caller is CGame::UpdateUnsynced (Game.cpp:1441), after camera->Upd

> verifier B (confirmed): The problem is real and blocks the prototype. The GL4 model drawer is the default whenever haveGL4 is true. ModelDrawer.h:285-292 loops from MODEL_DRAWER_GL4 upward and picks the best qualifying drawer, and ModelDrawerState.cpp:290 has CanEnable() return globalRendering->haveGL4. The GL4 vertex shader takes gl_Position from the UBO, not the matrix stack: ModelVertProgGL4.glsl:125/133 use cameraViewProj, :129 uses reflectionViewProj, and :372/377 use cameraViewInv for fog. The UBO is filled once 

### CAM-02 - Camera objects are a fixed static array by purpose, so no second player camera can exist
**Severity:** blocker - **Effort:** M - **Status:** unverified - **Depends on:** DSN-01; RenderView definition: per-target parameters consumed by model drawers

The six CCamera objects are a file-static array indexed by purpose. UpdateFrustum writes the culling camera via the global slot instead of `this`, so a second PLAYER-type CCamera instance would corrupt VISCUL. Config observers, arrays sized by camType and the clip-control matrix are also tied to the singleton set.

*Why it breaks:* CCamera::UpdateFrustum does `curCam = CCameraHandler::GetCamera(camType)` and copies that slot to VISCUL. A camera object that is not the global slot of its type copies the wrong camera's frustum. Each of the 6 cameras registers as a configHandler observer. Data is indexed by camType (Patch::lastDrawFrames[4], CProjectile::sortDist[3]). So a second PLAYER-type instance either needs a new camType or the arrays must be keyed by (view, type).

*Evidence:*
- `rts/Game/Camera.cpp:227` visCam->CopyState(CCameraHandler::GetCamera(camType)): uses the global slot, not *this
- `rts/Game/CameraHandler.cpp:94` static CCamera cameras[CAMTYPE_COUNT]; SetActiveCamera/GetCamera at 100-126
- `rts/Game/CameraHandler.cpp:107` InitConfigNotify registers every camera as a ConfigHandler observer (removed at :118)
- `rts/Game/CameraHandler.cpp:106` clip-control matrix baked from the global globalRendering capability at InitStatic
- `rts/Map/SMF/ROAM/Patch.h:212` arrays sized by camera type show ripple of adding camera types
- `rts/Sim/Projectiles/Projectile.h:132` std::array<float,3> sortDist indexed by camType
- `rts/Game/Camera.h:316` #define camera (CCamera::GetActive())
- `rts/Game/Camera.cpp:144` Update uses globalRendering pixelY/aspect/viewSize (144-155)
- `rts/Rendering/Units/UnitDrawer.cpp:1093` representative ambient camera->InView use; 17 uses in this file
- `rts/Rendering/LuaObjectDrawer.cpp:633` AddOpaqueMaterialObject uses camera->ProjectedDistance

*Direction:* Change CCamera::UpdateFrustum to copy `this`. Move the camera set into the RenderView struct: one set per view, containing PLAYER, UWREFL and VISCUL, with ENVMAP and SHADOW either shared or per-view by decision. Replace the per-camera config observers with one shared camera-config struct. Re-key the per-type arrays (patches, projectile sortDist) by view index. Keep the static array only for the primary view or as an alias to view 0.

### CAM-03 - CCamera::Update and CalcPixelDir derive viewport, aspect and LOD scale from globalRendering
**Severity:** blocker - **Effort:** M - **Status:** unverified - **Depends on:** DSN-01; DSN-03

Every Update() overwrites the camera's aspect ratio, LPP scale, projection size and viewport from the global primary-window geometry. A second window with a different size gets the wrong aspect, glViewport and LOD scale.

*Why it breaks:* Update() sets lppScale from globalRendering->pixelY and aspectRatio from globalRendering->aspectRatio. It then builds matrices from viewSizeX/Y and stores the viewport from viewPos/viewSize. Even if camera 2's aspect is set by hand, the next Update() (WorldDrawer::Draw calls it at :316, and every prvCam->Update() restore does too) resets it to the primary window's values. CalcPixelDir uses globalRendering viewPosX/viewSize. LuaObjectDrawer LOD uses lppScale, so LOD differs per view. A newly constructed camera has viewport {0,0,0,0} (Camera.cpp:48), so CalcViewPortCoordinates yields zero or NaN until Update.

*Evidence:*
- `rts/Game/Camera.cpp:144` lppScale/aspectRatio from globalRendering
- `rts/Game/Camera.cpp:153` UpdateMatrices(globalRendering->viewSizeX, viewSizeY, aspect) and UpdateViewPort(viewPosX, viewPosY, viewSizeX, viewSizeY) at :155
- `rts/Game/Camera.cpp:568` CalcPixelDir uses globalRendering view size and viewPosX
- `rts/Rendering/LuaObjectDrawer.cpp:324` model LOD factor uses camera->GetLPPScale(), which comes from globalRendering->pixelY
- `rts/Rendering/Units/UnitDrawerData.cpp:217` icon size base uses globalRendering view size
- `rts/Game/Camera.cpp:141` Update reads globalRendering view fields (141-162)
- `rts/Game/Camera.cpp:736` GetMoveVectorFromState uses winSizeX and the dual* viewport (736-757)
- `rts/Game/Camera.h:270` lppScale, aspectRatio, viewport[4] stored on the camera (270-273)
- `rts/Rendering/Env/AdvWater.cpp:349` prvCam->Update(); prvCam->LoadViewport() restore idiom (also DynWater.cpp:486)
- `rts/Rendering/Env/CubeMapHandler.cpp:222` UpdateMatrices fed from global viewSize
- `rts/Game/Camera.cpp:589` CalcViewPortCoordinates uses per-camera viewport[]
- `rts/Rendering/Env/AdvWater.cpp:134` CalcPixelDir with globalRendering sizes
- `rts/Rendering/UniformConstants.cpp:276` CalcPixelDir used for the per-frame mouse trace
- `rts/Game/Camera.cpp:154` viewport[] overwritten from globalRendering view geometry
- `rts/Game/Camera.cpp:159` LoadMatrices() unconditional side effect
- `rts/Game/Camera.cpp:138` CopyStateReflect calls Update()
- `rts/Rendering/Env/BumpWater.cpp:1038` prvCam->Update() restore after reflection
- `rts/Rendering/ShadowHandler.cpp:656` prvCam->Update() restore after shadow pass

*Direction:* Add a RenderViewport value (x, y, w, h, aspect, pixelX/Y) to CCamera, set when binding a camera to a view. Update(params) uses the camera's own viewport. GlobalRendering fields become the primary view's mirror. Do this before touching the other fixes.

### CAM-04 - Global active-camera pointer (camera macro / CAMTYPE_ACTIVE) is the only view selector
**Severity:** risk - **Effort:** L - **Status:** corrected - **Depends on:** DSN-01

All draw code learns which camera to use through CCamera::GetActive(), a static selector that sub-passes swap and restore. There is no notion of a per-target current camera.

*Why it breaks:* `camera` is the only view selector, and it is type-based (a type id stored in a static placeholder slot, not a pointer). Nested sub-passes (shadow, cubemap, water reflection) swap and restore the previous type. That already composes with a per-view `cameras[]`, so the selector itself is a cheap change (S). For a 2-window prototype, the more important coupling is that the 'current view camera' is not only reached through the selector. About 20 sites read cameras[CAMTYPE_PLAYER] directly (UniformConstants.cpp:167,230; BeamLaserProjectile.cpp:119; ProjectileDrawer.cpp:494,970,1206; GrassDrawer.cpp:518,616; BasicMeshDrawer.cpp:181; RoamMeshDrawer.cpp:219; ShadowHandler.cpp:130; MiniMap.cpp:1449). Visibility and sort caches are indexed by camType (UnitDrawerData.cpp:430-457, FeatureDrawerData.cpp:121-144, ProjectileDrawer.cpp:515, BasicMeshDrawer.h:43). A global VISCUL singleton is overwritten by every CCamera::UpdateFrustum (Camera.cpp:217-230). CCamera::Update also reads the global globalRendering viewSize and aspect ratio (Camera.cpp:144-155). The camera controllers drive the single PLAYER slot through the `camera` macro (CameraHandler.cpp:286-579). Any design must therefore back both `camera` and GetCamera(type) with the current view. Alternatively, it can reload the PLAYER slot per view and recompute the camType-keyed caches per view. The UniformConstants.cpp:195 hard-coded PLAYER restore should become 'restore prvCam->GetCamType()' as a hygiene fix, and GetCamera(ACTIVE) should resolve to the active camera. The MT particle fill is safe as long as views render sequentially on the main thread. It only becomes a problem if two views are ever drawn concurrently.

*Evidence:*
- `rts/Game/Camera.h:316` #define camera (CCamera::GetActive())
- `rts/Game/CameraHandler.cpp:100` SetActiveCamera only stores a type in cameras[ACTIVE]; GetActiveCamera (126) indexes the static array by it
- `rts/Rendering/UniformConstants.cpp:195` restore is hard-coded SetActiveCamera(CAMTYPE_PLAYER), not the previously active camera
- `rts/Rendering/ShadowHandler.cpp:650` GetSetActiveCamera(SHADOW) ... SetActiveCamera(prvCam->GetCamType()) (655) pattern, repeated in CubeMapHandler.cpp:204/241, BumpWater.cpp:1026/1036, AdvWater.cpp:339/349, DynWater.cpp:469/483
- `rts/Rendering/Common/ModelDrawer.h:324` asserts on the active camera's type being (not) SHADOW; type is also used as identity elsewhere
- `rts/Rendering/Env/Particles/Classes/SimpleParticleSystem.cpp:136` billboard basis taken from the active camera in Draw()
- `rts/Sim/Projectiles/WeaponProjectiles/BeamLaserProjectile.cpp:117` beam orientation from the active camera
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:120` MT fill reads the global camera from worker threads
- `rts/Game/CameraHandler.cpp:94` static cameras[CAMTYPE_COUNT]; ACTIVE slot only stores which type is active (100, 126)
- `rts/Lua/LuaOpenGL.cpp:1070` ResetWorldMatrices loads camera projection/view; used by Enable/Reset/Disable of all world callins (671-693)
- `rts/Lua/LuaOpenGL.cpp:1024` SetupScreenLighting uses camera->GetViewMatrix(); Billboard at 5469; ParseDrawUnit InView at 215; LOD distance at 1609
- `rts/Lua/LuaOpenGLUtils.cpp:194` named matrices (view/proj/viewproj/billboard, inverses) return pointers into the active camera (188-206)
- `rts/Lua/LuaMaterial.cpp:761` LuaMatUniforms::Execute feeds camera view/proj/pos/dir uniforms from the active camera (761-773)
- `rts/Lua/LuaUnsyncedRead.cpp:2113` GetVisibleUnits uses GridVisibility(nullptr,...) plus camera->InView (2148); IsUnitInView 1852; IsAABBInView 1941

*Direction:* Downgrade from blocker to risk. Keep the `camera` macro. Decide between (a) a per-view `cameras[]` indexed through a current-view pointer, with GetCamera(type) resolving through it, and (b) loading each view's camera state into the PLAYER slot immediately before its draw. Do (a) or (b) together with the camType-keyed visibility and sort caches and the VISCUL singleton. Make the UniformConstants restore use the previous type and fix GetCamera(CAMTYPE_ACTIVE). Treat the selector as an S-sized piece within the broader 'view context' problem. Do not rate it as a standalone prototype blocker.

> verifier B (corrected): The facts are mostly right but the severity and proposed fix are overstated. The selector is not what blocks the 2-window prototype. The `camera` macro expands to CCamera::GetActive() (Camera.h:316 -> Camera.cpp:96 -> CameraHandler.cpp:126), which indexes the static cameras[] array by the type stored in cameras[ACTIVE] (CameraHandler.cpp:94,100). That is a single-threaded, type-keyed indirection, and it works for N views if the views are rendered one after another on the main thread. The prototy

> verifier A (corrected): The facts mostly check out. `camera` expands to CCamera::GetActive() (Camera.h:316, Camera.cpp:96-100) and then to CCameraHandler::GetActiveCamera() (CameraHandler.cpp:126), which indexes the static `cameras[]` by the type stored in the ACTIVE slot (CameraHandler.cpp:94,100). Every cited Lua site reads the active camera (LuaOpenGL.cpp:215,1024,1070-1071,1609,5469; LuaOpenGLUtils.cpp:188-206; LuaMaterial.cpp:761-773; LuaUnsyncedRead.cpp:1852,1941,2148). The 'blocker' rating and the failure mechan

### CAM-05 - Hard-coded player-camera reads bypass the active-camera indirection
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** No per-view identity to key per-frame caches, flags and tokens on; About 110 billboard/LOD sites read the global active camera at draw time

Beyond the draw-flag prepass, 13 GetCamera(CAMTYPE_PLAYER) reads and other CAMTYPE_PLAYER uses fetch the main camera directly instead of the active or target's camera.

*Why it breaks:* The "active camera" is only an index. CCamera cameras[ACTIVE] stores a camType (CameraHandler.cpp:93,100,126), and the `camera` macro is CCamera::GetActive() (Camera.h:316). Fixed-function matrices and InView tests follow that index. Several subsystems instead read cameras[PLAYER] directly, and most do so in the once-per-frame Update phase, before any per-window draw. CGame::UpdateUnsynced runs shadowHandler.Update() (Game.cpp:1431), then worldDrawer.Update() (Game.cpp:1433), then UniformConstants::Update() (Game.cpp:1441). WorldDrawer::Update calls groundDrawer->Update() and projectileDrawer->UpdateDrawFlags() (WorldDrawer.cpp:219,227). All of these see only the PLAYER camera.

Per-site effects, with sub-severity:

- UBO (UniformConstants.cpp:167-179, 230-231, 272-276): BLOCKER. Model shaders read UBO cameraViewProj for gl_Position and cameraViewInv for worldCameraDir and fog (ModelVertProgGL4.glsl:125,133,372-390). The same function also clobbers the active camera with an unconditional SetActiveCamera(PLAYER) at line 195. The mouse trace in UpdateParamsImpl (lines 265-293) also uses camPlayer.
- Projectile flags (ProjectileDrawer.cpp:494-515): RISK. camPlayer->InView() and SetSortDist(PLAYER) are computed once per frame, so projectiles outside window 1's frustum get no draw flags in window 2.
- Grass (GrassDrawer.cpp:518-533): RISK. The in-view block lists are built from the PLAYER camera position in Update(), and Draw() (lines 561-587) only consumes them, so window 2 shows grass around window 1's camera.
- Shadow fit (ShadowHandler.cpp:130-133, 530-562): RISK. One shadow map is fitted to the PLAYER frustum, so window 2 gets wrong or missing shadows.
- ROAM (RoamMeshDrawer.cpp:219-220, 284, 360, 383; called via DrawMesh at 472-473): RISK. ROAM is the default (SMFGroundDrawer.cpp:53-57, defaultValue 1). Visibility uses the active camera, but tessellation distance uses the PLAYER camera position, and the mesh state is keyed only by pass (patchMeshGrid[shadowPass]). Window 2 gets a mesh tessellated for window 1.
- Projectile fx (ProjectileDrawer.cpp:970-982, 1206-1215): RISK. In ProjFXVertProg.glsl, gl_Position uses the fixed-function matrices (the active camera, lines 62-63) but fog uses the camPos uniform from the PLAYER camera (line 55), so each frame mixes both cameras.
- BeamLaser (BeamLaserProjectile.cpp:117 vs 119-120, 139): POLISH. Billboard axes use the active `camera`, but the LOD threshold uses the PLAYER camera.
- BasicMeshDrawer (BasicMeshDrawer.cpp:179-181): POLISH. It is the non-default drawer, and only the Shadow and WaterReflection passes force the PLAYER camera, on purpose, "to equal LOD".
- Minimap (MiniMap.cpp:1449): FULL. The minimap is out of prototype scope.

*Evidence:*
- `rts/Rendering/UniformConstants.cpp:167` GetCamera(PLAYER) at 167 and 230
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:219` playerCamera for tessellation
- `rts/Map/SMF/Basic/BasicMeshDrawer.cpp:181` GetCamera(PLAYER) fallback
- `rts/Rendering/ShadowHandler.cpp:130` playCam for shadow fit
- `rts/Rendering/Env/GrassDrawer.cpp:518` also 616
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:494` also 970
- `rts/Sim/Projectiles/WeaponProjectiles/BeamLaserProjectile.cpp:119` LOD chosen from PLAYER camera
- `rts/Game/UI/MiniMap.cpp:1449` minimap GetCamera(PLAYER)
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:970` camPlayer used for camPos/fog uniforms in DrawAlpha
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:1206` same in DrawGroundFlashes
- `cont/base/springcontent/shaders/GLSL/ProjFXVertProg.glsl:55` fogDist = length(pos - camPos)

*Direction:* Split this into two items.

(a) Per-view update steps. Introduce a per-view `BeginView(view)` that is not an accessor swap. For the view being rendered it should: copy that view's camera state into cameras[CAMTYPE_PLAYER] (CCamera::CopyState exists at Camera.cpp:103), call camera->Update with that view's size and aspect, and set active = PLAYER. Then re-run the once-per-frame steps that depend on the camera: ShadowHandler::Update and the shadow pass, the grass Update, the draw-flag prepass, and UniformConstants::Update followed by Bind. This keeps all 12 GetCamera(PLAYER) reads correct without touching them, because PLAYER is the camera of the view currently being rendered. Restore view 0 afterwards. Change UniformConstants.cpp:195 to restore prvCam instead of hard-setting PLAYER. For the UBO, use a ring or per-view slot, since BUFFERING=3 is sized for one upload per frame.

(b) Explicit per-view camera plumbing, later. Only where a PLAYER read must differ from the active camera, add a `MainCamera()` accessor on the current view. The reflection, refraction and shadow passes need this: the ProjectileDrawer fog camPos must be the viewer's camera, not the active UWREFL camera. Add a debug assert that PLAYER is read only inside BeginView/EndView.

For the prototype, skip or disable the following. Grass and ROAM can stay shared for now (ROAM per-view tessellation is a separate problem). For shadows, either set Shadows=0 or run the shadow pass per view. Leave MiniMap, BeamLaser and BasicMeshDrawer unchanged: the minimap is not drawn, the other two are polish.

> verifier A (corrected): The core mechanism holds. Live code fetches cameras[CAMTYPE_PLAYER] directly instead of the active camera at UniformConstants.cpp:167/230, RoamMeshDrawer.cpp:219, BasicMeshDrawer.cpp:181, ShadowHandler.cpp:130, GrassDrawer.cpp:518, ProjectileDrawer.cpp:494/970/1206, BeamLaserProjectile.cpp:119 and MiniMap.cpp:1449. Three details are wrong. (1) The count is 11 live reads, not 13: RoamMeshDrawer.cpp:183 is only a comment, and GrassDrawer.cpp:616 sits inside a dead /* ... */ block (opens line 594, 

> verifier B (corrected): The mechanism is real, but the claim needs correcting in four ways. (1) Inventory: the grep shows 12 GetCamera(PLAYER) hits, not 13. GrassDrawer.cpp:616 is dead code: it sits inside the commented-out body of DrawShadow, between /* at ~594 and */ at 638. Several sites are also out of scope or not live in the default config. MiniMap.cpp:1449 is GUI and is not drawn in the 3D-only prototype (full). BasicMeshDrawer.cpp:181 only matters when ROAM=0, and SMFGroundDrawer.cpp:53-57 defaults ROAM to 1; i

### CAM-06 - FPS direct control and unit tracking bind synced code and unit hiding to the one camera
**Severity:** full - **Effort:** M - **Status:** confirmed - **Depends on:** No per-view identity to key per-frame caches, flags and tokens on

Direct control writes the global camera from the sim, hides the controlled unit from model drawing, and tracking rewrites the camera each frame.

*Why it breaks:* CGroundMoveType::UpdateDirectControl calls camera->SetRotY from synced code for the local player's controlled unit. ShouldDrawOpaqueUnit returns false for gu->GetMyPlayer()->fpsController.GetControllee(), so window 2 would also never draw that unit. UpdateController(player, fpsMode) and unitTracker.SetCam are single-camera.

*Evidence:*
- `rts/Sim/MoveTypes/GroundMoveType.cpp:3331` camera->SetRotY in UpdateDirectControl (3305-3331)
- `rts/Rendering/Units/UnitDrawer.cpp:190` controllee never drawn
- `rts/Game/CameraHandler.cpp:274` fpsCon.oldDCpos / UpdateController(fpsMode) (258-297)
- `rts/Game/Game.cpp:1427` unitTracker.SetCam (1427-1428)

*Direction:* Prototype: FPS control and tracking apply to the primary target only, and window 2 can draw the controlled unit. Product: move the camera write out of the sim into the camera layer and make hiding depend on which target owns the FPS camera.

> verifier A (confirmed): Code matches the claim. GroundMoveType.cpp:3330-3331 writes the global camera rotation from the sim update when the local player controls the unit. UnitDrawer.cpp:190 returns false for the controllee in every pass except reflection, so a second window would also never draw that unit. CameraHandler.cpp:258-297 acts on a single global camera and camCon. It applies fpsCon.oldDCpos and fires camera events from lastCamPos/lastCamRot. Game.cpp:1427-1428 calls unitTracker.SetCam() on the single camera.

### CAM-07 - No independent camera driver: controller stack, transitions and unit tracker are a process singleton on the global camera
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** CAM-02

CCameraHandler lives in single static placement storage, and every controller reads or writes the global `camera`, `camHandler`, `mouse` and `globalRendering`. A second view has no way to own a controller or a transition state.

*Why it breaks:* What the prototype needs is narrower than the title says. The 3D draw path reads only the active camera slot (`camera` -> cameras[ACTIVE] -> cameras[CAMTYPE_PLAYER]), and CCamera::Update (Camera.cpp:141-162) just derives matrices, frustum and viewport from pos/rot/fov plus globalRendering sizes. So the minimum is to supply a per-view pose and load it into the player slot around that view's draw. The controller stack, CamTransitionState and UnitTracker can stay a process singleton bound to view 0. Two real side-couplings remain, and both are about caches computed once per frame from the primary view, not about the controller stack. CUnitDrawerData::Update (UnitDrawerData.cpp:224-230,256-265) derives iconZoomDist and sqCamDistToGroundForIcons from the primary controller and camera. The aspect and viewport inputs in CCamera::Update (Camera.cpp:144-155) come from globalRendering, which is a separate per-target-viewport problem.

*Evidence:*
- `rts/Game/CameraHandler.cpp:96` static camControllerMem[]/camHandlerMem; camHandler = new (camHandlerMem) CCameraHandler() at :112
- `rts/Game/CameraHandler.cpp:258` UpdateController drives the active camera via camCon.Update() and UpdateTransition() (:282-284); one CamTransitionState (CameraHandler.h:151)
- `rts/Game/Camera/FreeController.cpp:94` controllers mutate the global camera (also :50,80,127-136,166; FPSController.cpp:27,59; SpringController.cpp:131,167,198; OverheadController.cpp:104-105,127)
- `rts/Game/Camera/SpringController.cpp:224` camHandler->CameraTransition(...) global call (also OverheadController.cpp:123,137,193; DollyController.cpp:129)
- `rts/Game/UI/UnitTracker.cpp:253` writes camera and current controller together (:259-264,307-317)
- `rts/Game/CameraHandler.cpp:157` RegisterAction(viewfps...) on the single handler; configHandler->NotifyOnChange(this,...) at :177

*Direction:* Split the claim in two. (a) Prototype, effort S, and fold it into the existing camera-slot problem rather than a standalone blocker. Add a small per-view CameraPose {pos, rot, vfov} with a trivial driver (follow-primary-with-offset, fixed pose, or simple free-fly). Around each view's draw, push the pose into cameras[CAMTYPE_PLAYER] via SetPos/SetRot/SetVFOV and then call Update() after the view's size and aspect are set. Restore the primary pose afterwards, or draw the secondary view first. Keep the controller stack, UnitTracker, sound listener, CameraRotationChanged/CameraPositionChanged events and action registration primary-only. This avoids a pose-load path that needs copy-state plumbing for derived cameras, and it avoids touching ~40 global-camera call sites. (b) Product, severity full, effort L: refactor CCameraHandler into a per-view CameraRig (camera, controllers, CamTransitionState, tracker) with controllers taking a CCamera& or rig reference instead of the globals, and add per-view action routing. Defer this until after the prototype.

> verifier A (corrected): The code evidence checks out: CCameraHandler is placement-new'd into static camHandlerMem (CameraHandler.cpp:97,112), and the controllers write the global camera and call camHandler->CameraTransition/GetActiveCamera, as cited. The tween state is a single CamTransitionState (CameraHandler.h:151), and UnitTracker::SetCam writes camera and controller together (UnitTracker.cpp:253-317). But "blocker" is wrong. The claim's own last mechanism sentence says a second camera needs no controller stack. A 

> verifier B (corrected): The mechanism is real and the cited evidence checks out. CCameraHandler is a placement-new singleton in camHandlerMem (CameraHandler.cpp:97,112). It is a CommandReceiver that registers actions in Init() (:157-172) and observes config (:177). It owns a single camTransState (CameraHandler.h:151). Controllers mutate the global camera (FreeController.cpp:50,80,94,127-136; FPSController.cpp:27,59) and call camHandler->CameraTransition (SpringController.cpp:224; OverheadController.cpp:123,137,193; Dol

### CAM-08 - Reflection, refraction and env-map cameras are single slots derived from whichever camera is active
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** Per-frame uniform buffer carries the player camera matrices, fog and mouse state; Viewport and matrix reset idioms restore the primary window's state

UWREFL and ENVMAP exist once each and are re-derived from the then-active camera by CopyStateReflect or direct field writes. Their derived results (UBO reflection matrices, reflection draw flags and sort keys) are computed in the Update stage from PLAYER only.

*Why it breaks:* Reflection and env-map cameras are transient scratch slots re-derived from the active camera inside the draw (UpdateWater runs inside each world draw, WorldDrawer.cpp:437-439), so they stay correct per view as long as the view's camera is the active/PLAYER slot when the world draw runs. Three leftovers break that. (a) The UBO reflection matrices and the Update-time SO_REFLEC_FLAG/sort-dist computation are taken from a UWREFL camera that was last written by the previous frame's water pass, so it reflects the last-rendered view, not the one being drawn. (b) CopyStateReflect -> Update() takes aspectRatio and lppScale from globalRendering (Camera.cpp:144-145). If window 2 has a different aspect, the reflection projection no longer matches the screen-space sampling of the square reflection texture. This is a symptom of the globalRendering-as-viewport problem. (c) After the reflection/refraction passes, the water code resets state with FBO::Unbind() and globalRendering->LoadViewport() (BumpWater.cpp:571-572, DynWater.cpp:519-520, 422), so subsequent passes in window 2 get the primary window's viewport. This is the viewport-reset-idiom problem. In addition, BumpWater bakes the primary window's size into its shader constants and screen-copy refraction texture (BumpWater.cpp:181-182, 425-427, 905-912), which is a window-size problem rather than a camera problem.

*Evidence:*
- `rts/Game/Camera.cpp:127` CopyStateReflect asserts the source is not UWREFL; then Update(false,true,false) recomputes the frustum and overwrites VISCUL
- `rts/Rendering/Env/BumpWater.cpp:1026` reflection camera derived from the active camera, UpdateLoadViewport(0,0,reflTexSize..) at :1031, restore prvCam->Update() :1038
- `rts/Rendering/Env/CubeMapHandler.cpp:204` ENVMAP camera uses prvCam->GetPos() and globalRendering viewSize at :222
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:495` camUWRefl flags/sort dist computed at Update time (:530-535)
- `rts/Game/Camera.cpp:217` UpdateFrustum copies into VISCUL (227-230)
- `rts/Map/SMF/SMFReadMap.cpp:847` GridVisibility uses VISCUL
- `rts/Rendering/Env/CubeMapHandler.cpp:241` restores active cam type but not VISCUL/matrices
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:184` tessellation state keyed by shadowPass bool; non-shadow passes reuse MESH_NORMAL (475)
- `rts/Game/CameraHandler.cpp:94` static cameras[CAMTYPE_COUNT]

*Direction:* Do not make UWREFL or ENVMAP per-view cameras. Keep them as shared scratch slots and treat this as a checklist inside the already-needed per-view work. (1) Per-view UBO refresh: call UniformConstants::UpdateMatrices() at the start of each DrawView with the view's camera in the PLAYER slot, so reflection matrices are re-derived from that view for free. (2) Per-view visibility flags: in whatever mechanism computes OPAQUE/ALPHA flags per view, call CopyStateReflect(viewCam) before the UWREFL loop in UnitDrawerData/FeatureDrawerData/ProjectileDrawer::UpdateDrawFlags. For the prototype, either accept stale reflection flags or set Water=0 / BumpWaterReflection=0. (3) Make CCamera::Update take aspect and viewport from the view instead of globalRendering, so CopyStateReflect and prvCam->Update() inherit the view's aspect. (4) Share the ENVMAP cubemap from the primary camera position, which is acceptable for N views, and document that. No work is needed for VISCUL beyond camera->Update() at the start of each view.

> verifier A (corrected): Core mechanism holds in the current tree. UWREFL and ENVMAP are single static slots (CameraHandler.cpp:94). UWREFL is re-derived from whichever camera is active (UniformConstants.cpp:186-189, BumpWater.cpp:1026-1030, DynWater.cpp:469-473, AdvWater.cpp:339-343). The UBO reflection matrices and the SO_REFLEC_FLAG draw flags and sort distances are computed once in the Update stage from the single PLAYER camera. Nothing in the tree is multi-view aware, and there is no per-frame gating that would mas

> verifier B (corrected): The single-slot fact is real (static cameras[] at CameraHandler.cpp:94; UWREFL and ENVMAP re-derived from the active camera by CopyStateReflect at Camera.cpp:127-139, UniformConstants.cpp:186-195, BumpWater.cpp:1026-1038 and CubeMapHandler.cpp:204-218). It does not block the 2-window prototype, and several pieces of the stated mechanism are wrong or already covered by other problems.

1. Water reflection and refraction textures are not stale across views. CWorldDrawer::DrawAlphaObjects calls wat

### CAM-09 - VISCUL is a last-writer-wins culling camera used implicitly by GridVisibility and Lua queries
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** CAM-02

Every UpdateFrustum() of any non-VISCUL camera copies itself into the single VISCUL camera. GridVisibility(nullptr, ...) and the Lua visible-object queries read it, so the result depends on which camera was updated last.

*Why it breaks:* Camera.cpp:217-230: every non-VISCUL UpdateFrustum() copies GetCamera(camType) (not `this`) into the static VISCUL camera. GridVisibility(nullptr) (SMFReadMap.cpp:843-849) takes VISCUL's position and frustum for the quad range and frustum lines. It is correct today only because CWorldDrawer::Draw calls camera->Update() on entry (WorldDrawer.cpp:316) and the shadow and water passes restore with prvCam->Update(). With 2 windows, each view that updates its camera overwrites VISCUL, and after the draw loop VISCUL holds the last window's frustum.

Two caller classes break:
- Update-phase callers. GrassDrawer::Update (eventHandler.Update, Game.cpp:1424) and Lua Update callins read the previous frame's last writer instead of a defined view. GrassDrawer::Update also mixes PLAYER pos (:518, :530), ACTIVE camera pos (:67) and VISCUL frustum, then stores the result in the single static blockDrawer (:175) shared by all views.
- Lua queries (LuaUnsyncedRead.cpp:2113, :2201, :2274, :2538, :2622). Each intersects VISCUL quads with the active `camera` InView (:2148, :2228, :2293) or CalcViewPortCoordinates (:2562).

Separately, a camera object outside the static array never feeds VISCUL correctly, because of Camera.cpp:228.

*Evidence:*
- `rts/Game/Camera.cpp:217` UpdateFrustum copies state into VISCUL for every other camera type
- `rts/Map/SMF/SMFReadMap.cpp:847` GridVisibility(nullptr) uses VISCUL and calls CalcFrustumLines on it (:849)
- `rts/Rendering/Env/GrassDrawer.cpp:533` grass culling via nullptr camera
- `rts/Lua/LuaUnsyncedRead.cpp:2113` one of five Lua callers using nullptr
- `rts/Rendering/DebugColVolDrawer.cpp:345` nullptr caller
- `rts/Map/SMF/SMFReadMap.cpp:843` nullptr camera falls back to VISCUL
- `rts/Rendering/InMapDrawView.cpp:198` map markers culled with VISCUL
- `rts/Game/Camera.cpp:227` every camera update overwrites VISCUL

*Direction:* Make GridVisibility take an explicit camera and drop the nullptr-to-VISCUL default along with the CopyState-to-VISCUL side effect at Camera.cpp:227-230. For the prototype, handle only GrassDrawer. Compute grass visibility per view at draw time, or keep per-view cached lists keyed by view, using that view's camera for GridVisibility, the distance function and the change detection. The Lua queries, InMapDrawView and the debug drawers (about 8 callers) can be deferred to a "current or primary view" policy under the full-product work. Until then, a cheap interim is to have each view call its camera Update() immediately before any of these queries and treat Update-phase queries as primary-view only.

> verifier B (corrected): The mechanism is real, but it is not needed for the 2-window 3D-only prototype. Verified: every non-VISCUL UpdateFrustum() overwrites VISCUL (Camera.cpp:217-230). GridVisibility(nullptr) reads VISCUL (SMFReadMap.cpp:843-850). The 9 live callers are 5 in LuaUnsyncedRead plus DebugColVolDrawer, DebugVisibilityDrawer, InMapDrawView and GrassDrawer:533. GrassDrawer.cpp:624 is inside a comment block. The UniformConstants claim also holds: CopyStateReflect calls Update(false,true,false), which is Upda

> verifier A (corrected): The core mechanism holds in the current tree. Camera.cpp:217-230 copies every non-VISCUL camera into the single static VISCUL slot on each UpdateFrustum(). SMFReadMap.cpp:843-849 resolves cam==nullptr to VISCUL. All 9 live callers exist (5 in LuaUnsyncedRead.cpp, GrassDrawer.cpp:533, DebugColVolDrawer.cpp:345, DebugVisibilityDrawer.cpp:45, InMapDrawView.cpp:198). GrassDrawer.cpp:624 and DynWater.cpp:823 are inside comments and not live. GrassDrawer::Update does run from eventHandler.Update (Game

### CAM-10 - Camera is recomputed redundantly several times per frame
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** none

The PLAYER camera runs full Update() (matrix inversions plus fixed-function GL matrix loads) at least 4 times per frame, and every pass-restore adds more. This multiplies with the number of views.

*Why it breaks:* At the default ExpDecay transition mode the PLAYER camera runs a full CCamera::Update() four times per frame:
- UpdateController calls UpdateTransition() at CameraHandler.cpp:271, guarded by `currCamTransitionNum == EXP_DECAY` at :270, and again unconditionally at :284. Each call ends in camera->Update() (:442 or :452).
- Game.cpp:1430 calls camera->Update() in UpdateUnsynced, after Lua events and the unit tracker.
- WorldDrawer.cpp:316 calls camera->Update() at the start of CWorldDrawer::Draw.

Pass restores then add more full Updates of the PLAYER camera:
- ShadowHandler.cpp:656
- BumpWater.cpp:1038 (reflection restore) and :984 (refraction setup)
- AdvWater.cpp:350
- DynWater.cpp:485 and :492

Each Update() runs the following with no dirty flag:
- UpdateViewRange, UpdateDirsFromRot, UpdateMatrices, UpdateViewPort and UpdateFrustum (Camera.cpp:148-157).
- gluPerspectiveSpring and LookAtView.
- One InvertAffine and two full Invert() calls (Camera.cpp:262-265).
- UpdateFrustum, which includes a CopyState into the VISCUL camera (Camera.cpp:217-230).
- LoadMatrices(), which issues glMatrixMode and glLoadMatrixf (Camera.cpp:159, 289-297).

Total cost is on the order of microseconds per frame. The restores are not purely redundant: they also reset the VISCUL culling camera and re-establish the GL fixed-function matrix state after FBO passes. Update() also reads the process-wide globalRendering viewSizeX/viewSizeY/aspectRatio/pixelY (Camera.cpp:144-155), so any per-view caching scheme must key on the view's own size.

*Evidence:*
- `rts/Game/CameraHandler.cpp:270` UpdateTransition before and after the controller update (:284); each ends in camera->Update() (:442,452,477,514,544)
- `rts/Game/Game.cpp:1430` another camera->Update(); WorldDrawer.cpp:316 again
- `rts/Game/Camera.cpp:262` inverse matrices and billboard matrix recomputed each Update (:261-269)

*Direction:* Keep this as a low-priority cleanup that rides on the per-view camera ownership work; do not schedule it ahead of that work.

1. Split the pose update (position, rotation, FOV) from the derived-matrix and frustum rebuild, and add a dirty flag so the rebuild runs once per camera per frame.
2. Keep a separate cheap restore path for GL fixed-function matrices and the viewport. This already exists in spirit as `Update({false,false,false,false,false})` at ShadowHandler.cpp:578. The restore must also re-copy the frustum into the VISCUL camera, or restore VISCUL explicitly.
3. Fold the pass-restore calls into per-view pass state, not into camera recomputation, once each view owns its camera, viewport size and GL matrix state.

> verifier A (corrected): The redundancy is real in the default configuration. At default CamTransitionMode (ExpDecay) the PLAYER camera runs a full CCamera::Update() four times per frame: CameraHandler.cpp:271 and :284 (each ending in camera->Update() at :442/:452), Game.cpp:1430, and WorldDrawer.cpp:316. Each full Update rebuilds the projection, view, viewProj and billboard matrices, three inversions and the frustum, with no dirty flag (Camera.cpp:141-162, 233-270). Pass restores add more: ShadowHandler.cpp:656, BumpWa

### CAM-11 - Camera transition smoothing depends on global frame timers and vsync
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** none

Transition tweening and key-move speed use globalRendering lastFrameStart, lastSwapBuffersEnd, FPS and the VSync config. With two windows that each swap, these timers become ambiguous.

*Why it breaks:* What the code actually does:
1. FPS, lastFrameStart and lastFrameTime are written once per CGame::UpdateUnsynced (Game.cpp:1238-1239, 1278), and FPS comes from numDrawFrames++ in the same function (Game.cpp:1276, 1362-1363). A second swap or window cannot change their meaning. The claim that two swaps change what FPS means is wrong.
2. With the default CamFrameTimeCorrection = 0 (Camera.cpp:35-38), none of the swap timers are used. CameraTransitionExpDecay uses lastFrameStart (CameraHandler.cpp:344-346). UpdateTransitionExpDecay starts with transTime = lastFrameStart (:405), and the lastSwapBuffersEnd override is guarded by `vsync == 1 && camera->useInterpolate > 0` (:410-413). GetMoveVectorFromState uses lastFrameTime unless useInterpolate > 0 (Camera.cpp:715-718).
3. lastSwapBuffersEnd is written only at the end of the single CGlobalRendering::SwapBuffers (GlobalRendering.cpp:717), which swaps the single sdlWindow (:703). It is read only on the non-default useInterpolate > 0 path (CameraHandler.cpp:348, 411-412). Whether a second swap disturbs it depends on whether the prototype's window-2 swap routes through that function. A separate swap routine would leave it untouched.
4. The transition state is the singleton CCameraHandler::camTransState, and it drives the global `camera` (CameraHandler.cpp:339-355, 438-452, 554-558). There is no "camera 2 transition" to jitter. How a second camera gets its smoothing is a camera-ownership design question, not a timer problem.
5. The real vsync concern is frame pacing, not camera timing. CVerticalSync::SetInterval calls SDL_GL_SetSwapInterval, which applies only to the GL context current at call time (VerticalSync.cpp:76-87). A second window or context therefore needs its own swap interval, or two blocking swaps will lower the real frame rate. Because lastFrameTime and FPS are measured from real time, transitions would still run at correct wall-clock speed. This overlaps with any swap/vsync problem already in the catalogue.

*Evidence:*
- `rts/Game/CameraHandler.cpp:404` vsync config and lastSwapBuffersEnd/FPS drive transTime (:405-413)
- `rts/Game/Camera.cpp:715` camDeltaTime from lastFrameTime or 1000/FPS

*Direction:* Nothing is needed for the 2-window 3D-only prototype in the default config. Keep FPS, lastFrameStart and lastFrameTime derived once per CGame::UpdateUnsynced. Have only the primary window's swap write globalRendering->lastSwapBuffersEnd, or make the secondary swap path not touch it. Set the swap interval explicitly per window or context (swap interval 0 for secondary windows is the usual choice) as part of the swap/vsync problem. Treat the CamFrameTimeCorrection > 0 plus VSync == 1 combination as unsupported or document it for multi-window. Handle per-camera transition state under the camera-ownership problem.

> verifier A (corrected): The cited code exists, but the claimed mechanism mostly does not hold. FPS, lastFrameStart and lastFrameTime are all per draw frame, not per swap, so a second SwapBuffers does not change them. The swap-dependent path (lastSwapBuffersEnd) is opt-in and off by default. No "camera 2 transition" exists in the tree. Only a narrow residual survives, so polish is the right severity.

### CAM-12 - No automated tests cover camera math, and Update requires GL and globalRendering
**Severity:** polish - **Effort:** M - **Status:** corrected - **Depends on:** CAM-03

There are no test references to CCamera, and the class cannot be unit tested because Update() reads globalRendering and calls GL.

*Why it breaks:* Update() is one method that mixes globalRendering reads with a GL load, but the math pieces are separable. UpdateMatrices(vsx, vsy, aspect), UpdateViewPort, UpdateFrustum, UpdateViewRange and InView are public (Camera.h:241-245, :144). UpdateMatrices takes viewport size and aspect as arguments and only fills member matrices. GL is touched only in LoadMatrices/LoadViewport, and a headlessStubs GL stub library already exists (rts/lib/headlessStubs/gladstub.cpp:291 and :302 stub glLoadMatrixf and glMatrixMode). test/CMakeLists.txt:549-553 already links headlessStubs for the SerializeLuaState test, and myGL.h:59 and :73 already have UNIT_TEST guards. The actual obstacles are these:

1. The CCamera constructor calls UpdateFrustum() (Camera.cpp:56). For any non-VISCUL camera, UpdateFrustum ends by calling CCameraHandler::GetCamera(CAMTYPE_VISCUL) and GetCamera(camType) and then CopyState (:227-230). Those return elements of a file-static array (CameraHandler.cpp:94 and :125). So even constructing a PLAYER camera needs CameraHandler.cpp linked, and the VISCUL copy at :227-230 uses GetCamera(camType) rather than `this`. This is the bug behind the "UpdateFrustum-copies-this fix" in the claim.
2. UpdateViewRange reads readMap->GetCurrMinHeight() (:327), float3::maxxpos and globalRendering->minViewRange and maxViewRange (:352-353).
3. Camera.cpp includes MouseHandler, Ground, ReadMap, ConfigHandler and GlobalRendering (:5-15), and its config observers call configHandler (:69, :82). Linking it into a Catch2 test therefore drags in much of the engine.

The pure math can be tested today only if the camera's globals (CameraHandler, globalRendering, readMap) are decoupled or stubbed. GL is not the main problem.

*Evidence:*
- `rts/Game/Camera.cpp:141` Update() mixes pure math with globalRendering reads and GL (LoadMatrices at :159)
- `test:1` grep of test/ finds no CCamera or Camera.h reference

*Direction:* The split should go further than separating math from the GL load. Make UpdateFrustum take its VISCUL target as a parameter, or return the frustum, instead of reaching into CCameraHandler. Make the constructor free of CCameraHandler calls. Pass min and max view range and the map extents into UpdateViewRange. Then Catch2 tests can compile Camera.cpp, or a split-out CameraMath.cpp, with only test/engine stubs. Using headlessStubs for the leftover GL calls is enough, so GL need not be isolated from Camera.cpp. Effort is S to M. It is a nice-to-have regression net for the viewport decoupling and per-view VISCUL work, not a prototype blocker.

> verifier A (corrected): The core facts hold. A grep of test/ for CCamera, Camera.h, Frustum and camera finds nothing, so there is no camera test. CCamera::Update (Camera.cpp:141-162) reads globalRendering at :144, :145, :153 and :155, and ends in LoadMatrices() at :159. LoadMatrices (:289-297) calls glMatrixMode and glLoadMatrixf. The causal claim "cannot be unit tested because Update() reads globalRendering and calls GL" is too narrow and partly wrong. GL is not the binding obstacle, because tests already link headles

### CAM-13 - Spawn-time camera reads bake the active camera into unsynced effects
**Severity:** polish - **Effort:** S - **Status:** unverified - **Depends on:** none

Two effect spawners read the active camera when the effect is created, not when it is drawn.

*Why it breaks:* CStdExplosionGenerator::Explosion computes camVect = camera->GetPos() - pos and shifts the heat-cloud spawn position toward it (ExplosionGenerator.cpp:453-472). CStandardGroundFlash::InitCommon bakes side1/side2 from camera->GetDir() into the flash (GroundFlash.cpp:204-207). Whichever camera is active at spawn decides the result, and both windows then see the same baked result, so the second window is slightly off. This is cosmetic.

*Evidence:*
- `rts/Sim/Projectiles/ExplosionGenerator.cpp:453` camera->GetPos() in Explosion (453-472), withMutex at 474-480
- `rts/Rendering/GroundFlash.cpp:204` camera direction baked into the ground-flash orientation
- `rts/Sim/Projectiles/ExplosionGenerator.cpp:393` PredictExplosionVisible uses gu->spectatingFullView (393-436)
- `rts/Lua/LuaHandle.cpp:2259` Lua caller

*Direction:* Use the primary or PLAYER camera deterministically, or drop the nudge and compute orientation at draw time.

## Per-view caches & visibility

### PVC-01 - Engine-owned screen-sized textures exposed to Lua are single instances sized to the primary view
**Severity:** risk - **Effort:** M - **Status:** confirmed - **Depends on:** none

$map_gbuffer_*, $model_gbuffer_* and the depth copy used by Lua compositing are single global resources sized from globalRendering->viewSize and updated once per frame. A second view of a different size either reallocates them every view or reads a wrong-sized buffer.

*Why it breaks:* LuaOpenGLUtils maps the $...gbuffer... names to groundDrawer/unitDrawer GeometryBuffers and reports their size via GetWantedSize, i.e. viewSize. LuaObjectDrawer::Update (called once per frame from CWorldDrawer::Update) resizes the geometry buffer. DrawDeferredPass then binds it and uses a viewport of viewSize. The depth copy is created at viewSize and recreated on the global ViewResize event. Lua code reads these textures in DrawUnitsPostDeferred and DrawWorld and caches their size and its own matching FBOs. For two views of different sizes, per-view resizing thrashes allocations and invalidates Lua's cached sizes; with a single frame-level resize the second view renders into a buffer sized for the first.

*Evidence:*
- `rts/Lua/LuaOpenGLUtils.cpp:610` $map/$model gbuffer textures resolve to the global groundDrawer/unitDrawer geometry buffers (610-622, defs 495-496)
- `rts/Lua/LuaOpenGLUtils.cpp:975` gbuffer texture size reported via GetWantedSize (975-990)
- `rts/Rendering/GL/GeometryBuffer.cpp:208` GetWantedSize = globalRendering->viewSize (208-210); recreated at 181
- `rts/Rendering/LuaObjectDrawer.cpp:274` Update() sizes the buffer once per frame (called from WorldDrawer.cpp:214)
- `rts/Rendering/DepthBufferCopy.cpp:138` depth copy texture created at primary viewSize (138-140), blit rect at 72-73, recreated on ViewResize (55-63)

*Direction:* For the prototype, force equal sizes for all views, or resize the gbuffer and depth copy lazily at view begin from the active view's size. Treat per-view-size resources as a follow-up, using a small size-keyed cache or per-view instances. Run MakeDepthBufferCopy and the deferred pass inside each view scope, since both sit inside the per-view draw. Expose the current view size to Lua through the identity API. Effort M.

> verifier A (confirmed): Verified in the current tree. GeometryBuffer::GetWantedSize returns globalRendering->viewSize (GeometryBuffer.cpp:208-211) and Update() recreates the attachments whenever that size changes (179-205). LuaObjectDrawer::Update is called once per frame from WorldDrawer.cpp:214. DepthBufferCopy is a single instance: the textures are allocated at globalRendering->viewSize (DepthBufferCopy.cpp:138-140), they are recreated only on the global ViewResize (55-63), and the blit rectangles use the primary vi

> verifier B (confirmed): The mechanism holds in the code I read. GeometryBuffer::GetWantedSize returns globalRendering->viewSize (GeometryBuffer.cpp:208-210). Update() recreates the attachments only when that size changes (lines 181-205), and LoadViewport uses viewSize (line 216). LuaObjectDrawer::Update (line 274) is the per-frame call. DepthBufferCopy allocates at viewSize (lines 138-140), blits using viewPos/viewSize (lines 72-73), and is recreated only on ViewResize (lines 55-63). It is a risk and not a blocker. Win

### PVC-02 - Once-per-frame caches keyed on drawFrame and update-time PLAYER-camera caches
**Severity:** risk - **Effort:** L - **Status:** corrected - **Depends on:** Per-frame timing globals advance inside UpdateUnsynced, so CGame::Draw cannot simply run once per target; DSN-01; DSN-15

About 20 sites use globalRendering->drawFrame (often with camType) as a 'computed this frame' stamp, and several Update-phase caches read CAMTYPE_PLAYER, assuming exactly one view per drawFrame.

*Why it breaks:* Narrow this problem to "drawFrame/camType-stamped caches assume one view per drawFrame". Window 2 shares window 1's drawFrame and camType PLAYER, so:
(a) NanoParticleRenderer reuses window 1's gathered enemy set. This is a visible miss, but opt-in.
(b) The BasicMesh and ROAM Patch visibility stamps are never cleared, giving union overdraw only.
(c) The default-path cost is in ROAM. The tessellation state is a single set per shadowPass, so alternating cameras defeat the lastCamPos early-out and fight over tessellation and LOD distance (the PLAYER slot's position). That causes repeated full retessellation and upload when zoom levels differ.
(d) ShieldSegment caches the first caller's InView result and DrawShield event once per frame.
(e) ProjectileDrawer's alphaRange reuse is only safe while each view runs its below-water, refraction and above-water passes in order.
(f) The SMF streaming LRU and mip choice use the active camera, but the feature is off by default.
(g) The GrassDrawer Update-phase cache is built once from the PLAYER camera.
Drop LuaMaterial visFrame and the GL timer-query parity from the list; they are not view-dependent caches. Move the unit, feature and projectile draw-flag and UBO caches into the view-context/prepare-split problem.

*Evidence:*
- `rts/Rendering/Env/NanoParticles/NanoParticleRenderer.cpp:668` gather cache keyed on drawFrame and camType only
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:889` alphaRangeDrawFrame pass reuse
- `rts/Map/SMF/Basic/BasicMeshDrawer.cpp:100` visUpdateFrames[camType] = drawFrame
- `rts/Map/SMF/ROAM/Patch.cpp:804` lastDrawFrames[camType]=drawFrame; IsVisible uses >= (:811)
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:219` playerCamera used for LOD distance (219-220, 284); lastCamPos early-out at 203-209
- `rts/Rendering/Env/GrassDrawer.cpp:518` Update uses CAMTYPE_PLAYER; blockDrawer/farnearVA rebuilt only when it moves (524-543)
- `rts/Map/SMF/SMFGroundTextures.cpp:348` streaming from active cam and viewSize (348-353, 368-373)
- `rts/Rendering/Env/Particles/Classes/ShieldSegmentProjectile.cpp:154` once-per-drawFrame visibility and Lua callin cache
- `rts/Game/Game.cpp:1277` drawFrame++ once per UpdateUnsynced
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:205` tessellation cache compares camera direction per shadowPass
- `rts/Rendering/Env/GrassDrawer.cpp:527` lastVisibilityUpdate = drawFrame; GC keyed on lastSeen (:554)
- `rts/Lua/LuaMaterial.cpp:778` visFrame uniform = drawFrame
- `rts/Rendering/GlobalRendering.cpp:725` timer-query ring parity = drawFrame&1 (also :733)
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:515` SetSortDist(CAMTYPE_PLAYER) from the single player camera
- `rts/Map/SMF/Basic/BasicMeshDrawer.cpp:232` patch visibility keyed on (camType, drawFrame), union semantics
- `rts/Game/Camera.h:16` CAMTYPE_PLAYER..COUNT enum is the only view discriminator (lines 16-22)
- `rts/Game/CameraHandler.cpp:94` static CCamera cameras[CAMTYPE_COUNT]; GetCamera/GetActiveCamera at 125-126 index it
- `rts/Rendering/GlobalRendering.h:148` unsigned drawFrame, the single frame token; incremented at Game.cpp:1277

*Direction:* 1. (S, prototype) Add a monotonic GlobalRendering::viewSerial, bumped at each target's begin-view. Switch only the visibility stamps to it: Patch::UpdateVisibility/IsVisible (Patch.cpp:804/811) and BasicMeshDrawer (:100, :232, :262-285). Both set and read inside the same pass, so this is a 6-line change. Do NOT bump drawFrame per window. It also drives the grass GC (GrassDrawer.cpp:554), the SMF 120-frame LRU (SMFGroundTextures.cpp:369) and winChgFrame (GlobalRendering.cpp:1295-1298).
2. (S) Add a single BeginView() hook that invalidates the per-frame view caches. It sets NanoParticles gatheredValid=false, ShieldSegmentCollection lastAllowDrawFrame=-1 (or keys it on viewSerial), and ProjectileDrawer alphaRangeDrawFrame=0. This also makes the projectile reuse robust to future pass reordering.
3. (M) ROAM: a viewSerial does not fix it, because the tessellation is shared. Either (a) change CRoamMeshDrawer::Update to take the set of active view cameras and tessellate once per frame for the union of frusta with LOD distance = min over views, run from the per-frame update rather than per pass, or (b) hold per-view patchMeshGrid, pvflags, lastCamPos and a tri pool, at roughly Nx memory. For a first prototype, (a) or simply accepting the thrash is fine, and the thrash can be removed later.
4. Grass for the prototype: skip it in secondary views. The proper fix is a per-view visible-turf list.
5. Leave SMF streaming off for secondary views, or key the mip choice on the minimum distance across views.
6. Handle the unit/feature/projectile draw flags and the camera UBO in the separate per-view prepare problem.

> verifier A (corrected): The core mechanism holds, but the claim mixes real faults with non-issues and misses the part that makes it a blocker. Verified true in the current tree, assuming window 2 reuses CAMTYPE_PLAYER and the same drawFrame (drawFrame is bumped once in UpdateUnsynced, Game.cpp:1277). Nano gather reuse (NanoParticleRenderer.cpp:668-675) is real. Window 2's first Nano pass reuses window 1's culled enemy set whenever no UWREFL pass intervenes. BasicMeshDrawer stamps (BasicMeshDrawer.cpp:100,232) and Patch

> verifier B (corrected): The drawFrame-stamp family does not make the 2-window 3D-only prototype crash, raise GL errors or render empty. Each named site is either benign, opt-in or non-default, or a cheap local fix. The claim is also inflated and bundled. Real drawFrame-as-cache sites number about 9 in 7 files, not about 20. LuaMaterial.cpp:778 (visFrame) is just a time/animation uniform. GlobalRendering.cpp:725-733 timer queries are profiler-only and run on the main context (Game.cpp:1459 and 1567, ProfileDrawer.cpp:50

### PVC-03 - Projectile draw flags are culled once per frame against fixed PLAYER/UWREFL/SHADOW cameras
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** No per-view identity to key per-frame caches, flags and tokens on

UpdateDrawFlags decides which projectiles are opaque, alpha, reflected or shadowed using only the three global cameras. Every later pass consumes those flags, so a second view sees only what the first camera sees.

*Why it breaks:* UpdateDrawFlags bakes three camera frustum tests into per-projectile drawFlag bits and sortDist[3] values once per UpdateUnsynced (ProjectileDrawer.cpp:494-548). Every draw pass only consumes those bits (ShouldDrawProjectile at 715-725). The DrawAlpha gather loop is single-threaded and reads sortDist for camera->GetCamType() (900-910). Window 2's PLAYER-slot camera therefore sees only projectiles that passed window 1's frustum, and alpha order uses window 1's distances.

Two details the original claim missed.
1. The UWREFL camera is not current when UpdateDrawFlags runs. The mirrored camera is only refreshed by CopyStateReflect in UniformConstants::UpdateMatricesImpl (UniformConstants.cpp:186-189), which Game.cpp:1441 calls AFTER worldDrawer.Update at 1433. The reflection flags are therefore already a frame stale for the single view. The prototype shortcut of copying the target camera into the PLAYER slot and re-running the cull would still test window 2's SO_REFLEC bit against window 1's mirror camera unless CopyStateReflect is also re-run for the target. Reflection-only projectile loss in window 2 is a subtle glitch. The simplest mitigation is to drop the UWREFL frustum test and set SO_REFLEC from ObjectVisibleReflection alone.
2. DrawAlpha caches geometry across passes (alphaRangeDrawFrame == globalRendering->drawFrame at 889, saved at 949-955). That cache is safe only because each target's own below-water main pass runs first inside CWorldDrawer::DrawAlphaObjects (line 419, before UpdateWater at 437) and overwrites the saved range. It would break if a later design reorders passes across targets or skips a target's below-water pass.

*Evidence:*
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:494` camPlayer/camUWRefl/camShadow fetched; InView at 514/530/537 (494-548)
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:514` frustum test sets SO_OPAQUE/SO_ALPHAF and PLAYER sortDist
- `rts/Rendering/WorldDrawer.cpp:227` UpdateDrawFlags called from Update(), not from Draw()
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:906` alpha pass filters on flags computed for another camera
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:1222` counter-example: draw-time InView on the active camera
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:515` SetSortDist(CAMTYPE_PLAYER, ProjectedDistance)
- `rts/Sim/Projectiles/Projectile.h:132` std::array<float,3> sortDist; Get/SetSortDist(camType) at 101-102
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:900` sortCamType = camera->GetCamType() used to read precomputed distance (also line 72)
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:980` fxShader camPos/fog uniforms from camPlayer regardless of pass (970-982)
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:712` CanDrawProjectile uses gu->myAllyTeam and LOS
- `rts/Sim/Projectiles/WeaponProjectiles/BeamLaserProjectile.cpp:119` PLAYER camera distance used inside Draw while billboarding uses active camera

*Direction:* Split UpdateDrawFlags into two parts.
- Per-frame part, run once: drawPos, previousDrawFlag = drawFlag, ResetDrawFlag, CanDrawProjectile and the DRICON bit. These are target-independent, and DRICON feeds the minimap and the Lua getters.
- Per-view part, a new method such as CullProjectilesForView(camPlayer, camUWRefl, camShadow): clear only the view bits (OPAQUE, ALPHAF, REFRAC, REFLEC, SHOPAQ, SHTRAN), keep DRICON, and rerun the existing for_mt InView and sortDist body.

Call the per-view part at the start of each target's draw, after that target's camera has been copied into the PLAYER slot and its frustum updated. Draw passes run sequentially per target, so a single flag set is enough and no per-view flag array is needed. The previousDrawFlag corruption goes away because only the per-frame part touches it. Before the cull, refresh the UWREFL camera for the target (CopyStateReflect), or drop that frustum test as noted above. Leave the SHADOW bits as computed for the primary view until the shadow-map-per-target decision is made.

Do not replace the flags with draw-time InView in each pass as the default fix. The DrawAlpha gather loop at 905-914 is single-threaded and runs up to five times per frame (below-water, above-water, reflection, refraction and shadow), so it would be a performance regression on effect-heavy frames. The file already hoists per-frame invariants and uses for_mt precisely to avoid that.

Effort is S for the split plus a per-target call. It should be bundled with the identical unit and feature flag fix (CUnitDrawerData and CFeatureDrawerData UpdateObjectDrawFlags) so all three share one per-view cull hook.

> verifier A (corrected): The core mechanism is accurate in the current tree. CGame::UpdateUnsynced runs worldDrawer.Update once per draw frame (Game.cpp:1433, after camera->Update() at 1430), and that calls projectileDrawer->UpdateDrawFlags() (WorldDrawer.cpp:227). UpdateDrawFlags fetches the PLAYER, UWREFL and SHADOW camera slots once (ProjectileDrawer.cpp:494-496) and resets drawFlag, then sets SO_DRICON_FLAG for any projectile that passes CanDrawProjectile (505-510). It adds SO_OPAQUE/SO_ALPHAF/SO_REFRAC only inside 

> verifier B (corrected): The mechanism is real and I verified it in code. CWorldDrawer::Update (WorldDrawer.cpp:227, invoked at Game.cpp:1433) runs UpdateDrawFlags once per UpdateUnsynced. That function reads CCameraHandler::GetCamera for PLAYER, UWREFL and SHADOW (ProjectileDrawer.cpp:494-496) and sets SO_OPAQUE, SO_ALPHAF, SO_REFRAC, SO_REFLEC, SO_SHOPAQ and SO_SHTRAN from InView tests (514, 530, 537). DrawOpaque (839), DrawAlpha (906), DrawShadowOpaque (1024) and DrawShadowTransparent (1054) all gate on ShouldDrawPro

### PVC-04 - ROAM heightMapChanged latch is consumed by the first Update of any pass
**Severity:** risk - **Effort:** S - **Status:** unverified - **Depends on:** none

One bool covers both the normal and shadow mesh sets; the first Update() that runs clears it, so the other set (or another target) can early-out and miss terrain deformation.

*Why it breaks:* UnsyncedHeightMapUpdate marks patches dirty in both sets but sets a single heightMapChanged=true (RoamMeshDrawer.cpp:603-622). Update() only proceeds past the early bailout when heightMapChanged is set (:207) and clears it at the end (:449). With Shadows=1 the shadow Update runs first in the frame (GenerateIBLTextures precedes Draw, Game.cpp:1519-1526), clears the latch, and the Normal Update then bails out whenever the camera moved less than 16 elmos, so craters stay invisible until the camera moves. With more than one consumer per frame (extra mesh sets or targets sharing a set) this generalises.

*Evidence:*
- `rts/Map/SMF/ROAM/RoamMeshDrawer.h:72` single heightMapChanged bool
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:207` early bailout requires heightMapChanged==false to skip
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:449` cleared at end of any Update, for any pass
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:622` set once for both mesh sets
- `rts/Rendering/ShadowHandler.cpp:93` Shadows=2 (default) drops the MAP bit; Shadows=1 runs ground shadow Update before Normal

*Direction:* Replace the bool by a per-mesh-set (or per-view) dirty generation counter compared against the set's last-processed generation.

### PVC-05 - ROAM lazy-update heuristics thrash when the active camera alternates
**Severity:** risk - **Effort:** M - **Status:** unverified - **Depends on:** PVC-06; Per-view state ownership model for terrain-side caches; No per-view identity to key per-frame caches, flags and tokens on

The early-out, visibility-transition and retessellation bookkeeping assume one camera per mesh set. Alternating two cameras defeats the early-out and accumulates counters toward periodic forced full retessellations.

*Why it breaks:* lastCamPos/lastCamDir/lastGroundDetail are per mesh set only (RoamMeshDrawer.h:70-75). The early bailout (:203-209) compares the active camera to the previous call's, so with cameras A and B in the same set the bailout never triggers and a full Update (allocating std::vector<bool> patchesToTesselate at :222, looping all patches) runs on every call. pvflags is overwritten each call (:240), so patches visible only to the other camera count as 'left visibility' on every call; numPatchesLeftVisibility (:249-250) only resets on forced retessellation, so it reaches 50*numPatches and triggers Reset plus a full retessellation and index regeneration (:253-264, :376-391) every few hundred calls. The zoom-out average (:323-325) mixes distances measured against whichever PLAYER camera was current, and can also force full retessellation (> 2.5).

*Evidence:*
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:203` early bailout compares against lastCamPos/lastCamDir of the previous call
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:240` pvflags overwritten per call; other camera's patches look like exits
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:250` numPatchesLeftVisibility threshold 50*numPatches then forced retessellation
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:323` totalCameraDistanceRatioInv > 2.5 forces full retessellation
- `rts/Map/SMF/ROAM/RoamMeshDrawer.h:74` lastCamPos/lastCamDir indexed only by mesh set
- `rts/Map/SMF/ROAM/Patch.h:212` lastDrawFrames sized CAMTYPE_VISCUL
- `rts/Map/SMF/ROAM/Patch.cpp:804` lastDrawFrames[camType]=drawFrame; IsVisible at 811
- `rts/Map/SMF/ROAM/Patch.cpp:782` assert(camType < CAMTYPE_VISCUL)
- `rts/Map/SMF/Basic/BasicMeshDrawer.h:43` visUpdateFrames hard-coded to 4 slots
- `rts/Map/SMF/Basic/BasicMeshDrawer.cpp:232` DrawMesh indexes visUpdateFrames by active camType
- `rts/Game/CameraHandler.cpp:94` static CCamera cameras[CAMTYPE_COUNT]
- `rts/Game/Game.cpp:1277` drawFrame bumped once per frame, not per target
- `rts/Map/SMF/Basic/BasicMeshDrawer.cpp:100` visUpdateFrames[camType]=drawFrame; compared at 232 and 262-285
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:204` early-out on lastCamPos/Dir[shadowPass]; written at 446-447
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:219` tessellation distance from CAMTYPE_PLAYER (219-220, 383)
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:249` left-visibility counter forces full retessellation (239-264)
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:475` non-normal passes reuse MESH_NORMAL
- `rts/Map/SMF/SMFGroundDrawer.cpp:53` ROAM default 1
- `rts/Map/SMF/SMFGroundTextures.cpp:348` DrawUpdate mip selection from active camera; SetDrawFrame at 642

*Direction:* With per-target mesh sets (previous problem) each set keeps its own single-camera history and these heuristics stay valid. If a shared set is kept, replace by per-camera visibility bitsets and a union/min-distance LOD policy.

### PVC-06 - ROAM tessellation, upload and LOD follow a single camera
**Severity:** risk - **Effort:** L - **Status:** corrected - **Depends on:** OBJ-07; Per-view state ownership model for terrain-side caches; Draw-frame and camera-type keyed caches treat a second view of the same type as the same view; Object draw flags and transform uploads are gated by the one player camera

The default terrain mesh (ROAM=1) tessellates and uploads one index buffer per patch for the PLAYER camera. A second window with its own camera gets terrain tessellated for the wrong viewpoint, or untessellated patches (holes).

*Why it breaks:* Update() runs once per Normal or Shadow DrawMesh call. It takes visibility from the active camera and tessellation distance from the PLAYER camera. For a second window B with its own camera, B-visible patches are tessellated for A's position, so B gets A's LOD. Patches far from A but close to B are coarse, which shows as blocky terrain and melting cliffs.

Stale per-pass state also makes the two cameras interfere:
- lastCamPos, lastCamDir, lastGroundDetail, heightMapChanged and forceNextTesselation are per pass (indexed by shadowPass), not per camera. They are consumed at the end of whichever camera's Update runs first (:445-449). B's Update then never sees gldUpdated, or heightMapChanged on the early-out path at :203-207, so a ground-detail change or heightmap change is not retessellated for B-only patches.
- Because of the early-out keyed on lastCamPos (:203-205), alternating A/B Updates only bail out when the two camera poses are within 16 elmos and about 0.016 in direction, i.e. identical windows. Otherwise each Update runs the full UpdateVisibility plus the patch loop twice per frame.
- pvflags (patchVisFlags[0]) is a single vector. The A/B alternation makes every A-only or B-only patch look like it "left visibility" on one Update and "entered" it on the next. numPatchesLeftVisibility (:249) therefore climbs quickly. Once it passes 50x numPatches, and also either few patches are visible or it passes 100x numPatches (:250-254), it forces a full Reset and retessellation (:262, :376-391). That retessellates only the current camera's visible patches (:380), so the other window's patches fall back to FLT_MAX and are retessellated on its next Update, causing a periodic hitch.
- The FLT_MAX / empty-indices hole requires the patch centre to be within 500 elmos of A and not visible to A, which is rare.
- Non-Normal/Shadow passes (including TerrainDeferred and the reflection passes) only call Patch::UpdateVisibility(activeCam, patchMeshGrid[MESH_NORMAL]) (:474-476). They reuse whatever tessellation the last Normal Update produced for whichever camera ran last. This is existing design, not window-specific.
- Patch::IsVisible is keyed by cam type and drawFrame (Patch.cpp:809-811). If B's camera has the same CAMTYPE as A and the frame counter is shared, B draws the union of A's and B's visible patches. That is correct but overdraws. This is the dependency on the draw-frame and camera-type keyed cache problem.

*Evidence:*
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:187` Update(): cam = GetActiveCamera() used only for visibility
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:219` playerCameraPosition from CAMTYPE_PLAYER used for Tessellate at 360 and 383
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:289` retessellate only if >500 elmos away and 1.5x closer than last tessellation
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:397` GenerateIndices/Upload only for patches visible to the active camera
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:471` Normal and Shadow passes call Update(); other passes only UpdateVisibility
- `rts/Map/SMF/ROAM/Patch.cpp:606` camDistLODFactor from midPos.distance(camPos)
- `rts/Map/SMF/ROAM/Patch.cpp:177` Reset() sets camDistanceLastTesselation=FLT_MAX
- `rts/Map/SMF/ROAM/Patch.h:204` per-patch vertices/indices/border + VBOs :214-216 + VAOs :218-219
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:203` early-out on lastCamPos/lastCamDir[shadowPass]; state saved at 445-448
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:472` Normal and Shadow passes call Update(); other passes only UpdateVisibility on MESH_NORMAL with their camera
- `rts/Map/SMF/Basic/BasicMeshDrawer.cpp:180` Shadow/WaterReflection force cam = GetCamera(CAMTYPE_PLAYER) for LOD; GuiTraceRay at :189
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:398` GenerateIndices/Upload only for IsVisible(cam) patches (also 407-426)
- `rts/Map/SMF/ROAM/Patch.cpp:197` UploadStreamDrawData per-patch VBO realloc + VAO re-init; visibility cache at 804-811
- `rts/Map/SMF/SMFGroundDrawer.cpp:53` ROAM config default 1
- `rts/Map/SMF/ROAM/Patch.cpp:207` UploadStreamDrawData; VAO init :226-277
- `rts/Map/SMF/ROAM/Patch.cpp:638` lastCameraPosition stored after Tessellate
- `rts/Map/SMF/ROAM/RoamMeshDrawer.cpp:360` Tessellate with playerCameraPosition (also :383), Upload :423, Draw :488
- `rts/Map/SMF/Basic/BasicMeshDrawer.cpp:39` static read-only RenderBuffers per LOD (to 57)

*Direction:* Prototype: either (a) set ROAM=0 so CBasicMeshDrawer is used. It keeps its LOD in a member (drawPassLOD) recomputed from the active camera on every DrawMesh via Update(drawPass) at BasicMeshDrawer.cpp:104, uses read-only static buffers (:39-57), and only forces the player camera for the Shadow and WaterReflection passes (:180-181). Its costs are a GuiTraceRay per call, a per-camtype visUpdateFrames cache and an O(patches) frustum loop. Or (b) make ROAM state per view: an explicit camera parameter replacing GetCamera(PLAYER) in Update/Tessellate, plus per-view lastCamPos/lastGroundDetail/forceNextTesselation/pvflags/numPatchesLeftVisibility, or a separate CRoamMeshDrawer per target (which also needs the static CTriNodePool pools and the static forceNextTesselation, :45, made per instance). Longer term: share patch topology and tessellate against the minimum distance over all target cameras (and union visibility), keeping one index buffer per patch, or per-view index buffers over shared variance data. Effort stays M for per-view state, L for a shared-topology design.

> verifier B (corrected): The mechanism is real, but the severity is overstated. ROAM state is single-view: LOD comes from CAMTYPE_PLAYER (RoamMeshDrawer.cpp:219-220, 284, 360, 383), and the tessellation, index data and VAO live once per patch (Patch.h:204-219). Without a fix, window B gets terrain tessellated for A's viewpoint, or per-frame retessellation churn if the player camera slot is swapped. None of this crashes or raises a GL error, and the terrain is never empty. The damage is bounded:

- Terrain LOD falls off 

> verifier A (corrected): The core mechanism holds. CRoamMeshDrawer::Update takes visibility from GetActiveCamera() (RoamMeshDrawer.cpp:187,198,238,240) but every LOD decision from GetCamera(CAMTYPE_PLAYER) (:219-220,:284,:360,:383). Patch::Tessellate scales detail by midPos.distance(camPos) (Patch.cpp:606-610). GenerateIndices and Upload only run for patches that are visible to the active camera and have isChanged set (:397-425). There is one indices/indxVBO/VAO per patch (Patch.h:205,215,218) and one patchMeshGrid per 

### PVC-07 - Same-draw-frame dedupe tokens in effect renderers reuse the first target's culling
**Severity:** risk - **Effort:** M - **Status:** unverified - **Depends on:** No per-view identity to key per-frame caches, flags and tokens on

Nano particles, shields and the projectile alpha-range reuse key on drawFrame (and camType). The second target in the same frame takes the already-gathered path built for the first target's frustum.

*Why it breaks:* NanoParticles::Draw skips GatherVisibleEnemies when gatheredDrawFrame==drawFrame and gatheredCamType==camType; both targets are PLAYER in the same frame, so the instance buffer was culled by camera->GetFrustum() of window 1. ShieldSegmentCollection::AllowDrawing returns the cached allowDrawing computed with window 1's InView, and eventHandler.DrawShield fires once per shield per frame. ProjectileDrawer saves the alpha range at the below-water pass and reuses it in the above-water pass keyed on drawFrame; it only works if targets are strictly sequential. LuaMaterial's visFrame uniform feeds the same drawFrame to every target.

*Evidence:*
- `rts/Rendering/Env/NanoParticles/NanoParticleRenderer.cpp:668` cache key (drawFrame, camType)
- `rts/Rendering/Env/Particles/Classes/ShieldSegmentProjectile.cpp:154` frame-keyed early return
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:889` reusePass depends on drawFrame equality only
- `rts/Lua/LuaMaterial.cpp:778` visFrame.Execute(globalRendering->drawFrame)
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:954` saved range stamped with drawFrame
- `rts/Rendering/WorldDrawer.cpp:419` below-water pass always precedes UpdateWater and above pass
- `rts/Rendering/Env/NanoParticles/NanoParticleRenderer.cpp:613` cull uses the active camera frustum
- `rts/Rendering/Env/NanoParticles/NanoParticleRenderer.h:161` gatheredDrawFrame/gatheredCamType members
- `rts/Rendering/Env/NanoParticles/NanoParticleConfig.cpp:13` NanoParticlesGL4 default false
- `rts/Rendering/Env/Particles/Classes/ShieldSegmentProjectile.cpp:175` InView result cached for all views
- `rts/Rendering/Env/Particles/Classes/ShieldSegmentProjectile.cpp:168` Lua DrawShield once per shield and frame

*Direction:* Replace the drawFrame token with ViewPassSerial at these sites, so a target change invalidates the cache, and include camType. Audit the other drawFrame readers listed in the inventory. Decide whether DrawShield should fire per target.

### PVC-08 - Basic mesh drawer LOD pinned to PLAYER camera for shadow and reflection with per-call ray trace
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** none

CBasicMeshDrawer (the easiest multi-camera fallback) still picks LOD from a GuiTraceRay each draw and forces the PLAYER camera for Shadow and WaterReflection. LOD is cached in a member between DrawMesh and DrawBorderMesh.

*Why it breaks:* What holds:
- CalcDrawPassLOD swaps the camera for GetCamera(CAMTYPE_PLAYER) on Shadow and WaterReflection (BasicMeshDrawer.cpp:180-181). The comment says this is to force SP and NP to the same LOD.
- There is exactly one PLAYER camera: `static CCamera cameras[CAMTYPE_COUNT]` (CameraHandler.cpp:94). GetCamera(PLAYER) returns it unconditionally (:125).
- The water-reflection camera is a mirror of the previous camera, copied via CopyStateReflect (Camera.cpp:127-139).
- Whether this breaks window B depends on how B's camera is represented. If B uses a different camera object or type, its reflection and shadow LOD is chosen from the other window's camera. If the prototype swaps B's state into cameras[PLAYER] before drawing B, this resolves itself.
- The LOD choice is therefore pinned to the process-wide main camera.

What the claim got wrong:
1. Citation lines. :218 and :247 are DrawSquareMeshPatch and DrawBorderMeshPatch, not DrawMesh and DrawBorderMesh.
   - DrawMesh calls Update(drawPass) itself at :224, immediately before it reads drawPassLOD, so DrawMesh never sees a stale value.
   - Only DrawBorderMesh (:251) reads the member without recomputing. Every caller invokes it directly after a DrawMesh of the same pass and camera: SMFGroundDrawer.cpp:422-423 for Normal after the forward or deferred pass, and :498-500 for Shadow.
   - With sequential per-target rendering, no other DrawMesh can intervene. The "only correct if no other DrawMesh intervenes" hazard is real but minor.
   - The only stale case is existing behaviour. If DrawForwardPass bails at SMFGroundDrawer.cpp:319 and deferred is off, DrawBorder reuses whatever LOD the shadow pass left behind. This is not multi-target specific.
2. The ray trace is cheap. The call at :189 passes groundOnly=true and ignoreWater=true. GuiTraceRay returns at TraceRay.cpp:406-407, right after CGround::LineGroundCol and LinePlaneCol. It never reaches the quadfield, unit or feature code (:423 onward), and it reads no sim unit state. The cost is one heightmap ray march per DrawMesh call. Normal already pays it twice a frame (deferred plus forward). Per-target multiplication is a minor cost, not a hotspot.
3. This drawer is not the default, so it is not on the prototype path unless ROAM=0. CONFIG(int, ROAM) defaults to 1 (SMFGroundDrawer.cpp:53-57) and drawerMode is chosen at :74. The default CRoamMeshDrawer has larger multi-camera problems of its own. Its tessellation and visibility state is keyed only by `shadowPass` (RoamMeshDrawer.cpp:189-198), it reads the PLAYER camera for distances (:219), and it increments drawFrame itself at :539. Basic is a secondary fallback, not "the easiest multi-camera path".

Related defect in the same file that the claim misses: meshVisPatches[].visUpdateFrames[camType] is written as globalRendering->drawFrame (:100) and tested with `< drawFrame` (:232, :262-285). drawFrame advances once per CGame::Draw (Game.cpp:1277), not per target. Two targets rendered in one Draw share a drawFrame, and Update only sets entries, never clears them. Window B therefore also draws every patch window A marked visible in that frame when both use the same camType. The result is over-draw (a union of visible sets), not missing geometry.

*Evidence:*
- `rts/Map/SMF/Basic/BasicMeshDrawer.cpp:180` PLAYER camera forced for Shadow/WaterReflection LOD
- `rts/Map/SMF/Basic/BasicMeshDrawer.cpp:189` GuiTraceRay per call
- `rts/Map/SMF/Basic/BasicMeshDrawer.cpp:104` drawPassLOD member set in Update
- `rts/Map/SMF/Basic/BasicMeshDrawer.h:62` drawPassLOD is a member consumed by later draw calls

*Direction:* Pass the target's own camera into CalcDrawPassLOD for the Shadow and WaterReflection cases. In practice that means the main camera of the target that the pass serves, not the global PLAYER slot, and it requires the render-target abstraction to expose that camera. Do the same for the other drawers' PLAYER reads, such as the ROAM drawer at RoamMeshDrawer.cpp:219. Make DrawBorderMesh recompute the LOD or take it as an argument, instead of relying on a member left by DrawMesh. Optionally cache the ground-ray distance per (target, camera) per frame. Key visUpdateFrames by (target, camType), or clear it per target, so that window B does not draw window A's visible patches. For the prototype, the simplest approach is to leave the drawer on its default (ROAM) path and note this fix as a follow-up for the ROAM=0 path.

> verifier A (corrected): The core defect is real. CalcDrawPassLOD hard-wires the single global PLAYER camera for the Shadow and WaterReflection passes (BasicMeshDrawer.cpp:180-181). That PLAYER camera is one static slot, cameras[CAMTYPE_PLAYER] (CameraHandler.cpp:94, :125). Several parts of the claim are wrong or overstated. The "member cache" hazard barely applies, the ray trace is a cheap heightmap march rather than a sim-state trace, and the drawer is not the default. Net effect is a wrong LOD on one drawer, not a cr

### PVC-09 - Lua per-frame caches and drawFrame-keyed state break when world callins fire per view
**Severity:** full - **Effort:** M - **Status:** confirmed - **Depends on:** none

Lua widgets and gadgets compute view-dependent data in Update/DrawGenesis (once per draw frame) and consume it in DrawWorld. They also key caches on Spring.GetDrawFrame(). With N views the callin runs N times per drawFrame, so view 2 reuses view 1's data.

*Why it breaks:* eventHandler.Update() runs once per CGame::UpdateUnsynced, then DrawGenesis once, then each draw callin runs once per view. Engine and Lua key per-frame state on globalRendering->drawFrame: Spring.GetDrawFrame returns it and LuaMatUniforms pushes it as visFrame. DrawWorld in view 2 sees the same drawFrame and the same Lua-side visible-unit lists computed (via camera->InView) during the previous callin or in Update. Camera-position-dependent LOD and culling lists would be stale. CPU cost also scales linearly with the number of views for LuaUI world widgets. Precedent: DrawWorldPreParticles already fires several times per frame, but widgets are written for that.

*Evidence:*
- `rts/Lua/LuaUnsyncedRead.cpp:1181` GetDrawFrame returns globalRendering->drawFrame split into two 16-bit halves (1183-1184)
- `rts/Lua/LuaMaterial.cpp:778` visFrame uniform = globalRendering->drawFrame
- `rts/Game/Game.cpp:1424` eventHandler.Update() once per draw frame, before camera->Update (1430) and all Draw callins
- `cont/base/springcontent/LuaGadgets/Gadgets/object_statusbars_default.lua:298` gadget:DrawWorld reads Spring.GetCameraPosition() at draw time (works per view only if camera is swapped)
- `cont/LuaUI/widgets.lua:1287` widgetHandler:DrawWorld loops widgets with no per-view information

*Direction:* Add a per-view draw counter and a documented contract that DrawWorld-class callins may fire several times per drawFrame with different cameras. Provide Spring.GetDrawingView() (see identity problem) so widgets can key caches by view. In the prototype restrict overlay world callins to the primary view (see the firing policy problem) so existing widgets see no behaviour change.

> verifier A (confirmed): Cited code matches. Game.cpp:1424 calls eventHandler.Update() once per UpdateUnsynced, before camera->Update (1430). LuaUnsyncedRead.cpp:1181-1185 returns globalRendering->drawFrame as two 16-bit halves. LuaMaterial.cpp:778 pushes visFrame from the same global. drawFrame is per-process, not per-view, so N views share one value and one Update/DrawGenesis pass. That makes the claim true: Lua caches computed in Update, or keyed on drawFrame, would be reused across views. It does not block the proto

### PVC-10 - Per-projectile view state (drawFlag byte, sortDist[3]) has no room for extra views and is Lua-visible
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** PVC-03

The per-projectile storage that carries per-view results is fixed at three float distances and eight flag bits. It is exposed to Lua as the drawFlag mask.

*Why it breaks:* CProjectile keeps exactly one frame's worth of view results: sortDist[3] indexed by CCamera camType (Projectile.h:132) and a single drawFlag byte. UpdateDrawFlags fills them once per frame (ProjectileDrawer.cpp:514-548) against the fixed global PLAYER/UWREFL/SHADOW cameras. This only limits a design that culls N views in one pass and keeps results for all of them alive at once. A design that re-culls per view into the same slots has no limitation. The OOB read (GetSortDist) is reachable only if someone invents a CAMTYPE >= 3 for the secondary window. Also note that DrawAlpha already runs against a single active camera (ProjectileDrawer.cpp:900), so the sort key can be computed there instead of being cached on the projectile.

*Evidence:*
- `rts/Sim/Projectiles/Projectile.h:132` std::array<float,3> sortDist
- `rts/Sim/Projectiles/Projectile.h:101` GetSortDist(camType) unchecked index
- `rts/Sim/Objects/WorldObject.h:14` DrawFlags is uint8_t with 7 bits used
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:900` sort key indexed by active camera's camType
- `rts/Lua/LuaUnsyncedRead.cpp:2365` Lua API exposes drawFlag semantics

*Direction:* For the prototype, do nothing here beyond the dependsOn fix. Parameterise UpdateDrawFlags by view, or time-share the PLAYER/UWREFL/SHADOW slots per window, and re-run it before each view's draw. That overwrites drawFlag/sortDist per view. Re-run cost is one for_mt over renderProjectiles. Two small cleanups are worth doing: (a) previousDrawFlag (:504) would be clobbered by the second run in the same frame, so guard it (only roll it on the primary view or once per frame). It is read for projectiles by nothing in Lua, so this is only an internal consistency detail. (b) Do not add new CAMTYPEs for secondary windows. Camera type is a role (player/reflection/shadow), not a window identity. Several per-camType arrays would silently break: BasicMeshDrawer.h:43 visUpdateFrames[4], Patch.h:212 lastDrawFrames[CAMTYPE_VISCUL] with the assert at Patch.cpp:782, and the camType loops at FeatureDrawerData.cpp:121 and UnitDrawerData.cpp:430. The cleanest sortDist fix, effort S and independent of everything else, is to delete sortDist and compute it at draw time in the DrawAlpha DP loop (ProjectileDrawer.cpp:905-912): ParticleSortKey would use camera->ProjectedDistance(p->drawPos) + p->sortDistOffset with the active camera. ProjectedDistance is a single dot product (Camera.h:220). p->drawPos is view-independent and already set in UpdateDrawFlags. SmokeTrailProjectile sets sortDistOffset (:99) and the offset keeps working. That removes the OOB hazard and 12 bytes per projectile, and drops the creg CR_IGNORED(sortDist) entry (Projectile.cpp:49). Only if the product later needs simultaneous N-view culling should per-view flags move into drawer-owned arrays, and then keyed to renderProjectiles position and rebuilt every frame, not treated as a stable index.

> verifier A (corrected): The storage facts hold. sortDist is std::array<float,3> (Projectile.h:132) and is indexed by camType with no bounds check (Projectile.h:101-102). DrawAlpha takes the key from camera->GetCamType() (ProjectileDrawer.cpp:900,910). The flag byte is uint8_t with bits 1,2,4,8,16,32,128 used and only 64 free (WorldObject.h:14-23,97). The alpha blend is GL_ONE/ONE_MINUS_SRC_ALPHA (:939). The Lua claim is false: the title ("is Lua-visible"), the cited LuaUnsyncedRead.cpp:2329-2369, and the BACKWARDS_COMP

> verifier B (corrected): The storage limits are real, but the claim is overstated for the prototype and has two factual errors. (1) Real: Projectile.h:132 declares sortDist as std::array<float,3>, and the accessors at Projectile.h:101-102 index it by camType with no bounds check. CAMTYPE_PLAYER/UWREFL/SHADOW are 0..2, ENVMAP=3 and VISCUL=4 (Camera.h:16-22). DrawFlags uses bits 1,2,4,8,16,32 and 128 (WorldObject.h:14-22), so only bit 64 is free. (2) It is derivative, not independent. The OOB read only happens if the desi

### PVC-11 - Reflection/refraction projectile flags use a stale single UWREFL camera
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** PVC-03

Reflection draw flags are computed before the reflection camera is updated for the frame, and only one reflection camera exists.

*Why it breaks:* Only the reflection flag and reflection sort distance are affected. UpdateDrawFlags (WorldDrawer.cpp:227, called from CGame::UpdateUnsynced via worldDrawer.Update at Game.cpp:1433) culls SO_REFLEC_FLAG and sets the UWREFL sort distance against the UWREFL camera (ProjectileDrawer.cpp:495, 530-535). That camera is only refreshed afterwards, by UniformConstants::Update (Game.cpp:1441 -> UniformConstants.cpp:185-196) and by the water passes during Draw (DynWater.cpp:469-486, BumpWater.cpp:1026-1038, AdvWater.cpp:339), so it holds the previous frame's mirrored PLAYER camera. This lag exists in a single view. Refraction is unaffected: SO_REFRAC_FLAG is set from the fresh PLAYER camera (ProjectileDrawer.cpp:514-523), and the refraction pass uses the live `camera`. At draw time the reflection camera is correct, because each water pass calls CopyStateReflect from the active camera, and billboards are built with it. The multi-target consequence (flags built against one fixed UWREFL camera) is the parent problem about once-per-frame flag culling, which also covers unit and feature flags.

*Evidence:*
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:530` reflection flags from camUWRefl at update time
- `rts/Game/Game.cpp:1441` UWREFL camera refreshed after UpdateDrawFlags
- `rts/Rendering/Env/DynWater.cpp:469` reflection camera derived from the PLAYER camera

*Direction:* Do not track this as a separate multi-target blocker. Fold the per-target-flag aspect into the parent problem. Keep a small note: when flags move to per-target computation, evaluate the UWREFL flags after that target's reflection camera has been derived (CopyStateReflect from the target's own camera), and decide whether secondary targets render reflections at all (CanDrawReflectionPass gate). Drop the claim about refraction.

> verifier B (corrected): The code evidence is accurate, but the severity is overstated and the problem is mostly a duplicate of its own parent. Verified: Game.cpp:1430 updates the PLAYER camera, then worldDrawer.Update (1433) reaches WorldDrawer.cpp:227 and UpdateDrawFlags. Only after that does UniformConstants::Update (Game.cpp:1441, UniformConstants.cpp:185-196) refresh the UWREFL camera via CopyStateReflect. The only other writers are the water passes (BumpWater.cpp:1026-1038, DynWater.cpp:469-486, AdvWater.cpp:339),

> verifier A (corrected): The core mechanism is real: CProjectileDrawer::UpdateDrawFlags tests the UWREFL camera (ProjectileDrawer.cpp:495, 530-535) before that camera is refreshed in the same frame. Order in CGame::UpdateUnsynced: camera->Update() at Game.cpp:1430, then worldDrawer.Update at 1433. That reaches UpdateDrawFlags at WorldDrawer.cpp:227. UniformConstants::Update at Game.cpp:1441 only then reaches UpdateMatricesImpl (UniformConstants.cpp:185-196), which does CopyStateReflect. The only other writers of the UWR

### PVC-12 - SMF texture streaming selects mip levels from the active camera at Update time
**Severity:** full - **Effort:** S - **Status:** confirmed - **Depends on:** none

With SMFTextureStreaming enabled (off by default, on in safemode) CSMFGroundTextures::DrawUpdate picks per-tile mip levels and unloads tiles using one camera and the primary view diagonal.

*Why it breaks:* DrawUpdate (called from CSMFGroundDrawer::Update, SMFGroundDrawer.cpp:543) uses GetActiveCamera() (SMFGroundTextures.cpp:348) and globalRendering->viewSize for the screen-diagonal (:351-353). TexSquareInView tests the same camera (:326), tiles not in that view for 120 draw frames are downgraded to mip 3 (:368-373), and the wanted mip comes from distance to that camera (:377-411). Tiles seen only by window B stay at whatever level they had (BindSquareTexture refreshes SetDrawFrame, :630-644) and are never upgraded.

*Evidence:*
- `rts/Map/SMF/SMFGroundTextures.cpp:46` SMFTextureStreaming default false, safemode true
- `rts/Map/SMF/SMFGroundTextures.cpp:348` GetActiveCamera() at Update time
- `rts/Map/SMF/SMFGroundTextures.cpp:351` view diagonal from globalRendering->viewSize
- `rts/Map/SMF/SMFGroundTextures.cpp:368` unload decision from one camera's view
- `rts/Map/SMF/SMFGroundTextures.cpp:641` draw-frame stamp refreshed by any pass

*Direction:* Collect required mip level per tile as the max over all target cameras (feed from each target's PrepareView), or disable streaming when more than one target exists.

> verifier A (confirmed): I checked each cited line in the current tree and the mechanism holds. SMFTextureStreaming defaults to false and has safemodeValue(true) (SMFGroundTextures.cpp:46), and the flag is read at :74. DrawUpdate returns immediately when streaming is off (:345-346). When it is on, it takes GetActiveCamera() once (:348) and builds the screen diagonal from globalRendering->viewSizeX/Y (:351-353). TexSquareInView tests that same camera (:326, :339). A tile out of that view is dropped to mip 3 only if mip <

### PVC-13 - Water reflection/refraction render-to-texture state is singleton and cached in members
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** LOP-03

Reflection/refraction RTT use one set of textures/FBOs and one UWREFL camera slot, and cache per-pass vectors in the water object for Draw() to upload later.

*Why it breaks:* Water RTT is a per-process singleton, not per-view: one IWater unique_ptr (IWater.h:55), one reflect FBO/texture (plus a refract FBO/texture for DynWater/BumpWater), one UWREFL camera slot, and instance flags drawReflection/drawRefraction. This is safe only while UpdateWater(view i) -> Draw(view i) stay adjacent inside a per-view CWorldDrawer pass (WorldDrawer.cpp:437-439). That is true today, so adjacency is a constraint to preserve, not a bug. The reflection and refraction passes are re-rendered per view, which costs a full scene redraw per window. Concrete defects that appear once there are two views with different cameras:

1. **Reflection culling uses a single UWREFL slot.**
   - CUnitDrawerData::UpdateObjectDrawFlags runs once per frame, from ModelDrawerData.h:221-225 (UpdateCommon). It loops over camTypes and tests `cam->InView` against CCameraHandler::GetCamera(CAMTYPE_UWREFL), then sets a per-object SO_REFLEC_FLAG (UnitDrawerData.cpp:430-474). The feature drawer does the same at FeatureDrawerData.cpp:122/182-190, and projectiles at ProjectileDrawer.cpp:495-534.
   - The UWREFL camera is only rewritten inside UpdateWater (AdvWater.cpp:343, DynWater.cpp:473, BumpWater.cpp:1030). At flag-update time it therefore holds whichever view reflected last, or the previous frame.
   - The reflection pass gates on that flag (UnitDrawer.cpp:205/238, FeatureDrawer.cpp:64/94). View 1's reflection will drop objects that are only visible in view 1's reflected frustum.
   - This overlaps the broader "draw flags are computed once against one player camera" problem, so cross-reference it.

2. **Viewport and aspect are reset from globalRendering.**
   - After the pass, `prvCam->Update()` (AdvWater.cpp:350, DynWater.cpp:485, BumpWater.cpp:1038) uses the default updateViewPort=true. That overwrites the view camera's viewport from globalRendering->viewPos/Size (Camera.h:120, Camera.cpp:154-155).
   - Camera::Update also takes aspectRatio and the projection size from globalRendering (Camera.cpp:144-153). A reflect camera built via CopyStateReflect therefore has window 1's aspect.
   - Each water path then ends with FBO::Unbind() (AdvWater.cpp:353, DynWater.cpp:422, BumpWater.cpp:571) and globalRendering->LoadViewport() (BumpWater.cpp:572, 986, DynWater.cpp:519). Those resets assume one default framebuffer and one view.

3. **Draw-time geometry and textures assume one target size.**
   - AdvWater::Draw builds its screen-space grid from globalRendering->viewPosX/viewSizeX/Y (AdvWater.cpp:134-136).
   - BumpWater sizes refractTexture, depthTexture and the refract depth RBO from the viewSize at construction (BumpWater.cpp:181-182, 332, 353, 391).
   - It then screen-copies with glCopyTexSubImage2D using the current globalRendering->viewSize (BumpWater.cpp:906, 912). A second window of a different size overflows the texture and raises GL_INVALID_VALUE. It is only an issue for BumpWater, which is non-default.

4. **Dynamic water simulation is anchored to one camera.**
   - DynWater::Update runs once per sim frame (WorldDrawer.cpp:236). It sets camPosBig from the global `camera` position (DynWater.cpp:432-435) and then advances the wave simulation (AddShipWakes, DrawWaves) around that point.
   - A second camera elsewhere on the map sees water simulated for the first camera's region. This is non-default (water mode 2 only).

*Evidence:*
- `rts/Rendering/Env/AdvWater.cpp:339` GetSetActiveCamera(UWREFL), CopyStateReflect, RTT into singleton texture
- `rts/Rendering/Env/DynWater.cpp:476` reflectRight/Up/Forward cached in members (494-496 for refract)
- `rts/Rendering/Env/BumpWater.cpp:946` singleton reflectTexture/refractTexture bound in Draw
- `rts/Rendering/Env/IWater.cpp:139` DrawReflections/DrawRefractions (183) full scene redraw with global flags
- `rts/Rendering/Env/IWater.h:57` drawReflection/drawRefraction instance flags
- `rts/Game/Camera.cpp:127` CopyStateReflect is parametric in the source camera (good) but writes one fixed UWREFL slot

*Direction:* Keep water as a shared (not per-view) simulation resource, but make the RTT-using part view-scoped.

1. Make PrepareWater(view)+Draw(view) an indivisible unit inside the per-view pass, and assert that adjacency. Share a single set of 512x512 reflect textures plus the DynWater vectors if the passes are serialized per view.
2. Make the SO_REFLEC_FLAG visibility a per-view decision. Either compute it from the view's own reflect camera inside the view pass, or skip the flag and cull reflection-pass draws against the UWREFL camera directly.
3. Stop `prvCam->Update()` and `FBO::Unbind()` from resetting global state by passing the view's viewport and target explicitly.
4. If the prototype's windows may differ in size, size the BumpWater refract/depth textures per view, or restrict the prototype to AdvWater or BasicWater.
5. Defer DynWater (anchoring the wave simulation to a camera) to a later phase.

> verifier B (corrected): The code facts are accurate but the failure scenario is conditional on a design the engine does not force, so as written this is not a prototype blocker or even a risk. AdvWater, DynWater and BumpWater each have one reflect texture/FBO and one UWREFL camera slot, and DynWater caches reflect*/refract* vectors in members (AdvWater.cpp:36,96-99,339; DynWater.cpp:476-478,494-496,353-366; BumpWater.cpp:946-947). But UpdateWater and Draw are already strictly adjacent in the per-view draw path: CWorldD

> verifier A (corrected): The cited lines are accurate. AdvWater has one 512x512 reflectTexture/reflectFBO (AdvWater.cpp:30-36, 96-99) and one UWREFL slot (339). DynWater caches reflect*/refract* vectors in members (DynWater.cpp:476-478, 494-496) and uploads them in Draw (353-366). BumpWater binds the single reflectTexture/refractTexture (946-947). IWater::DrawReflections/DrawRefractions flip the instance flags drawReflection/drawRefraction (IWater.cpp:144/177, 188/219), and CopyStateReflect is parametric (Camera.cpp:127

## GPU buffers, UBOs & shader uniforms

### BUF-01 - Per-frame UniformConstants UBO holds one target's camera, view and mouse state
**Severity:** blocker - **Effort:** M - **Status:** corrected - **Depends on:** DSN-03; Per-frame timing globals advance inside UpdateUnsynced, so CGame::Draw cannot simply run once per target; CAM-04; LOP-03; RenderView definition: per-target parameters consumed by model drawers; Visibility flags and transform uploads come from a single camera; No per-view identity to key per-frame caches, flags and tokens on

UniformConstants updates both UBOs once per draw frame in UpdateUnsynced and binds them once in Draw. They contain the PLAYER camera matrices, view geometry, fog parameters and the mouse position/world ray.

*Why it breaks:* The mechanism is correct, but the impact list needs narrowing. The fields that actually change the 2-window prototype's output are the ones the engine GL4 shaders read. ModelVertProgGL4.glsl reads cameraViewProj (:125,:133), reflectionViewProj (:129), cameraViewInv for worldCameraDir and fog distance (:372-377), shadowView (:369) and fogParams (:377). ShadowGenVertProgGL4.glsl reads shadowView/shadowProj (:105,:116). ModelFragProgGL4.glsl reads fogColor (:143). viewGeometry, mouseScreenPos, mouseStatus and mouseWorldPos are declared in the GLSL copies but have no engine-shader consumer (grep of *GL4.glsl shows declarations only). They matter only to Lua shaders via gl.GetEngineUniformBufferDef (LuaShaders.cpp:1359-1368), so they are not a prototype blocker and belong to the later Lua/GUI work. The mouse-ray cost (:279) is a separate efficiency note: skip it, or compute it only for the input-owning target. Two details were missing. First, UpdateMatricesImpl ends with CCameraHandler::SetActiveCamera(CAMTYPE_PLAYER) (:195). It does not restore prvCam's type, so any scheme that selects a per-target active camera by CAMTYPE would be silently reset when UpdateMatrices runs. It also feeds prvCam (whatever was active) into reflCam->CopyStateReflect, which asserts prvCam is not UWREFL (Camera.cpp:127-139). Second, the shadow matrices in the UBO come from shadowHandler.Update(), which is fed only by the PLAYER camera (ShadowHandler.cpp:128-135) and runs before the UBO update (Game.cpp:1431). The UBO shadowView/shadowProj therefore stay consistent only with one shadow map built for one camera. A per-target UBO refresh does not fix shadows by itself, because that needs per-view shadow maps or a shared one covering both frusta. Also, BumpWater::DrawReflection (BumpWater.cpp:1026-1038) recomputes the UWREFL camera itself for its FBO pass and does not touch the UBO. Reflection-pass GL4 models therefore read the reflectionViewProj that UpdateMatricesImpl derived at update time (UniformConstants.cpp:185-196), so the UBO refresh must complete before each target's reflection pass.

*Evidence:*
- `rts/Rendering/UniformConstants.cpp:161` UpdateMatricesImpl: camPlayer matrices, shadow, reflection, minimap
- `rts/Rendering/UniformConstants.cpp:222` viewGeometry from globalRendering->viewSize/viewPos; fog params 230-233
- `rts/Rendering/UniformConstants.cpp:250` mouse-ray trace per frame (250-293)
- `rts/Game/Game.cpp:1441` UniformConstants::Update once in UpdateUnsynced; Bind once at 1464
- `cont/base/springcontent/shaders/GLSL/ModelVertProgGL4.glsl:125` gl_Position = cameraViewProj * worldPos; reflectionViewProj at 129
- `rts/Rendering/UniformConstants.h:21` UniformMatricesBuffer/UniformParamsBuffer layouts (21-93)
- `rts/Game/Game.cpp:1464` Bind() once per Draw
- `rts/Rendering/UniformConstants.cpp:167` cameraView/Proj/ViewProj/Inv/Billboard from CAMTYPE_PLAYER (167-179)
- `rts/Rendering/UniformConstants.cpp:279` per-frame mouse GuiTraceRay for mouseWorldPos (250-293)
- `rts/Rendering/UniformConstants.cpp:129` SB_BUFFERSUBDATA UBOs, no ring slicing
- `rts/Rendering/UniformConstants.cpp:335` BindBufferRange idx 0/1 (UniformConstants.h:121-122)
- `rts/Rendering/UniformConstants.cpp:186` GetSetActiveCamera(UWREFL) side effect
- `rts/Rendering/UniformConstants.cpp:230` fogParams from camPlayer far plane
- `cont/base/springcontent/shaders/GLSL/ModelVertProgGL4.glsl:129` reflectionViewProj from UBO; 125/133 cameraViewProj; 377 fogParams
- `rts/Rendering/UniformConstants.cpp:163` screenView/screenProj copied from globalRendering
- `cont/base/springcontent/shaders/GLSL/ShadowGenVertProgGL4.glsl:116` shadow pass uses UBO shadowProj/shadowView
- `rts/Rendering/UniformConstants.cpp:131` numElems=1, SB_BUFFERSUBDATA
- `rts/Lua/LuaShaders.cpp:1359` gl.GetEngineUniformBufferDef returns the GLSL definitions of both UBOs to Lua (1359-1368)
- `cont/examples/Widgets/gui_buildsquare_gl4.lua:157` example Lua widget compiling shaders against the engine UBO definitions (157-162)
- `rts/Rendering/UniformConstants.cpp:185` reflection matrices via GetSetActiveCamera(UWREFL) and SetActiveCamera(PLAYER) (185-196); shadow matrices 181-183
- `rts/Rendering/Common/ModelDrawerState.cpp:317` GL4 Enable picks NORMAL/REFLCT/REFRAC camera mode from GetDrawMode
- `rts/Rendering/UniformConstants.cpp:134` 1-element SB_BUFFERSUBDATA stream buffers; cheap to refill per target

*Direction:* Keep the UniformMatricesBuffer and UniformParamsBuffer layouts and binding points (0 and 1) unchanged, and add UniformConstants::UpdateForView(view) that re-fills and re-uploads the camera-dependent fields before each target's reflection/shadow/main passes. Do not move viewGeometry or fogParams into a new third buffer. The std140 struct text is hardcoded in four engine shaders (ModelVertProgGL4.glsl:15-100, ModelFragProgGL4.glsl:13+, ShadowGenVertProgGL4.glsl:15+, ShadowGenFragProgGL4.glsl) and is exported to Lua widgets through GetEngineUniformBufferDef (generated from creg member names/offsets, UniformConstants.h:136-182). A split would therefore be a breaking ABI change for shaders and widgets, which coding-agents/BACKWARDS_COMPATIBILITY.md says to avoid. Per target, the cheapest correct change is to refresh the matrices buffer (camera, reflection and, once per-view shadows exist, shadow matrices) and the viewGeometry and fogParams fields (about 1.5 KB) with BufferSubData, and leave time, sun, wind, map and team colors as per-frame. Replace the hard-coded SetActiveCamera(PLAYER) reset at :195 with restoring the previous camera type. Make the camera that is read a parameter instead of the global PLAYER slot (UpdateMatricesImpl hardcodes CCameraHandler::GetCamera(CAMTYPE_PLAYER) at :167 and :230). Skip the GuiTraceRay mouse block for non-primary targets. Effort S to M. It depends on the camera-per-view and active-camera-selection work.

> verifier B (confirmed): The core mechanism is real and I verified it. UniformConstants::Update() runs once per draw frame (Game.cpp:1441, in UpdateUnsynced) and Bind() runs once per Draw (Game.cpp:1464). UpdateMatricesImpl reads the single CAMTYPE_PLAYER camera (UniformConstants.cpp:167-179). It also copies shadowHandler's DRAWING matrices (181-183) and derives reflectionViewProj from the previously active camera (185-196). The buffers are plain 1-element SB_BUFFERSUBDATA UBOs (131-132; StreamBuffer.h:195-251).

The GL

> verifier A (corrected): The core mechanism holds in the current tree. UniformConstants::Update() runs once per draw frame from CGame::UpdateUnsynced (Game.cpp:1441) and is bound once in CGame::Draw (Game.cpp:1464). UpdateMatricesImpl copies the CAMTYPE_PLAYER camera (UniformConstants.cpp:167-179) and shadowHandler's DRAWING matrices (:181-183) into one single-element glBufferSubData UBO (:131-132, BufferSubDataImpl::Unmap in StreamBuffer.h:240-251). ModelVertProgGL4.glsl:125/133 does gl_Position = cameraViewProj * worl

### BUF-02 - [arch B] Indexed UBO/SSBO bindings are per-context and the ring rebind picks the wrong slice
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** DSN-02; Single shared GL context versus one context per window

UBO 0/1 and SSBO 0/1 bind points are context state. The SSBO range is bound only on frames with an update, right before the ring advances, so any later rebind (for example per new context) binds the unwritten next slice. Not needed under A or C.

*Why it breaks:* Same core as claimed, with these changes. (a) The ring problem is SSBO-only. UniformConstants builds its UBOs as SB_BUFFERSUBDATA (UniformConstants.cpp:132). BufferSubDataImpl (StreamBuffer.h:195) has no SwapBuffer override, so allocIdx stays 0 (init at StreamBuffer.cpp:43). A UBO rebind with allocIdx is therefore always correct, and UniformConstants::Bind (:335-336, called once per Draw at Game.cpp:1464) only needs to be re-issued per context. Only SSBOs from TransformsUploader/ModelUniformsUploader rotate: when supportPersistentMapping is true they are SB_PERSISTENTMAP with PERSISTENT_MAPPING_BUFFERING=3 (ModelsDataUploader.cpp:35,180-188), and PersistentMapImpl::SwapBuffer advances allocIdx at StreamBuffer.h:475-477. With the non-persistent fallback (numBuffers=1) the rebind is harmless. (b) 'Unwritten' is imprecise. The update list is armed with SetTrueValue(numBuffers) (ModelsDataUploader.cpp:140), so each changed element is written once per slice over numBuffers frames. In steady state with no changes all slices are identical and a rebind of any slice is fine. While transforms change every frame, which is normal in play, allocIdx points at the slice that was last written numBuffers frames ago, so units in a newly bound context would lag 2-3 frames. This is a stale-data glitch, not garbage. The slice to rebind is (allocIdx-1+numBuffers)%numBuffers, i.e. the last one Updated and bound, which is not exposed today. (c) Missing mechanism: on growth (ModelsDataUploader.cpp:55-60, StreamBuffer.h:119-123) Resize() deletes and recreates the GL buffer, so the buffer id changes. Any per-context binding table has to be invalidated and rebound in every context, not only the one running Update. Under B this is a cross-context sync point in addition to the slice choice. (d) The Lua cases (LuaVBOImpl.h:153-154 reserves UBO 0-5 and SSBO 0-3, and Lua VBO:BindBufferRange at LuaVBOImpl.cpp:1445 goes through VBO::BindBufferRangeImpl, VBO.cpp:191-201) matter only for Lua draw callins that bind once and rely on persistence. They are not part of a 3D-only prototype. The Shader.cpp:438 ARB env params and the glClipControl at GlobalRendering.cpp:1714 are separate per-context state items and should be tracked as their own problems, not folded in here.

*Evidence:*
- `rts/Rendering/ModelsDataUploader.cpp:86` early return when no update; :109-110 BindBufferRange then SwapBuffer; :157 init-time bind
- `rts/Rendering/GL/StreamBuffer.cpp:131` BindBufferRange offset = allocIdx*byteSize (to 141)
- `rts/Rendering/ModelsDataUploader.h:52` SSBO binding indices 0/1 (:88)
- `rts/Rendering/UniformConstants.cpp:335` UBO 0/1 bound only here, called once at Game.cpp:1464
- `rts/Rendering/GL/VBO.cpp:123` Delete unbinds bbrItems in the current context only
- `rts/Lua/LuaOpenGL.cpp:5075` Lua image-unit binding is also context state
- `rts/Rendering/ModelsDataUploader.cpp:157` SSBO bound at init; rebound at 109 only when NeedUpdate()
- `rts/Rendering/Shaders/Shader.cpp:438` ARB programs set values via glProgramEnvParameter4fARB (context state); DynWater.cpp:346-366
- `rts/Rendering/GlobalRendering.cpp:1714` glClipControl set once in InitGLState
- `rts/Lua/LuaVBOImpl.h:153` Lua may bind UBO >= 6 and SSBO >= 4; engine reserves 0-5/0-3

*Direction:* Label this a blocker only if architecture B (N shared GL contexts) is selected. Under A (one context made current on each window in turn) or C (offscreen render plus present) nothing here applies: the single context keeps its indexed bindings across windows, because nothing in rts/ unbinds them between frames. In that case the only action is to keep UniformConstants::Bind (Game.cpp:1464) once per frame, or re-issue it per target if the UBO contents change per camera. Under B, add a per-context binding table covering the engine-reserved indices (UBO 0/1, SSBO 0/1) and rebind at every target switch. For SSBOs store the last-written slice offset, for example a getter that records the offset at BindBufferRange time in UpdateCommon (:109) or computes (allocIdx-1+numBuffers)%numBuffers after SwapBuffer, instead of reusing allocIdx. Invalidate and rebind in all contexts after SSBO Resize (:60). Also order cross-context visibility of the persistent coherent writes with a flush or fence before another context reads them. Expect B to need much more than this, because VAOs and FBOs are not shareable between contexts.

> verifier B (corrected): The ring mechanism is real but narrower than claimed, and it exists only under arch B (N shared contexts). The claim's own text says "Not needed under A or C", so "blocker" is wrong for the prototype as a whole. A 2-window prototype can use one context made current on each window in turn (A) or an offscreen render plus blit (C), and then this problem does not exist.

What I verified, and what holds:
- ModelsDataUploader.cpp:86 returns early when nothing needs an update. Otherwise it unbinds (:90

> verifier A (corrected): The core mechanism checks out in the current tree. Indexed UBO/SSBO binding points are per-context GL state. ModelsDataUploader.cpp UpdateCommon returns early at :86 when !ul.NeedUpdate(). Otherwise it calls ssbo->BindBufferRange(idx) at :109 and then SwapBuffer() at :110, and the only other bind is the init-time one at :157. IStreamBufferConcept::BindBufferRange (StreamBuffer.cpp:134) uses allocIdx*byteSize, so a rebind after the swap selects the next slice. Only one GL context exists today (SD

### BUF-03 - Streaming buffers and fences assume one upload and one swap per frame
**Severity:** risk - **Effort:** S - **Status:** unverified - **Depends on:** CTX-09

The transforms SSBO is a 3-deep persistent ring whose fences are created at SwapBuffers. Per-view Update calls halve its depth, and per-window SwapBuffers would place the fence between views. The instance VBO is rewritten with plain glBufferSubData at offset 0 on every Submit.

*Why it breaks:* PERSISTENT_MAPPING_BUFFERING=3 (ModelsDataUploader.cpp:35). UpdateCommon ends with BindBufferRange + SwapBuffer (109-110). SwapBuffer queues the fence into lockList (StreamBuffer.h:384-387, StreamBuffer.cpp:52-56), and the GLsync is only created in PutBufferLocks called from CGlobalRendering::SwapBuffers (GlobalRendering.cpp:688-689, StreamBuffer.cpp:20-34). The fence therefore covers all draws since the last swap. If each window swaps separately, the fence is created after window 1's draws but before window 2's. The next-but-two Map can then overwrite the slot while window 2's draws still read it. If Update is called per view instead, the ring rotates twice per frame and the next frame's Map waits on the previous frame (WaitBuffer, StreamBuffer.cpp:58-77): a CPU stall. S3DModelVAO::Submit rebuilds modelDataToInstance and writes instVBO via glBufferSubData at offset 0 for every bin (3DModelVAO.cpp:411-434, 444) with no orphaning, so each extra view doubles the implicit-sync hazards. Uniforms use SB_BUFFERSUBDATA with 1 buffer (ModelsDataUploader.cpp:329-341).

*Evidence:*
- `rts/Rendering/ModelsDataUploader.cpp:35` 3-deep ring; SwapBuffer at 110
- `rts/Rendering/GL/StreamBuffer.cpp:20` PutBufferLocks creates fences for everything queued
- `rts/Rendering/GlobalRendering.cpp:688` PutBufferLocks and SwapRenderBuffers once per SwapBuffers call
- `rts/Rendering/GL/StreamBuffer.h:350` Map waits fences[allocIdx]; SwapBuffer queues the lock (384)
- `rts/Rendering/Models/3DModelVAO.cpp:432` instVBO overwritten with glBufferSubData at offset 0 per Submit

*Direction:* Keep transform and uniform uploads once per frame, view-independent. Call PutBufferLocks and swaps only after the last view's draws (or per window but only after both views are submitted). Consider ring-allocating instance ranges per Submit (as the immediate path does with immediateBaseInstance) or orphaning instVBO.

### BUF-04 - [arch B] Cross-context synchronisation of shared objects and in-place streamed buffers
**Severity:** full - **Effort:** XL - **Status:** corrected - **Depends on:** DSN-02

The engine reuses shared buffers in place (SubData into the same region, instance buffer overwritten per Submit) and relies on single-context command ordering. Fences are per-slice and created in whichever context is current. With several contexts, GL gives no ordering between them.

*Why it breaks:* Only the SB_PERSISTENTMAP ring SSBOs (transformsUploader and modelUniformsUploader, 3 slots) carry fences. They are written once per frame in UpdateUnsynced and read by all targets afterwards. Under a single context (A) or offscreen-plus-present (C) there is no cross-context hazard. The one real issue is fence timing. PutBufferLocks() and RenderBuffer::SwapRenderBuffers() currently run inside CGlobalRendering::SwapBuffers (GlobalRendering.cpp:688-689). With a swap per window, the fence is created between window 1's and window 2's draws, so window 2's reads are not covered by the slot's fence. Under B, the extra real concerns are non-shared container objects (VAO, FBO, indexed binding points) and a flush plus fence or glFinish for GPU-written shared textures such as shadow maps and cubemaps. They are not the streamed-buffer SubData ordering described in the claim.

*Evidence:*
- `rts/Rendering/Models/3DModelVAO.cpp:432` instVBO SetBufferSubData at offset 0 each Submit (immediate path :480-482)
- `rts/Rendering/GL/StreamBuffer.h:240` BufferSubDataImpl::Unmap uploads in place (to 251)
- `rts/Rendering/GL/RenderBuffers.h:817` RenderBuffer upload mirrors CPU index into the same GPU range (to 823)
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:777` in-place decal instance SubData (to 787)
- `rts/Map/SMF/ROAM/Patch.cpp:207` patch VBO SubData in place (to 217)
- `rts/Rendering/GL/StreamBuffer.cpp:58` WaitBuffer spin on glClientWaitSync, flushes waiting context only (to 77)
- `rts/Rendering/GL/StreamBuffer.cpp:28` fences created in whichever context is current; lockList is global (StreamBuffer.h:81)

*Direction:* 1. Record the architecture decision first: A (one glContext, SDL_GL_MakeCurrent(window_i, glContext), SDL_GL_SwapWindow per window) for the prototype. This makes the claim moot.
2. Split SwapBuffers into a per-window present step (the swap and the READ_FRAMEBUFFER unbind) and a once-per-frame end-of-frame step. The end-of-frame step covers RenderBuffer::SwapRenderBuffers, IStreamBufferConcept::PutBufferLocks and glClearErrors. Run it after the last target has been drawn, and ideally before the first swap. Effort S.
3. Only if B (shared contexts, or one render thread per window) is chosen later:
   - call glFlush at the end of each target's draw;
   - make the sync objects and lockList per frame, or create the fence after the last draw call of the last target;
   - add glWaitSync or glFinish at the hand-off points that cross contexts, covering GPU-written shared textures such as shadow maps, cubemaps and sky textures;
   - re-bind the indexed UBO/SSBO ranges in each context;
   - duplicate VAOs, FBOs and queries per context. This is the dominant cost and is tracked as a separate problem.
   Do not duplicate every streamed buffer per context.

> verifier A (corrected): The cited code exists as described, but the claim overstates its effect. (1) It only applies if arch B (several GL contexts) is chosen. The engine has exactly one GL context today: GlobalRendering.cpp:474/490/515 creates one `glContext`, and LoadLock.h:21/32 and GlobalRendering.cpp:612 only move that single context between threads. (2) "A fence created in a never-flushed context spins forever" does not hold. The only place fences are created is PutBufferLocks (StreamBuffer.cpp:32). It is called 

> verifier B (corrected): This is not a blocker for the 2-window, 3D-only prototype. It exists only under architecture B (N shared GL contexts). The claim's own direction says "Under A or C none of this is needed". A, one context made current on each window in turn, is a far cheaper prototype path. Under B, a single-threaded prototype would probably still work, so the effect would be a risk at most. Several mechanism details are wrong or overstated.

1. The engine has exactly one GL context today. GlobalRendering.cpp:612

### BUF-05 - SwapBuffers fuses presenting a window with once-per-frame buffer bookkeeping
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** Present path hard-wired to the single sdlWindow and assumes it is current; CTX-02; Context ownership hardwired to the single sdlWindow (make-current, swap, destroy, load handoff); TGT-01; Window and GL context are CGlobalRendering singletons with no per-target MakeCurrent; GL context and drawable topology is hardwired to one window; Context topology (one context with N drawables vs one per window) must be decided before validation can be specified; CGame::Draw welds timing, update and draw so a per-window call double-advances frame state

The same function swaps the window, advances every RenderBuffer ring, places the stream-buffer fences, emits FrameMark, fires the Lua timing callin, clears GL errors and stamps lastSwapBuffersEnd. Calling it once per window runs all of that N times per frame.

*Why it breaks:* SwapBuffers is a fused once-per-frame end-of-frame hook and window present. The facts, with the corrections above applied:

- RenderBuffer::SwapRenderBuffers (GlobalRendering.cpp:688) only clears the CPU vertex/index vectors and resets the upload indices (RenderBuffers.h:391-417). All render buffers use BUFFERSUBDATA/BUFFERDATA, so there is no GPU ring advance and no fence for them.
- PutBufferLocks (:689, StreamBuffer.cpp:20-35) creates glFenceSync objects only for the stream buffers that queued a lock. In practice that is the TransformsUploader persistent-map SSBO, which queues its own lock and advances its own ring in UpdateCommon (ModelsDataUploader.cpp:109-110) once per CGame::Update (Game.cpp:1434). WaitBuffer (StreamBuffer.cpp:58-77) spins on that fence at the next Map (Update), 3 frames later.
- Calling SwapBuffers per window therefore:
  - places that fence after view 1 but before view 2's commands. This is a theoretical early-free of view 2's reads that is only exposed if the GPU lags 2 or more frames.
  - with one GL context per window, a fence covers only the creating context's earlier commands. View 2's reads in the other context would then not be fenced at all. This is topology-dependent and depends on the context-topology design decision.
  - clears the CPU render-buffer vectors between views. This is harmless as long as every view fills and draws its own geometry within its draw call. Any RenderBuffer filled during Update and drawn during Draw would be wiped.
  - emits FrameMark (:713) and DbgTimingInfo(TIMING_SWAP) (:716, consumed by ProfileDrawer.cpp:640 and Lua) once per window.
  - stamps lastSwapBuffersEnd (:717) after each swap, which skews the camera-transition interpolation phase.
  - makes window 2's SDL_GL_SwapWindow block on vsync a second time per frame (swap interval set at VerticalSync.cpp:76-87), a frame-pacing concern.
- Not affected: UniformConstants, ModelUniformsUploader and the ProjectileDrawer saved range under view-granular swaps (see reason).

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:688` SwapRenderBuffers and PutBufferLocks inside SwapBuffers (671-718)
- `rts/Rendering/GlobalRendering.cpp:689` IStreamBufferConcept::PutBufferLocks() inside SwapBuffers
- `rts/Rendering/GlobalRendering.cpp:713` FrameMark per swap
- `rts/Rendering/GlobalRendering.cpp:716` eventHandler.DbgTimingInfo(TIMING_SWAP) and lastSwapBuffersEnd (:717) per swap
- `rts/Rendering/GL/RenderBuffers.h:407` TypedRenderBuffer::SwapBuffer() swaps vbo/ebo and Clear()s CPU vertex/index lists
- `rts/Rendering/GL/RenderBuffers.h:595` comment: pending range valid until the next SwapBuffer()
- `rts/Rendering/GL/StreamBuffer.cpp:20` PutBufferLocks fences all queued stream buffers
- `rts/Rendering/GL/StreamBuffer.cpp:66` busy-spin glClientWaitSync(…,1ns) in WaitBuffer
- `rts/Rendering/GL/StreamBuffer.h:83` DEFAULT_NUM_BUFFERS = 3
- `rts/Rendering/ModelsDataUploader.cpp:110` per-frame SSBO ring advanced at Update, fenced at swap
- `rts/Lua/LuaOpenGL.cpp:5352` Lua-exposed gl.SwapBuffers
- `rts/Rendering/GL/StreamBuffer.cpp:32` fence created on the current context
- `rts/Game/CameraHandler.cpp:412` tween uses lastSwapBuffersEnd (also 348, 406)
- `rts/Rendering/ModelsDataUploader.cpp:35` 3-slice persistent ring; :180-190 SB_PERSISTENTMAP; :109-110 bind then SwapBuffer
- `rts/System/SpringApp.cpp:906` one SwapBuffers per frame
- `rts/Rendering/GlobalRendering.cpp:671` SwapBuffers combines frame work (688-689, 713, 717) with window swap (703)
- `rts/Rendering/GL/RenderBuffers.h:54` SwapRenderBuffers rotates all buffers once per swap
- `rts/Game/LoadScreen.cpp:326` another caller
- `rts/Rendering/GlobalRendering.cpp:703` SDL_GL_SwapWindow(sdlWindow) only
- `rts/Rendering/GlobalRendering.cpp:679` clearErrors, ring swap, fences, DWM flush, SwapWindow, FrameMark in one function
- `rts/Rendering/ModelsDataUploader.cpp:109` SSBO bound then SwapBuffer queued at upload time
- `rts/Rendering/GlobalRendering.cpp:683` early return before swap/lock when !allowSwapBuffers
- `rts/Rendering/GL/StreamBuffer.cpp:65` unbounded spin in WaitBuffer, L_DEBUG only
- `rts/Rendering/GlobalRendering.cpp:717` lastSwapBuffersEnd = spring_now()
- `rts/Rendering/GlobalRendering.cpp:725` timer queries use drawFrame&1 (720-733)

*Direction:* Split SwapBuffers into two steps.

- EndFrame(): run once per frame after the last view is submitted. It does glClearErrors, SwapRenderBuffers, PutBufferLocks, FrameMark, the TIMING_SWAP event and the lastSwapBuffersEnd stamp.
- Present(target): the DWM flush, GL_READ_FRAMEBUFFER=0 and SDL_GL_SwapWindow for one window only. Run it once per window and make it take the window as an argument.

Place the PutBufferLocks fence after the last view's commands. Under a one-context-per-window design, either place a fence per context or use a glFinish/flush handoff for the shared persistent SSBO. Take lastSwapBuffersEnd and TIMING_SWAP from the primary window only. For the prototype, calling the existing SwapBuffers once per frame for the primary window plus a minimal SDL_GL_SwapWindow for window 2 is enough, so this refactor is not a prototype blocker.

> verifier B (corrected): The fusion is real: GlobalRendering.cpp:671-718 does glClearErrors, SwapRenderBuffers, PutBufferLocks, the READ-FBO unbind, DwmFlush, SDL_GL_SwapWindow(sdlWindow), FrameMark, DbgTimingInfo(TIMING_SWAP) and lastSwapBuffersEnd in one function. The claimed mechanism is mostly wrong, though, and the "blocker" rating does not hold.

1. RenderBuffers have no ring and no fences. TypedRenderBuffer::bufferTypeDefault is SB_BUFFERSUBDATA (RenderBuffers.h:795), and nothing in rts/ overrides it. A grep for 

> verifier A (corrected): The fusion is real: CGlobalRendering::SwapBuffers (GlobalRendering.cpp:671-718) does glClearErrors, SwapRenderBuffers, PutBufferLocks, the DWM flush, SDL_GL_SwapWindow(sdlWindow), FrameMark, DbgTimingInfo(TIMING_SWAP) and the lastSwapBuffersEnd stamp in one call. Callers are SpringApp.cpp:906 (once per frame), SpringApp.cpp:373, SplashScreen.cpp:107, LoadScreen.cpp:326 and LuaOpenGL.cpp:5352. The hazard story is mostly wrong in the current tree, and the severity is overstated.

1. SwapRenderBuff

### BUF-06 - Decals are drawn unculled and re-evaluated for every target
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** none

Each Draw runs an O(decals) visibility pass on the CPU and issues one instanced draw covering every decal, with no camera culling. Cost scales with target count.

*Why it breaks:* Draw() (GroundDecalHandler.cpp:746) calls UpdateDecalsVisibility() at 761 on every invocation. It is invoked once per world pass by WorldDrawer.cpp:349. The pass iterates decalOwners (1549, only DECAL_PLATE entries do real work, with a dynamic_cast<const CUnit*> at 1560) and idToCmInfo (1607). All inputs are per-process, not per-camera: gu->myAllyTeam, gu->spectatingFullView, gu->myTeam and gs->frameNum + globalRendering->timeOffset. The second call in the same frame therefore finds no changes and decalsUpdateList stays clean, so the instVBO upload (777-789) runs once. Each target then runs glDrawArraysInstanced(..., 36, decals.size()) at 830. No CPU frustum culling happens (Camera.h is included but unused). The vertex shader (GroundDecalsVertProg.glsl:228-237) discards only zero-alpha or zero-visMult decals, and the rest is cut by GPU clipping. The cost is therefore a duplicated O(decals) CPU walk plus 36*decals vertex invocations per extra target. It causes no GL error and no wrong output.

*Evidence:*
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:761` UpdateDecalsVisibility per Draw
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:830` instanced draw of all decals
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:777` instance upload is once-per-update, correct for N targets

*Direction:* Keep the visibility/colour-map update to a single evaluation per draw frame. Either add a once-per-frame hook called from the shared unsynced update stage, or guard UpdateDecalsVisibility() with a last-evaluated key of (gs->frameNum, globalRendering->timeOffset). GameFramePost is the wrong hook because timeOffset is draw-rate. Per-target frustum culling (compacted index buffer or multi-draw) is optional once decal counts warrant it. If per-target viewer team or LOS is ever supported, visMult must move out of the shared instance buffer into a per-target visibility mask.

> verifier A (corrected): The mechanism holds in the current tree. CGroundDecalHandler::Draw() calls UpdateDecalsVisibility() unconditionally at GroundDecalHandler.cpp:761, after the early-outs at 749-759. That function walks every decalOwners entry (1549), skipping non-DECAL_PLATE entries (1553), and every idToCmInfo colour-map entry (1607). It then issues glDrawArraysInstanced(GL_TRIANGLES, 0, 36, decals.size()) at 830. The only caller is WorldDrawer.cpp:349, so the work repeats once per rendered world target.

Three d

### BUF-07 - Params UBO mixes per-frame, per-view and per-input-window fields and recomputes all of them per update
**Severity:** polish - **Effort:** M - **Status:** corrected - **Depends on:** Engine UBO pair carries a single view's camera, shadow and reflection data

UniformParamsBuffer combines frame-constant data (sun, map, time, 2048 team colours), per-view data (viewGeometry, fogParams scaled by the player camera far plane) and per-window input (mouse position/buttons plus a world-ray pick), all rebuilt by one function.

*Why it breaks:* Same root cause as claimed, with narrower and more precise effects. A single-element UBO (UniformConstants.cpp:130-132) is filled by one function that reads three kinds of state. Frame-constant state: time, map, sun, wind, team and custom colours. Per-view state: viewGeometry from globalRendering and fogParams from camPlayer far-plane. Per-window input: mouse* and the GuiTraceRay pick from the single global `mouse`. Far plane is a per-camera value, recomputed in CCamera::UpdateViewRange from each camera's position and forward vector, so window 2's camera will have a different far plane than the player camera. Window 2 would therefore get window 1's fog start/end in the forward model shader (ModelVertProgGL4.glsl:376-378). Engine shaders never read viewGeometry or mouse*. Those fields reach only Lua GL4 shaders through GetEngineUniformBufferDef (LuaShaders.cpp:1359-1368), whose GLSL text is generated from the struct's CR member offsets (UniformConstants.h:136-144). Splitting or reordering the struct therefore changes a Lua-visible definition, so the split should keep binding 1's layout compatible. Per-view rewrites of a single GL buffer cost a full glBufferSubData of the 33 KB struct each time (StreamBuffer.h:240-251). BUFFERING=3 (UniformConstants.h:124) is declared but not used.

*Evidence:*
- `rts/Rendering/UniformConstants.cpp:216` rndVec3 = guRNG.NextVector() every update
- `rts/Rendering/UniformConstants.cpp:222` viewGeometry from globalRendering
- `rts/Rendering/UniformConstants.cpp:231` fogParams scaled by camPlayer far plane
- `rts/Rendering/UniformConstants.cpp:250` mouseScreenPos/mouseStatus from global mouse (255-262)
- `rts/Rendering/UniformConstants.cpp:295` rewrites all team colours and custom palette every update
- `rts/Rendering/UniformConstants.h:92` teamColor[MAX_PALETTE_COLORS] dominates the 33 KB block
- `rts/Rendering/UniformConstants.cpp:265` per-frame TraceRay for mouseWorldPos using PLAYER camera
- `rts/Game/Game.cpp:1441` UBO updated once per UpdateUnsynced; Bind at 1464
- `cont/base/springcontent/shaders/GLSL/ModelFragProgGL4.glsl:37` mouse fields exposed in the shader UBO block (38-40)

*Direction:* For the prototype, fold this into the per-view matrices UBO fix. Right after each view's camera Update(), call UniformConstants Update() (or a new UpdateForView(camera, viewport)) so that fogParams.xy come from that view's camera far plane and viewGeometry from that view's viewport. Leave the mouse fields and rndVec3 as-is, or write mouseStatus offscreen and mouseWorldPos.w=0 on secondary views. Keep a CPU-side shadow copy of UniformParamsBuffer. Compute the frame-constant parts (sun, map, time, team colours) once per frame, and re-patch only fogParams, viewGeometry and optionally the mouse fields per view before uploading, so there is no repeated GuiTraceRay or palette rewrite. Do NOT split the std140 layout into separate per-view and per-frame blocks for the prototype. ModelVertProgGL4.glsl:54, ModelFragProgGL4.glsl and ShadowGenVertProgGL4.glsl carry hard-coded copies of the struct layout at binding=1. Lua shaders also depend on GetGLSLDefinition, so a split is a backwards-compatibility break (see coding-agents/BACKWARDS_COMPATIBILITY.md) for later. The proper long-term split (frame / per-view / per-window-input, with the mouse block owned by the window that has input focus) is a full-product item.

> verifier A (corrected): The core code claims check out in the current tree. UpdateParamsImpl (UniformConstants.cpp:214-306) builds frame-constant, per-view and per-window-input fields in one pass. rndVec3 comes from guRNG (216). viewGeometry comes from globalRendering viewSize/viewPos (222). fogParams uses camPlayer->GetFarPlaneDist() (230-233). mouseScreenPos/mouseStatus come from the single global `mouse` (250-263). mouseWorldPos comes from a GuiTraceRay using camPlayer and mouse->lastx/lasty (265-293). All team and 

> verifier B (corrected): The code facts are accurate, but the 2-window 3D-only prototype does not fail or visibly corrupt because of them, so "risk" is overstated. Verified: UpdateParamsImpl (UniformConstants.cpp:214-306) takes viewGeometry from globalRendering (222), fogParams from camPlayer->GetFarPlaneDist() (230-233), mouse fields from the global mouse (250-263), runs one GuiTraceRay (279), advances guRNG (216) and rewrites 255+1792 colours (295-305). It runs once per UpdateUnsynced (Game.cpp:1441) and is bound once

### BUF-08 - RenderBuffer staging and instance buffers scale with view count
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** none

Per-frame CPU vertex lists, RenderBuffer upload volume and the S3DModelVAO instance VBO rewrites grow linearly with the number of views, and existing sizing/overflow behaviour is tuned for one.

*Why it breaks:* TypedRenderBuffer appends verts/indcs to per-frame std::vectors. DrawElements/DrawArrays only advance the start indices (RenderBuffers.h:866-920), and the vectors are cleared only in SwapBuffer()->Clear() (:391-417), reached from RenderBuffer::SwapRenderBuffers() inside CGlobalRendering::SwapBuffers (GlobalRendering.cpp:688), which runs once per present. If two views are drawn before a single swap, VA_TYPE_PROJ/C/T/TC accumulation roughly doubles against fixed initial capacities of 1<<20 verts and 1<<21 indices (RenderBuffers.cpp:14,16,19,21). UploadVBO/UploadEBO (RenderBuffers.h:802-853) then realloc once via the base IStreamBuffer::Resize (StreamBuffer.h:119-123): Kill(false), then Init(), whose CreateBuffer calls glBufferData(nullptr) (StreamBuffer.cpp:79-90). The capacity is kept afterwards, and there is no glFinish or fence on this SB_BUFFERSUBDATA path. A latent hazard is that this realloc discards GPU content already uploaded this frame (only [uploadIndex, size) is re-uploaded), so a mid-frame Resize would corrupt any range later re-drawn via DrawElementsRange, for example the ProjectileDrawer alpha-range reuse at ProjectileDrawer.cpp:949-985. That hazard exists with one view; more views raise the odds. S3DModelVAO::Submit does glBufferSubData of that bin's instances at offset 0 of one STREAM_DRAW VBO for every bin Submit (3DModelVAO.cpp:432-434; VBO.h:84 defaults elemOffset to 0), so the number of rewrites is passes x bins x views. Each is GL-ordered and small, a driver implicit-sync cost rather than a correctness problem. With a separate present per window, SwapRenderBuffers would run per window instead of once per frame. That swap-per-present coupling is a more important design point than capacity.

*Evidence:*
- `rts/Rendering/GL/RenderBuffers.cpp:12` initial capacities per typed buffer
- `rts/Rendering/GL/RenderBuffers.h:810` Resize on overflow in UploadVBO; default type SB_BUFFERSUBDATA at 795
- `rts/Rendering/Models/3DModelVAO.cpp:432` instVBO.SetBufferSubData(allRenderModelData) at offset 0 per Submit; overflow skip at 412
- `rts/Rendering/Models/3DModelVAO.hpp:45` INSTANCE_BUFFER_NUM_BATCHED/IMMEDIATE sizes
- `rts/Rendering/GlobalRendering.cpp:688` all render buffers swapped on present
- `rts/Rendering/GL/RenderBuffers.h:593` pending-range contract valid until the next SwapBuffer
- `rts/Rendering/GL/RenderBuffers.cpp:21` fixed initial capacity per frame

*Direction:* Keep as polish. Drop the ring-stall and instance-capacity claims. The first thing to decide is where SwapRenderBuffers/PutBufferLocks run when there are N windows (per frame versus per present), because that sets whether per-frame accumulation scales with views at all. After that, measure two views. Only if profiling shows a problem, raise the initial typed-buffer capacities (a one-time realloc) and/or rotate the instVBO write offset per Submit as the immediate region already does. Consider fixing the Resize data-loss hazard separately.

> verifier A (corrected): The core point holds: per-frame RenderBuffer CPU vectors and the per-Submit instance-VBO rewrites scale linearly with the number of views, and both are GL-ordered, so this is cost rather than correctness. Three parts of the stated mechanism are wrong. (1) The "glFinish + fence waits for ring types" stall does not apply. All typed render buffers are constructed with the default SB_BUFFERSUBDATA (RenderBuffers.h:795; RenderBuffers.cpp:13-27 use the 2-arg ctor). BufferSubDataImpl (StreamBuffer.h:19

### BUF-09 - UniformConstants update/bind mechanics have no per-view slice or ordering model
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** Engine UBO pair carries a single view's camera, shadow and reflection data

The UBOs are single-slice BufferSubData buffers; the BUFFERING=3 ring constant is unused and BindBufferRange cannot address a sub-slice, so N updates per frame can only overwrite the same GL buffer region.

*Why it breaks:* The UBO pair has no per-view slice model. UniformConstants::Init creates two single-element BufferSubDataImpl buffers (UniformConstants.cpp:131-138). Update() does Map, fill and glBufferSubData of the whole struct (UniformConstants.cpp:308-326, StreamBuffer.h:240-251). Bind() calls BindBufferRange, which always uses offset allocIdx*byteSize with allocIdx fixed at 0 (StreamBuffer.cpp:134, StreamBuffer.cpp:43), so there is no way to address a sub-slice. BUFFERING=3 at UniformConstants.h:124 is dead code. The per-frame flow is one Update at Game.cpp:1441, at the end of UpdateUnsynced, and one Bind at Game.cpp:1464. Both run once per frame, before GenerateIBLTextures and before any view-specific camera selection.

Within a single GL context, re-running Update then Bind before each view is correct. glBufferSubData is ordered after the already-issued draws. Update's Unmap does glBindBuffer(GL_UNIFORM_BUFFER, 0) (StreamBuffer.h:250), but that only clears the generic binding, not the indexed binding point, so Bind is idempotent and need not follow Update. Three corrections to the original claim:
- The UBO type is chosen explicitly at UniformConstants.cpp:132, so the autodetect branch at StreamBuffer.h:580 is never reached for these buffers.
- A per-view Update is not free of side effects. UpdateParamsImpl runs a GuiTraceRay, reads mouse->lastx/lasty, rebuilds the 33 KB team/palette colour table (UniformConstants.cpp:265-304) and draws a guRNG vector (UniformConstants.cpp:216). UpdateMatricesImpl also flips the global active camera to CAMTYPE_UWREFL and back (UniformConstants.cpp:185-196). Calling it twice per frame repeats all of that.
- Bind is idempotent only within one GL context. Indexed UBO binding points are per-context state. If each window gets its own GL context, Bind() must run in every context, and cross-context ordering of BufferSubData needs a flush or fence. rts/ has a single SDL_GL_CreateContext (GlobalRendering.cpp:474-515), so this is a design decision for the multi-window context model, not a current bug.

*Evidence:*
- `rts/Rendering/GL/StreamBuffer.h:240` BufferSubDataImpl::Unmap uploads the whole mapped range with glBufferSubData; SwapBuffer not overridden (no-op, base at line 109)
- `rts/Rendering/GL/StreamBuffer.cpp:134` BindBufferRange uses allocIdx*byteSize and byteSize: no element offset/size parameters
- `rts/Rendering/GL/StreamBuffer.h:580` UNIFORM_BUFFER target forced to SB_BUFFERSUBDATA on autodetect
- `rts/Rendering/UniformConstants.h:124` BUFFERING = 3 declared but never used
- `rts/Rendering/UniformConstants.cpp:128` numElems = 1, type SB_BUFFERSUBDATA

*Direction:* Prototype: move UniformConstants Update and Bind into a per-view hook that runs after that view's camera, viewport and shadow selection (see the dependsOn problem). Split Update into UpdateParams, once per frame (time, sun, fog colour, teamColor, map data), and UpdateViewMatrices plus the small per-view params (viewGeometry, fogParams, mouse*), once per view. This keeps the repeated upload near 1.7 KB and avoids repeating the trace ray and palette rebuild. Keep the single-slice BufferSubData buffers. Add the multi-slice (views x frames) layout with elemOffset/elemCount on BindBufferRange only if profiling shows stalls, or once a design needs several views' blocks live at once, for example Lua deferred or replay passes. If windows get separate GL contexts, add a per-context Bind and a flush or fence between views. Effort S for the split, M for slice support.

> verifier B (corrected): The facts are right but the severity is not. BUFFERING=3 is unused (UniformConstants.h:124, the only occurrence in Rendering/ outside StreamBuffer). The buffers are numElems=1 BufferSubDataImpl (UniformConstants.cpp:128-138). Unmap does a whole-struct glBufferSubData (StreamBuffer.h:240-251). BindBufferRange always binds [allocIdx*byteSize, byteSize) (StreamBuffer.cpp:131-135). The claim itself concedes that Update->Bind->draw(view1)->Update->draw(view2) is correct under GL ordering, and it is. 

> verifier A (corrected): The mechanism is real, but "risk" is overstated and I downgraded it to polish. I checked every cited line in the current tree and they hold. StreamBuffer.h:240-251 uploads the mapped range with glBufferSubData. BufferSubDataImpl does not override SwapBuffer, so the base no-op at StreamBuffer.h:109 applies. StreamBuffer.cpp:43 initialises allocIdx to 0 and StreamBuffer.cpp:134 always binds [0, byteSize). UniformConstants.h:124 BUFFERING has no other reference in rts/. UniformConstants.cpp:131-132

## Offscreen / screen-sized render targets

### FBO-01 - DepthBufferCopy is a single global depth texture and FBO sized from the primary viewport
**Severity:** risk - **Effort:** M - **Status:** confirmed - **Depends on:** DSN-17

The depth copy used by ground decals and soft particles is allocated once at the primary view size and refilled from the primary view rectangle every frame. It is always active because ProjectileDrawer holds a consumer.

*Why it breaks:* depthTextures[2] and depthFBOs[2] (non-MS and MS) are created at globalRendering->viewSize and recreated only on the ViewResize event. MakeDepthBufferCopy blits from the currently bound FBO using the global view rect, once per frame after terrain. Consumers sample the texture either normalised (ProjFXFragProg screenUV) or with texelFetch(gl_FragCoord). If target 2 has another size, the blit is clipped or leaves stale texels, so soft particles and decals are wrong. The MS variant is allocated with the global msaaLevel and needs equal sample counts to the target. With drawGround off for a target the copy is stale from the other target. Cost is about 8.3 MB per 1080p target, multiplied by msaaLevel for the MS copy.

*Evidence:*
- `rts/Rendering/DepthBufferCopy.cpp:55` only recreated through ViewResize
- `rts/Rendering/DepthBufferCopy.cpp:70` blit src/dst rects from globalRendering->viewPos/viewSize, source = current FBO (-1) (70-90)
- `rts/Rendering/DepthBufferCopy.cpp:136` texture created from supportDepthBufferBitDepth, viewSize and msaaLevel (136-140)
- `rts/Rendering/DepthBufferCopy.h:45` single instance with consumersCount/depthTextures/depthFBOs arrays (45-47)
- `rts/Rendering/WorldDrawer.cpp:343` one copy per world pass, after terrain
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:440` always holds a non-MS consumer, so the copy exists even in default config (reads at 967 and 1203)
- `cont/base/springcontent/shaders/GLSL/ProjFXFragProg.glsl:58` texture(depthTex, screenUV) normalised over the whole texture
- `cont/base/springcontent/shaders/GLSL/GroundDecalsFragProg.glsl:250` texelFetch(depthTex, ivec2(gl_FragCoord.xy)) assumes texel == fragment
- `rts/Rendering/DepthBufferCopy.cpp:72` source and destination rects from the global view size
- `rts/Rendering/DepthBufferCopy.cpp:138` texture allocated at the global view size
- `rts/Rendering/GL/FBO.cpp:350` Blit(-1,..) reads whichever FBO is currently bound
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:967` second consumer binds the same texture
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:966` depth copy bound for soften

*Direction:* Effort is S. A full TargetId -> {texture, fbo} map is more than the prototype needs, because the copy is transient within one target's pass.

Minimal fix: give MakeDepthBufferCopy an "ensure size" step.
- At the top of MakeDepthBufferCopy (or in a RenderTarget BeginTarget hook), compare the stored texture size and msaaLevel with the current target's viewSize.
- If they differ, run RecreateTextureAndFBO. Keep a small cache keyed by size, or hold one entry per target, so two differently sized windows do not reallocate roughly 8 MB twice per frame.
- Since ViewResize is only fired for the primary, this check removes any dependence on it.

Per-target (really per-GL-context) FBO ownership becomes mandatory if the second window gets its own GL context. Texture objects can be shared between contexts, but FBO container objects cannot (DepthBufferCopy.cpp:146-153 creates the FBO in the current context). In that case the texture can stay shared while the FBO is created per context. Decide this alongside the context-sharing design question.

Two things must change together or window 2 stays wrong even after the copy is fixed:
- GroundDecalHandler's screenSizeInverse uniform is set only in ReloadDecalShaders and ViewResize from the global view size (GroundDecalHandler.cpp:473, 1698). It must be set per target, because the decal shader reconstructs world position from gl_FragCoord * screenSizeInverse (GroundDecalsFragProg.glsl:255).
- ProjectileDrawer reads CAMTYPE_PLAYER for camPos and fog (ProjectileDrawer.cpp:970, 1206). That is the per-camera problem, not a depth-copy problem.

Prototype workaround if the fix is deferred: pin both windows to the same size, or force SoftParticles=0 and skip the decals pass for the secondary view.

> verifier A (confirmed): Every cited line checks out in the current tree. DepthBufferCopy.cpp:55-63 is the only resize path (ViewResize event) and recreates textures only for kinds with consumers. MakeDepthBufferCopy (70-90) builds its src and dst rects from globalRendering->viewPos*/viewSize*, and the source is the currently bound FBO (-1). FBO::Blit resolves -1 via glGetIntegerv(GL_FRAMEBUFFER_BINDING) at FBO.cpp:350-353. CGame::Draw unbinds the FBO right before CWorldDrawer::Draw (Game.cpp:1521-1526), so the source i

> verifier B (confirmed): The mechanism holds. Both the texture size and the blit rectangles come from globalRendering->viewSize, and nothing re-syncs them when the target changes.

Checked in code:
- CreateTextureAndFBO sizes the texture from globalRendering->viewSizeX/Y (DepthBufferCopy.cpp:138, MS variant :140).
- The only resize path is the ViewResize event (DepthBufferCopy.h:22-27, .cpp:55-63). That event is fired only from CGame::ResizeEvent (Game.cpp:1127), which handles the one primary window.
- MakeDepthBufferCo

### FBO-02 - View-size-keyed GPU resources are singletons sized from the primary view
**Severity:** risk - **Effort:** L - **Status:** corrected - **Depends on:** DSN-03; Context-sharing and default-framebuffer strategy undecided

Several GPU resources are sized from viewSizeX/Y at creation or resize and used for every draw: the deferred G-buffers, the depth-buffer copy, the water refraction/screen textures and the Refract/Dyn water sizes.

*Why it breaks:* These GPU resources are single instances sized from the global `viewSize` at creation or on `ViewResize`. They are re-filled and consumed inside each view's own draw, so they are correct for N sequential same-size views. They go wrong only when a target's size differs from the size baked in:
- DepthBufferCopy's texture is sized from the primary view (DepthBufferCopy.cpp:138). The blit clips (no GL error), and `ProjFXVertProg.glsl:64` samples that texture with full-range NDC UVs, so window 2's soft-particle depth lookups are misaligned.
- The decal `screenSizeInverse` uniform is window-1-sized (GroundDecalHandler.cpp:473/1698).
- BumpWater and RefractWater `glCopyTexSubImage2D` into textures sized at construction. A larger window 2 raises GL_INVALID_VALUE, and BumpWater's baked `ScreenInverse` and `ViewPos` defines (BumpWater.cpp:425-427) are wrong.
- G-buffers (opt-in) are created for the primary size by `GeometryBuffer::Update`, which `WorldDrawer::Update` calls once per frame (WorldDrawer.cpp:214). A larger window gets cropped output; a smaller one gets the wrong normalised composite UVs.

*Evidence:*
- `rts/Rendering/GL/GeometryBuffer.cpp:208` G-buffer size = globalRendering view size
- `rts/Rendering/LuaObjectDrawer.cpp:283` geomBuffer->Update once per frame
- `rts/Rendering/DepthBufferCopy.cpp:72` blit rects from viewPos/viewSize; texture sized at 136-140
- `rts/Rendering/Env/BumpWater.cpp:181` screenTextureX/Y fixed at construction; defines at 425-442; glCopyTexSubImage2D at 906/912
- `rts/Rendering/Env/BumpWater.cpp:906` glCopyTexSubImage2D with current viewSize (also :912)
- `rts/Rendering/Env/RefractWater.cpp:48` refraction texture at viewSize; copy at 65
- `rts/Rendering/Env/DynWater.cpp:55` refractSize from viewSizeY
- `rts/Rendering/Env/BumpWater.cpp:425` ScreenInverse baked into shader defines (also :442)
- `rts/Rendering/DepthBufferCopy.cpp:122` texture alloc at viewSize (to 140); rects :72-73; ViewResize :55-63
- `rts/Map/SMF/SMFGroundDrawer.h:104` single geomBuffer
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:474` 1/viewSize shader constants (also :1699)
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:473` screenSizeInverse uniform set only at init and ViewResize (1698)
- `rts/Map/SMF/SMFGroundDrawer.cpp:576` UpdateGeometryBuffer sizes from the global view

*Direction:* Keep single instances and treat them as per-view scratch, with a "begin target" hook that runs when the active target changes (this is not a full per-target resource set):

1. **Prototype policy.** Create window 2 at the same size as window 1, with resizing locked or applied to both. If you need different sizes, disable these in the secondary view: soft particles, deferred rendering, and water modes 2/3/4. This costs S.

2. **Lazy size check at target switch.**
   - Make `DepthBufferCopy::MakeDepthBufferCopy` (or a new `EnsureSize`) call `RecreateTextureAndFBO` when the texture size differs from the current view size.
   - Call `GeometryBuffer::Update(false)` and `UpdateGeometryBuffer` per target. This is free for equal sizes (GeometryBuffer.cpp:199), and only alternating different sizes would thrash.
   - Re-push the decal `screenSizeInverse` uniform in the begin-target hook instead of only in `ViewResize`.
   - Add an assert or one-shot log when a target's size differs from a resource's size, so mismatches are not silent.

3. **Later, for independently sized targets.**
   - Allocate scratch at the maximum target size and use sub-rect viewports.
   - Add a per-view UV scale uniform for `ProjFXVertProg.glsl:64`, because soft particles sample with whole-texture NDC UVs. Decals can stay as they are, because they use `texelFetch(gl_FragCoord)`.
   - Convert BumpWater's `ScreenInverse`, `ViewPos` and `BlurBase` defines (BumpWater.cpp:425-442) into uniforms. Without that, a per-target shader recompile like `ResizeEvent`'s kill-and-recreate of water (Game.cpp:1115) is needed.
   - Size the water screen-copy and depth textures to the maximum target size.

Effort: S for the prototype policy plus the lazy-size hook, M for the max-size/uniform conversion. A per-RenderTarget set of every resource is not justified.

> verifier A (corrected): The core mechanism is real in the current tree. Several GPU resources are sized once from globalRendering->viewSize* and reused for every draw, and a second window of a different size would crop, mis-normalise or (for BumpWater) raise GL errors. I downgrade blocker to risk and trim the claim in four ways. (1) The claim's own direction, same-size windows, removes the failure, and with equal sizes nothing here breaks. (2) The default config exercises only the DepthBufferCopy, decal and soft-partic

> verifier B (corrected): The mechanism is real, but "blocker" is overstated. It only bites when the two windows differ in size, and the claim's own direction ("force both windows to the same size to defer this") shows it is not needed for a first prototype.

1. **Everything cited is per-view scratch.** Each resource is refilled inside the same view's draw before it is consumed:
   - DepthBufferCopy: `MakeDepthBufferCopy` runs inside `DrawOpaqueObjects` (WorldDrawer.cpp:343), right before decals and particles read it.
  

### FBO-03 - [arch B] Per-context FBO duplication and global FBO bookkeeping
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** DSN-02

FBOs are per-context, created eagerly in FBO's constructor and tracked in a static list. The persistent FBOs split into once-per-frame update FBOs that can stay primary-only and per-target draw-phase FBOs that need one copy per context. Not needed under A or C.

*Why it breaks:* Keep the core claim and mark it conditional on arch B. In a shared-context design the textures and renderbuffers are shared but every FBO id is context-local, so any FBO created in the primary context and bound in a secondary one is either an empty incomplete FBO (INVALID_FRAMEBUFFER_OPERATION, nothing drawn) or aliases that context's own FBO of the same name. The default prototype (shadows on, decals on) breaks on DepthBufferCopy: MakeDepthBufferCopy (DepthBufferCopy.cpp:83-89) is called inside the per-target draw at WorldDrawer.cpp:343. It blits into depthFBOs[] created via CreateTextureAndFBO (DepthBufferCopy.cpp:146-153) in the primary context. Window 2 would blit into an invalid or aliased FBO, so decals and projectiles that sample depthBufferCopy->GetDepthBufferTexture read stale or empty depth. On maps with visible water, BumpWater reflectFBO/refractFBO (BumpWater.cpp:380-396) hit the same problem, driven from UpdateWater (BumpWater.cpp:561-575, called from WorldDrawer.cpp:437). Corrections to the claim: (1) The shadow map FBO is not a camera-independent update-phase FBO. CShadowHandler::Update (ShadowHandler.cpp:128-135) calls SetShadowMatrix(playCam, shadCam), which fits the light matrix, ortho scales and projMidPos to the single player camera (ShadowHandler.cpp:493-533, 675 ff.). With two independent cameras the shadow map must be per target (or per camera), not primary-only. Under B it needs one copy of the FBO and the map per target, or a union-fit design. (2) The reflection cubemap is camera-positioned: CubeMapHandler::CreateReflectionFace does curCam->SetPos(prvCam->GetPos()) (CubeMapHandler.cpp:218) and renders one face per call (UpdateReflectionTexture, 135-175, with currReflectionFace state). It is approximate and low severity, but it is per-camera state rather than purely global. (3) BumpWater coastFBO and dynWavesFBO are updated in CBumpWater::Update (BumpWater.cpp:542-558), not per target, so only reflect/refract are draw-phase. (4) FBO::GLContextLost walks activeFBOs but skips everything except FBOs with reloadOnAltTab. Only three set it: ProjectileDrawer.cpp:382, BumpWater.cpp:300 and BumpWater.cpp:399. It is only called for fullscreen hide/show (SpringApp.cpp:1172-1195). GLContextReinit iterates fboTexData (texture and renderbuffer ids, shared across shared contexts), not FBOs. maxAttachments and maxSamples are device limits. So the global bookkeeping problem is real but small; only activeFBOs is context-bound. (5) The raw FBO count is undercounted. Lua FBOs (LuaFBOs.cpp:502 glGenFramebuffersEXT) and RmlUi (RmlUi_Renderer_GL3_Recoil.cpp:553) are also raw and bypass activeFBOs. Lua FBOs created in one context's DrawWorld callin and used in another are a separate hazard. (6) Two evidence items do not support the claim. The ShadowHandler.cpp:629 comment is about render-target (FBO) switches, not GL context switches. The VAO entries (ModernSky.h:29, SkyBox.h:40, DebugCubeMapTexture.h:27) are a distinct non-shared container-object problem (VAOs), not FBOs.

*Evidence:*
- `rts/Rendering/GL/FBO.cpp:238` Init: glGenFramebuffersEXT + bind, push to activeFBOs (to 259)
- `rts/Rendering/GL/FBO.h:31` constructors call Init eagerly
- `rts/Rendering/GL/FBO.cpp:167` GLContextLost/Reinit loop over all FBOs in the current context (to 231)
- `rts/System/SpringApp.cpp:1172` hide/show events call FBO::GLContextReinit/Lost for fullscreen (to 1195)
- `rts/Rendering/WorldDrawer.cpp:437` water->UpdateWater runs reflect/refract FBO passes inside the per-target draw
- `rts/Rendering/WorldDrawer.cpp:249` GenerateIBLTextures: once-per-frame FBO passes (shadows, cubemaps, sky)
- `rts/Rendering/Env/BumpWater.cpp:380` reflect/refract FBO creation (to 396)
- `rts/Rendering/Env/DynWater.cpp:239` raw glGenFramebuffersEXT
- `rts/Rendering/GL/myGL.cpp:464` transient FBO pair per glSpringBlitImages call
- `rts/Rendering/GlobalRendering.cpp:474` SDL_GL_CreateContext(sdlWindow), one context
- `rts/Rendering/GlobalRendering.cpp:612` MakeCurrentContext always uses (sdlWindow, glContext)
- `rts/System/LoadLock.h:21` load lock re-binds the context to the main window, and clears it at line 32
- `rts/Rendering/GL/FBO.h:200` static activeFBOs / fboTexData / maxAttachments registries (200-204) assume one context
- `rts/Rendering/Env/BumpWater.h:90` reflectFBO, refractFBO, coastFBO, dynWavesFBO
- `rts/Rendering/ShadowHandler.cpp:629` comment: context switches are one of the slowest GL operations
- `rts/Rendering/GlobalRendering.cpp:346` single glContext member
- `rts/Rendering/GlobalRendering.cpp:613` SDL_GL_MakeCurrent(sdlWindow, glContext)
- `rts/Rendering/ShadowHandler.h:144` FBO smOpaqFBO member; Init at ShadowHandler.cpp:333
- `rts/Rendering/Env/CubeMapHandler.cpp:102` reflectionCubeFBO.Init(false)
- `rts/Rendering/Env/AdvWater.cpp:96` reflectFBO/bumpFBO created and attached
- `rts/Rendering/Env/DynWater.cpp:571` raw frameBuffer FBO handle bound in DrawWaves
- `rts/Map/SMF/SMFReadMap.cpp:430` shadingFBO created
- `rts/Rendering/Env/ModernSky.h:29` VAO vao used in Draw; SkyBox.cpp:256 skyVAO; DebugCubeMapTexture.cpp:12 vao

*Direction:* Keep the arch-B-only, blocker-if-B framing. Reclassify FBO owners as: (a) truly shared update-phase FBOs that can stay in the primary context: ProjectileDrawer perlinFB, ModernInfoTexture, SMF shadingFBO, BumpWater coastFBO and dynWavesFBO (the first two are called from WorldDrawer::Update). (b) Per-camera or per-target draw-phase FBOs needing one instance per context: DepthBufferCopy, BumpWater reflect/refract, AdvWater and DynWater, GeometryBuffer, Lua FBOs, and the shadow map, because it is fitted to the player camera and so needs one per target or a union-fit design. The reflection cubemap needs one per camera or an accepted approximation. Scope the FBO::GLContextLost/activeFBOs rework to the three reloadOnAltTab FBOs. Treat VAOs as a separate, parallel problem under B.

> verifier B (corrected): The mechanism is real but only exists if the N-shared-contexts architecture (B) is chosen. It is not a blocker for the 2-window 3D-only prototype, because the claim itself says it is "not needed under A or C" and A is the simplest viable path. Verified facts: there is exactly one SDL_GL_CreateContext path, on sdlWindow (GlobalRendering.cpp:474, 490, 515), with glContext assigned once at 587 and MakeCurrentContext hardwired to (sdlWindow, glContext) at 613. No SDL_GL_SHARE_WITH_CURRENT_CONTEXT is

> verifier A (corrected): The core mechanism holds, but only under architecture B, and four details are wrong or overstated. Confirmed: FBO::Init calls glGenFramebuffersEXT eagerly and pushes to activeFBOs (FBO.cpp:250-256; FBO.h:31). There is one SDL_GL_CreateContext site (GlobalRendering.cpp:474/490/515) and no SDL_GL_SHARE_WITH_CURRENT_CONTEXT anywhere in rts/. glContext is a single member assigned once (GlobalRendering.cpp:587), and MakeCurrentContext always uses (sdlWindow, glContext) (612-613). The persistent FBO m

### FBO-04 - Default-framebuffer coupling: FBO::Unbind() means the screen and readbacks target the window back buffer
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** DSN-02; Context-sharing and default-framebuffer strategy undecided

About 20 call sites restore 'the screen' via FBO::Unbind() (bind 0). Two effects read the current default framebuffer back. This is correct for A, wrong for C, and fragile for hidden or occluded windows.

*Why it breaks:* The 'bind 0 means the screen' convention (20 FBO::Unbind() sites, 7 raw bind-0 calls in DynWater, and glBindFramebuffer(GL_READ_FRAMEBUFFER, 0) at GlobalRendering.cpp:692) is valid when one GL context is current on window i, because FBO 0 is then that window's drawable. It is only wrong if world drawing is redirected into a per-target offscreen FBO and presented later (design C); every restore-to-0 site would then bind the window instead of the target. For the 2-window prototype under A this is a non-issue. The real consequences are narrower. (a) Default-framebuffer readbacks (BumpWater refraction and depth copy, DepthBufferCopy blit, RefractWater.cpp:65, Lua gl.CopyToTexture/ReadPixels, Screenshot, AVI capture) read whichever drawable is current, so they need the target's window made current and drawn first. The occluded-pixel-ownership garbage is a theoretical, driver-dependent hazard that equally affects today's single covered window. (b) Hidden and minimized windows must be skipped per window. Today only the process-global globalRendering->active (set at SpringApp.cpp:1165/1185, tested at Game.cpp:1471) skips drawing, and minimizing either window would silence both. (c) The readbacks size their rects and textures from process-global viewSize/viewPos: the depth copy rect at DepthBufferCopy.cpp:72-73 and its texture at :138, and the BumpWater screen textures and baked ScreenTextureSizeInverse shader define at BumpWater.cpp:181-182, 332, 353, 426. That coupling, not the binding, is what actually breaks a second window of a different size. It belongs in the viewport-size or per-target-resource problem, not here.

*Evidence:*
- `rts/Rendering/GL/FBO.cpp:322` Unbind always glBindFramebuffer(0); comment at 334 'continue with screen FBO'
- `rts/Game/Game.cpp:1522` FBO::Unbind + camera->LoadViewport before WorldDrawer::Draw (to 1524)
- `rts/Rendering/Env/BumpWater.cpp:407` FBO::Unbind after pre-pass setup (also :571)
- `rts/Rendering/Env/BumpWater.cpp:906` readback from default FB
- `rts/Rendering/DepthBufferCopy.cpp:84` FBO::Blit(-1, ...) reads the current FBO
- `rts/Rendering/GL/FBO.cpp:350` fromID<0 resolves to current FBO
- `rts/Game/Game.cpp:1523` frame-level restore to 0
- `rts/Rendering/Env/BumpWater.cpp:571` mid-frame restore to 0
- `rts/Rendering/Env/DynWater.cpp:608` raw bind 0; repeated at 644, 688, 761, 1010, 1130, 1214
- `rts/Lua/LuaOpenGL.cpp:4613` RenderToTexture restores 0 outside shadow mode
- `rts/Rendering/GL/myGL.cpp:535` temp-FBO blit restores only nonzero bindings

*Direction:* Keep this as a design-conditional item. If A (one context re-made-current per window) is chosen, no FBO::Unbind() rewrite is needed for the prototype. Call SDL_GL_MakeCurrent(window i) before the per-window draw, skip hidden or minimized windows per window, and fix the viewSize-keyed readback sizing in the size-coupling problem. Introduce RenderTarget::Bind() (restoring the target FBO id) only if design C is chosen. At that point all 20 Unbind sites, the DynWater raw binds, LuaOpenGL.cpp:4631 and the Blit(-1) paths must be converted together. Reclassify as a blocker only in the C branch. Track per-window visibility and active state, and screenshot, Lua and video readbacks per target, as separate full-severity items.

> verifier B (corrected): The code facts are accurate, but the severity is overstated for the 2-window 3D-only prototype. The claim concedes the behaviour is correct under design A (each window renders into its own default framebuffer). I confirmed that. FBO::Unbind() is a bare glBindFramebufferEXT(0) at FBO.cpp:335. Game.cpp:1522-1524 does Unbind plus LoadViewport before WorldDrawer::Draw. The other bind-0 sites are real, from the BumpWater, AdvWater, DynWater, SkyBox, GrassDrawer, info-texture and TextureRenderAtlas ca

> verifier A (corrected): The code facts check out. FBO::Unbind() is a bare glBindFramebuffer(0) (FBO.cpp:322-336). There are 20 static FBO::Unbind() call sites in rts/ (Game.cpp:1523 plus info-texture, sky, grass, water and atlas code). DynWater adds 7 raw bind-0 calls (608, 644, 688, 761, 1010, 1130, 1214). BumpWater::Draw copies the default framebuffer with glCopyTexSubImage2D at 906/912, and refraction=1 and depthCopy=true are the defaults (BumpWater.cpp:46,48). DepthBufferCopy blits from -1, the current FBO (DepthBu

### FBO-05 - Lua offscreen helpers restore framebuffer 0 as 'the screen'
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** none

gl.RenderToTexture, gl.RawBindFBO(nil) and gl.BlitFBO(nil,...) hard-code FBO 0 as the default target. If a view's pass runs in a non-zero FBO, Lua restores the wrong framebuffer mid-view.

*Why it breaks:* RenderToTexture restores framebuffer 0, or the current binding only in the DRAW_WORLD_SHADOW mode (LuaOpenGL.cpp 4613-4631). RawBindFBO(nil) binds the raw id, default 0 (LuaFBOs.cpp 698-701). BlitFBO with a nil FBO uses 0 for read or draw (LuaFBOs.cpp 797-798). Each treats 0 as 'the screen'. That is wrong whenever a view's target is a non-zero FBO. Candidates are an offscreen per-view FBO, a VR eye buffer, or Lua draw callins invoked during a view whose target is such an FBO. If each window renders to its own window's default framebuffer, FBO 0 is correct. This only becomes a real problem once Lua draw callins run per view, or the design moves to offscreen per-view FBOs, which the VR requirement makes likely.

*Evidence:*
- `rts/Lua/LuaOpenGL.cpp:4613` currentFBO queried only for DRAW_WORLD_SHADOW (4613-4616); restore at 4631 yields 0 otherwise
- `rts/Lua/LuaFBOs.cpp:698` RawBindFBO(nil) binds default/raw id 0 (698-701)
- `rts/Lua/LuaFBOs.cpp:771` BlitFBO nil src/dst => framebuffer 0 (771-798)
- `rts/Lua/LuaFBOs.cpp:656` ActiveFBO saves and restores the actual bound FBO; the safe pattern to copy
- `rts/Rendering/Env/IWater.cpp:174` DrawWorldReflection fires with the water reflection FBO bound

*Direction:* Do not treat this as a prototype blocker. Decide first whether views draw to the window default FB or to an offscreen FBO. If offscreen, have RenderToTexture always query GL_FRAMEBUFFER_BINDING. Have RawBindFBO(nil) and BlitFBO(nil) resolve to a view-scope 'current target FBO' id instead of 0. Fix this when Lua callins are made per-view.

> verifier B (corrected): The mechanism is real and I confirmed it in the code. RenderToTexture (LuaOpenGL.cpp 4613-4631) queries GL_FRAMEBUFFER_BINDING only when drawMode == DRAW_WORLD_SHADOW and otherwise restores FBO 0. RawBindFBO(nil) (LuaFBOs.cpp 698-701) binds 0 by default. It does not matter for the 2-window, 3D-only prototype. First, each window would render into its own default framebuffer, which is FBO 0 in that window's GL context, so the hard-coded 0 is correct there. A per-view offscreen FBO is a design choi

> verifier A (corrected): The code matches the claim, but it only breaks the prototype if views render into a non-zero FBO. In RenderToTexture (LuaOpenGL.cpp 4613-4631), currentFBO is read only for DRAW_WORLD_SHADOW and otherwise stays 0, so it rebinds FBO 0. RawBindFBO(nil) binds luaL_optinteger(L,3,0) (LuaFBOs.cpp 698-701). BlitFBO with a nil FBO binds 0 (LuaFBOs.cpp 797-798). ActiveFBO saves and restores the real binding (LuaFBOs.cpp 656-662). A prototype where each window draws straight to its own default framebuffer

### FBO-06 - Map and model G-buffers are global and resized by polling the primary view size
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** DSN-17; DepthBufferCopy is a single texture sized to the primary view; RenderView definition: per-target parameters consumed by model drawers

SMFGroundDrawer and LuaObjectDrawer each own one GeometryBuffer (about 50-58 MB at 1080p), sized from globalRendering->viewSize and exposed to Lua through global accessors.

*Why it breaks:* GeometryBuffer (GeometryBuffer.h:10-74) is sized by GetWantedSize() = globalRendering->viewSize (GeometryBuffer.cpp:208-211). Update() reallocates by polling prev/curr size (179-206) from three places: WorldDrawer::Update once per frame (LuaObjectDrawer::Update at WorldDrawer.cpp:214, ground Update at :219), the CSMFGroundDrawer ctor (SMFGroundDrawer.cpp:135-137) and the Lua setter CSMFGroundDrawer::SetDrawDeferredPass (SMFGroundDrawer.h:53-57). The deferred passes call the static GeometryBuffer::LoadViewport (viewSize), bind, draw, UnBind (FBO::Unbind, FBO 0) and reset to globalRendering->LoadViewport (SMFGroundDrawer.cpp:252-296, LuaObjectDrawer.cpp:495-536). The ground composite (SMFGroundDrawer.cpp:366-389) samples the whole G-buffer via texCoords = gl_Vertex.xy*0.5+0.5 (SMFCompositeVertProg.glsl:6). A window whose size differs from the primary-sized buffer therefore gets a clipped viewport (larger) or a stretched lower-left sub-image (smaller). Lua reaches the buffers via groundDrawer->GetGeometryBuffer() (instance member, SMFGroundDrawer.cpp:70) and ModelDrawer::GetGeometryBuffer() (static, ModelDrawer.h:47). Lua's gl.TextureInfo-style size query reports GetWantedSize (LuaOpenGLUtils.cpp:975,987), not the allocated size. The deferred on/off flags are process-global statics (LuaObjectDrawer.cpp:47-49, ModelDrawer.h:93,175), so "disable deferred only on window 2" needs a per-target override. All of it is gated by the opt-in configs AllowDeferredMapRendering and AllowDeferredModelRendering, default false (SMFGroundDrawer.cpp:47,571-577; LuaObjectDrawer.cpp:30,249,278), so the buffers do not exist in the default configuration.

*Evidence:*
- `rts/Rendering/GL/GeometryBuffer.cpp:208` G-buffer size comes from globalRendering viewSize; LoadViewport at 213
- `rts/Rendering/LuaObjectDrawer.cpp:283` geomBuffer->Update in LuaObjectDrawer::Update (274-296), created with new at 255
- `rts/Rendering/WorldDrawer.cpp:214` update phase runs once per frame before drawing (LuaObjectDrawer::Update 214, ground Update 219)
- `rts/Map/SMF/SMFGroundDrawer.cpp:252` hard viewport set before the deferred pass
- `cont/base/springcontent/shaders/GLSL/SMFCompositeFragProg.glsl:11` texture(depthTex/colorTex, texCoords) over the full buffer (vertex stage line 6 builds texCoords)
- `rts/Lua/LuaOpenGLUtils.cpp:610` $map_gbuffer_* / $model_gbuffer_* resolve to global GeometryBuffer textures (610-622)
- `rts/Lua/LuaUnsyncedCtrl.cpp:4547` Lua can enable deferred drawing at runtime (4565 for models)
- `rts/Rendering/GL/GeometryBuffer.cpp:144` MS textures use globalRendering->msaaLevel (151)
- `rts/Rendering/GL/GeometryBuffer.cpp:213` static LoadViewport() uses viewSize
- `rts/Rendering/GL/GeometryBuffer.h:49` Bind/UnBind; UnBind() = FBO::Unbind() (FBO 0)
- `rts/Map/SMF/SMFGroundDrawer.cpp:296` viewport reset to globalRendering after the pass
- `rts/Map/SMF/SMFGroundDrawer.cpp:366` composite = fixed full-screen quad of G-buffer colour/depth
- `rts/Map/SMF/SMFGroundDrawer.cpp:547` G-buffer resize only in the once-per-frame Update
- `rts/Map/SMF/SMFGroundDrawer.cpp:47` defaults off: AllowDeferredMapRendering / AllowCombinedMapRendering
- `rts/Rendering/LuaObjectDrawer.cpp:479` DrawDeferredPass: LoadViewport, Bind, UnBind, globalRendering->LoadViewport at 536
- `rts/Rendering/LuaObjectDrawer.cpp:274` single geomBuffer updated once per frame
- `rts/Rendering/LuaObjectDrawer.cpp:30` AllowDeferredModelRendering defaults to false

*Direction:* Prototype: make deferred and combined map and model passes a per-target gate, not a per-target buffer. Set a flag in BeginTarget so the secondary target takes the forward path. That means drawDeferred is effectively false and combinedPass is false at SMFGroundDrawer.cpp:407-416, and the ModelDrawer.h:328 call to LuaObjectDrawer::DrawDeferredPass is skipped. Do not assert, because Lua can flip drawDeferred at any time via SetDrawGroundDeferred and SetDrawModelsDeferred. Known caveat: if Lua disabled the forward pass (SetDrawForwardPass(false)), the secondary shows no ground or models, so also force forward on for that target. Full product: the passes run sequentially, so one shared G-buffer per drawer is enough. Per-target bundles would double the roughly 50 MB (map) plus roughly 50 MB (models) at 1080p for no benefit, since nothing reads a G-buffer across targets. Give GeometryBuffer an explicit size or viewport taken from the current render view, instead of the global viewSize in GetWantedSize and the static LoadViewport. Resize only when the target size changes (call Update from BeginTarget, not once per frame in WorldDrawer::Update), and accept that realloc thrash occurs if two targets of different sizes alternate. Mitigations: allocate to the max target size and scale composite texcoords by the viewport ratio, or keep one buffer per distinct size. Resolve $map_gbuffer_* and $model_gbuffer_* to "the buffer of the target currently being drawn". If the design uses separate GL contexts, note that FBOs (GeometryBuffer::buffer) are not shared across contexts, so this buffer must be recreated per context; that is the generic per-context FBO problem, not G-buffer specific. Effort: S for the gate, M for properly size-aware shared buffers plus Lua semantics.

> verifier A (corrected): The core mechanism holds in the current tree. Both G-buffers are single global instances sized from globalRendering->viewSize (GeometryBuffer.cpp:208-211). The deferred passes use GeometryBuffer::LoadViewport (GeometryBuffer.cpp:213-217; SMFGroundDrawer.cpp:252, LuaObjectDrawer.cpp:495), draw into the global buffer, UnBind to FBO 0 and reset the viewport (SMFGroundDrawer.cpp:296, LuaObjectDrawer.cpp:536). The composite samples the whole buffer with a full-screen quad (SMFCompositeVertProg.glsl:6

> verifier B (corrected): The mechanism is real and I confirmed each piece in code, but it does not meet the "risk" bar for the 2-window, 3D-only prototype. Confirmed: GeometryBuffer::GetWantedSize reads globalRendering->viewSizeX/Y (GeometryBuffer.cpp:208-211). Update only reallocates when the polled size changes (179-206). It is called once per frame from CWorldDrawer::Update (WorldDrawer.cpp:214 via LuaObjectDrawer::Update at LuaObjectDrawer.cpp:274-296, and :219 via CSMFGroundDrawer::Update at SMFGroundDrawer.cpp:537

### FBO-07 - Screenshot, AVI and Lua read-back read the current drawable using window-1 geometry
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** FBO::Unbind and raw glBindFramebuffer(0) encode 'restore the screen' with no current-target indirection; Decision: render straight into each window's default framebuffer or into offscreen per-target FBOs; Draw path has no split between once-per-frame work and per-target work; DSN-15

All screen read-back uses glReadPixels or glCopyTexSubImage2D on whatever drawable is current, sized from globalRendering->winSize or the caller's coordinates.

*Why it breaks:* What is confirmed in the current tree:
1. TakeScreenshot (Screenshot.cpp:47-58) sizes from globalRendering->winSizeX/Y. It pads x up to a multiple of 4 at line 49, so it can read wider than the window. It then calls glReadPixels(0,0,...) with no target parameter. The caller, ScreenShotActionExecutor (UnsyncedGameCommands.cpp:2496-2505), takes no target either.
2. The screenshot really does run between frames. input.PushEvents() is at the top of the main loop (SpringApp.cpp:931). It calls SDL_PollEvent and then PushEvent (InputHandler.cpp:26-29). SDL_KEYDOWN calls activeController->KeyPressed synchronously (SpringApp.cpp:1275-1286), and f11/f12 are bound to "screenshot" (KeyBindings.cpp:219-220). So it executes after the previous frame's SwapBuffers (SpringApp.cpp:906), and SwapBuffers has just done glBindFramebuffer(GL_READ_FRAMEBUFFER,0) (GlobalRendering.cpp:692).
3. In the 2-window case the read gets the wrong surface and size. The context is current on whichever window was swapped or made current last. That window's back buffer is read using window 1's winSizeX/Y.
4. AVI reads via readOpenglPixelDataThreaded (AVIGenerator.cpp:402) from the current read framebuffer. That runs mid-frame, before swap, from Game.cpp:1553-1557 via AviVideoCapturing::RenderFrame (AviVideoCapturing.cpp:87-97).
5. Lua gl.ReadPixels (LuaOpenGL.cpp:6559) and gl.SaveImage (6684) read the bound framebuffer with caller-supplied raw coordinates. gl.CopyToTexture (4455) is gated by CheckDrawingEnabled.

Corrections:
(a) "Back buffer content is undefined after swap" already applies to the single-window engine, because the key-triggered screenshot is already taken after swap. It is not multi-window specific. The multi-window-specific faults are which drawable is current and the use of window-1 geometry. Multi-target capture should probably read from a persistent per-target colour surface (an offscreen FBO or a pre-swap read) rather than the post-swap back buffer.
(b) The AVI size is fixed at StartCapturing from globalRendering->viewSizeX/Y rounded down to a multiple of 4 (AviVideoCapturing.cpp:62-65), not at AVIGenerator.cpp:402. The whole AVI path is Windows-only (it needs an HWND and AVI_CAPTURING). The SDL_GetWindowWMInfo(GetWindow()) at AVIGenerator.cpp:343 is also a single-window assumption: it only minimises the fullscreen window.
(c) The ShadowHandler.cpp:139 citation is a mis-cite. SaveShadowMapTextures (called only from UnsyncedGameCommands.cpp:372) dumps the shadow depth and colour textures through glSaveTexture, not a drawable. It would collide on drawFrame only if shadows were generated once per target. That belongs to the shadow-ownership problem, not this one.
(d) The GlobalRendering.cpp:692 citation is incidental. It resets the read FBO to 0 before every swap, which merely sets the stage for the post-swap screenshot read.

*Evidence:*
- `rts/Rendering/Screenshot.cpp:47` uses globalRendering->winSizeX/Y and current framebuffer
- `rts/Rendering/AVIGenerator.cpp:402` glReadPixels of the framebuffer for video
- `rts/Lua/LuaOpenGL.cpp:6684` gl.SaveImage glReadPixels from current read buffer (6559 for ReadPixels)
- `rts/Game/Game.cpp:1553` video capture hook once per Draw
- `rts/Game/UnsyncedGameCommands.cpp:2500` screenshot action triggered outside a draw
- `rts/Rendering/AVIGenerator.cpp:343` SDL_GetWindowWMInfo(globalRendering->GetWindow())
- `rts/Rendering/GlobalRendering.cpp:692` glBindFramebuffer(READ,0) before swap for external capture
- `rts/Rendering/Screenshot.cpp:58` glReadPixels from the current read buffer
- `rts/Game/UnsyncedGameCommands.cpp:2496` ScreenShot action calls TakeScreenshot directly with no target
- `rts/System/SpringApp.cpp:931` input.PushEvents() runs before Update/Draw/Swap in the main loop
- `rts/Rendering/ShadowHandler.cpp:139` debug dump filenames keyed on drawFrame only

*Direction:* Give TakeScreenshot, the video capturer and the Lua read-back calls an explicit target id. Resolve each one to that target's read surface and geometry (its own FBO or default framebuffer plus its size) before reading. Either capture before the target's swap, or read from a persistent offscreen colour attachment so post-swap undefined contents do not matter. Size the AVI, and the screenshot's 4-pixel padding, from that target. Default to the primary target when no id is given, and decide whether Lua read-back outside a draw callin should be allowed at all.

> verifier A (corrected): The core mechanism holds in the current tree. Every screen read-back (TakeScreenshot, AVI, gl.ReadPixels, gl.SaveImage, gl.CopyToTexture) reads whatever drawable or framebuffer is current, with single-window geometry. Nothing in the tree handles a target (a grep for RenderTarget and similar found nothing). It does not block the 2-window 3D-only prototype, so full is right. Three parts of the stated mechanism and evidence are off, listed in refinedMechanism and extraEvidence.

### FBO-08 - Lua multisample sample counts pinned to one global msaaLevel
**Severity:** polish - **Effort:** S - **Status:** unverified - **Depends on:** none

Lua multisampled textures and RBOs always use globalRendering->msaaLevel so that blits from the default framebuffer work. A second window with a different sample count breaks those blits.

*Why it breaks:* CreateTexture ignores the requested sample count and uses msaaLevel ('has to equal the default FB or blitting breaks'). LuaRBOs does the same. BlitFBO with a nil source reads the default framebuffer. If windows are created with different MSAA settings, resolve blits between them and Lua MS targets raise GL_INVALID_OPERATION. LuaTextures also checks the single supportMSAAFrameBuffer capability.

*Evidence:*
- `rts/Lua/LuaOpenGL.cpp:4270` samples forced to globalRendering->msaaLevel (4269-4273)
- `rts/Lua/LuaRBOs.cpp:194` RBO samples = msaaLevel (194-197)
- `rts/Lua/LuaTextures.cpp:79` supportMSAAFrameBuffer capability check for GL_TEXTURE_2D_MULTISAMPLE
- `rts/Lua/LuaFBOs.cpp:797` BlitFBO from/to default framebuffer

*Direction:* Require all render targets to share the same pixel format and sample count in v1 (document and assert). Later, make msaaLevel a property of the current view target.

## Shadows, reflections, water & sky

### ENV-01 - Camera-dependent pre-passes (shadow map, env cubemap, water reflection) are single instances tied to the player camera
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** Per-frame draw pipeline has no per-target loop and restores state by hard-coded main-window calls; Per-drawFrame uniform buffer carries one camera and one viewport and is filled in the Update phase; No per-view identity to key per-frame caches, flags and tokens on; Which render passes are per-target versus shared per frame is undecided

Shadow fitting, cubemap position and water reflection/refraction are derived from the one player or active camera. Without a per-target recompute, window 2 shows window 1's shadow frustum and reflection.

*Why it breaks:* SHADOWS (the main risk). With the default CAM_CENTER mode, ShadowHandler::Update() (ShadowHandler.cpp:128-135, called from Game.cpp:1431) computes light-space scales and centre from CAMTYPE_PLAYER: GetOrthoProjectedFrustumRadius/CalcShadowProjectionPos (ShadowHandler.cpp:699-701, :783-861) and ComposeLightMatrix (:493-522, which also uses the player camera's forward/right/up). The results sit in members (viewMatrix, projMatrix, shadowProjScales, projMidPos), and shaders read them through GetShadowMatrixRaw (BumpWater.cpp:936, ModelDrawerState.cpp:153, SMFRenderState.cpp:185/296) and the per-frame UBO (UniformConstants.cpp:181-183). If window 2 looks elsewhere, its shadow lookups fall outside window 1's map. The depth texture uses GL_CLAMP_TO_BORDER with border colour 1.0 (ShadowHandler.cpp:346, :350, :363), so those pixels are treated as lit and window 2 shows no shadows. The recompute needs more than rerunning CreateShadows. The shadow pass draws only units and features with SO_SHOPAQ_FLAG (UnitDrawer.cpp:264). That flag is set in CUnitDrawerData::UpdateObjectDrawFlags (UnitDrawerData.cpp:430-481) using cam->InView against the shadow camera, which is fitted to the player camera. FeatureDrawerData.cpp:125/193 and ProjectileDrawer.cpp:494-538 (per-camtype sort distances) do the same. These run once per frame from WorldDrawer::Update (WorldDrawer.cpp:225-227). A per-target shadow pass therefore also has to redo the shadow-camtype draw-flag evaluation per target, or those objects are never queued. The cheap prototype option is ShadowProjectionMode=0 (MAP_CENTER, ShadowHandler.cpp:703-705). Its coverage is the whole map and does not depend on the camera, so one shadow map and one set of flags serve both windows. The light-matrix rotation still tracks the player camera, which is harmless for coverage.

ENV CUBEMAP (minor, polish-level for the prototype). The cube is centred on the active camera at CubeMapHandler.cpp:218. Only one face (or two with mapSkyReflections) is rendered per call, driven by the member currReflectionFace (CubeMapHandler.cpp:145-175). Calling it once per target would interleave faces from two camera positions in one cube texture. The fix is to keep it per frame, centred on a designated primary camera. Sky reflections barely depend on position, so the visual impact is small.

WATER (corrected). UpdateWater runs inside the per-draw path (WorldDrawer.cpp:437) and picks up the active camera through GetSetActiveCamera(CAMTYPE_UWREFL) plus CopyStateReflect (BumpWater.cpp:1026-1030). A looped CWorldDrawer::Draw therefore recomputes reflection and refraction per target, provided the active camera is set per target. What breaks is the hard-coded FBO::Unbind() plus globalRendering->LoadViewport() at BumpWater.cpp:571-572 and :986, and the textures sized from the main window's viewSize (:181-182, :391). The only Update-phase water item is the UBO reflection matrices, built from the player camera once per frame (UniformConstants.cpp:186-195).

*Evidence:*
- `rts/Rendering/ShadowHandler.cpp:130` Update uses GetCamera(PLAYER) to SetShadowMatrix (128-135)
- `rts/Rendering/ShadowHandler.cpp:648` CreateShadows sets shadow camera, swaps active camera, restores via Update()
- `rts/Rendering/Env/CubeMapHandler.cpp:204` face camera positioned from previous active camera
- `rts/Rendering/WorldDrawer.cpp:249` GenerateIBLTextures runs shadows, cubemap, sky/shading textures once per frame
- `rts/Game/Game.cpp:1431` shadowHandler.Update() in Update phase
- `rts/Game/Game.cpp:1519` worldDrawer.GenerateIBLTextures() once per Draw
- `rts/Game/Game.cpp:1522` FBO::Unbind + camera->LoadViewport restore after pre-passes (1522-1524)
- `rts/Rendering/WorldDrawer.cpp:252` CreateShadows, then UpdateReflectionTexture (:264)
- `rts/Rendering/ShadowHandler.cpp:629` CreateShadows relies on caller to unbind and restore viewport
- `rts/Rendering/UniformConstants.cpp:181` shadow matrices exported through the UBO
- `rts/Rendering/ShadowHandler.cpp:627` CreateShadows renders the single shadow map (627-657)
- `rts/Rendering/Env/CubeMapHandler.cpp:169` currReflectionFace round-robin once per call (169-175)
- `rts/Rendering/Env/CubeMapHandler.cpp:218` curCam->SetPos(prvCam->GetPos()) centres the cube on the active camera (203-224)
- `rts/Rendering/ShadowHandler.cpp:32` Shadows default 2

*Direction:* Split the item. (a) Shadows: for the prototype, force or default ShadowProjectionMode to MAP_CENTER so shadow fit and shadow draw flags are camera-independent and shared across targets. For the full solution, make the shadow matrices, shadow-camtype draw flags and UBO shadow matrices a per-target computation, with the shadow map rendered after the previous target has finished drawing (one GL context, sequential, so the singleton texture is safe). (b) Cubemap: keep it per frame on a designated primary camera, and note that sharing is an accepted approximation. (c) Water: handle it under the hard-coded restore, per-target viewport and FBO-size items, not as an Update-phase pre-pass. Also move the UBO reflection matrices into the per-target data.

> verifier B (corrected): The claim bundles three passes with different severities, and only the shadow part is a real prototype problem. (1) SHADOW: the mechanism holds and affects a default-config run. ShadowProjectionMode defaults to SHADOWPROMODE_CAM_CENTER (ShadowHandler.cpp:34, enum at ShadowHandler.h:47). That mode sizes and centres the ortho box on CAMTYPE_PLAYER's frustum (ShadowHandler.cpp:700-701, 807, 836). CShadowHandler::Update() runs once per frame in UpdateUnsynced (Game.cpp:1431) and CreateShadows runs o

> verifier A (corrected): The core is true in the current tree. The default ShadowProjectionMode is SHADOWPROMODE_CAM_CENTER (ShadowHandler.cpp:34, enum value 1 in ShadowHandler.h:47). CShadowHandler::Update() fits the shadow matrices and CAMTYPE_SHADOW frustum to CAMTYPE_PLAYER (ShadowHandler.cpp:130-134). It runs once per Draw in UpdateUnsynced (Game.cpp:1431). CreateShadows renders one singleton map and leaves the FBO unbind and viewport restore to the caller (ShadowHandler.cpp:627-657; Game.cpp:1519-1524). CreateRefl

### ENV-02 - Shader constants and uniforms baked from view size at creation or resize
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** FBO-02; DSN-17; DepthBufferCopy is a single texture sized to the primary view

Some shaders receive view size as compile-time #defines or program uniforms set once, not per draw.

*Why it breaks:* Only two things are really baked.

(1) BumpWater.cpp:425-427,442 puts 1/viewSize, viewPos and blurBase/viewSizeY into the shader source as #defines. BumpWaterFS.glsl:44-46,229,234,298 uses them to turn gl_FragCoord into reflection/refraction texture UVs. Baking them requires a shader recompile, and the only recompile path is Game.cpp:1115-1118 (KillWater + SetWater from CGame::ResizeEvent, called from GlobalRendering.cpp:1296). BumpWater has no ViewResize handler at all (grep found none in the file). Window 2 at a different size gets UVs outside [0,1] or mis-scaled distortion. Default water is WATER_RENDERER_REFLECTIVE (IWater.cpp:28), so this only bites players who pick the BumpWater mode (4).

(2) The decal uniform screenSizeInverse (GroundDecalsFragProg.glsl:34) is set at shader load and in ViewResize (GroundDecalHandler.cpp:473, 1698). GetFragmentWorldPos (GroundDecalsFragProg.glsl:248-256) uses it to turn gl_FragCoord.xy into NDC xy before unprojection. The depth texel is picked separately by texelFetch(depthTex, ivec2(gl_FragCoord.xy)), which does not use the uniform. So the wrong constant skews the reconstructed world XY. The out-of-range or stale depth texel comes from the depth-copy texture being sized to the primary view (DepthBufferCopy.cpp:57-62,138), which is the DepthBufferCopy singleton problem listed in dependsOn.

Items to drop from this claim.
- DynWater.cpp:340-341 and RefractWater.cpp:73-80 are per-draw reads of globalRendering->viewSize*. They are correct as soon as the per-target view size is swapped in. What is baked there is texture sizing (RefractWater::LoadGfx at lines 48/51, DynWater refractSize at line 55), which belongs to the view-size-keyed resources problem.
- UniformConstants viewGeometry (UniformConstants.cpp:222) is declared in ModelVertProgGL4/ShadowGenVertProgGL4/ModelFragProgGL4 but no in-tree shader reads it. A grep for viewGeometry finds only the struct declarations. It only matters for out-of-tree Lua shaders. The camera-matrix half of that UBO (UniformConstants.cpp:167-171, uploaded once per frame at Game.cpp:1441) is a separate per-target UBO problem.
- DepthBufferCopy.cpp:72 (viewPos in the blit source rect) is not a multi-window issue. viewPos is non-zero only in dualScreenMode (GlobalRendering.cpp:1454,1496,1516). Each separate window's default framebuffer has viewPos = (0,0).

*Evidence:*
- `rts/Rendering/Env/BumpWater.cpp:425` #define ScreenInverse/ViewPos at 425-427, BlurBase at 442; screenTextureX/Y fixed at 181-182
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:473` screenSizeInverse set at shader load
- `cont/base/springcontent/shaders/GLSL/GroundDecalsFragProg.glsl:255` GetWorldPos(gl_FragCoord.xy * screenSizeInverse, depth); texelFetch at 250-252 ignores viewPos
- `rts/Rendering/Env/DynWater.cpp:340` dx/dy from viewSize per draw
- `rts/Rendering/Env/RefractWater.cpp:73` program env params from viewSize (73-80)
- `rts/Rendering/DepthBufferCopy.cpp:72` blit source rect offset by viewPos (opposite convention)
- `rts/Rendering/UniformConstants.cpp:222` viewGeometry (vsx,vsy,vpx,vpy) already exists in the UBO but engine shaders do not read it
- `rts/Game/Game.cpp:1441` UBO updated once per frame in UpdateUnsynced, Bind at 1464
- `cont/base/springcontent/shaders/GLSL/ModelFragProgGL4.glsl:18` viewGeometry exposed to shaders (declared, vsx/vsy/vpx/vpy)
- `rts/Rendering/Env/BumpWater.cpp:906` glCopyTexSubImage2D from viewPos/viewSize each Draw
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:1693` only other writer is ViewResize
- `cont/base/springcontent/shaders/GLSL/GroundDecalsFragProg.glsl:250` texelFetch by gl_FragCoord into the depth copy
- `cont/base/springcontent/shaders/GLSL/GroundDecalsFragProg.glsl:243` unproject via gl_ModelViewProjectionMatrixInverse
- `cont/base/springcontent/shaders/GLSL/GroundDecalsVertProg.glsl:337` gl_ModelViewProjectionMatrix
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:502` binds the shared depth copy

*Direction:* Make the decal shader (and BumpWater, if it must support N targets) take screen size per target pass instead of baking it. Either declare the existing UniformsParams UBO block in GroundDecalsFragProg.glsl and read viewGeometry.xy, or set screenSizeInverse each Draw() from the current target's size. Upload the UBO values per target pass, together with the camera matrices. Remove the global-event ViewResize and the "recreate water on resize" path (Game.cpp:1115-1118) as the correctness mechanism. They carry no target identity, so the wrong target's size would be baked or pushed. For BumpWater, keep only target-independent values in #defines and move ScreenInverse, ViewPos and BlurBase to uniforms. For the prototype, forcing both windows to the same size removes the constant mismatch, but the depth copy and BumpWater textures still need per-target sizing.

> verifier B (corrected): The core mechanism is real but narrower than claimed, and it only bites when the second target has a different size from the primary.

What holds up:
- BumpWater bakes `ScreenInverse`, `ViewPos` and `BlurBase` as `#define`s from `globalRendering` at shader creation (BumpWater.cpp:425-442, loaded at 470-471). Its `ScreenTextureSizeInverse` comes from `screenTextureX/Y`, fixed in the constructor (BumpWater.cpp:181-182).
- The BumpWater fragment shader uses `ViewPos`, `ScreenInverse` and `ScreenTex

> verifier A (corrected): The core mechanism holds for two consumers. BumpWater (BumpWater.cpp:425-427,442) and the ground-decal shader (GroundDecalHandler.cpp:473-476, 1698-1701) take primary-view size once, at construction or on the global ViewResize. In single-window play they stay correct only because Game.cpp:1115-1118 destroys and recreates the whole water on every resize, and eventHandler.ViewResize() (Game.cpp:1127) re-pushes the decal uniform. Neither mechanism has a target parameter, so window 2 gets window 1's

### ENV-03 - Shadow map projection is fitted to the player camera and generated once per frame
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** DSN-01; Shared IBL passes rely on a single batched FBO and viewport restore before the first draw; LOP-03; Draw-flag bitmask encodes visibility for a single player camera; PVC-03

The shadow camera's orthographic fit (light orientation and radius) is computed from CAMTYPE_PLAYER in CShadowHandler::Update, and CreateShadows runs once in GenerateIBLTextures. Default ShadowProjectionMode is CAM_CENTER, i.e. fitted to the camera frustum.

*Why it breaks:* SetShadowMatrix(playCam, shadCam) orients the light matrix on playerCam->forward/right/up and sizes the ortho box from the projected player frustum radius. The single shadow depth texture is then sampled by both windows. Where camera 2 looks outside camera 1's fitted box, shadows are missing or wrong. The shadow camera's CULLING matrices (also used for per-object SHADOW draw flags) match only view 1.

*Evidence:*
- `rts/Rendering/ShadowHandler.cpp:130` shadow fitted to the PLAYER camera
- `rts/Rendering/ShadowHandler.cpp:493` ComposeLightMatrix uses playerCam forward/right/up; GetOrthoProjectedFrustumRadius(playerCam...) at :701/:783
- `rts/Rendering/ShadowHandler.cpp:34` ShadowProjectionMode default CAM_CENTER; Shadows default 2 at :32
- `rts/Rendering/WorldDrawer.cpp:252` CreateShadows once per frame inside GenerateIBLTextures (249-259)
- `rts/Rendering/ShadowHandler.cpp:128` Update fits the shadow camera from the player camera only
- `rts/Rendering/ShadowHandler.cpp:327` single smOpaqFBO + depth/colour textures created in InitFBOAndTextures (327-434)
- `rts/Rendering/UniformConstants.cpp:181` UBO shadowView/Proj from shadowHandler DRAWING matrices
- `rts/Map/SMF/SMFRenderState.cpp:185` representative global consumer of GetShadowMatrixRaw (also 296, ModelDrawerState.cpp:153, GrassDrawer.cpp:339, GroundDecalHandler.cpp:827, BumpWater.cpp:936)
- `rts/Rendering/ShadowHandler.cpp:505` ComposeLightMatrix uses playerCam->forward/right/up
- `rts/Rendering/ShadowHandler.cpp:700` CAM_CENTER fit from playerCam frustum; CalcShadowProjectionPos 829-861
- `rts/Rendering/ShadowHandler.cpp:346` CLAMP_TO_BORDER with border 1.0 (350, 363): outside the map reads as lit
- `rts/Rendering/ShadowHandler.cpp:739` function-static curMapDiameter
- `rts/Rendering/Common/ModelDrawerState.cpp:208` shadowMatrix uniform from the global shadow handler
- `rts/Rendering/ShadowHandler.cpp:93` Shadows=2 clears the MAP bit; terrain casts no shadows by default
- `rts/Map/SMF/SMFRenderState.cpp:253` terrain binds shadow textures and shadowMat
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:507` decals bind the same shadow map
- `rts/Rendering/Env/GrassDrawer.cpp:370` grass uses shadowMatrix uniform
- `rts/Rendering/ShadowHandler.cpp:459` shadow passes for unit/feature drawers, once per frame
- `rts/Rendering/Units/UnitDrawerData.cpp:476` shadow flags from the single shadow camera
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:537` projectile shadow flags from camShadow->InView
- `rts/Rendering/ShadowHandler.cpp:456` projectile shadow draw once per frame

*Direction:* Prototype: use the zero-code workaround, either ShadowProjectionMode=0 (MAP_CENTER, constant resolution, valid for any view) or Shadows=-1. Cheap code fix (S): generalise ShadowHandler.cpp:675-861 to take a span of cameras. CalcShadowProjectionPos currently reads exactly 8 verts from one CCamera at :836. Concatenate 8N frustum verts, clip and clamp each as today, average them for the centre, and take the x/z bounds over all of them, still capped at 2*boundingRadius. Pick one fixed camera, e.g. view 0, for ComposeLightMatrix's in-plane rotation. Constraints to record:\n(1) Every view camera's frustum must be updated before shadowHandler.Update() at Game.cpp:1431. camera->Update() at :1430 only refreshes the single active camera.\n(2) Keep Update, UpdateDrawFlags (WorldDrawer.cpp:225-227), the UBO copy (UniformConstants:181) and CreateShadows once per frame, in that order. Calling shadowHandler.Update() per view without re-running CreateShadows would leave the matrices out of sync with the texture contents, giving misaligned shadows, which is worse than missing ones.\n(3) With two distant views the union fit saturates at the map-diameter clamp, so resolution degrades towards MAP_CENTER quality. Accept this for the prototype.\n(4) Per-view shadow maps (the full solution) need one FBO and texture per view, or per-view re-render inside PrepareView with the UBO, shadow camera and draw flags refreshed each time.

> verifier A (corrected): The core mechanism holds in the current tree, and every cited line checks out. CShadowHandler::Update (ShadowHandler.cpp:128-135) fetches CAMTYPE_PLAYER and calls SetShadowMatrix/SetShadowCamera. SetShadowMatrix (:530-562) builds the light matrix from ComposeLightMatrix(playerCam) (:493-522, axes at :505) and sizes the ortho box via GetShadowProjectionScales(playerCam, ...) (:533, :675-726). The default mode is CAM_CENTER (:34, enum value 1 at ShadowHandler.h:47), so projScales.x = GetOrthoProje

> verifier B (confirmed): The mechanism is real and I verified it in code. CShadowHandler::Update (ShadowHandler.cpp:128-135, called once per frame at Game.cpp:1431) takes the fixed CAMTYPE_PLAYER camera. It passes that camera to SetShadowMatrix (:530). ComposeLightMatrix orients the light on playerCam forward/right/up (:505). With the default ShadowProjectionMode=CAM_CENTER (:34) the ortho box is sized and centred from that camera's 8 frustum verts (GetOrthoProjectedFrustumRadius :783, CalcShadowProjectionPos :829-861).

### ENV-04 - Water renderers bake the construction-time window size into screen-copy textures and shader constants
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** DSN-17; Window-size shader constants are set at resize time or in a once-per-frame UBO

BumpWater and RefractWater size their screen-copy and depth textures from the view size at construction. BumpWater also compiles the window size into the shader as constants. The default AdvWater is unaffected.

*Why it breaks:* BumpWater and RefractWater snapshot globalRendering->viewSize once at construction (BumpWater.cpp:181-182, RefractWater.cpp:48/51). They size the refract texture, depth copy texture and refract depth RBO from it (BumpWater.cpp:332, 353, 391). BumpWater also compiles ScreenInverse, ScreenTextureSizeInverse, ViewPos and BlurBase into the shader as GLSL defines (425-427, 442). Draw() copies the current framebuffer with glCopyTexSubImage2D using the live globalRendering viewPos/viewSize (BumpWater.cpp:906, 912; RefractWater.cpp:65). With a second target whose size differs from the construction size, two things go wrong. (a) If the target is larger than the texture, the copy raises GL_INVALID_VALUE, so the refraction and depth data are not updated. (b) For any size mismatch, reftexcoord = screenPos * ScreenInverse (glsl:46/228) and BlurBase use the wrong scale, so reflection sampling is mis-scaled. screencoord for refraction and depth is still correct for smaller targets, because the copy sits at texture origin (0,0) and uses the baked texture size. ViewPos is 0 for any normal window, so it is irrelevant for the prototype. Resize currently works only because CGame::ResizeEvent kills and recreates the water (Game.cpp:1115-1118). The water object is a single global (IWater.cpp:43), so it cannot differ per window. The default AdvWater (Water=1) is unaffected. The user's own config uses Water=4, so BumpWater will be exercised in practice. DynWater has no equivalent bug (refractSize is just a 512/1024 tier).

*Evidence:*
- `rts/Rendering/Env/BumpWater.cpp:181` screenTextureX/Y = globalRendering view size at construction
- `rts/Rendering/Env/BumpWater.cpp:332` refraction texture, depth texture (353) and RBO (391) sized from screenTexture
- `rts/Rendering/Env/BumpWater.cpp:425` ScreenInverse/ViewPos/BlurBase baked as GLSL constants (442)
- `rts/Rendering/Env/BumpWater.cpp:906` glCopyTexSubImage2D with globalRendering view size (912 for depth)
- `cont/base/springcontent/shaders/GLSL/BumpWaterFS.glsl:44` screenPos = gl_FragCoord.xy - ViewPos; reftexcoord = screenPos * ScreenInverse
- `rts/Rendering/Env/RefractWater.cpp:48` subSurfaceTex sized from viewSize (51 for the T2D path); copy at 65
- `rts/Rendering/Env/DynWater.cpp:55` refractSize chosen from viewSizeY
- `rts/Rendering/Env/IWater.cpp:27` default Water = REFLECTIVE (AdvWater), fixed 512/128 targets (AdvWater.cpp:96-102)
- `rts/Rendering/Env/AdvWater.cpp:134` mesh from camera->CalcPixelDir(globalRendering view)
- `rts/Game/Camera.cpp:571` CalcPixelDir reads globalRendering viewSize/viewPos
- `rts/Game/Camera.cpp:144` CCamera::Update: aspect/matrices/viewport from globalRendering
- `rts/Game/Game.cpp:1115` water recreated on resize

*Direction:* Do not allocate per-target screen-copy textures. WorldDrawer.cpp:434-439 runs UpdateWater and Draw once per world draw. The glCopyTexSubImage2D at BumpWater.cpp:906/912 and the draw that samples it (957) both happen inside one Draw() call. Targets render sequentially, so a single shared set of screen-copy and depth-copy textures is enough. The shader already separates texture size from view size (ScreenTextureSizeInverse vs ScreenInverse, BumpWaterFS.glsl:45-46), so a texture at least as large as the biggest target works with only a uniform change.

Steps:
1. Turn ScreenInverse, ScreenTextureSizeInverse, ViewPos and BlurBase into `uniform vec2`. Remove the GLSLDefineConstf2 lines at BumpWater.cpp:425-427 and 442. Set them every Draw() with waterShader->SetUniform next to eyePos (BumpWater.cpp:932). The VS only mentions them in comments, so only the FS changes.
2. In Draw(), before the copy, grow the refraction and depth textures with glTexImage2D to max(current, viewSize) if the current target is larger. Never shrink. Use that texture size for ScreenTextureSizeInverse.
3. Drop the unused refractFBO depth RBO (line 391).
4. Replace the kill-and-recreate in CGame::ResizeEvent (Game.cpp:1115-1118) with nothing for BumpWater. Grow-on-demand makes it unnecessary, and recreation would use whichever window's globalRendering size is current and recompile the shaders.
5. Apply the same pattern to CRefractWater: grow subSurfaceTex, and compute the v[] uniforms from the current viewSize.
6. As a stopgap for the prototype, force Water=1 or 0 on the secondary window, or create both windows at the same size.

Effort: S-M. Remaining dependencies: globalRendering view size must be swapped per target, and the FBO::Unbind() / LoadViewport() resets in UpdateWater (BumpWater.cpp:571-572) must be fixed. Those are separate problems.

> verifier A (corrected): The core mechanism is real and verified in the current tree. CBumpWater's ctor sets screenTextureX/Y from globalRendering->viewSizeX/Y (BumpWater.cpp:181-182). The refract texture (332), depth texture (353) and refract depth RBO (391) are sized from them. ScreenInverse (425), ScreenTextureSizeInverse (426), ViewPos (427) and BlurBase (442) are baked into the shader as GLSL defines. Draw() runs glCopyTexSubImage2D with the current globalRendering viewPos/viewSize (906, 912). The shader uses them 

> verifier B (corrected): The mechanism is real for BumpWater and RefractWater. The severity is too high, and the DynWater part is wrong.

Confirmed in code:
- BumpWater.cpp:181-182 sizes screenTextureX/Y from viewSize at construction.
- The refraction texture (332), depth texture (353) and RBO (391) use that size.
- ScreenInverse, ViewPos and BlurBase are compiled in as GLSL constants (425-442).
- Draw() runs glCopyTexSubImage2D(viewPos, viewSize) into the baked-size texture every call (906, 912).
- The shader derives s

### ENV-05 - Dynamic water keeps a camera-centred persistent wave field
**Severity:** full - **Effort:** L - **Status:** corrected - **Depends on:** none

WATER_RENDERER_DYNAMIC simulates a scrolling heightfield that follows the camera position, so it cannot serve two cameras from one set of textures.

*Why it breaks:* CDynWater::Update() (DynWater.cpp:426-453) is not called every rendered frame. It runs once per new sim frame from CWorldDrawer::Update (WorldDrawer.cpp:229-237, `IWater::GetWater()->Update()`), using whichever camera is active then. It sets oldCamPosBig = camPosBig and recomputes camPosBig from camera->GetPos() clamped to the map (432-435). It then runs AddShipWakes, AddExplosions and DrawWaves (446-449), so the whole persistent wave simulation lives in the sim-rate update path, not the per-window draw path. DrawWaves scrolls the 1024^2 wave textures by (camPosBig - oldCamPosBig)/WF_SIZE (529-530, 566). The field covers only WF_SIZE=4096 elmos (WH_SIZE=2048, DynWater.cpp:44-46) around camPosBig. Wake and explosion injection is culled to |pos - camPosBig| < WH_SIZE-50 (1072-1073, 1095-1096, 1184-1185), so anything near a second camera but outside window 1's field is never injected. The per-draw part (UpdateWater -> DrawHeightTex, WorldDrawer.cpp:437) samples the field with camPosX/Z from the current draw camera (738-744) but offsets by the stale camPosBig. For a camera more than about 2000 elmos from window 1's camera the sampled UVs fall outside the 1024^2 field. DrawWaterSurface (786-790) recomputes camPosBig2 from the current camera and builds the mesh around it, while the vertex program's wave UV offset still uses camPosBig (349), so the mesh follows window 2 but samples a field anchored on window 1. DrawOuterSurface (1291-1292) also uses camPosBig2. A per-target fix therefore needs more than a second texture set. Each target would need its own wave state, with a simulation step per target that is told every target's camera position. A single sim-rate Update() cannot do this unless the target list is available there.

*Evidence:*
- `rts/Rendering/Env/DynWater.cpp:426` Update: camPosBig derived from camera->GetPos (432-435)
- `rts/Rendering/Env/DynWater.cpp:529` DrawWaves scrolls the field by the camPosBig delta (529-565); DrawHeightTex 738-744
- `rts/Rendering/Env/DynWater.cpp:340` Draw uses viewSize and camera fov for the surface (340-352)
- `rts/Rendering/Env/DynWater.cpp:432` camPosBig from camera->GetPos()
- `rts/Rendering/Env/DynWater.cpp:738` camPosX/Z from active camera in DrawHeightTex
- `rts/Rendering/Env/DynWater.cpp:1072` wake culling relative to camPosBig (1095, 1184)

*Direction:* Document DynWater as single-camera. Either force a fallback to another renderer (e.g. Reflective or Bumpmapped) when more than one render target is active, or give it a per-target resource bundle. The bundle would hold waveTex1/2/3, waveHeight32, camPosBig/oldCamPosBig and the dyn-wave simulation step, with the step driven per target and fed that target's camera. For the prototype, just ensure Water != 2 on the secondary window, or log and fall back.

> verifier A (corrected): The core claim holds in the current tree. CDynWater keeps one set of 1024x1024 RGBA16F wave textures (waveTex1/2/3, DynWater.cpp:150-179) plus one camPosBig/oldCamPosBig anchored to a single camera (DynWater.h:90-92). Two cameras cannot be served from that state. Two details in the mechanism were imprecise. Severity "full" is right, not "blocker": the default is Water=REFLECTIVE (IWater.cpp:28), so the 2-window 3D-only prototype does not depend on this unless someone selects Water=2.

### ENV-06 - Headless stubs, CMake source lists and multi-toolchain CI need updates
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** none

engine-legacy and engine-headless compile the same Rendering sources (headless with -DHEADLESS and stub SDL/GL), so any new window code must build both ways. The SDL stub returns one static pointer for every window and lacks window-id APIs.

*Why it breaks:* Same mechanism as claimed, with two corrections. (1) The impact is build, link and CI breakage, not prototype behaviour. engine-legacy and engine-headless compile the same rts/Rendering sources via engineSources. Headless additionally gets -DHEADLESS, and its SDL is replaced by headlessStubs, which links SDL2 COMPILE_ONLY. Any new unguarded SDL call missing from sdlstub.c (SDL_GetWindowID, SDL_GL_GetCurrentWindow, SDL_GetWindowFromID, SDL_GetWindowSize, SDL_GetWindowPosition, SDL_GL_GetDrawableSize) fails to link engine-headless. That breaks the `check` target and the CI Build step on all three platforms. The existing code avoids this with `#ifdef HEADLESS` in GlobalRendering.cpp (1548, 1585, 627) rather than by adding stubs. (2) New GL entry points used by multi-target code may also need stub entries in rts/lib/headlessStubs/gladstub.cpp. The SDL_Window*-keyed registry collision only matters if headless creates more than one target, which it should not, since headless is a single 8x8 dummy window.

*Evidence:*
- `rts/lib/headlessStubs/sdlstub.c:55` SDL_CreateWindow returns one static address
- `rts/lib/headlessStubs/sdlstub.c:166` SDL_GL_CreateContext returns one static address
- `rts/builds/headless/CMakeLists.txt:10` -DHEADLESS at folder level
- `rts/builds/headless/CMakeLists.txt:53` GameHeadless built from the Game sources with stubs
- `rts/builds/legacy/CMakeLists.txt:84` engine-legacy built from engineSources
- `rts/Rendering/CMakeLists.txt:4` explicit source list
- `test/CMakeLists.txt:82` `check` target depends on engine-headless
- `.github/workflows/engine-build.yml:62` 3-platform build matrix
- `rts/lib/headlessStubs/sdlstub.c:65` SDL_GL_MakeCurrent is a no-op, no current-context tracking
- `rts/Rendering/GlobalRendering.cpp:627` existing HEADLESS special-casing in DestroyWindowAndContext

*Direction:* Treat this as a standing constraint on the multi-target code, not a standalone task.

1. Keep the render-target registry on engine-owned target ids, not SDL_Window*. Never create secondary targets under HEADLESS (config with headlessValue(0), or an early return). Wrap any new SDL calls that the stub lacks (SDL_GetWindowID, SDL_GL_GetCurrentWindow, SDL_GetWindowFromID, SDL_GL_GetCurrentContext, SDL_GL_GetDrawableSize) in `#ifndef HEADLESS`, as GlobalRendering.cpp:1548-1577 does. Alternatively add trivial stubs to rts/lib/headlessStubs/sdlstub.c.
2. Add any new Rendering .cpp to the explicit list in rts/Rendering/CMakeLists.txt.
3. Before merging, run the headless build (`docker-build-v2/build.sh --compile linux -t engine-headless`, or `cmake --build . --target engine-headless`), and spot-check the arm64 and windows/MinGW CI legs.
4. Do not block the prototype on this. Build and run engine-legacy first, and do the headless pass once before merge.

> verifier A (corrected): The mechanism holds against the current tree, but the severity is too high. I re-read each cited location and they all match. sdlstub.c:55-58 has SDL_CreateWindow returning the address of a function-local static, and sdlstub.c:166-169 does the same for SDL_GL_CreateContext. SDL_GL_MakeCurrent at :65 is a no-op. The file defines no SDL_GetWindowID, SDL_GL_GetCurrentWindow, SDL_GetWindowFromID, SDL_GetWindowSize or SDL_GetWindowPosition. rts/lib/headlessStubs/CMakeLists.txt links the real SDL2 onl

> verifier B (corrected): The mechanism is real, but the severity is overstated. A broken engine-headless link does not make the 2-window prototype crash, raise GL errors or render wrongly. The prototype runs on engine-legacy, and the user's loop builds only `install-spring-legacy`. It is a merge and CI gate, so it belongs under "full". What I confirmed in the code:

- **The stub lacks the window-id calls.** sdlstub.c has no SDL_GetWindowID, SDL_GL_GetCurrentWindow, SDL_GetWindowFromID or SDL_GL_GetCurrentContext. A grep

### ENV-07 - Info textures are single map-sized instances keyed to the local ally team
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** DSN-17

LOS, air LOS, radar and path overlays are map-sized textures with one combiner texture, updated once per frame from gu->myAllyTeam. Shared between targets this is fine; different teams per window would not work.

*Why it breaks:* All eight CModernInfoTexture instances are created once in CInfoTextureHandler (InfoTextureHandler.cpp:20-27, held in a global spring::unordered_map) and updated by CInfoTextureHandler::Update (:150-163), called once from CGame::UpdateUnsynced at Game.cpp:1384-1388. That runs only when `newSimFrame || forceUpdate`, i.e. at most 30Hz, before any per-target draw, so it belongs in the shared phase.

Corrections to the original text:
1. Sizes are not uniformly map-sized.
   - Los, AirLos and Radar are LOS-resolution (Los.cpp:18, AirLos.cpp:19, Radar.cpp:22).
   - Path, Metal and MetalExtraction are half-map (Path.cpp:36, Metal.cpp:17, MetalExtraction.cpp:24).
   - Height is map size + 1 (Height.cpp:30).
   - Only the combiner is pwr2-map-sized RGB10_A2 (Combiner.cpp:24,33).
   - None is window- or viewport-sized, which is why sharing is safe.
2. Not all eight own an FBO. CMetalTexture (Metal.cpp:11-37) never creates or binds its inherited `fbo` and just uploads the metal map. The other seven bind an FBO: Combiner at Combiner.cpp:36 and :123, and the rest via CreateFBO plus RunFullScreenPass, which does `fbo.Bind(); glViewport(texSize); ... FBO::Unbind(); globalRendering->LoadViewport()` (ModernInfoTexture.cpp:25-36).
3. The ally-team input lives in the leaf textures, not the combiner.
   - Los.cpp:89,105; AirLos.cpp:90,103; Radar.cpp:104,117-119 read gu->myAllyTeam.
   - Only Path.cpp:229 reads gu->spectating and gu->spectatingFullView; the other textures do not.
   - MetalExtraction.cpp:99 and Radar.cpp:134 inherit it transitively by sampling the "los" info texture.
   - The combiner (Combiner.cpp:113-151) reads only `gu->gameTime` plus whatever the Lua info shader samples.
4. The combiner is updated only when an overlay mode is active (`IsUpdateNeeded() = !disabled`, Combiner.h:29). The overlay mode (`curMode`, `SwitchMode` at Combiner.cpp:62-87, driven by InfoTextureHandler::SetMode at :92-100) is also a single global, so both windows would show the same overlay.
5. Per-player UI state is another per-view input. CPathTexture::Update/IsUpdateNeeded read `selectedUnitsHandler.selectedUnits` and `guihandler->inCommand/commands/buildFacing` (Path.cpp:98,115-121,239) and pass gu->myAllyTeam to TestUnitBuildSquare (:245). Its update is also gated by `lastUsage`, which is refreshed by whichever consumer calls GetTexture (:125-130).
6. The sharing conclusion depends on GL object sharing. Textures are visible across windows only on one shared context or share-listed contexts, but FBOs are never shared between contexts. If the prototype uses separate contexts, the Update pass must run on the context that owns the FBOs (or recreate them), which links to the context-ownership problem.

The consumers read the combined texture globally and unchanged for every target. These are SMFRenderState.cpp:260 (unit 14), SMFReadMap.cpp:826, GrassDrawer.cpp:654,733, GroundDecalHandler.cpp:515, BumpWater.cpp:952, Lua via LuaOpenGLUtils.cpp:341,595-605, and the minimap (out of prototype scope).

*Evidence:*
- `rts/Rendering/Map/InfoTexture/Modern/Los.cpp:89` gu->myAllyTeam LOS source (105); AirLos.cpp:90,103; Radar.cpp:104,119; Path.cpp:229-266
- `rts/Rendering/Map/InfoTexture/Modern/Combiner.cpp:33` map-sized GL_RGB10_A2 combiner texture and FBO (35-46)
- `rts/Game/Game.cpp:1384` infoTextureHandler->Update on sim cadence, shared phase (1388)
- `rts/Rendering/Map/InfoTexture/Modern/ModernInfoTexture.cpp:25` RunFullScreenPass resets FBO/viewport (25-36)

*Direction:* Keep the info-texture update in the shared phase (once per frame, on the shared context) for the prototype, since both windows show the same myAllyTeam and the same global overlay mode. Make sure its viewport/FBO reset does not leak into the per-target phase, and that the FBOs live on the context that runs it. For full support, add a per-view descriptor (viewAllyTeam, fullView, and selection/build context for the Path texture). Key the LOS, AirLos and Radar inputs, the MetalExtraction transitive dependency, and the combiner texture and overlay mode, by that view. There is a partial precedent in the tree: UnitDrawer's icon params already carry viewAllyTeam/isFullView (UnitDrawer.h:29, UnitDrawer.cpp:511-515), and MiniMap.cpp:1913 and LuaOpenGL.cpp:1382 fill it. Share textures that do not depend on the view (Height, Metal).

> verifier A (corrected): The core claim holds. The info textures are single, global, ally-team-keyed, non-window-sized resources updated once in the shared UpdateUnsynced phase, so they can be shared in the 2-window prototype (same sim, same myAllyTeam) and only need per-view keying for a full product. The mechanism text has several inaccuracies, and I corrected them: not every texture is map-sized, not every subclass uses an FBO, the spectating flags are read by only one texture, the combiner is not updated uncondition

### ENV-08 - ViewResize carries no target and Game::ResizeEvent rebuilds view-sized state globally
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** TGT-02; DSN-17

A resize fans out through the parameterless ViewResize event and CGame::ResizeEvent, which kills and recreates the water renderer.

*Why it breaks:* ViewResize is a global, target-less "the window changed size" notification wired to the primary window only.

- Any SDL window's SIZE_CHANGED reaches SpringApp::MainEventHandler (SpringApp.cpp:1135-1157). The handler ignores event.window.windowID and calls SaveWindowPosAndSize, UpdateGLConfigs, UpdateGLGeometry, InitGLState and UpdateInterfaceGeometry. Geometry is always read from the main sdlWindow (GlobalRendering.cpp:1571).
- It then calls activeController->ResizeEvent(), which is CGame::ResizeEvent (Game.cpp:1105). That does the minimap UpdateGeometry, the water kill and recreate (1116-1118), gameTextInput.ViewResize and eventHandler.ViewResize (1126-1127).
- ViewResize fans out to DepthBufferCopy, GroundDecalHandler and every Lua handle. DepthBufferCopy recreates its depth textures and FBOs from the global viewSize (DepthBufferCopy.cpp:55-63, 138, 140). GroundDecalHandler resets screenSizeInverse (GroundDecalHandler.cpp:1698-1701), which GroundDecalsFragProg.glsl:255 uses as gl_FragCoord.xy * screenSizeInverse. CLuaHandle::ViewResize pushes the main window's numbers (LuaHandle.cpp:2588-2606).
- With a second SDL window, window-2 resize, show and creation events trigger this whole chain with unchanged main-window values. The result is a needless water rebuild and Lua re-layout.
- The new size of window 2 is never recorded. A one-shot event keyed to window resize is also the wrong trigger for per-target state. Even with no resize at all, window 2's draw needs its depth copy, decal uniform and water textures bound or sized per target at draw time.

Related windowID-blind SIZE_CHANGED consumers:
- RmlUi_Backend.cpp:568-576 sets the Rml viewport and winX/winY from event.window.data1/data2 of whichever window fired. It is registered through input.AddHandler (RmlUi_Backend.cpp:164), so a secondary-window resize overwrites the main GUI viewport.
- InitGLState (GlobalRendering.cpp:1727) calls glClear on whichever drawable is current when the event pump runs, and DepthBufferCopy's ViewResize creates FBOs and textures. Both run in the event pump, so the right GL context and drawable must be made current first.

*Evidence:*
- `rts/Game/Game.cpp:1105` ResizeEvent: minimap geometry, KillWater/SetWater, ViewResize (1105-1129)
- `rts/System/Events.def:142` SETUP_EVENT(ViewResize) has no arguments
- `rts/System/EventHandler.cpp:673` ViewResize dispatch
- `rts/Rendering/DepthBufferCopy.cpp:55` ViewResize recreates textures from current viewSize
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:1693` ViewResize resets the screenSizeInverse uniform
- `rts/Lua/LuaHandle.cpp:2579` Lua ViewResize callin
- `rts/System/SpringApp.cpp:1135` SIZE_CHANGED handler (1135-1157); whole MainEventHandler (1095-1252) never reads windowID
- `rts/Rendering/GlobalRendering.cpp:1571` geometry always read from the main sdlWindow
- `rts/Game/Game.cpp:1115` water killed and recreated on every resize (1115-1118), then ViewResize (1127)
- `rts/Rendering/DepthBufferCopy.h:22` WantsEvent ViewResize (22-27); GroundDecalHandler.h:144,164 is the only other listener
- `rts/System/SpringApp.cpp:1185` HIDDEN/MINIMIZED sets globalRendering->active=false; CLOSE at 1249 calls RequestQuit
- `rts/Game/Game.cpp:1471` Draw early-outs when !globalRendering->active

*Direction:* Keep the proposed direction, but split and re-scope it.

1. Prototype minimum, which belongs to the window-geometry problem: filter or route SDL window events by windowID in MainEventHandler. Only the primary window may drive ResizeEvent, `active`, RequestQuit and the geometry re-read. Make window 2 non-resizable, or log and ignore its resize.
2. Later: introduce an internal ViewResize(target) carrying the target's size. Per-target resources are created or resized when the target is created or resized, or lazily at draw time. This covers the depth copy, the decal screenSizeInverse (better set per draw), and water resolution-dependent textures.
3. Stop the kill and recreate of water in CGame::ResizeEvent once the resolution-dependent water state is per-target or per-draw.
4. Do not fire the Lua ViewResize for secondary targets in the prototype. Add a target ID to the Lua payload later, in a backward-compatible way (see coding-agents/BACKWARDS_COMPATIBILITY.md).
5. Make the RmlUi viewport update windowID-aware.

> verifier B (corrected): The code facts are real, but the claim is overstated as a prototype "risk". (1) The mechanism holds: SpringApp::MainEventHandler (SpringApp.cpp:1095-1252) never reads event.window.windowID. A SIZE_CHANGED from any window runs UpdateGLGeometry and then activeController->ResizeEvent() (SpringApp.cpp:1135-1157). CGame::ResizeEvent (Game.cpp:1105-1129) always does KillWater/SetWater(wt) (1115-1118) and then eventHandler.ViewResize(). The listeners are DepthBufferCopy::ViewResize (DepthBufferCopy.cpp

> verifier A (corrected): The mechanism holds in the current tree. I opened every cited line.

- CGame::ResizeEvent (Game.cpp:1105-1129) reads the water ID, calls KillWater() then SetWater(wt) (1116-1118), then fires eventHandler.ViewResize() (1127). SetWater only returns early on an unchanged ID, and KillWater has already nulled `water`, so the renderer really is rebuilt (IWater.cpp:95-98).
- The rebuild is not cosmetic. CBumpWater initialises screenTextureX/Y from viewSizeX/Y (BumpWater.cpp:181-182), and CRefractWater 

### ENV-09 - Consume-once update flags and round-robin counters require an exactly-once shared stage
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** LOP-03; Shadow map and environment reflection cubemap are single instances fit to the player camera

Sun/sky/water 'updated' edges are consumed with std::exchange and the cubemap/specular counters advance per call, so the shared stage must run exactly once per frame in the owning context.

*Why it breaks:* The edge flags (SkyLight cacheDir, sunLighting/sky/waterRendering updated) have a single consumer, CWorldDrawer::GenerateIBLTextures (WorldDrawer.cpp:268-283). That function has a single call site, CGame::Draw (Game.cpp:1519), which today runs once per draw frame before the camera/viewport restore at 1521-1524. All of its outputs are single shared instances: the shading texture, sky texture, specular cubemap and reflection cubemap. They stay valid for every window as long as the contexts share textures and the stage runs once per frame. The only failure modes are (a) a future design that runs the stage per window against non-shared per-context copies of these textures, where the second window never sees the update edge, or (b) a naive loop of the whole draw body per window, which advances the reflection face cycle N times per frame. (b) is benign: it only changes the refresh rate. The note does not need its own prototype work item.

*Evidence:*
- `rts/Rendering/WorldDrawer.cpp:268` 4 consuming reads (268-271)
- `rts/Rendering/Env/SkyLight.h:13` Update() copies cacheDir=lightDir and returns true once
- `rts/Rendering/Env/ISky.h:47` same pattern
- `rts/Rendering/Env/CubeMapHandler.cpp:170` currReflectionFace modulo counter
- `rts/Lua/LuaUnsyncedCtrl.cpp:4225` sunLighting->SetUpdated() from Lua
- `rts/Rendering/Env/SunLighting.h:28` IsUpdated = std::exchange(updated,false)

*Direction:* Fold this into the shared-stage split as an acceptance note. GenerateIBLTextures (shadow, reflection cube, specular cube, shading texture) runs exactly once per frame, outside the per-view loop, with the context that owns the FBOs current. That stage stays the sole consumer of the sunLighting, sky and water IsUpdated() flags and of ISkyLight::Update(). Per-view code must not read them. Only if per-context texture copies are ever chosen, replace the consume-once bools with monotonically increasing generation counters (the owner writes, each context tracks its last-seen value). The round-robin currReflectionFace and specularTexIter counters then become per-resource state, not shared. For the prototype, do nothing beyond keeping the stage out of the per-view loop.

> verifier A (corrected): The code facts are accurate. WorldDrawer.cpp:268-271 does four consume-once reads (SkyLight.h:13-19 Update() overwrites cacheDir, and std::exchange in ISky.h:47-49, SunLighting.h:28-30 and WaterRendering.h:15-17). Grep finds no other IsUpdated() or GetLight()->Update() consumer. currReflectionFace (CubeMapHandler.cpp:169-174) and specularTexIter (CubeMapHandler.cpp:272-273) advance once per call. Lua sets the flags at LuaUnsyncedCtrl.cpp:4205, 4225, 4292 and 4868. But the 'risk' rating does not 

> verifier B (corrected): The mechanism is real and the line references check out. For the 2-window, 3D-only prototype it is not a risk, because it is satisfied by construction and the worst naive outcome is harmless. It is a constraint on the shared-stage split problem (the one named in dependsOn), not a separate defect.

What I verified:
- WorldDrawer.cpp:268-271 does read four edge flags in GenerateIBLTextures. ISkyLight::Update() (SkyLight.h:13-19) compares and then writes cacheDir. ISky.h:47, SunLighting.h:28 and Wa

### ENV-10 - Environment reflection cubemap is generated from the primary camera position
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** none

The env reflection cubemap used by model shaders is rendered one face per frame from the previous active camera's position, so it is only right for window 1.

*Why it breaks:* Same as claimed, with three corrections. (1) Drop the viewSize remark. CCamera::UpdateMatrices ignores vsx/vsy for PROJTYPE_PERSP and builds the projection from the explicit aspect argument, which is 1.0 here (Camera.cpp:238-239, 598-605). The ENVMAP camera is its own CCamera slot and its viewport is set to reflTexSize via UpdateLoadViewport(0,0,reflTexSize,reflTexSize) (CubeMapHandler.cpp:220), so it does not depend on globalRendering->viewSize. (2) The positional error is not new to multi-window. In one window the cube is already centred on the camera, not on each unit, so every off-centre unit reflects an approximation. Multi-window only moves the centre to whichever camera is active when GenerateIBLTextures runs, normally window 1's. (3) If the implementation runs GenerateIBLTextures once per window, the one-face-per-call round-robin would be fed alternating camera positions. The six faces of the single cube would then be assembled from different centres, and the update cadence would double. The prototype should therefore run UpdateReflectionTexture once per frame, using the primary camera. The skyReflectionTexID cube is built by the same function and has the same property. It is sampled by the SMF terrain shader (SMFRenderState.cpp:266) and exposed to Lua (LuaOpenGLUtils.cpp:546,549).

*Evidence:*
- `rts/Rendering/Env/CubeMapHandler.cpp:204` ENVMAP camera pos copied from the previous active camera (218)
- `rts/Rendering/WorldDrawer.cpp:264` UpdateReflectionTexture once per frame in GenerateIBLTextures
- `rts/Rendering/Env/CubeMapHandler.cpp:218` curCam->SetPos(prvCam->GetPos())
- `rts/Rendering/Common/ModelDrawerHelpers.cpp:54` unit shaders bind the single env reflection cube
- `rts/Rendering/WorldDrawer.cpp:263` UpdateReflectionTexture once per frame
- `rts/Rendering/Common/ModelDrawerHelpers.cpp:52` cubemaps bound for every model pass

*Direction:* Keep one shared cube, generated once per frame from the primary camera, and run GenerateIBLTextures once per frame rather than once per window. This is acceptable for the 2-window prototype. Later options are to centre the cube on the map centre, to keep one cube per camera and bind the matching one per target, or to render it lazily for the active target. The final choice is a design decision (dsn).

> verifier A (corrected): The core mechanism holds in the current tree. CubeMapHandler::CreateReflectionFace (CubeMapHandler.cpp:204-218) calls GetSetActiveCamera(CAMTYPE_ENVMAP), which returns the previously active camera (CameraHandler.h:79-81). It then does curCam->SetPos(prvCam->GetPos()). Each call renders one face into the single shared envReflectionTexID, or one sky-only face into skyReflectionTexID, then advances currReflectionFace (CubeMapHandler.cpp:145-175). It is called once per frame from CWorldDrawer::Gener

### ENV-11 - Multi-monitor, multi-GPU and mixed refresh behaviour
**Severity:** polish - **Effort:** M - **Status:** confirmed - **Depends on:** none

Display queries and window bounds are written for the primary window's display. Windows on monitors driven by different GPUs or refresh rates are untested and have no per-target display state.

*Why it breaks:* GetCurrentDisplayIndex uses the primary window; GetWindowPosSizeBounded clamps to the union of all displays; numDisplays is global. Hybrid-GPU setups present through a copy when a window sits on the other GPU's output. Mixed refresh rates interact with the per-target swap interval.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:1389` display index from the primary window
- `rts/Rendering/GlobalRendering.cpp:1170` GetAllDisplayBounds union of displays
- `rts/Rendering/GlobalRendering.cpp:1191` GetWindowPosSizeBounded clamps to the union
- `rts/System/SpringApp.cpp:1243` DISPLAY_CHANGED only remakes the primary current

*Direction:* Keep a per-target display index and bounds; test on dual-monitor and hybrid-GPU hardware. Treat mixed refresh as expected tearing on the secondary until proven otherwise.

> verifier A (confirmed): Cited code matches. GetCurrentDisplayIndex (GlobalRendering.cpp:1389) uses the single sdlWindow. GetAllDisplayBounds (1170) unions all displays and GetWindowPosSizeBounded (1191) clamps to that union. numDisplays is a global member (237). The DISPLAY_CHANGED handler (SpringApp.cpp:1243) only calls MakeCurrentContext on the global context. None of this crashes or corrupts a two-window 3D-only prototype, so polish is the right severity. Hybrid-GPU copy and mixed-refresh behaviour are environmental

### ENV-12 - SDL2 2.0.8 API floor and SDL3 migration exposure
**Severity:** polish - **Effort:** S - **Status:** unverified - **Depends on:** none

The Linux CI image installs Ubuntu 18.04's libsdl2-dev (SDL 2.0.8), and the code already hacks around missing newer events. All new window code must stick to APIs present in 2.0.8; no SDL3 usage or plan exists.

*Why it breaks:* SDL_WINDOWEVENT_DISPLAY_CHANGED is defined locally as 18 because the Linux SDL is old, and MouseInput notes 2.0.8 behaviour. New code using newer SDL2 APIs would break the Linux build, and scattering SDL calls over more call sites raises the future SDL3 port cost.

*Evidence:*
- `docker-build-v2/images/all-linux/Dockerfile:15` libsdl2-dev from Ubuntu 18.04
- `rts/System/SpringApp.cpp:172` RECOIL_SDL_WINDOWEVENT_DISPLAY_CHANGED workaround
- `rts/System/Input/MouseInput.cpp:51` SDL 2.0.8 specific comment
- `rts/build/cmake/FindSDL2.cmake:1` custom SDL2 imported target

*Direction:* Use only SDL_GetWindowID, SDL_GL_MakeCurrent, SDL_GetWindowFromID and SDL_GL_GetCurrentWindow-level APIs. Hide SDL calls for target create/destroy/present behind one small class to ease a later SDL3 port.

## World drawers

### WRD-01 - Grass block visibility and ground-texture streaming are computed in the Update stage for the player camera
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** CAM-09; Per-view state ownership model for terrain-side caches; ReadMap::GridVisibility(nullptr) culls against the last camera that updated its frustum; No per-view identity to key per-frame caches, flags and tokens on

CGrassDrawer::Update builds the visible grass block lists from the PLAYER camera and VISCUL once per frame (only when the PLAYER camera moves). Window 2 reuses the same lists. SMF texture streaming (default off) also follows the active camera.

*Why it breaks:* The claimed mechanism is right for grass. Corrections:

(a) The near/far lists, the sort and the near-billboard alpha come from the Update-time active camera (GetGrassBlockCamDist uses camera->GetPos(), GrassDrawer.cpp:67), while the frustum cull in GridVisibility(nullptr) uses VISCUL, i.e. the last camera Update()d in the previous frame. Update and the cull can therefore disagree, and with two views the cull is probably window 2's.
(b) The far-billboard distance-fade alpha is baked into the cached per-block vertex arrays (gl_Normal.z, GrassVertProg.glsl:102) by whichever view draws first after Update (:449-488).
(c) The texture-streaming half is opt-in and only mis-selects mips for window 2. It does not unload squares, because BindSquareTexture refreshes drawFrame in any normal draw.

*Evidence:*
- `rts/Rendering/Env/GrassDrawer.cpp:518` Update uses GetCamera(CAMTYPE_PLAYER); oldCamPos/oldCamDir gate (514-546)
- `rts/Rendering/Env/GrassDrawer.cpp:570` Draw uses cached blockDrawer lists (561-584)
- `rts/Map/SMF/SMFGroundTextures.cpp:348` DrawUpdate uses the active camera for mip selection; unloading after 120 frames (:369-373); streaming config default false (:46)
- `rts/Rendering/Env/GrassDrawer.cpp:175` static CGrassBlockDrawer blockDrawer holds the cached lists
- `rts/Rendering/Env/GrassDrawer.cpp:533` GridVisibility(nullptr, &blockDrawer, ...)
- `rts/Rendering/Env/GrassDrawer.cpp:539` lists sorted by distance to the Update-time camera
- `rts/Rendering/Env/GrassDrawer.cpp:445` billboard VAs bake distance alpha and are cached per block
- `rts/Rendering/Env/GrassDrawer.h:118` oldCamPos/oldCamDir/lastVisibilityUpdate single-camera state
- `rts/Rendering/Env/GrassDrawer.cpp:166` lastSeen = drawFrame in DrawFarQuad
- `rts/Rendering/Env/GrassDrawer.cpp:449` updateBillboards one-shot rebuild; g.lastFar=drawFrame at 458
- `rts/Rendering/Env/GrassDrawer.cpp:553` GC compares lastSeen with drawFrame-50

*Direction:* Prototype, S effort. (1) Draw grass in the primary view only, with a view-is-primary gate at WorldDrawer.cpp:355. (2) Make Update independent of VISCUL by passing an explicit primary-view camera to readMap->GridVisibility(cam, ...) at GrassDrawer.cpp:533. The caller must call CalcFrustumLines first, per SMFReadMap.cpp:848-850. This only works once per-view cameras exist; if window 2 reuses the PLAYER camera slot, the PLAYER state is clobbered by the last draw. Use the same camera for blockDrawer.cx/cy and for GetGrassBlockCamDist, so Update uses one camera consistently. (3) Keep SMFTextureStreaming off, or force it off when more than one view exists.

Full solution, M effort. Remove the camera dependence from the cached data. The grass shader already receives camPos (GrassVertProg.glsl:9) and uses gl_Normal.z only as a CPU-baked fade (:102), so compute the distance fade in the shader. That makes the far VAs view-independent and drops the updateBillboards one-shot rebuild. Then either keep per-view state (oldCamPos, oldCamDir, lastVisibilityUpdate, the three lists, farnearVA) in a small struct keyed by view id, or rebuild the lists inside Draw per view. Treat the 120-frame unload of ground textures as a union over views, which BindSquareTexture's stamping already approximates.

> verifier A (corrected): The core claim holds. Grass block lists are built once per frame in the Update stage into a single static blockDrawer, and every Draw() reuses them regardless of view. There is no per-view state, so window 2 only gets grass derived from view 1. It is a visible-glitch risk, not a crash: re-drawing the client-side VAs is harmless (CVertexArray::DrawArrays does not reset the array), and grass only exists on maps with a "grass" info map when GrassDetail is non-zero and the GPU is not AMD (GrassDrawe

> verifier B (corrected): The core mechanism is real and I confirmed it in the code. CGrassDrawer::Update (GrassDrawer.cpp:514-558) runs once per frame from eventHandler.Update() (Game.cpp:1424). It keys rebuilds off the PLAYER camera only (:518-527) and fills one static blockDrawer (:175). Draw (:570-584) replays those lists for every view. The prototype will not crash and will not raise GL errors, because the far-grass vertex arrays are client-side arrays (VertexArray.cpp:344-363) and the shader gets camPos/camUp/camRi

### WRD-02 - Projectile sort distances, billboard fill and fx shader uniforms assume the player camera
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** OBJ-01

Projectile alpha sorting uses per-projectile sort distances computed at Update time from the PLAYER camera. fx shader uniforms (camPos, fog) are read from the PLAYER camera, while CPU-built billboard vertices read the active camera.

*Why it breaks:* Three separable sub-issues. (a) Sort key: DrawAlpha builds the key at ProjectileDrawer.cpp:910 from p->GetSortDist(camera->GetCamType()), and that distance was cached in UpdateDrawFlags (:515, :531, :538) from camPlayer/camUWRefl/camShadow state at Update time. It is stale for any second view that does not overwrite the PLAYER camera before Update. A view with its own camType index >= 3 reads past the end of sortDist[3] (undefined behaviour). (b) Fog uniforms: camPos and fogParams come from GetCamera(PLAYER) at :970-982 (DrawAlpha) and :1206-1215 (DrawGroundFX). The vertex shader computes fogDist = length(pos - camPos) at ProjFXVertProg.glsl:55. A particle near window 2 but far from camera 1 gets the wrong fog tint. fogParams also scales by the PLAYER camera's far plane (Camera.h:203). Billboard corners use the active camera (EmgProjectile.cpp:95-98, about 45 uses under Particles/ and about 64 under Sim/Projectiles/), so they are consistent per view. The fog mismatch only exists if the window camera is not in the PLAYER slot. (c) Draw flags (:514-528) are the duplicate of the dependency. UWREFL and SHADOW flags and sort distances (:530-548) have the same problem: they read the reflection and shadow cameras' state from before this frame's per-view reflection and shadow setup.

*Evidence:*
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:514` camPlayer->InView then SetSortDist(PLAYER...) at :515 and flags :517-527
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:900` sortCamType = camera->GetCamType(); ParticleSortKey uses GetSortDist(camType) (:69-73)
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:980` fxShader camPos/fogParams from GetCamera(PLAYER) (:970)
- `rts/Sim/Projectiles/WeaponProjectiles/EmgProjectile.cpp:95` billboard quad uses camera->GetRight()/GetUp() (active camera); also Flare, Fire, Missile, Starburst, BeamLaser etc.

*Direction:* 1. Solve the per-view flag problem once, in the dependency item, not here. Two options: re-run the flag loop per view with the window camera swapped into the PLAYER slot, or compute union flags and add a view check at draw time. If the loop is re-run per view, split it. drawPos and the previousDrawFlag/drawFlag handover (:502-505) must run once per frame, because previousDrawFlag is read by Lua GetRenderObjectsDrawFlagChanged (LuaUnsyncedRead.cpp:2365) and a second pass would overwrite it with window 1's flags. Only the InView/AddDrawFlag part should be per view. A union-flags version needs no draw-time InView check in DrawAlpha or DrawOpaque; the GPU clips the extras, at a cost of wasted quad fill.
2. Sort key: delete the cached sortDist[3] array. In the DrawAlpha fill loop at :905-912, compute camera->ProjectedDistance(p->drawPos) + p->sortDistOffset (drawPos is already view-independent), then feed it to DistanceSortKey/ParticleSortKey. This is effort S. It removes the out-of-bounds hazard for new camera types and the stale Update-time value. Only one dot product per drawn sorted particle is added to the serial loop.
3. Fog uniforms: use the window's primary camera, not `camera`. The active camera is the mirrored UWREFL camera during reflection and refraction passes, so a plain swap to `camera` would change today's behaviour (fog measured from the player position) in those passes. If the prototype puts the window camera in the PLAYER slot, no change is needed at :970 and :1206, but the dependency on that design should be stated. Also fix the DrawGroundFX site.
4. Sibling issue found while checking: the NanoParticles gather cache is keyed on drawFrame and camType only (NanoParticleRenderer.cpp:660-678). A second view with the same camType in the same frame reuses the visible set culled against window 1's camera. ProjectileDrawer's alphaRange reuse (:887-889, :954) is also keyed on drawFrame only, but it is safe as long as each view runs its full pipeline in sequence (the below-water pass refills and re-saves the range before the refraction and above-water passes consume it, WorldDrawer.cpp:419-459). It breaks if passes from different views are interleaved.

> verifier A (confirmed): Every cited mechanism holds in the current tree. UpdateDrawFlags runs once from CWorldDrawer::Update (WorldDrawer.cpp:227). It uses only CCameraHandler::GetCamera(CAMTYPE_PLAYER) (ProjectileDrawer.cpp:494). It sets SO_ALPHAF/OPAQUE/REFRAC flags and SetSortDist(PLAYER, ...) only inside `if (camPlayer->InView(...))` (:514-528). ShouldDrawProjectile (:715-725) tests only those stored flags, so a particle outside camera 1's frustum is skipped by every later draw pass, whatever camera is active. Draw

> verifier B (corrected): The mechanisms are real: ProjectileDrawer.cpp:514-515 caches sortDist[PLAYER] at Update time, :900/:910 sorts with it, and :970-982 and :1206-1215 take fog from GetCamera(PLAYER). But the claim bundles three pieces of different weight. (1) The InView flag piece (:514-528) is the same defect as the dependsOn item, and it is what actually drops particles in window 2, so it should not be counted twice here. (2) The sort-order piece is a visual glitch only. ProjectedDistance is forward.dot(pos - cam

### WRD-03 - UpdateWater runs mid-pass inside DrawAlphaObjects and mixes global and per-view work
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** LOP-03

UpdateWater is called from DrawAlphaObjects after the opaque pass, and AdvWater's UpdateWater also redraws a purely frame-global bump animation texture.

*Why it breaks:* UpdateWater is invoked from DrawAlphaObjects between the below-water and above-water alpha passes (WorldDrawer.cpp:437), so its nested RTT passes (IWater::DrawReflections/DrawRefractions, IWater.cpp:139-223) run in the middle of the view's main pass. Each water mode then ends by restoring "the main target" from process-global state. (a) FBO::Unbind(), which is a hard glBindFramebufferEXT(0) (FBO.cpp:335), is called at AdvWater.cpp:353, BumpWater.cpp:571 and DynWater.cpp:422. This is fine only if the view target is a window's default framebuffer. If the target is an offscreen FBO, the rest of the pass (above-water alpha, water->Draw, DrawWaterPost) lands on FB 0. (b) prvCam->Update() / camera->Update() rederives matrices, aspect and viewport from globalRendering->viewSize*/viewPos*/aspectRatio (Camera.cpp:144-155), and BumpWater.cpp:572, BumpWater.cpp:986 and DynWater.cpp:519 call globalRendering->LoadViewport(). The restored per-view camera and viewport are therefore overwritten mid-pass with global-window values. This is correct only if globalRendering is swapped per view. (c) Mixed global and per-view work: for AdvWater, only the 128x128 bump animation (AdvWater.cpp:263-326) is frame-global and is redone per UpdateWater call, which is cheap and idempotent. For DynWater the issue is camera-centred shared state. DynWater::Update computes camPosBig from camera->GetPos() once per sim frame (DynWater.cpp:434-435), and UpdateWater->DrawHeightTex recomputes camPosX/camPosZ from camera->GetPos() (DynWater.cpp:738-739). The wave clipmap is therefore centred on one camera, and window 2 samples it with a different eye position. (d) Screen-copy modes (RefractWater.cpp:48,51,65 and BumpWater.cpp:906,912) size and copy the refract/depth textures from globalRendering->viewSizeX/Y and viewPos. These need a per-view size and offset, and must stay in Draw because they need the main framebuffer content.

*Evidence:*
- `rts/Rendering/WorldDrawer.cpp:436` water->UpdateWater(game) mid alpha pass, then water->Draw (439)
- `rts/Rendering/Env/AdvWater.cpp:263` bump animation (frame-global) rendered inside UpdateWater
- `rts/Rendering/Env/BumpWater.cpp:561` UpdateWater: refraction/reflection RTT then FBO::Unbind + LoadViewport
- `rts/Rendering/Env/DynWater.cpp:399` UpdateWater: DrawHeightTex + refraction + reflection + Unbind
- `rts/Rendering/Env/RefractWater.cpp:62` screen copy inside Draw, needs main FB content

*Direction:* Do not introduce a new UpdateShared(); reuse the existing IWater::Update() once-per-sim-frame hook. Move CAdvWater's bump-animation FBO pass there (or guard it by gs->frameNum), and make DynWater's camPosBig/camPosX/camPosZ centring explicit about which camera it serves, for example primary view only or per-view clipmaps. Keep UpdateWater as the per-view RTT stage, and call it from the view pass with an explicit view-context parameter that carries target FBO/FB id, viewport and camera. Replace the hard-wired FBO::Unbind(), globalRendering->LoadViewport() and camera->Update() restores with restores to that view context. Moving the call before the opaque pass is optional hygiene and not needed for the 2-window prototype. Make the screen-copy textures per-view sized. Prototype shortcut: render windows sequentially, swapping globalRendering view fields and the active camera per window, so the existing mid-pass restore stays correct.

> verifier B (corrected): The code facts are real, but they do not make the 2-window prototype fail, and the proposed refactor is not needed for it. (1) The mid-pass placement works per view as written. WorldDrawer.cpp:436-439 runs UpdateWater and then Draw back to back, so each view gets its own reflection/refraction texture generated from that view's active camera just before it is sampled. This is the produce-then-consume pairing a multi-view frame needs. In the default AdvWater mode (IWater.cpp:28 default is REFLECTI

> verifier A (corrected): The call site is real, but the mechanism is overstated in places and the fix is partly already in the tree. UpdateWater is called mid-pass at WorldDrawer.cpp:437, with a ZoneScopedN at 436 (the claim cites 436 for the call), and Draw follows at 439. AdvWater::UpdateWater really does render the 128x128 bump animation from gs->frameNum only (AdvWater.cpp:263-326). It is the default mode: CONFIG Water defaults to WATER_RENDERER_REFLECTIVE (IWater.cpp:27-28, mapped to CAdvWater at :105). The refract

## Lua API & callins

### LUA-01 - Lua-created GL objects carry no context identity
**Severity:** risk - **Effort:** M - **Status:** unverified - **Depends on:** DSN-02

Lua FBOs, VAOs, queries, display lists and image-unit bindings are created in whichever context runs the Lua callin and freed at handle shutdown, with no record of which context owns them.

*Why it breaks:* gl.CreateFBO, the FBO and RBO behind render-to-texture textures, LuaVAOImpl::CondInitVAO, gl.CreateQuery, gl.CreateList and gl.BindImageTexture all execute during Lua callins (including DrawWorld in the 3D path). Under A they just work. Under B an object created during one window's callin is invalid or aliased in the other, and the handle's cleanup deletes names in the wrong context. Display lists, textures, RBOs and shaders are shared and fine. CondInitVAO only compares VBO ids, not context. The prototype still runs LuaRules/LuaGaia DrawWorld for the 3D view, so Lua involvement for window 2 needs a decision.

*Evidence:*
- `rts/Lua/LuaFBOs.cpp:502` glGenFramebuffersEXT at LuaFBO creation
- `rts/Lua/LuaTextures.cpp:117` fbo=true Lua textures create a framebuffer once (117-118)
- `rts/Lua/LuaVAOImpl.cpp:233` VAO re-created only when VBO ids change
- `rts/Lua/LuaOpenGL.cpp:6348` gl.CreateList glGenLists/glNewList
- `rts/Lua/LuaOpenGL.cpp:6712` gl.CreateQuery glGenQueries
- `rts/Rendering/WorldDrawer.cpp:323` eventHandler.DrawWorld runs Lua draw callins inside the world pass
- `rts/System/SpringApp.cpp:817` LuaOpenGL::Free/Init on reload
- `rts/Lua/LuaVAOImpl.cpp:224` CondInitVAO: lazily make_unique<VAO>() and cache, no context check (224-233)
- `rts/Lua/LuaOpenGL.cpp:6711` gl.CreateQuery glGenQueries; global occlusionQueries vector (179)
- `rts/Lua/LuaOpenGL.cpp:258` one-time glEnable(GL_VERTEX_PROGRAM_POINT_SIZE) in Init (256-267)

*Direction:* Decide whether Lua draw callins run for secondary targets. For A nothing is needed. For B tag each Lua-owned GL object with its creating context and run Lua only in the owning context, or duplicate FBO/VAO/query state per context.

### LUA-02 - ViewResize and geometry getters describe only the primary window; SDL window events are not routed per window
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** LUA-11

ViewResize carries only primary-window geometry and is also consumed by engine singletons on the same event bus. SDL window events are handled without checking windowID. A resize or minimize of window 2 is therefore treated as the main window's, recreating water and stopping drawing.

*Why it breaks:* An event for window 2 (resize, minimize or hide) is treated as the main window's. A resize re-reads primary geometry, which is unchanged, then runs a pointless water kill/recreate and a ViewResize broadcast. A minimize or hide of window 2 sets the global active=false, so CGame::Draw sleeps 10ms and skips drawing both windows, with a forced frame at most every 30s. FBO::GLContextLost is also called if the primary is in fullscreen. Today there is only one window, so the issue arises only once a second window exists. ViewResize carries primary-only geometry, as claimed.

*Evidence:*
- `rts/System/SpringApp.cpp:1135` SIZE_CHANGED handler (1135-1157): no windowID filter anywhere in the file (switch on event.window.event at 1099)
- `rts/System/SpringApp.cpp:1179` MINIMIZED/HIDDEN sets globalRendering->active=false (1185)
- `rts/Game/Game.cpp:1105` ResizeEvent recreates water (1115-1119) and broadcasts ViewResize (1127)
- `rts/Lua/LuaHandle.cpp:2579` ViewResize callin table built only from globalRendering screen/window/view fields (2588-2606)
- `rts/Rendering/DepthBufferCopy.cpp:55` engine consumer of the same ViewResize bus; also GroundDecalHandler.cpp:1693
- `rts/Game/Game.cpp:1471` CGame::Draw returns early when !globalRendering->active

*Direction:* As the minimum for the prototype, filter MainEventHandler on event.window.windowID == the primary window id. Handle window 2 SIZE_CHANGED separately by updating only its target's viewport. Make active per target, or skip the active=false toggle for secondary windows. The Lua ViewResize viewID field and the split of the engine-internal notification can come later, as full-product work.

> verifier B (corrected): The core mechanism is real. MainEventHandler (SpringApp.cpp:1095-1198) switches on event.window.event with no windowID check, and a grep for windowID under rts/System finds nothing. So any window's SIZE_CHANGED runs UpdateGLConfigs/UpdateGLGeometry/InitGLState and activeController->ResizeEvent(). Any window's MINIMIZED/HIDDEN sets globalRendering->active=false (1185), and CGame::Draw returns early when !active (Game.cpp:1471). SHOWN/RESTORED/MAXIMIZED on any window also sets active=true, and wit

> verifier A (corrected): The mechanism holds. SpringApp::MainEventHandler (rts/System/SpringApp.cpp:1095-1198) never checks event.window.windowID, and a grep of rts/*.cpp finds no windowID use at all. SIZE_CHANGED (1135-1157) re-reads the primary geometry and calls activeController->ResizeEvent(). CGame::ResizeEvent (rts/Game/Game.cpp:1105-1129) kills and recreates the water at 1116-1118 and calls eventHandler.ViewResize() at 1127. MINIMIZED/HIDDEN set globalRendering->active=false at 1185, and CGame::Draw returns early

### LUA-03 - Lua camera API and draw callins have no target concept
**Severity:** full - **Effort:** L - **Status:** unverified - **Depends on:** DSN-01; LUA-11

Spring.GetCameraState/SetCameraState, position/direction/FOV/frustum getters, WorldToScreenCoords, TraceScreenRay, offset and dolly functions all act on the single active camera or controller. gl.GetViewRange(default) returns the placeholder slot.

*Why it breaks:* Reads go through `camera` (active) and writes through camHandler's single current controller. Spring.SetCameraOffset writes posOffset/tiltOffset on the active camera. Lua GL matrix and uniform paths (LuaOpenGLUtils, LuaMaterial) read `camera`, correct per view only if the active pointer is swapped during each view's draw callins. Camera events (CameraPositionChanged/RotationChanged) are emitted from the one handler. TraceScreenRay bounds-checks against globalRendering->viewSize. DrawWorld* callins do not say which view is rendering, so widgets caching camera state per Update break.

*Evidence:*
- `rts/Lua/LuaUnsyncedRead.cpp:3008` GetCameraState via camHandler->GetState (:3013); pos/dir/rot/fov :3083-3126; frustum planes :3151-3163
- `rts/Lua/LuaUnsyncedRead.cpp:3261` TraceScreenRay uses globalRendering->viewSizeX/Y and camera->CalcPixelDir (:3273); WorldToScreenCoords :3184
- `rts/Lua/LuaUnsyncedCtrl.cpp:1265` SetCameraOffset writes camera->posOffset/tiltOffset; SetCameraState :1311 and Dolly :1330+ via camHandler
- `rts/Lua/LuaOpenGLUtils.cpp:194` matrices 'view/projection' from active camera (also LuaMaterial.cpp:761-773, LuaOpenGL.cpp:1024,1070)
- `rts/Lua/LuaOpenGL.cpp:1252` gl.GetViewRange default CAMTYPE_ACTIVE -> GetCamera(5) returns the placeholder slot, not the active camera
- `rts/Game/CameraHandler.cpp:286` eventHandler.CameraRotationChanged/PositionChanged emitted from the single handler (:292-296)
- `rts/Lua/LuaUnsyncedRead.cpp:3010` GetCameraState via camHandler (3010-3072); position/direction/rotation/FOV/vectors from camera at 3081-3166
- `rts/Lua/LuaUnsyncedRead.cpp:3216` TraceScreenRay uses globalRendering viewPos/viewSize (3222-3263) and camera->CalcPixelDir (3273); WorldToScreenCoords 3184; GetPixelDir 3331
- `rts/Lua/LuaUnsyncedCtrl.cpp:1243` SetCameraTarget drives camHandler current controller (1243-1245); SetCameraOffset writes camera->posOffset (1265-1270); SetCameraState 1309-1312
- `rts/Game/CameraHandler.cpp:88` camHandler singleton and static camera array (94)

*Direction:* Default behaviour unchanged (primary view). Inside Draw* callins the active camera is the rendering view's camera. Add an optional target/viewId argument to camera getters, setters and TraceScreenRay/WorldToScreenCoords, plus a callin or query exposing the current view id. Fix the GetViewRange(ACTIVE) placeholder bug. Keep changes additive per coding-agents/BACKWARDS_COMPATIBILITY.md.

### LUA-04 - Lua draw callins and draw-flag API carry no view identity
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** Draw-flag bitmask encodes visibility for a single player camera

Each drawer pass invokes Lua callins (Draw*UnitsLua, DrawUnitsPostDeferred, DrawUnit) and Lua can read drawFlag/isIcon through Spring.GetRenderUnits, GetUnitDrawFlag, IsUnitIcon, none of which say which view they apply to.

*Why it breaks:* Callins and draw-flag state carry no VIEW (window/render-target) identity. The claim that Lua cannot tell which camera is in use is too strong. Spring.GetCameraPosition/Direction/Rotation/FOV/Vectors read the `camera` macro (Camera.h:316 -> CCamera::GetActive() -> CCameraHandler::GetActiveCamera, CameraHandler.cpp:126). That is the engine's active-camera slot, which the engine already switches per pass (PLAYER, UWREFL, SHADOW). Gadgets therefore already learn the camera from active-camera queries, so the API gap is only view/window identity and per-view draw state.

The real problems, in order of impact:
1. drawFlag, previousDrawFlag and the icon bit are computed once per frame in the Update phase from the fixed camera-type set PLAYER..SHADOW (UnitDrawerData.cpp:430-437, called via UpdateCommon at ModelDrawerData.h:221-230). They are then read by GetRenderUnits, GetRenderFeatures, GetUnitDrawFlag, GetFeatureDrawFlag, IsUnitIcon and the *DrawFlagChanged diffs. With two views they can only describe one view (or a union). A gadget calling Spring.GetRenderUnits(mask) from a callin running for window 2 gets window 1's visible set. That is a visible-wrong-output risk only for Lua that uses these APIs.
2. The uniform drawFlag byte is uploaded once per frame as part of the model uniform SSBO (ModelDrawerData.h:204-207, ModelsDataUploader.cpp:352), not per view. A per-view value would need N copies of the uniforms or a separate per-view visibility buffer. This is more than a Lua-API tweak, and the claim's "update the uniform drawFlag per view" understates it.
3. Draw callins (DrawOpaqueUnitsLua, DrawAlphaUnitsLua, DrawShadowUnitsLua, DrawUnit, DrawUnitsPostDeferred) simply repeat per view. Gadget-side per-frame caches keyed on callin order break, but that is a gadget-compat concern rather than an engine bug.

This problem is the Lua-API consequence of the single-camera drawFlag problem it dependsOn. It is not a prototype blocker: window 2 may show wrong gadget-drawn effects, but nothing crashes and no GL error is raised.

*Evidence:*
- `rts/Rendering/Units/UnitDrawer.h:141` Lua callins invoked from every pass
- `rts/Lua/LuaUnsyncedRead.cpp:2319` GetRenderObjects filters on drawFlag; Spring.GetRenderUnits at 2433
- `rts/Lua/LuaUnsyncedRead.cpp:1901` Spring.IsUnitIcon reads the shared icon bit (1912)
- `rts/Rendering/Common/ModelDrawerData.h:207` drawFlag exported to shaders via ModelUniformData
- `rts/Rendering/LuaObjectDrawer.cpp:528` DrawUnitsPostDeferred notified per pass

*Direction:* Make the drawFlag problem (per-view visibility) the root fix. Then expose a small view identity to Lua. For example, add Spring.GetCurrentView(), valid inside draw callins, and make the active-camera queries follow the current view. Do not add a new callin argument: the existing positional booleans are API-stable, and extra args would need the BACKWARDS_COMPATIBILITY review. Document that GetRenderUnits, GetUnitDrawFlag and IsUnitIcon refer to the primary view (or add an optional viewID parameter) until per-view flags exist. For the prototype it is enough to document 'primary view only' and not fire gadget draw callins for secondary windows, or fire them with the active camera swapped.

> verifier A (corrected): The cited code does what is claimed. The Lua unit/feature draw callins are forwarded on every pass, and their arguments are only pass booleans (UnitDrawer.h:141-149, FeatureDrawer.h:58-66, ModelDrawer.h:360/389/443, LuaHandle.cpp:3128-3150). DrawUnitsPostDeferred fires once per deferred pass (LuaObjectDrawer.cpp:528-534). The draw-flag APIs read the single per-object byte (LuaUnsyncedRead.cpp:1387/1720/1912/2329/2365), and drawFlag is copied into ModelUniformData (ModelDrawerData.h:207, ModelsMe

### LUA-05 - Lua GL4 widgets assume one engine UBO state and per-context VBO/VAO bindings
**Severity:** full - **Effort:** L - **Status:** corrected - **Depends on:** none

Lua DrawWorld-type callins and widget-owned VBO/VAO/UBO/SSBO objects assume the engine UBO describes the view being drawn and that their buffers are bound once.

*Why it breaks:* Lua DrawWorld* callins (WorldDrawer.cpp:323, 367) run inside the per-view world draw. Their GL4 shaders read camera data from the shared UniformConstants UBO, which Game.cpp:1464 binds once per draw frame. With N views, the UBO contents must be refreshed and rebound before each view's Lua callins. Lua-owned buffers use the fixed index ranges at LuaVBOImpl.h:153-154 (UBO above 5, SSBO above 3). Those indices are global across views, which is fine under shared contexts. VAOs and FBOs built by widgets are not shareable across separate GL contexts, which only matters for option B. DrawGenesis (Game.cpp:1468) and DrawScreen* are once-per-frame or per-GUI callins and are out of scope for the prototype.

*Evidence:*
- `rts/Lua/LuaVBOImpl.h:153` Lua-managed binding index ranges
- `rts/Game/Game.cpp:1468` eventHandler.DrawGenesis runs after the single Bind()
- `rts/Rendering/WorldDrawer.cpp:323` eventHandler.DrawWorld() inside the per-view world draw

*Direction:* Make the engine UBO per-view first (a dependency owned by the engine-UBO problem). Then dispatch DrawWorld* with that view's UBO bound. Expose a view id to Lua. Decide shared-context (option A) or per-context object creation (option B). Treat the Lua-specific work as a follow-up to the engine UBO work.

> verifier A (corrected): Cited code matches. Game.cpp:1464 binds the per-frame UniformConstants UBO once, and DrawGenesis runs at 1468. WorldDrawer.cpp:323 calls eventHandler.DrawWorld inside the world draw. LuaVBOImpl.h:153-154 reserve fixed uboMinIndex and ssboMinIndex binding indices for Lua. The problem is real but forward-looking. I did not check where the UBO contents are updated, so the per-view staleness claim rests on the Game.cpp:1464 bind-once pattern only. The mechanism needs two fixes. First, the LuaVBOImpl

### LUA-06 - Lua map G-buffer textures and DrawGround callins have no target identity
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** Deferred and combined terrain passes use a primary-sized G-buffer and hard-reset viewport and FBO

Lua reads the ground G-buffer through named textures bound to the single GeometryBuffer, and the five DrawGround callins fire once per ground draw with no way to tell which target is drawing.

*Why it breaks:* Lua gets the ground G-buffer only through named textures bound to the single CSMFGroundDrawer::geomBuffer ($map_gb_* and $map_gbuffer_* -> LuaOpenGLUtils.cpp:610-615). Its size is tied to primary globalRendering->viewSize (GeometryBuffer.cpp:208-211, reported to Lua at LuaOpenGLUtils.cpp:975). The DrawGroundPre/PostForward and Pre/Deferred/PostDeferred callins take no arguments (CLuaHandle DRAW_CALLIN macro, LuaHandle.cpp:2789-2794). They fire from DrawDeferredPass and DrawForwardPass, so they would run once per target-draw (and per water or reflection pass, as they already do) with no way for a handler to tell which target or view is active. Inside a target's own ground draw the G-buffer is freshly rewritten, so in-pass reads are consistent. Stale or cross-target content is only seen by Lua that reads the map G-buffer textures before the ground draw of the current target. That includes DrawGenesis (Game.cpp:1468) and DrawWorld (WorldDrawer.cpp:323), which see the previous frame's last-drawn target. By default none of the five callins fire: they need AlwaysSendDrawGroundEvents or an active Lua map shader, and the Deferred and PostDeferred ones also need their AllowDrawMap* configs (all default false). The engine UBO is built from the PLAYER camera once per frame (UniformConstants.cpp:167-171) and is consumed by Lua shaders too, but that is a separate engine-level item. Spring.SetMapShader being global (SMFGroundDrawer.cpp:511-515) is expected behavior, not a defect.

*Evidence:*
- `rts/Lua/LuaOpenGLUtils.cpp:610` map G-buffer textures bound from the single ground geometry buffer
- `rts/Map/SMF/SMFGroundDrawer.cpp:268` DrawGroundPreDeferred dispatched inside the ground draw
- `rts/Map/SMF/SMFGroundDrawer.cpp:335` DrawGroundPreForward dispatched inside the forward pass
- `rts/System/EventHandler.h:294` five DrawGround* callins
- `rts/Rendering/UniformConstants.cpp:167` per-frame matrix UBO built from the PLAYER camera

*Direction:* Keep this as a "full" Lua API/design item. For the 2-window prototype, simply suppress the DrawGround* dispatch (and ideally Lua draw callins) for secondary targets, or keep them primary-only. Longer term, add a target/view id argument to the draw callins, or a Spring.GetDrawingView-style query. Key the $map_gb_* and $model_gb_* named textures to the active target, or document them as valid only inside the current target's ground-draw window. Resolve the actual G-buffer sizing and viewport reset under the dependsOn claim.

> verifier A (corrected): The code facts check out: the $map_gb_*/$map_gbuffer_* names resolve to the one ground geometry buffer (LuaOpenGLUtils.cpp:610-615 through readMap->GetGroundDrawer()->GetGeometryBuffer(), with a single CSMFGroundDrawer::geomBuffer member). The five callins are dispatched without arguments at SMFGroundDrawer.cpp:269, 286, 304, 313, 336 and 346 (the CLuaHandle DRAW_CALLIN macro calls RunCallIn with 0 args). The UBO is built from the PLAYER camera at UniformConstants.cpp:167-171, once per frame (Ga

### LUA-07 - Lua RenderToTexture, gl.Viewport, gl.Scissor and screen-matrix helpers assume the main window
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** Viewport restore idiom resets to the main window rect (19 LoadViewport sites)

Lua's own state handling is partly robust (FBO scopes query and restore the current FBO) but several paths hard-code window 1: gl.Scissor offsets by viewPos, screen matrices come from globalRendering, and RenderToTexture restores FBO 0.

*Why it breaks:* Verified, with narrower scope than stated. (a) gl.Scissor calls glScissor(x + viewPosX, y + viewPosY, ...) (LuaOpenGL.cpp:3272). viewPos is the dual-screen sub-rectangle offset inside one window and is 0 when DualScreenMode is off (GlobalRendering.cpp:1433-1443, 1516, 1529). It is a single global, so a per-target view rect is needed. It has no effect on the 2-window prototype. gl.Viewport (3300) passes raw coordinates through and does not add viewPos, so the two are already inconsistent. Lua viewport changes do not leak out of a callin, because EnableCommon does glPushAttrib(AttribBits), which includes GL_VIEWPORT_BIT, and DisableCommon pops it (LuaOpenGL.cpp:593-609, 624-626) when safeMode is true (default, line 102). (b) SetupScreenMatrices and ResetScreenMatrices load globalRendering->screenProjMatrix and screenViewMatrix (989-993, 1088). These are not a plain window ortho. UpdateScreenMatrices (GlobalRendering.cpp:1615-1647) builds an off-axis frustum from screenSize, winPos and viewPos, so the matrices encode the window's desktop position. A second window at a different desktop position needs its own pair. Only the Screen, ScreenEffects, ScreenPost and Menu draw modes use them. (c) RenderToTexture (4613-4631) restores FBO 0 unless in shadow mode. This is wrong today in the DRAW_WORLD_REFLECTION and DRAW_WORLD_REFRACTION modes, since BumpWater.cpp:1015 and 1033 bind reflectFBO and call DrawReflections, which reaches eventHandler.DrawWorldReflection at IWater.cpp:174. It is only correct for N windows if each window has its own context whose FBO 0 is its default framebuffer. It breaks if world rendering goes into an offscreen FBO and is blitted to windows. The viewport itself is saved and restored correctly through glPushAttrib(GL_VIEWPORT_BIT) at 4620 and 4629. (d) The world draw modes are already target-agnostic: ResetWorldMatrices uses the global camera's projection and view matrices (1070-1071), so they follow the active camera if that pointer is swapped per target. For the prototype's Lua DrawWorld callins from gadgets, none of (a) to (c) is triggered in the normal single-screen case.

*Evidence:*
- `rts/Lua/LuaOpenGL.cpp:3272` glScissor uses globalRendering->viewPos
- `rts/Lua/LuaOpenGL.cpp:989` screen matrices from globalRendering
- `rts/Lua/LuaOpenGL.cpp:4613` RenderToTexture saves FBO only in shadow mode
- `rts/Lua/LuaFBOs.cpp:250` FBO scopes query and restore the current FBO (robust)
- `rts/Lua/LuaContextData.h:118` MatrixStateTracker lives in Lua context data

*Direction:* Split it by when it matters. Prototype: nothing needed here, provided each window gets its own GL context (so FBO 0 stays correct) and the active-camera pointer is swapped per target, since ResetWorldMatrices already reads it. Product: (1) Save the current FBO unconditionally in RenderToTexture, as the FBO scopes already do. Better still, route the restore through a per-target "current default framebuffer" notion and fix the reflection and refraction case as a bug in its own right. (2) Replace the viewPos reads in gl.Scissor, gl.GetViewSizes and gl.DrawMiniMap with a per-target view rect. Decide whether gl.Viewport should also be target-relative, which fixes the Scissor/Viewport asymmetry. (3) Make screenProj and screenView per-target, recomputed from that window's winPos and screenSize. This is needed when per-target GUI or Lua DrawScreen arrives.

> verifier A (corrected): The cited lines are accurate in the current tree: Scissor adds viewPos (LuaOpenGL.cpp:3272), SetupScreenMatrices loads the globalRendering matrices (989-993), RenderToTexture saves the FBO only when drawMode == DRAW_WORLD_SHADOW and otherwise restores 0 (4613-4631), the FBO scopes query and restore the current FBO (LuaFBOs.cpp:249-270), and GLMatrixStateTracker lives in Lua context data and is a pure depth counter (MatrixStateTracker.h:20-62), so it is unaffected as claimed. The framing needs co

### LUA-08 - Lua window and GL API surface assumes one window
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** Primary-only window and context handles in CGlobalRendering; LUA-11

Lua can query and set only 'the' window (geometry, view geometry, display mode, title, icon, minimise) and call gl.SwapBuffers, which targets the primary. Nothing identifies a window or target.

*Why it breaks:* All of these Lua functions target the one primary window held in CGlobalRendering. Reads (LuaUnsyncedRead.cpp): GetViewGeometry returns globalRendering->viewSize/viewPos at lines 894-897, GetDualViewGeometry returns dualViewSize/dualViewPos at 913-916, GetWindowGeometry returns winSize/winPos/winBorder at 939-947, GetWindowDisplayMode calls SDL_GetWindowDisplayMode(globalRendering->GetWindow()) at 962, and GetScreenGeometry returns globalRendering->screenSize/screenPos at 1001-1004. GetScreenGeometry with an explicit displayIndex goes through GetDisplayBounds (1008), which uses GetCurrentDisplayIndex() = SDL_GetWindowDisplayIndex(sdlWindow) when no index is given (GlobalRendering.cpp:1389-1397).

Writes:
- SetWindowGeometry (LuaUnsyncedCtrl.cpp:5415-5431) does not touch an SDL window directly. CGlobalRendering::SetWindowPosHelper (GlobalRendering.cpp:1342-1366) only writes the global config keys WindowPosX/Y, XResolution/YResolution(Windowed), Fullscreen and WindowBorderless. CGlobalRendering::ConfigNotify, registered at 358-371, sets winChgFrame (1281), and the next frame SetWindowAttributes(sdlWindow) (1304) applies the keys to the primary window only. A second window would therefore be moved or resized by the same keys, or ignored.
- SetWindowMinimized and SetWindowMaximized call SDL_MinimizeWindow or SDL_MaximizeWindow on the member sdlWindow (GlobalRendering.cpp:1829-1844).
- SetWMCaption calls globalRendering->SetWindowTitle, which calls SDL_SetWindowTitle(sdlWindow) (1202-1214).
- SetWMIcon does not go through globalRendering methods. It calls WindowManagerHelper::SetIcon, which calls SetIconSurface(globalRendering->GetWindow(), bmp) (WindowManagerHelper.cpp:45).
- gl.SwapBuffers (LuaOpenGL.cpp:5344-5354) returns early unless the handle is PersistOnReload (a LuaMenu). Otherwise it calls globalRendering->SwapBuffers(true, true), which does SDL_GL_SwapWindow(sdlWindow) on the primary (GlobalRendering.cpp:703).

LuaIntro (LuaIntro.cpp:188-192, 205-207) and LuaMenu (LuaMenu.cpp:193-195, 228-230) expose the same functions. LuaMenu registers neither SetWindowMinimized nor SetWindowMaximized. EventHandler.h has a single arg-less ViewResize() at line 281, and the draw callins at 284-305 take no target argument.

Corrections to the original claim: "GetViewGeometry" was listed twice, and the second one should read GetDualViewGeometry; SetWindowGeometry is an indirect config write, not a direct window write; and SetWMIcon goes through WindowManagerHelper rather than globalRendering.

*Evidence:*
- `rts/Lua/LuaUnsyncedCtrl.cpp:5415` SetWindowGeometry -> globalRendering->SetWindowPosHelper (5425); minimized/maximized at 5438-5453
- `rts/Lua/LuaUnsyncedRead.cpp:962` display mode from GetWindow()
- `rts/Lua/LuaUnsyncedRead.cpp:1008` display bounds queries
- `rts/Lua/LuaOpenGL.cpp:5352` gl.SwapBuffers swaps the primary only
- `rts/Lua/LuaIntro.cpp:190` window APIs also exposed to LuaIntro and LuaMenu
- `rts/Lua/LuaUnsyncedCtrl.cpp:5551` SetWMCaption -> globalRendering->SetWindowTitle
- `rts/Lua/LuaUnsyncedRead.cpp:959` GetWindowDisplayMode uses globalRendering->GetWindow() (single SDL_Window)
- `rts/Lua/LuaUnsyncedRead.cpp:934` GetWindowGeometry returns the single window's geometry
- `rts/System/EventHandler.h:281` only ViewResize exists as a view-related event

*Direction:* Leave Lua unchanged for the prototype: the engine creates and owns the second window. Later add a render-target id to the Lua API. Options are an optional targetID parameter on the window and geometry functions, with the primary as default for backwards compatibility, or a Spring.GetRenderTargets / CreateRenderTarget family. SetWindowGeometry needs a per-target path that bypasses the global config keys. All SDL window calls need main-thread marshalling like the existing SetWindowTitle QueuedFunction path (GlobalRendering.cpp:1210-1213). Draw callins and ViewResize need a target argument or a queryable current target. gl.SwapBuffers needs a target or should stay primary-only.

> verifier A (corrected): The core claim holds in the current tree. Every Lua window, geometry and display-mode function and gl.SwapBuffers is hard-wired to the single globalRendering window, and there is no window or target identity anywhere in the Lua API or event surface. A repo-wide grep finds no RenderTarget, CreateWindow or windowID concept in rts/ outside rts/lib. Severity "full" is right. The 2-window 3D-only prototype creates its second window in the engine, so it needs no Lua window management. The prototype st

### LUA-09 - Lua-visible and Lua-created window-sized resources assume one screen
**Severity:** full - **Effort:** L - **Status:** corrected - **Depends on:** FBO-06

Lua sees the G-buffer textures, global view sizes and one ViewResize callin. Lua code in DrawWorld* callins runs inside every target's WorldDrawer::Draw but was written for window 1's sizes and bound resources.

*Why it breaks:* Lua Draw* callins are dispatched through CEventHandler's DRAW_CALLIN macro (rts/System/EventHandler.cpp:680-700). They would fire per render target, with LuaOpenGL::EnableDraw*/ResetDraw* setting up matrices from the global camera (LuaOpenGL.cpp:1070-1071). Lua resources (FBOs and textures from LuaTextures.cpp:117-125 and LuaFBOs, plus gl.CopyToTexture targets) are sized by the Lua script from window 1's gl.GetViewSizes. They therefore mismatch a differently sized target 2, and are invalid if target 2 uses a non-sharing GL context. The engine-side fact is only "callins fire per target". The size mismatch is a Lua-script assumption, not an engine bug, and the context-validity part reduces to the GL context-sharing decision.

*Evidence:*
- `rts/Lua/LuaOpenGLUtils.cpp:610` G-buffer named textures resolve to global buffers (610-622)
- `rts/Lua/LuaOpenGL.cpp:1232` gl.GetViewSizes returns globalRendering->viewSize
- `rts/Lua/LuaHandle.cpp:2579` ViewResize callin carries only primary window geometry (2588-2606)
- `rts/Lua/LuaTextures.cpp:108` Lua textures with fbo=true create FBO + depth RBO (117-125) using supportDepthBufferBitDepth
- `rts/Lua/LuaFBOs.cpp:649` ActiveFBO sets its own viewport and restores previous FBO (649-667)
- `rts/Lua/LuaOpenGL.cpp:4455` gl.CopyToTexture uses glCopyTexSubImage2D from the bound framebuffer (ReadPixels 6559, 6684)
- `rts/Rendering/WorldDrawer.cpp:323` eventHandler.DrawWorld inside Draw, hence once per target (DrawWorldPreUnit at 367)

*Direction:* Prototype: put one guard at the top of the DRAW_CALLIN macro in EventHandler.cpp. Optionally apply the same guard to DRAW_ENTITY_CALLIN (726-740) and the DrawWorldPreParticles / DrawOpaqueUnitsLua-style wrappers (1016-1056) so that secondary targets skip Lua draw hooks and return false (no skipEngineDrawing). It is safest to key the guard on a per-target flag such as 'isSecondaryTarget'.

For the prototype, also prefer a single shared GL context with SDL_GL_MakeCurrent per window rather than separate contexts. That keeps Lua FBOs and VAOs valid, and the engine's own FBOs and VAOs.

Full product: pass a target id and size to the world-draw callins, scope per-target Lua textures and FBOs, and fire ViewResize per target. Reclassify this as 'full' and fold the G-buffer part into the existing G-buffer problem.

> verifier A (corrected): The core mechanism holds in the current tree. Lua world-draw callins fire inside the shared per-frame draw path, and everything Lua can query or create is sized from, or bound to, a single primary view. Two parts of the claim are overstated: gl.ActiveFBO is not a defect, and the claim that Lua FBOs are invalid in a second context only holds under one design choice. Severity stays risk: the prototype will not crash, but it will show wrong output whenever the running game has widgets or gadgets us

> verifier B (corrected): The core fact is real: eventHandler.DrawWorld/DrawWorldPreUnit run inside CWorldDrawer::Draw (WorldDrawer.cpp:323, :367) and the other Lua world-draw callins fire from the drawers. So they will fire once per target. But this does not block the 2-window, 3D-only prototype, because the dispatch has a single choke point. All 21 world and screen Draw* callins go through the DRAW_CALLIN macro (EventHandler.cpp:680-700, instantiated at 703-723). It early-returns when the listener list is empty, so one

### LUA-10 - Lua-visible shadow/reflection textures and pass callins are not view-aware
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** none

Lua can bind $shadow/$reflection/$sky_reflection/$specular and receives DrawWorldShadow/Reflection/Refraction callins with no view identity.

*Why it breaks:* Lua draw callins (DrawWorld*, DrawWorldShadow, DrawWorldReflection, DrawWorldRefraction and the like) are dispatched per pass from engine code with no view/target identity (ShadowHandler.cpp:444, IWater.cpp:174 and 216, EventHandler.cpp:1016). With N views, each fires N times per frame. Spring.GetDrawFrame is identical for all views of one frame, because drawFrame is incremented once per CGame::Draw (Game.cpp:1277; LuaUnsyncedRead.cpp:1183), and the visFrame uniform has the same value (LuaMaterial.cpp:778). Gadgets that do once-per-draw-frame work, cache per-frame state in a callin, or want to skip or specialise secondary views therefore cannot distinguish the views. Named textures ($shadow, $reflection, $specular, $sky_reflection), the shadow matrix and shadowParams (LuaOpenGLUtils.cpp:192,556-561; LuaMaterial.cpp:769-770) are served from single global instances. That is correct for sequential per-view rendering, provided the engine regenerates the shadow map, water reflection and cubemaps before each view. Shadow generation and water reflection per view are separate engine-side problems. gl.GetShadowMapParams (LuaOpenGL.cpp:6872-6879) and shadowParams are compile-time constants and need no per-view variant.

*Evidence:*
- `rts/Lua/LuaOpenGLUtils.cpp:557` $shadow/$shadow color -> shadowHandler texture ids (546-552 cubemaps)
- `rts/Lua/LuaOpenGL.cpp:6874` gl.GetShadowMapParams from the global handler
- `rts/Rendering/Env/IWater.cpp:174` DrawWorldReflection callin, no view id (216 refraction)
- `rts/Rendering/LuaObjectDrawer.cpp:309` LOD factor from InShadowPass/DrawReflectionPass globals
- `rts/Rendering/WorldDrawer.cpp:249` GenerateIBLTextures runs CreateShadows once per frame (252-259); called from Game.cpp:1519
- `rts/Rendering/ShadowHandler.cpp:130` shadow params derived from CAMTYPE_PLAYER; ComposeLightMatrix(playerCam, light) at 493
- `rts/Rendering/Env/IWater.cpp:216` DrawWorldRefraction (216) and DrawWorldReflection (174) fired inside the water passes
- `rts/Rendering/Env/BumpWater.cpp:1026` reflection camera is CopyStateReflect of the active (player) camera (1026-1036)
- `rts/Lua/LuaOpenGL.cpp:6872` gl.GetShadowMapParams exposes the single shadow map's parameters

*Direction:* Downgrade to full/dsn. For the prototype, rely on sequential per-view rendering, where each view regenerates shadow, reflection and cubemap state before its draw and the global handlers hold the current view's state; no Lua change is needed. For the product, expose a view/target identity to Lua, for example a Spring.GetRenderViewID() query or an extra callin argument. Also decide drawFrame semantics (per frame, or per view-pass) and document that named textures and the shadow matrix always refer to the view currently being drawn. Do not add per-view GetShadowMapParams (the value is constant) or per-view $specular/$sky_reflection (they are global environment maps).

> verifier B (corrected): The 2-window 3D-only prototype does not fail, raise GL errors or show wrong output because of the Lua layer. The mechanism is partly wrong. What is real is a Lua API/design gap (no view identity in callins), which is not needed for the prototype.

1. The Lua accessors are thin wrappers over the same state the engine's own shaders use. The wrapped calls are `shadowHandler.GetShadowTextureID`/`GetColorTextureID` (LuaOpenGLUtils.cpp:556-561), `GetShadowMatrix` (LuaOpenGLUtils.cpp:192), `GetShadowMa

> verifier A (corrected): The cited code exists, but the claim overstates it. Several pieces are not per-target defects, and the one that is real is a Lua-API design gap that does not block the 2-window 3D-only prototype. Confirmed: $shadow/$shadow_color resolve to the single shadowDepthTexture/shadowColorTexture (LuaOpenGLUtils.cpp:556-561), the shadow matrix is served from the global shadowHandler (LuaOpenGLUtils.cpp:192, LuaOpenGL.cpp:1078, LuaMaterial.cpp:769), and DrawWorldShadow, DrawWorldReflection and DrawWorldRe

### LUA-11 - No Lua-visible render target identity in callins or queries
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** DSN-04; DSN-03; No per-view identity to key per-frame caches, flags and tokens on; OVL-01

No draw callin or query tells Lua which view is drawing, and the view size/geometry getters return only primary-window values. Widgets cannot size per-view resources, key caches or choose behaviour per view.

*Why it breaks:* No draw callin or Lua query exposes which view is being rendered. RunDrawCallIn calls RunCallIn(L, hs, 0, 0), so all DRAW_CALLIN-macro callins get zero arguments (LuaHandle.cpp:2773-2794). DrawScreen and DrawScreenEffects push only globalRendering->viewSizeX/Y (2986-2987). DrawUnit and DrawFeature push the id and GetDrawMode (LuaHandleSynced.cpp:230-231, 267-268). gl.GetViewSizes, Spring.GetViewGeometry and the mouse callins (LuaHandle.cpp:3470-3623) read the primary globalRendering view fields. gl.Scissor adds the primary viewPos (LuaOpenGL.cpp:3272). The camera getters read the global `camera` (LuaUnsyncedRead.cpp:3081-3128). ViewResize is only triggered for the main window (Game.cpp:1127). widgetHandler:DrawWorld and the other forwarders drop any engine arguments (cont/LuaUI/widgets.lua:1287-1307). Widgets therefore cannot size per-view resources, key caches or branch per view. If per-view globals are swapped during draw, the getters will return the current view's values, but there is still no view ID to key on.

*Evidence:*
- `rts/Lua/LuaHandle.cpp:2789` DRAW_CALLIN macro generates argument-less callins via RunDrawCallIn (2773-2794)
- `rts/Lua/LuaHandle.cpp:2986` DrawScreenCommon pushes globalRendering->viewSizeX/Y only
- `rts/Lua/LuaHandleSynced.cpp:231` drawMode passed to callins (204-231); no target
- `rts/Lua/LuaOpenGL.cpp:1232` gl.GetViewSizes returns globalRendering->viewSizeX/Y
- `rts/Lua/LuaUnsyncedRead.cpp:892` Spring.GetViewGeometry: 'Get main view geometry'
- `cont/LuaUI/widgets.lua:1287` widgetHandler:DrawWorld() calls w:DrawWorld() with no arguments
- `rts/Lua/LuaHandle.cpp:2579` ViewResize callin and geometry table (2579-2606)
- `rts/Lua/LuaHandle.cpp:3470` mouse coordinates converted with viewPos/viewSize (3470-3623)
- `rts/Lua/LuaUnsyncedRead.cpp:906` GetDualViewGeometry (906-916)
- `rts/Lua/LuaOpenGL.cpp:1234` gl.GetViewSizes (1234-1235); Scissor adds viewPos at 3272
- `rts/ExternalAI/AICallback.cpp:1354` AI callback view size (1354-1357)
- `rts/System/Events.def:142` ViewResize has no args
- `rts/Rendering/WorldDrawer.cpp:323` eventHandler.DrawWorld() (also 346 DrawPreDecals, 367 DrawWorldPreUnit, 440 DrawWaterPost)
- `rts/Rendering/Env/IWater.cpp:174` DrawWorldReflection; Refraction at 216
- `rts/Map/SMF/SMFGroundDrawer.cpp:269` DrawGround* callins (269, 286, 304, 313, 336, 346)
- `rts/Lua/LuaUnsyncedRead.cpp:3081` GetCameraPosition uses active camera (3081-3125); GetViewGeometry at 892
- `rts/Lua/LuaOpenGL.cpp:671` EnableDrawWorld matrix reset logic (604-693)
- `rts/Lua/LuaHandle.cpp:2773` RunDrawCallIn enables drawing and runs the callin with no target data
- `rts/Lua/LuaOpenGL.cpp:1067` ResetWorldMatrices loads camera->GetProjectionMatrix()/GetViewMatrix() of the global camera
- `rts/Lua/LuaOpenGL.cpp:528` FIXME: glViewport depends on the mode, not handled in ResetGLState
- `rts/Lua/LuaOpenGL.cpp:3300` gl.Viewport sets glViewport with no restore

*Direction:* Keep the query-API approach, but couple it to the globalRendering migration instead of treating it as a separate feature.

- **Lua draw callins run synchronously on the main draw thread inside the view loop.** A single "current drawing view id" field, set and cleared by the multi-view driver around each view's pass, is enough. No Lua-state plumbing is needed. Add only Spring.GetDrawingView() and Spring.GetViews() (cheap, S effort).
- **If the migration makes the active view's size and position the thing globalRendering->viewSize*/viewPos* reflect during that view's pass** (a swap or active-view pointer), then the existing getters (GetViewGeometry, gl.GetViewSizes, gl.Scissor's viewPos offset) already return the active view's values inside draw callins. The optional viewID? parameter then only matters outside draw callins.
- **Do not change DrawScreen*/ViewResize arguments or the widgetHandler:DrawWorld forwarding for the prototype.** The claim is right that widgetHandler would swallow trailing arguments. The query API sidesteps that.
- **For the prototype, add a debug log of the view id at the start of each view's pass.** If any Lua suppression is wanted for the secondary view, reuse the existing noLuaCall parameter in UnitDrawer.cpp:296 and FeatureDrawer.cpp:300, plus a per-pass guard around eventHandler.DrawWorld* in WorldDrawer.cpp. That is an engine-side firing policy, not a Lua id.
- **Separate from this problem:** gl.Viewport (LuaOpenGL.cpp:3300) and the FIXME at LuaOpenGL.cpp:528 (ResetGLState does not reset the viewport) can leave a leaked viewport after a Lua callin. The per-view driver should re-apply its viewport after each Lua call-in rather than rely on Lua cleaning up.

> verifier A (corrected): The mechanism holds in the current tree. The problem is a missing Lua-facing API, so the claimed severity "risk" is too high.

Checked and true:
- RunDrawCallIn pushes nothing (LuaHandle.cpp:2773-2787, `RunCallIn(L, hs, 0, 0)`). The DRAW_CALLIN macro (2789-2794) generates DrawGenesis, DrawWorld, DrawWorldPreUnit, DrawPreDecals and DrawWaterPost from it.
- DrawScreenCommon pushes only viewSizeX/Y (2986-2987).
- DrawUnit and DrawFeature push only the id and game->GetDrawMode() (LuaHandleSynced.cpp

> verifier B (corrected): The mechanism is real, but "risk" overstates it for the 2-window, 3D-only prototype. The code facts check out. RunDrawCallIn (LuaHandle.cpp:2773-2787) calls RunCallIn(L, hs, 0, 0) with no arguments. DRAW_CALLIN is at 2789. CUnsyncedLuaHandle::DrawUnit pushes only unit->id and game->GetDrawMode() (LuaHandleSynced.cpp:230-231), and DrawFeature does the same at 267-268. gl.GetViewSizes (LuaOpenGL.cpp:1232-1237) and Spring.GetViewGeometry (LuaUnsyncedRead.cpp:892-899, documented "Get main view geome

### LUA-12 - Screen copy and readback functions use raw window coordinates of an implicit drawable
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** none

gl.CopyToTexture, gl.ReadPixels and gl.SaveImage read the current read framebuffer in raw window coordinates, with no bounds checks and no view offset. Lua sizes its textures from the primary window, so a different-size second view copies out-of-bounds or the wrong region.

*Why it breaks:* The PostDeferred callin docs promise frame compositing via gl.CopyToTexture, and widgets size screen-copy textures from the primary view geometry at Initialize/ViewResize. CopyToTexture binds the Lua texture and calls glCopyTexSubImage2D(x,y,w,h) from the current read buffer with no check against the destination or source size. ReadPixels and SaveImage call glReadPixels directly (neither calls CheckDrawingEnabled). In window 2, coordinates built from the primary window's viewPos/viewSize either read undefined pixels (outside the drawable) or raise GL_INVALID_VALUE (region larger than the texture). Coordinate conventions are also inconsistent: gl.Scissor adds globalRendering->viewPos, gl.Viewport and the copy/read functions do not, and GetFixedState reports the raw scissor box.

*Evidence:*
- `rts/Lua/LuaOpenGL.cpp:4455` glCopyTexSubImage2D with unchecked user x,y,w,h (4428-4460)
- `rts/Lua/LuaOpenGL.cpp:6559` ReadPixels: glReadPixels from the current framebuffer (6541-6559), no CheckDrawingEnabled
- `rts/Lua/LuaOpenGL.cpp:6684` SaveImage glReadPixels plus optional readbuffer switch (6676-6684)
- `rts/Lua/LuaOpenGL.cpp:3272` gl.Scissor adds viewPos; gl.Viewport at 3300 does not
- `rts/Lua/LuaHandle.cpp:2886` DrawGroundPostDeferred / DrawUnitsPostDeferred (2894) docs promise compositing via gl.CopyToTexture
- `rts/Rendering/DepthBufferCopy.cpp:72` engine's own copy applies viewPos offset and viewSize; the convention Lua users mirror

*Direction:* For the prototype, suppress Lua draw callins for secondary views, so no change is needed here. Later, validate CopyToTexture rects against the destination texture size and the current read-framebuffer size, and raise a Lua error. Expose the current view geometry to Lua, for example through a gl.GetViewport helper or a view-resize callin argument.

> verifier B (corrected): The mechanism is real. gl.CopyToTexture (LuaOpenGL.cpp:4455) passes unchecked user x,y,w,h to glCopyTexSubImage2D. gl.ReadPixels (6559) calls glReadPixels directly without CheckDrawingEnabled. But it only matters if Lua callins run while a secondary view is drawn. The prototype is 3D-only with no GUI or LuaUI. The simplest workaround is to not fire the Lua draw callins (PostDeferred, DrawWorld and similar) for secondary views, and the prototype does not need Lua widgets to run on window 2. With 

> verifier A (confirmed): I read the cited code and it matches the claim. gl.CopyToTexture (LuaOpenGL.cpp 4428-4455) calls CheckDrawingEnabled, then passes the user's x,y,w,h straight to glCopyTexSubImage2D with no check against the source or destination size and no viewPos offset. gl.ReadPixels (6541-6559) has no CheckDrawingEnabled call and reads straight from the current read buffer. gl.Scissor (3272) adds globalRendering->viewPosX/Y, while gl.Viewport (3300) passes its arguments through unchanged. That is the convent

### LUA-13 - Lua-material LOD uses stale global LOD factor and shared per-object currentLOD
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** Camera slots are one per CAMTYPE and drawers use the ambient active camera

LOD for Lua-material objects is chosen inside the draw-filter using GLOBAL_LOD_FACTORS that were set by the previous pass, and the result is stored on the shared object.

*Why it breaks:* LuaObjectMaterialData::GLOBAL_LOD_FACTORS (LuaObjectMaterial.h:224, defined at LuaMaterial.cpp:1050) is a process-wide static, one value per LuaObjType. It is written only by SetDrawPassGlobalLODFactor (ModelDrawer.h:366, :395, :452, after the filter loop and after the bins are populated) and by DrawSingleObjectCommon (LuaObjectDrawer.cpp:575). It is read by CalcCurrentLOD (LuaObjectMaterial.h:158) when ShouldDraw*Unit/Feature calls Add*MaterialObject inside the per-pass filter loop. Each pass's bin population therefore uses the factor from the previous pass of the same object type, which already holds in one window. In frame order the shadow pass runs first (WorldDrawer.cpp:257 -> ShadowHandler.cpp:459-460), then any reflection or refraction pass (IWater.cpp:168-169, :211-212), then the deferred and forward opaque passes (ModelDrawer.h:328-333), then the alpha pass. With N windows rendered sequentially, the first pass of window k also inherits the last factor of window k-1. That factor is LODScale * camera->GetLPPScale() of window k-1's camera, and lppScale is set in CCamera::Update from the global globalRendering->pixelY and the camera's tanHalfFov (Camera.cpp:144). The effect is a constant LOD-threshold scale error, only for multi-LOD Lua-material objects (lodCount > 1) and only when the windows differ in vertical size or FOV. The per-object currentLOD is not a sequential-rendering race, because the filter and the bin draw happen back to back within one pass. gl.Unit and gl.Feature recompute currentLOD in GLObjectDrawWithLuaMat (LuaOpenGL.cpp:1609) before drawing, so they are not reading a stale LOD index, only a stale global factor.

*Evidence:*
- `rts/Rendering/LuaObjectDrawer.cpp:307` SetDrawPassGlobalLODFactor reads camera->GetLPPScale (310-324)
- `rts/Rendering/Common/ModelDrawer.h:364` LOD factor set after the object loop (366-367)
- `rts/Lua/LuaObjectMaterial.h:199` AddObjectForLOD writes shared currentLOD; GLOBAL_LOD_FACTORS static at 224
- `rts/Rendering/LuaObjectDrawer.cpp:628` Add*MaterialObject computes lodDist from the ambient camera

*Direction:* Compute the factor from the active view camera and pass type at the start of each pass, before the filter loop (move or duplicate SetDrawPassGlobalLODFactor ahead of DrawOpaqueObjects, DrawAlphaObjects and DrawShadowObjects in ModelDrawer.h). Alternatively, pass the factor as a parameter to AddObjectForLOD instead of using a static. Make each render target's camera derive lppScale from its own pixel size rather than globalRendering->pixelY. Keep passes sequential. No per-view currentLOD storage is needed because the filter and bin draw happen back to back. This is not a prototype blocker, and the same fix also repairs the existing shadow-to-opaque stale inheritance in a single window.

> verifier B (corrected): The mechanism is real but its severity is overstated. (1) The stale-factor ordering exists in the single-window engine today. In DrawOpaquePassImpl (ModelDrawer.h:350-367) the object loop runs ShouldDrawOpaqueUnit -> AddOpaqueMaterialObject (UnitDrawer.cpp:214, LuaObjectDrawer.cpp:628-636) -> AddObjectForLOD (LuaObjectMaterial.h:199-204) with GLOBAL_LOD_FACTORS still holding the value set by the previous pass. SetDrawPassGlobalLODFactor only runs afterwards (ModelDrawer.h:366). The previous pass

> verifier A (corrected): The core mechanism is real and I checked it against the current tree. During the filter loop, AddObjectForLOD (LuaObjectMaterial.h:199-205) calls CalcCurrentLOD, which reads the static GLOBAL_LOD_FACTORS (LuaObjectMaterial.h:158, :224). It then writes currentLOD on the per-object LuaObjectMaterialData via SetCurrentLOD (:166, :204). The filter reaches it through ShouldDrawOpaqueUnit (UnitDrawer.cpp:214), AddOpaqueMaterialObject (LuaObjectDrawer.cpp:628-636) and camera->ProjectedDistance. SetDraw

## Input, window events & picking

### INP-01 - Cached cursor ray and press-time camera state are only valid for the camera that produced them
**Severity:** risk - **Effort:** M - **Status:** unverified - **Depends on:** none

`mouse->dir` is cached once per frame and consumers pair it with `camera->GetPos()` evaluated later. This is consistent only while the same camera object is active. Rendering B by swapping the active camera mixes cameras.

*Why it breaks:* UpdateCursorCameraDir (Game.cpp:1438, MouseHandler.cpp:965-969, 708) computes dir from the active camera. About 25 consumers compute `camera->GetPos() + mouse->dir`: GuiHandler.h:46,59,95; GuiHandler.cpp:1165-1166 and 3573-3574; SelectionKeyHandler.cpp:342-344 and 522-523; StartPosSelecter.cpp:106-113; UnsyncedGameCommands.cpp:3630; CMouseHandler::GetWorldMapPos (711-719) and GetCurrentTooltip (759-778). ButtonPressEvt also caches bp.camPos/bp.dir at press time (MouseHandler.cpp:358-359) for box selection (459, 608, 633). `camera` is a macro for CCamera::GetActive() (Camera.h:316). If the implementation renders B by activating another camera, any consumer that runs during that scope (DrawMapStuff, Lua DrawWorld calling TraceScreenRay) pairs B's position with A's ray. Outside draw the active camera stays PLAYER, so interaction in A is still consistent.

*Evidence:*
- `rts/Game/UI/MouseHandler.cpp:708` GetCursorCameraDir uses the active camera; UpdateCursorCameraDir at 965
- `rts/Game/UI/MouseHandler.cpp:358` press caches bp.camPos/bp.dir; reused at 459, 608, 633
- `rts/Game/Game.cpp:1438` dir refreshed once per frame in UpdateUnsynced
- `rts/Game/UI/GuiHandler.h:46` camera->GetPos(), mouse->dir pairs (59, 95)
- `rts/Game/UI/SelectionKeyHandler.cpp:342` camera + mouse->dir ground pick (522)
- `rts/Game/Camera.h:316` #define camera (CCamera::GetActive())

*Direction:* Bundle {camera*, pointer pixel, dir} into a PickContext passed to consumers. Forbid reading the `camera` macro in input paths. Keep the active camera equal to the interactive target's camera except inside an RAII render scope, and disable input consumers inside that scope. Store press-time state per target.

### INP-02 - Input and OS integration hooks are bound to the primary window
**Severity:** risk - **Effort:** L - **Status:** corrected - **Depends on:** CTX-03; Secondary window creation path and pixel-format parity

Mouse position, warping, grabbing, relative mode, the Win32 WndProc subclass, hardware cursor, icon and AVI capture all use the primary SDL window. Events from another window are interpreted in the primary's coordinate space.

*Why it breaks:* Verified chain: InputHandler.cpp:26 polls SDL globally and every handler sees every window's events. IMouseInput::HandleSDLMouseEvent (MouseInput.cpp:68-94) stores event.motion/button x,y (window-local to whichever window got the event) into mousepos and calls mouse->MouseMove/MousePress/MouseRelease/MouseWheel with no windowID check. CMouseHandler::MouseMove (MouseHandler.cpp:253-257) writes lastx/lasty from those coordinates and calls UpdateCursorCameraDir() against the primary `camera`. Concrete consequences:
 - Hovering window 2 moves the engine cursor and ray in the primary's space.
 - The edge-scroll test (Camera.cpp:736-757, applied at CameraHandler.cpp:315-322 when WindowedEdgeMove, default true, CameraHandler.cpp:60) compares mouse->lastx/lasty against globalRendering->winSizeX/viewSizeY. The primary camera therefore edge-scrolls whenever the pointer sits in window 2's border band.
 - A click in window 2 goes through MousePress and selection/orders using the primary camera's pick ray.
 - MOUSEWHEEL (MouseInput.cpp:90-93) carries no coordinates and zooms the primary camera.
Enter/leave is only a transient flicker: moving between windows emits LEAVE(old) then ENTER(new), so offscreen ends up false. LEAVE still recentres mousepos and lastx/lasty to the primary's view centre (MouseInput.cpp:103-106, MouseHandler.cpp:235-244), which clobbers the position, and leaving window 2 for the desktop triggers the same recentre/offscreen logic.
Platform and OS hooks that stay primary-only: the Win32 SpringWndProc is installed on one HWND (MouseInput.cpp:153-168, re-run from SpringApp.cpp:1153), so window 2 gets no hardware-cursor WM_SETCURSOR override. X11 Bind() calls XDefineCursor on the primary window only (HwMouseCursor.cpp:728). SDL_SetWindowGrab and SDL_WarpMouseInWindow target only the primary (GlobalRendering.cpp:1323, MouseInput.cpp:221).
Evidence to drop or reclassify:
 - AVIGenerator.cpp:343 only fetches the HWND to minimise/restore the main window around encoder init when fullscreen (:346-354). It is not a capture hook.
 - Screenshot.cpp:47-58 is a render-target readback (winSizeX/Y plus glReadPixels on the bound framebuffer), not input.
 - glxHandler.cpp:23 is a process-wide GLX function loader, not per-window.
 - SpringApp.cpp:905 is about buffer swap policy.
 - GlobalRendering.cpp:578-584 (BlockCompositing) and :1649 (UpdateWindowBorders into the single winBorder) are window-attribute/geometry items, not input.
 - LuaUnsyncedRead.cpp:962 is a Lua API semantics question (documented as "main window").
Those belong under window management, capture or API-semantics problems rather than input.

*Evidence:*
- `rts/System/Input/MouseInput.cpp:95` ENTER/LEAVE not tied to a window; recenters to primary view
- `rts/System/Input/MouseInput.cpp:221` warp targets the primary window
- `rts/System/Input/MouseInput.cpp:157` WndProc subclass installed for the primary HWND only
- `rts/Game/UI/HwMouseCursor.cpp:221` hardware cursor uses GetWindow()
- `rts/Rendering/AVIGenerator.cpp:343` capture bound to the main window
- `rts/Rendering/GlobalRendering.cpp:1202` SetWindowTitle/SetWindowInputGrabbing on sdlWindow (1202-1340)
- `rts/System/Input/MouseInput.cpp:153` InstallWndCallback subclasses the primary HWND only
- `rts/Rendering/Screenshot.cpp:47` winSizeX/Y of the primary
- `rts/System/Platform/WindowManagerHelper.cpp:45` icon set on the primary window
- `rts/Lua/LuaUnsyncedRead.cpp:962` display mode of GetWindow()
- `rts/Rendering/GlobalRendering.cpp:578` BlockCompositing on sdlWindow only
- `rts/Rendering/GlobalRendering.cpp:1649` UpdateWindowBorders into one winBorder with DWM workaround
- `rts/Rendering/GL/glxHandler.cpp:23` Wayland GLX loading skipped
- `rts/System/Input/MouseInput.cpp:212` Wayland warp restriction comment
- `rts/System/SpringApp.cpp:905` skipping swaps can upset some drivers

*Direction:* Prototype: in IMouseInput::HandleSDLMouseEvent (MouseInput.cpp:65), drop SDL_MOUSEMOTION, SDL_MOUSEBUTTON*, SDL_MOUSEWHEEL and SDL_WINDOWEVENT_ENTER/LEAVE events whose windowID != SDL_GetWindowID(globalRendering->GetWindow()). Apply the same filter to the key and text events in SpringApp::MainEventHandler if secondary windows should not feed text input. The window-scoping of SDL_WINDOWEVENT (CLOSE -> RequestQuit, SIZE_CHANGED, MOVED, FOCUS_LOST, HIDDEN) is tracked under the dependsOn item and must land together with this change. Later: carry a target id with each event, convert to target-local pixel coordinates, pick against that target's camera, and give each target its own grab, cursor and WndProc/XDefineCursor binding. Split screenshot/AVI/Lua-API/glx items into separate problems.

> verifier A (corrected): The core mechanism holds in the current tree. Every SDL event is polled globally (InputHandler.cpp:26) and fanned out to all handlers. Nothing in rts/ filters by windowID (grep for windowID, GetWindowID and GetWindowFromID returns nothing outside rts/lib). IMouseInput::HandleSDLMouseEvent (MouseInput.cpp:65-116) takes event.motion/button x,y as-is, handles ENTER/LEAVE for any window, and WarpPos (:221), the Win32 WndProc subclass (:153-168), HwMouseCursor (:221, :596, :703, :721, :728), WindowMa

### INP-03 - Mouse, button and wheel events are consumed without checking the source window
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** Window lifecycle events are applied to the primary window regardless of which window sent them; SDL window events are not attributed to a window

Pointer events from any window flow into the one CMouseHandler and IMouseInput, so moving or clicking in B changes A's cursor, ray, hover, selection and camera.

*Why it breaks:* HandleSDLMouseEvent (MouseInput.cpp:65-94) stores event.motion.x/y and button x/y straight into mousepos and calls mouse->MouseMove/MousePress/MouseRelease/MouseWheel. SDL reports those coordinates relative to the window with pointer focus (B). CMouseHandler::MouseMove sets lastx/lasty and UpdateCursorCameraDir, casting A's camera through B's pixel (MouseHandler.cpp:249-257). Camera::GetMoveVectorFromState uses lastx/lasty with A's winSizeX for edge scroll, so the pointer near B's border scrolls A's camera (Camera.cpp:736-757). LMB press/release in B runs MousePress/MouseRelease: orders, box select and TraceRay in A's world (MouseHandler.cpp:335-426, 525-643). Wheel in B zooms A's camera (MouseHandler.cpp:665; SDL wheel events carry no coordinates). activeReceiver is a single static, so a drag cannot belong to a particular target.

*Evidence:*
- `rts/System/Input/MouseInput.cpp:65` mouse events used without windowID
- `rts/Game/UI/MouseHandler.cpp:249` MouseMove writes lastx/lasty then UpdateCursorCameraDir
- `rts/Game/UI/MouseHandler.cpp:335` MousePress picks and issues commands through the single camera/receivers
- `rts/Game/UI/MouseHandler.cpp:650` MouseWheel zooms the current controller with no target
- `rts/Game/Camera.cpp:736` edge-scroll vector from global mouse and winSizeX
- `rts/Game/UI/InputReceiver.h:62` single static activeReceiver
- `rts/System/Input/MouseInput.cpp:101` LEAVE resets mousepos to primary view centre
- `rts/System/SpringApp.cpp:1204` FOCUS_LOST releases keys, clears emulation, ungrabs mouse
- `rts/System/Input/MouseInput.cpp:221` SDL_WarpMouseInWindow(primary)

*Direction:* Prototype: in HandleSDLMouseEvent, compare event.{motion,button,wheel}.windowID with the primary window's SDL_GetWindowID. For the SDL_WINDOWEVENT enter/leave case, use event.window.windowID. Drop pointer events from secondary windows, or optionally route them to a secondary-camera controller (an orbit/free-fly camera that is not CMouseHandler). Also ignore ENTER/LEAVE from secondary windows, so that LEAVE does not recenter mousepos or trigger WindowLeave on A. Do the same for keyboard events, and do not let FOCUS_LOST on B release A's input state. This is a few lines. Later, introduce a per-target input context and route by windowID, as the original direction proposes.

> verifier A (confirmed): Verified in the current tree. HandleSDLMouseEvent (MouseInput.cpp:65-94) copies event.motion.x/y and event.button.x/y into mousepos and calls the single global mouse handler. There is no windowID check anywhere in rts/*.cpp (grep for windowID returns nothing). CMouseHandler::MouseMove (MouseHandler.cpp:249-257) writes lastx/lasty, subtracting the primary globalRendering->viewWindowOffsetY, then calls UpdateCursorCameraDir. Camera.cpp:736-757 uses mouse->lastx/lasty against the primary globalRend

> verifier B (corrected): The mechanism is real. HandleSDLMouseEvent (MouseInput.cpp:65-116) never reads windowID (grep for windowID under rts/System finds nothing). It writes event.motion.x/y and button x/y straight into mousepos and calls the single mouse->MouseMove/MousePress/MouseRelease/MouseWheel. Pointer events from window B would therefore move A's cursor and camera, and could issue orders in A. This does not crash and raises no GL errors. A 3D-only prototype can be driven entirely from window A, so the worst cas

### INP-04 - Pointer enter/leave, offscreen flag and implicit capture track a single window
**Severity:** risk - **Effort:** S - **Status:** unverified - **Depends on:** INP-03

SDL emits LEAVE for the old window and ENTER for the new one when the pointer crosses windows. Both are applied to the one global offscreen flag and cursor position, and Rml sees them too.

*Why it breaks:* MouseInput.cpp:95-111 maps any ENTER to mouse->WindowEnter() (offscreen=false) and any LEAVE to mouse->WindowLeave() (offscreen=true, lastx/lasty = A's view centre, MouseHandler.cpp:229-244) and reseeds mousepos from A's view geometry. A to B crossing therefore yields LEAVE(A)+ENTER(B): if the order is Leave then Enter (platform-dependent, not verified here) offscreen ends false while the pointer is in B, so the cursor, tooltips and edge-scroll act as if it were in A; the reverse order ends true. Rml gets ProcessMouseLeave (RmlUi_SystemInterface.cpp:212-214) and calls SDL_CaptureMouse on every button down/up (185/188). Button capture means a drag started in A and released over B arrives with A's windowID but out-of-bounds coordinates. Whether the click that focuses an unfocused window is also delivered as a command depends on an SDL hint (SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH), not on code in this repo.

*Evidence:*
- `rts/System/Input/MouseInput.cpp:95` ENTER/LEAVE handling unconditional; LEAVE reseeds mousepos from primary viewport (103-106)
- `rts/Game/UI/MouseHandler.cpp:229` WindowEnter/WindowLeave flip offscreen and reset lastx/lasty
- `rts/Rml/Backends/RmlUi_SystemInterface.cpp:185` SDL_CaptureMouse(true) on button down, false on up (188); LEAVE at 212

*Direction:* Track pointerWindowID from motion/enter/leave events and set offscreen = (pointer window != the interactive target). Handle focus-click policy explicitly (swallow the focusing click or not). Verify the SDL version and platform ordering of LEAVE/ENTER on Windows, X11 and Wayland.

### INP-05 - Camera controllers and edge-scroll read global mouse, window size and focus
**Severity:** full - **Effort:** L - **Status:** unverified - **Depends on:** CAM-07; DSN-05

Controllers and CCamera::GetMoveVectorFromState read the global mouse (lastx/lasty/dir), globalRendering window and view sizes, dual-screen offsets and focus. Key movement state lives on the active CCamera.

*Why it breaks:* SpringController::MouseWheelMove uses mouse->dir, ScreenEdgeMove compares mouse->lasty with globalRendering->viewSizeY, and edge scrolling uses mouse->lastx and winSizeX. Pixel-size scaling (pixelSize) uses globalRendering->viewSizeY, so pan speed per pixel is wrong for a differently sized view. FreeController zeroes velocities when !globalRendering->active (focus of the primary window). Action executors write movState on the active camera. Mouse picking (MouseHandler 40 uses, GuiHandler 24, SelectionKeyHandler 9) is hard-wired to the active camera plus global mouse.

*Evidence:*
- `rts/Game/Camera.cpp:736` GetMoveVectorFromState uses mouse->lastx/lasty and global winSize
- `rts/Game/Camera/SpringController.cpp:178` mouse->lasty vs globalRendering->viewSizeY; pixelSize from viewSizeY at :407 (OverheadController.cpp:213)
- `rts/Game/Camera/FreeController.cpp:87` if (!globalRendering->active) velocities zeroed
- `rts/Game/UnsyncedGameCommands.cpp:772` CameraMoveActionExecutor writes camera->SetMovState on the active camera
- `rts/Game/UI/MouseHandler.cpp:708` GetCursorCameraDir uses camera->CalcPixelDir; mouse->dir updated at :965-969 and Game.cpp:1438
- `rts/Game/CameraHandler.cpp:258` UpdateController: edge-move flags from global fullScreen (264-265)
- `rts/Game/Camera/SpringController.cpp:407` pixelSize from global viewSizeY; 178-179 mouse->lasty vs viewSizeY
- `rts/Game/Camera/OverheadController.cpp:213` pixelSize from global viewSizeY; zoom via mouse->dir (OverheadController.h:21)
- `rts/Game/UI/UnitTracker.cpp:253` tracker writes camera and controller directly (to 317)
- `rts/Game/UI/MiniMap.cpp:613` minimap pans the current controller (680 wheel)

*Direction:* Out of scope for the 3D-only prototype; plan an 'input view' concept. Route SDL events to the focused window's view. Give each rig its own cursor state (pos, dir). Replace globals in controllers with the rig's viewport and input state.

### INP-06 - Cursor visibility and hardware cursor binding are process-global or primary-window-only
**Severity:** full - **Effort:** M - **Status:** confirmed - **Depends on:** Window and mouse events from any SDL window are handled as primary-window events

With the default software cursor, the SDL cursor is hidden app-wide and the engine draws the cursor into A's backbuffer, so B has no pointer. Hardware cursor and the Win32 WM_SETCURSOR hook bind to the primary window only.

*Why it breaks:* HardwareCursor defaults to false (MouseHandler.cpp:56). SDL_ShowCursor(DISABLE) is process-global (MouseHandler.cpp:864, 911, 947) and the software cursor is drawn by mouse->DrawCursor() at the end of A's frame (Game.cpp:1545, MouseHandler.cpp:1049-1115). Over B's client area no cursor is visible. CWin32MouseInput hooks WM_SETCURSOR on globalRendering->GetWindow() only (MouseInput.cpp:153-168), and the X11/Wayland/Win32 hardware-cursor code queries the primary window's WM info (HwMouseCursor.cpp:221, 593-596, 701-703, 718-721).

*Evidence:*
- `rts/Game/UI/MouseHandler.cpp:56` HardwareCursor default false
- `rts/Game/UI/MouseHandler.cpp:911` ToggleHwCursor(false): SDL_ShowCursor(SDL_DISABLE) application-wide (also 947)
- `rts/Game/Game.cpp:1545` mouse->DrawCursor() in the primary screen pass
- `rts/System/Input/MouseInput.cpp:153` WndProc subclass on the primary HWND only
- `rts/Game/UI/HwMouseCursor.cpp:221` hardware cursor setup via primary window WM info (593, 701, 718)
- `rts/Game/UI/MouseHandler.cpp:1094` software cursor drawn at lastx/lasty
- `rts/System/Input/MouseInput.cpp:131` SpringWndProc blocks WM_SETCURSOR; installed for the main window only (153-166)
- `rts/System/Input/MouseInput.cpp:221` SDL_WarpMouseInWindow(globalRendering->GetWindow())

*Direction:* Decide the cursor policy for secondary targets (system cursor visible and neutral, or engine-drawn per target). If SDL_ShowCursor stays global, show the OS cursor over non-interactive targets. Apply the WndProc hook and hardware cursor per window.

> verifier A (confirmed): Verified in the current tree. HardwareCursor defaults to false (MouseHandler.cpp:56). SDL_ShowCursor(SDL_DISABLE) is called at 864, 911 and 947. mouse->DrawCursor() is called once per frame at Game.cpp:1545. The WM_SETCURSOR hook is installed on the SDL_GetWindowWMInfo(globalRendering->GetWindow()) HWND only (MouseInput.cpp:153-168). HwMouseCursor.cpp reads the primary window's WM info at 221, 596, 703 and 721. The warp also targets the primary window (MouseInput.cpp:221). A secondary window wou

### INP-07 - Interactive input mapping and focus handling assume one window
**Severity:** full - **Effort:** L - **Status:** corrected - **Depends on:** SDL window and input events are not dispatched by windowID

Mouse y mapping subtracts viewWindowOffsetY, edge scroll uses window-1 winSizeX and the mouse position, the UBO mouse fields use the primary view, and focus loss resets global input state.

*Why it breaks:* Every SDL mouse event, whichever window raised it, goes to the single `IMouseInput::HandleSDLMouseEvent` (`rts/System/Input/MouseInput.cpp:65-113`). It stores `event.motion.x/y` and `event.button.x/y` in `mousepos` and forwards them to the global `mouse->MouseMove/MousePress/MouseRelease`. Those treat the coordinates as window-1 coordinates. `MouseHandler.cpp:255`, `:342` and `:535` subtract `globalRendering->viewWindowOffsetY`. That is the dual-screen offset of the first screen inside one OS window (`GlobalRendering.cpp:1525`), and it is 0 for a normal window. The coordinates are then used by the single camera and GUI: `UpdateCursorCameraDir`, `GetCursorCameraDir`, selection and commands.

Edge scroll: `CCamera::GetMoveVectorFromState(false)` (`Camera.cpp:736-768`) compares `mouse->lastx` against `globalRendering->winSizeX` (the primary window width) and `mouse->lasty` against `viewSizeY`, or the dual view when `dualScreenMode` is set. It is called from `CCameraHandler::UpdateController` (`CameraHandler.cpp:315-322`) for the single active camera controller. Pointer motion in window 2, or at window 2's edges, therefore scrolls window 1's camera. Window 2 has no mouse state of its own, and no controller of its own to scroll.

Mouse-leave tracking: only LEAVE recentres. `MouseInput.cpp:101-106` sets `mousepos` to the primary view centre. `CMouseHandler::WindowLeave` (`MouseHandler.cpp:235-244`) sets `offscreen = true` and sets `lastx/lasty` to `GetViewMouseCenter()` (`:700-706`), which is built from `viewPosX/viewSizeX/viewSizeY`. ENTER (`MouseInput.cpp:97-100`, `MouseHandler.cpp:229-233`) only clears `offscreen`. Moving the pointer from window 1 to window 2 sends LEAVE(w1) then ENTER(w2). The shared `offscreen` flag flips back to false while `lastx/lasty` stay at the window-1 centre until a motion event arrives.

The per-frame constants UBO is global. It writes `mouseScreenPos` from `mouse->lastx` and `viewPosY + viewSizeY - lasty - 1` (`UniformConstants.cpp:250-253`), and the mouse bits from the single `mouse` object (`:255-262`). It also runs a `GuiTraceRay` from the single `CAMTYPE_PLAYER` camera at the mouse position (`:265-293`) to fill `mouseWorldPos`. One UBO value therefore cannot be correct for two views.

Focus: `SDL_WINDOWEVENT_FOCUS_LOST` for any window runs `LuaDebugExtra::ClearEmulatedInput`, `KeyInput::ReleaseAllKeys`, simulated releases for pressed mouse buttons, an unlock of MMB scroll and `SetWindowInputGrabbing(false)` (`SpringApp.cpp:1204-1240`). Clicking from window 1 to window 2 therefore looks like the whole game lost focus. `ReleaseAllKeys` (`KeyInput.cpp:176-196`) pushes synthetic `SDL_KEYUP` for every held key, and `FOCUS_GAINED` only re-polls state through `KeyInput::Update` (`:117-164`). `MouseInput.cpp:221` warps the cursor only in `globalRendering->GetWindow()`, and `SDL_SetRelativeMouseMode` at `MouseHandler.cpp:845/869` is global. MMB-scroll and mouse-lock therefore always act on window 1.

*Evidence:*
- `rts/Game/UI/MouseHandler.cpp:255` y -= viewWindowOffsetY (also 342, 535)
- `rts/System/Input/MouseInput.cpp:69` mousepos from SDL events, ENTER/LEAVE recentres on the primary view (69-111)
- `rts/Game/Camera.cpp:736` edge scroll uses winSizeX and mouse->lasty
- `rts/Rendering/UniformConstants.cpp:250` mouseScreenPos from the primary viewPos/Size
- `rts/System/SpringApp.cpp:1204` FOCUS_LOST resets keys and mouse

*Direction:* Keep this item after the prototype. Cover the minimum prototype safety (ignore or route window-2 mouse and keyboard events, and filter FOCUS_LOST/GAINED and CLOSE by window) under the dependsOn item on windowID dispatch. Then add a per-target input context: window id, a view rect for converting window-local to view coordinates, and mouse state (`lastx`, `lasty`, `offscreen`, buttons, pointer-lock). Give each target its own camera controller, edge-scroll test and UBO mouse fields, and decide which target owns GUI, selection and picking. Only drop key state and mouse grabs when focus leaves all engine windows. Treat `viewWindowOffsetY` and the dual-screen fields as a separate per-target concept, not as the multi-window mechanism.

> verifier A (corrected): The core mechanism holds in the current tree. All input is routed through one global CMouseHandler and one globalRendering window, with no window identity anywhere. `windowID`, `SDL_GetWindowID` and `SDL_GetWindowFromID` have zero hits in rts/ outside rts/lib. All five cited sites do what is claimed, but two details need fixing. First, `viewWindowOffsetY` is not an OS-window offset. It is the dual-screen, multi-monitor vertical offset of the first screen inside the single window (`GlobalRenderin

### INP-08 - Keyboard actions and text input have no notion of a target
**Severity:** full - **Effort:** M - **Status:** unverified - **Depends on:** none

SDL keyboard state is global, so keys from any window reach the same activeController path and bindings that mutate the single camera and handler. There is no input-focus-target concept, and IME/text input rects use the primary window.

*Why it breaks:* KeyInput::Update polls SDL_GetKeyboardState globally (KeyInput.cpp:117-164). MainEventHandler routes KEYDOWN/KEYUP/TEXTINPUT to activeController with no window info (SpringApp.cpp:1265-1301). Binding executors such as CameraMoveActionExecutor set movState on the active camera (UnsyncedGameCommands.cpp:771-780, 4082-4092), and camHandler actions (viewta, togglecammode, ...) operate on the single handler. When B has keyboard focus, movement keys silently move A's camera unless the router drops them. SDL_SetTextInputRect is positioned in A's coordinates (GameControllerTextInput.cpp:43, LuaUnsyncedCtrl.cpp:5371) while text events come from the focused window. Clipboard is OS-global and needs no change.

*Evidence:*
- `rts/System/Input/KeyInput.cpp:117` global keyboard state polling
- `rts/System/SpringApp.cpp:1275` KEYDOWN/KEYUP/TEXTINPUT routed to activeController (1265-1301)
- `rts/Game/UnsyncedGameCommands.cpp:771` movement actions set movState on `camera`
- `rts/Game/GameControllerTextInput.cpp:43` SDL_SetTextInputRect in primary coordinates
- `rts/Game/CameraHandler.h:107` camHandler is itself a CommandReceiver on the single handler

*Direction:* Define inputFocusTarget = target owning SDL keyboard focus at dispatch time. Action executors receive that target (camera, controller, pointer) via context. For v1 let keyboard drive the primary only and ignore or forward keys from non-interactive targets. Position the IME rect relative to the focused window.

### INP-09 - Lua input callins and mouse queries have no window identity
**Severity:** full - **Effort:** L - **Status:** confirmed - **Depends on:** LUA-11

MousePress, MouseMove, MouseWheel and Spring.GetMouseState convert coordinates with the primary window's viewPos/viewSize, and the UBO mouse fields do the same. Input from a secondary window is undefined or mis-attributed.

*Why it breaks:* CLuaHandle mouse callins subtract globalRendering->viewPosX and flip with viewSizeY - y - 1. GetMouseState uses mouse->lastx/lasty with the same globals. UniformConstants writes mouseScreenPos from the same state. There is no windowID or viewID in callin parameters, and TraceScreenRay's coordinate space is the primary view's. Picking in window 2 cannot be expressed. Out of prototype scope (no input needed for display-only views), but required for any interactive use.

*Evidence:*
- `rts/Lua/LuaHandle.cpp:3470` MousePress converts with viewPosX and viewSizeY (3470-3471); same pattern at 3504, 3532, 3593, 3622
- `rts/Lua/LuaUnsyncedRead.cpp:3933` GetMouseState uses primary viewPos/viewSize (3933-3934)
- `rts/Rendering/UniformConstants.cpp:250` mouseScreenPos in UBO from primary window globals (250-253)
- `rts/Lua/LuaUnsyncedRead.cpp:3928` GetMouseState from global mouse
- `rts/Lua/LuaUnsyncedRead.cpp:3216` TraceScreenRay with global view bounds and active camera
- `rts/Lua/LuaUnsyncedRead.cpp:3179` WorldToScreenCoords uses the active camera (3327 GetPixelDir)
- `rts/Lua/LuaUnsyncedCtrl.cpp:1235` camera control mutates the single camHandler (1309, 1330-1501)
- `rts/Lua/LuaUnsyncedCtrl.cpp:3180` WarpMouse global

*Direction:* Track which view owns the pointer (SDL windowID -> view) and pass or expose it: Spring.GetMouseState(viewID?) plus a trailing view argument on mouse callins and GetMouseView(). Needs the input and window-event routing work first.

> verifier A (confirmed): Verified in the current tree. LuaHandle.cpp lines 3470-3471, 3504-3505, 3532-3533, 3593-3594 and 3622-3623 convert mouse coordinates with globalRendering->viewPosX and viewSizeY, and none of the callins carries a window or view id. LuaUnsyncedRead.cpp 3933-3934 (GetMouseState) does the same from the global mouse->lastx/lasty. UniformConstants.cpp 250-253 builds mouseScreenPos from the same globals. I did not open the TraceScreenRay, camera-control or WarpMouse citations; they were not re-checked

### INP-10 - Mouse coordinates are untyped ints in three conventions with no target attached
**Severity:** full - **Effort:** L - **Status:** confirmed - **Depends on:** none

Every consumer takes raw int x/y and interprets them through globalRendering, using three different conventions. There is no way to say which target a coordinate belongs to.

*Why it breaks:* mouse->lastx includes viewPosX but lasty subtracts viewWindowOffsetY (MouseHandler.cpp:255, 342, 535), and GetViewMouseCenter adds viewPosX to x only (700-706). Lua uses a bottom-left origin: x - viewPosX, viewSizeY - y - 1 (LuaUnsyncedRead.cpp:3933-3934, 3222-3223). CInputReceiver::MouseX/MouseY are static helpers on globalRendering (InputReceiver.h:41-47). StartPosSelecter converts with viewSizeY directly (StartPosSelecter.cpp:139-140). Camera::GetMoveVectorFromState special-cases dualScreenMode (Camera.cpp:740-743), and aGui/List.cpp:321 calls SDL_GetMouseState, which is relative to the focused window. There are 68 uses of mouse->lastx/lasty/dir/GetWorldMapPos in 22 files, and 62 uses of the dual-screen offsets in 10 files. SDL window-local, engine-view-local and Lua/GL bottom-left spaces all coexist without a type.

*Evidence:*
- `rts/Game/UI/MouseHandler.cpp:342` y minus viewWindowOffsetY on press (255, 535)
- `rts/Game/UI/MouseHandler.cpp:700` GetViewMouseCenter mixes viewPosX and viewSizeY
- `rts/Lua/LuaUnsyncedRead.cpp:3933` Lua coordinates relative to viewPosX, flipped Y (3934, 3222-3223)
- `rts/Game/UI/InputReceiver.h:41` static MouseX/MouseY helpers use globalRendering
- `rts/Game/UI/StartPosSelecter.cpp:139` ad-hoc conversion with viewSizeY (140)
- `rts/Game/Camera.cpp:740` dual-screen special case in edge-move
- `rts/aGui/List.cpp:321` SDL_GetMouseState relative to the focused window

*Direction:* Introduce a ScreenPoint {target, x, y, origin} type with explicit conversion helpers (to GL, to Lua, to target-local), and migrate consumers incrementally. Keep dualScreenMode as one implementation of 'two viewports in one target'. Do this before adding any per-target pointer.

> verifier A (confirmed): I checked the cited lines in the current tree and they match. MouseHandler.cpp subtracts viewWindowOffsetY from y at lines 255, 342 and 535, and adds viewPosX to x at line 828. GetViewMouseCenter at 703 adds viewPosX to x only. LuaUnsyncedRead.cpp uses x - viewPosX with a flipped Y at 3933-3934, and wx/wy at 3222-3223. StartPosSelecter.cpp:140 uses viewSizeY directly. Camera.cpp:740 special-cases dualScreenMode. aGui/List.cpp:321 calls SDL_GetMouseState. The result is several untyped coordinate 

### INP-11 - Mouse grab, relative mode and warping are bound to the primary window
**Severity:** full - **Effort:** M - **Status:** confirmed - **Depends on:** none

Warp, grab and middle-click scroll code target the primary SDL window, while SDL relative mode is process-wide. Interaction in B and cross-window behaviour are undefined.

*Why it breaks:* IMouseInput::WarpPos calls SDL_WarpMouseInWindow(globalRendering->GetWindow()) (MouseInput.cpp:221). SetWindowInputGrabbing grabs sdlWindow (GlobalRendering.cpp:1318-1331), which confines the pointer to A and makes B unreachable with the mouse. HideMouse/ShowMouse call SDL_SetRelativeMouseMode (MouseHandler.cpp:845, 869) and then seed lastx/lasty with the primary view centre (700-706, 855-880), so MMB-scroll or FPS mode started in B uses A's centre and warps the pointer into A. WarpMouse (822-834) and Spring.WarpMouse (LuaUnsyncedCtrl.cpp:3180-3184) take no window.

*Evidence:*
- `rts/System/Input/MouseInput.cpp:221` SDL_WarpMouseInWindow on the primary window
- `rts/Rendering/GlobalRendering.cpp:1318` grab via SDL_SetWindowGrab(sdlWindow)
- `rts/Game/UI/MouseHandler.cpp:845` SDL_SetRelativeMouseMode (869); HideMouse seeds lastx/lasty from primary centre (871-876)
- `rts/Game/UI/MouseHandler.cpp:822` WarpMouse offsets by primary viewPosX
- `rts/Lua/LuaUnsyncedCtrl.cpp:3180` Spring.WarpMouse has no target

*Direction:* Make warp, grab and relative mode take the interactive target's SDL_Window. Keep relative mode tied to the keyboard-focus target. Decide grab semantics with several windows (confine to focused window only, or release on focus change).

> verifier A (confirmed): Verified in the current tree. IMouseInput::WarpPos calls SDL_WarpMouseInWindow(globalRendering->GetWindow(), ...) at MouseInput.cpp:221. SetWindowInputGrabbing and GetWindowInputGrabbing use the single sdlWindow (GlobalRendering.cpp:1315-1329). HideMouse calls SDL_SetRelativeMouseMode(SDL_TRUE) (MouseHandler.cpp:869), which is process-wide in SDL2. It then seeds lastx/lasty from GetViewMouseCenter() (871-876) and calls SetPos on that centre. WarpMouse offsets by globalRendering->viewPosX (828). 

### INP-12 - Synthetic SDL events and input emulation carry no window identity
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** Window lifecycle events are applied to the primary window regardless of which window sent them

The engine injects SDL events and emulated input without a windowID (some fields left uninitialised). A windowID-based router would drop or misroute them, leaving stuck buttons and keys.

*Why it breaks:* The FOCUS_LOST handler (SpringApp.cpp:1224) and KeyInput::ReleaseAllKeys (KeyInput.cpp:188) push stack-allocated, uninitialised SDL_Events. Their windowID is undefined, and so are other fields such as key.repeat and button.clicks. Any future per-window event router would drop or misroute them, which would reintroduce hung buttons and keys. SDL_QUIT (CrashHandler.cpp:946) is unaffected because SDL_QuitEvent has no windowID. Emulated input (MouseHandler.cpp:308-325) never goes through SDL events and acts on the single global mouse state at lastx/lasty, so it needs an explicit target once there are multiple windows.

*Evidence:*
- `rts/System/SpringApp.cpp:1224` synthetic MOUSEBUTTONUP with uninitialised windowID
- `rts/System/Input/KeyInput.cpp:188` synthetic KEYUP with uninitialised windowID
- `rts/System/Platform/Linux/CrashHandler.cpp:946` SDL_QUIT pushed with only type set
- `rts/Lua/LuaDebugExtra.cpp:128` emulated mouse bypasses SDL events and is target-less (169, 249)
- `rts/Game/UI/MouseHandler.cpp:308` SetButtonEmulated fires MousePress/Release at lastx/lasty

*Direction:* Zero-initialise every synthetic event. Stamp the focus-lost releases with the ID of the window that lost focus, or release the keys/buttons for all windows. Define windowID 0 as broadcast/primary in the router. Give the emulation APIs an optional target argument. Not needed for the 2-window 3D-only prototype, which can keep input on the primary window only.

> verifier B (corrected): The code facts are right. SpringApp.cpp:1224-1231 and KeyInput.cpp:188-194 build a stack SDL_Event without initialising windowID, which is uninitialised stack data. But this only matters if the feature adds a windowID-based input router. The 2-window, 3D-only prototype needs no per-window input routing. Both windows can be fed by the primary window's existing input path, or the second window can ignore input. Focus-loss release already works for the primary window as it does today. The claim the

> verifier A (corrected): The code matches the claim. SpringApp.cpp:1224-1231 and KeyInput.cpp:188-194 build a stack SDL_Event without zero-initialisation and push it, so windowID (and key.repeat) is garbage. MouseHandler.cpp:308-325 fires MousePress/MouseRelease at lastx/lasty with no SDL event. However, nothing in the current tree routes by windowID, so there is no present failure. The risk only exists once a windowID-based router is added, which makes this a dependent constraint and not a prototype blocker or risk. Th

### INP-13 - Window close and Alt+F4 are wired to whole-game quit
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** Window lifecycle events are applied to the primary window regardless of which window sent them

Any SDL_WINDOWEVENT_CLOSE calls RequestQuit and ends the game. Closing or Alt+F4 on window B would terminate the session.

*Why it breaks:* As claimed. The CLOSE case ignores event.window.windowID, and RequestQuit() is global. A per-window close needs a router that compares windowID against the primary window and swallows or hides secondary windows. SDL2's last-window-closed SDL_QUIT behaviour is library-side and was not checked.

*Evidence:*
- `rts/System/SpringApp.cpp:1249` CLOSE -> RequestQuit for any window
- `rts/System/SpringApp.cpp:1076` RequestQuit: AllowQuit veto then globalQuit
- `rts/System/SpringApp.cpp:1262` SDL_QUIT same path
- `rts/Rendering/GlobalRendering.cpp:617` DestroyWindowAndContext handles a single window

*Direction:* For the prototype, change nothing: close on any window keeps RequestQuit. Later, once a window registry exists, switch on event.window.windowID. A secondary window's CLOSE hides or destroys only that target. A primary window's CLOSE keeps RequestQuit. Keep SDL_QUIT as a whole-game quit. Consider fixing this together with the window-lifecycle-routing problem instead of tracking it separately.

> verifier A (corrected): The mechanism is true. SDL_WINDOWEVENT_CLOSE (SpringApp.cpp:1249-1251) and SDL_QUIT (1262-1264) both call the static RequestQuit(). It has no window-ID check and sets gu->globalQuit after the Lua AllowQuit veto (1076-1093). Today there is only one window, so this is untested for N windows. For the 2-window prototype, closing window B quitting the session is acceptable behaviour. It causes no crash, GL error or wrong render output, so "risk" is too high. This is needed for a complete product, not

> verifier B (corrected): The mechanism is real. SpringApp.cpp:1249-1251 handles SDL_WINDOWEVENT_CLOSE by calling RequestQuit() without looking at event.window.windowID, and SDL_QUIT at 1262-1264 does the same. Closing window B would therefore end the whole session. This does not make the 2-window 3D-only prototype crash, raise GL errors or render wrong output. It is a deliberate-user-action behaviour. For the prototype, "closing either window quits the game" is an acceptable and arguably natural semantic, because both w

### INP-14 - Focus loss on either window wipes all key, button and grab state
**Severity:** polish - **Effort:** S - **Status:** confirmed - **Depends on:** INP-12

FOCUS_LOST is handled as application-wide focus loss. Clicking from A to B releases every key and button, unlocks MMB scroll and drops the mouse grab.

*Why it breaks:* The FOCUS_LOST handler (SpringApp.cpp:1204-1241) clears emulated input, calls KeyInput::ReleaseAllKeys, pushes mouse-up for each pressed button, toggles MMB scroll off and calls SetWindowInputGrabbing(false), with no windowID check. With two windows every focus switch fires FOCUS_LOST(old) then FOCUS_GAINED(new) (1200-1203, KeyInput::Update only). Held modifiers and drags are cancelled, and grab (grabinput) is dropped permanently because nothing re-enables it on FOCUS_GAINED. A focus loss from B while the pointer is in A also cancels A's drag.

*Evidence:*
- `rts/System/SpringApp.cpp:1204` FOCUS_LOST: ReleaseAllKeys (1216), synthetic mouse-ups (1220-1232), MMB unlock (1235), grab off (1240)
- `rts/System/SpringApp.cpp:1200` FOCUS_GAINED only refreshes KeyInput state
- `rts/Rendering/GlobalRendering.cpp:1318` SetWindowInputGrabbing targets sdlWindow only

*Direction:* Treat focus loss as application focus loss only when SDL_GetKeyboardFocus() is null after the event is processed. When focus moves between engine windows, just transfer inputFocusTarget and leave key/button/grab state alone. Re-apply grab policy per target on FOCUS_GAINED.

> verifier A (confirmed): Verified SpringApp.cpp:1200-1241. FOCUS_GAINED only calls KeyInput::Update. FOCUS_LOST has no window-ID check. It clears emulated input, releases all keys, pushes mouse-ups for pressed buttons, toggles MMB scroll off, and calls SetWindowInputGrabbing(false). The only other callers of SetWindowInputGrabbing are the UnsyncedGameCommands.cpp:2525-2532 console command and GlobalRendering.cpp:622, so nothing re-enables grab on focus gain. Single-window behaviour is the same (alt-tab), so the issue is

## Main loop, frame pacing & threading

### LOP-01 - CGame::Draw runs UpdateUnsynced itself, so a second Draw call advances time and counters twice
**Severity:** blocker - **Effort:** M - **Status:** unverified - **Depends on:** TGT-01

The unsynced update (time, interpolation, camera controller, uploaders, UBO) is the first statement of CGame::Draw, and the controller interface offers only one Draw() per loop iteration.

*Why it breaks:* Calling Draw per target re-runs UpdateUnsynced. That bumps drawFrame and numDrawFrames (FPS counts views), and recomputes timeOffset. It computes deltaDrawFrameTime from a lastFrameStart set microseconds earlier, so lastFrameTime is about 0 ms. That value scales mouse-wheel zoom (0.2*lastFrameTime), free-camera dt, particle animation and the cursor spin. It also halves the avgFrameTime average, re-runs camHandler->UpdateController, the model/transform uploaders and the UBO update, and the drawFrame parity flips the GL timer queries. SpringApp::Update is controller-agnostic (one bool Draw(), one swap), so menu, pregame and loadscreen controllers cannot be asked for several views either.

*Evidence:*
- `rts/Game/Game.cpp:1451` Draw calls UpdateUnsynced
- `rts/Game/Game.cpp:1277` drawFrame++, lastTimeOffset/timeOffset recomputation (1275-1353)
- `rts/Game/Game.cpp:1231` deltaDrawFrameTime from lastFrameStart -> lastFrameTime (:1239)
- `rts/Game/Game.cpp:1362` FPS counter (1362-1369)
- `rts/Game/Game.cpp:1376` camera controller, shadow, worldDrawer, UBO updates (1376-1441)
- `rts/Game/CameraHandler.cpp:330` consumer of lastFrameTime (wheel zoom)
- `rts/Game/GameController.h:18` single virtual bool Draw(); no per-view hook
- `rts/System/SpringApp.cpp:902` swap = activeController->Draw()
- `rts/Game/Game.cpp:1241` avgFrameTime mix, gameTime/modGameTime accumulation (1231-1247)
- `rts/Game/Game.cpp:1519` GenerateIBLTextures then Unbind/LoadViewport/WorldDrawer::Draw (1519-1527)
- `rts/Game/Game.cpp:1559` draw-mode reset, avgDrawFrameTime, lastDrawFrameTime (1559-1569)
- `rts/System/SpringApp.cpp:899` one Update then one Draw then one SwapBuffers per frame (899-906)
- `rts/Game/Game.cpp:1226` UpdateUnsynced writes lastFrameStart/Time (1238-1239), drawFrame (1277), timeOffset (1283-1356), FPS (1363)
- `rts/Game/Game.cpp:1448` Draw calls UpdateUnsynced first (1451)
- `rts/Game/Game.cpp:1430` camera->Update and worldDrawer.Update once per frame (1430-1441)
- `rts/Rendering/GlobalRendering.h:130` timing fields (130-151) are CR_MEMBER and stay per-frame

*Direction:* Hoist UpdateUnsynced out of Draw, either into CGame::Update or a BeginFrame hook called once. Split Draw into DrawView(target) (3D) and DrawScreen(target) (GUI/overlays, primary only), and keep EndFrame once. Extend CGameController with something like NumViews() or DrawTarget(i) so SpringApp::Update loops targets only for CGame and other controllers stay single-target.

### LOP-02 - Main loop has a single Draw and a single Swap; no per-target present phase
**Severity:** blocker - **Effort:** M - **Status:** corrected - **Depends on:** SwapBuffers fuses frame-boundary work with per-window present; CTX-02; GL context and drawable topology is hardwired to one window

SpringApp::Update does one controller Draw followed by one SwapBuffers on the primary window. There is no hook to render the 3D world again for another target and present it, and no policy for frames where Draw is skipped.

*Why it breaks:* The engine has exactly one present site on the main path. SpringApp::Update (SpringApp.cpp:899-906) calls activeController->Draw() under CLoadLock, which holds the GL context current on sdlWindow (LoadLock.h:20-21), then calls globalRendering->SwapBuffers. That function (GlobalRendering.cpp:671-718) fuses two things: per-frame ring-buffer boundary work (RenderBuffer::SwapRenderBuffers and PutBufferLocks, :688-689) and the per-window present (SDL_GL_SwapWindow(sdlWindow), :703). CGame::Draw (Game.cpp:1448-1572) is monolithic: it unbinds the FBO and loads the viewport for the primary, calls worldDrawer.Draw() once (:1521-1526), then draws the GUI. Nothing can request a second world pass for another drawable or camera, and the present cannot be split per window.

*Evidence:*
- `rts/System/SpringApp.cpp:899` Update: controller Update/Draw then SwapBuffers
- `rts/System/SpringApp.cpp:906` single SwapBuffers(swap,false)
- `rts/Game/Game.cpp:1521` FBO::Unbind + camera->LoadViewport before the single worldDrawer.Draw()
- `rts/Game/Game.cpp:1526` world is drawn exactly once per frame
- `rts/Rendering/GlobalRendering.cpp:683` swap even when Draw returned false
- `rts/Game/Game.cpp:1264` skipping draws at 2 Hz and returns
- `rts/Game/Game.cpp:1519` GenerateIBLTextures, Unbind, LoadViewport, worldDrawer.Draw
- `rts/Game/Game.cpp:1550` post-screen-pass glEnable/glLoadIdentity
- `rts/Rendering/WorldDrawer.cpp:287` ResetMVPMatrices sets 2D ortho state for the GUI

*Direction:* Do not place the secondary pass after the primary present. Instead:

(1) Split CGlobalRendering::SwapBuffers into EndFrameBuffers() (RenderBuffer swap plus PutBufferLocks, once per frame) and PresentWindow(window).

(2) Add a world-only hook that runs inside the frame, after the primary pass and before EndFrameBuffers. Either call it in CGame::Draw right after worldDrawer.Draw() at Game.cpp:1526, or call a new Game::DrawSecondaryViews() from SpringApp::Update between Draw() and the swap. It loops over secondary targets and does the following for each: SDL_GL_MakeCurrent(win2, ctx), switch the active camera slot as the cubemap pass does, bind and viewport for that target, run a 3D-only world pass, and restore the primary.

(3) After all passes, run EndFrameBuffers once and then present each window.

(4) Gate secondary draws and presents on the primary Draw() return value and on the target being visible.

(5) Set swap interval 0 on secondary windows, and let the primary window own vsync pacing.

(6) Defer controller-agnostic handling: LoadScreen, menu and splash do not need secondary windows in the prototype.

> verifier A (corrected): The core mechanism holds in the current tree. SpringApp::Update does UpdateWindow (SpringApp.cpp:887), then one activeController->Draw() under CLoadLock (899-902), then one globalRendering->SwapBuffers(swap,false) (906). CGame::Draw does FBO::Unbind + camera->LoadViewport + one worldDrawer.Draw() (Game.cpp:1521-1526). ForceSwapBuffers defaults to 1 (GlobalRendering.cpp:62, :284), so the primary window swaps even when Draw returned false (GlobalRendering.cpp:683). A grep of rts/ (excluding lib) f

> verifier B (corrected): The core claim is real, and it is a blocker for the prototype. SpringApp::Update (SpringApp.cpp:899-906) runs one controller Draw and one SwapBuffers on sdlWindow. CGame::Draw (Game.cpp:1521-1526) calls FBO::Unbind, camera->LoadViewport and worldDrawer.Draw() once, with no seam for a second world pass. CWorldDrawer::Draw (WorldDrawer.cpp:302-331) takes no target argument. Without a per-target pass and present, window 2 shows nothing. No workaround such as skipping a pass on the primary avoids th

### LOP-03 - CGame::Draw has no shared-stage/per-view split and relies on deferred unbind
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** Context-sharing and default-framebuffer strategy undecided

Draw runs GenerateIBLTextures then a single WorldDrawer::Draw. There is no notion of once-per-frame shared work versus per-view work, and the shared passes deliberately leave their FBO bound for CGame::Draw to unbind and restore.

*Why it breaks:* Verified: CGame::Draw (Game.cpp:1448-1572) is one monolithic sequence.
- It starts with UpdateUnsynced (:1451).
- It then runs GenerateIBLTextures (:1519), then FBO::Unbind plus camera->LoadViewport (:1522-1524), then worldDrawer.Draw (:1526).
- The screen and GUI pass follows (:1530-1548), and SwapBuffers happens later in SpringApp::Update (SpringApp.cpp:906), fused with RenderBuffer swap and SDL_GL_SwapWindow (GlobalRendering.cpp:688-703).
- CShadowHandler::CreateShadows leaves smOpaqFBO bound (ShadowHandler.cpp:629-633; no Unbind anywhere in CreateShadows or DrawShadowPasses, the only Unbinds are :182 and :428). CubeMapHandler::UpdateReflectionTexture does the same (:139-143).
- The unbind happens only in CGame::Draw. The code comments saying GenerateIBLTextures does it are stale. This deferred-unbind contract is real, but it is safe for any loop that keeps lines 1519-1526 together.

What a naive per-view loop actually does:
- It re-runs CreateShadows with identical matrices. They were computed once per frame in UpdateUnsynced (Game.cpp:1431, ShadowHandler::Update at :128-135).
- It re-runs one cubemap face and advances currReflectionFace twice per frame (CubeMapHandler.cpp:169-175).
- It does not redo the spec, sky or shading textures, because the update flags are consume-on-read (ISky.h:47-49, WaterRendering.h:15-17, SunLighting.h:28-30, SkyLight.h:13-19).
- Output stays correct. Cost is a duplicated shadow pass and cubemap face. Each loop iteration still re-establishes FBO 0 and the viewport.

Hazards the original claim missed:
- A shadow map that is not really shared. The shadow light matrix and projection are fitted to CAMTYPE_PLAYER: ComposeLightMatrix orients to playerCam forward/right/up (ShadowHandler.cpp:505-519), and SetShadowMatrix (:530-533) derives scales from playerCam. The default ShadowProjectionMode is CAM_CENTER (ShadowHandler.cpp:34). A once-per-frame shadow stage gives wrong, clipped or blurry shadows in any view whose camera differs from the player camera.
- A consume-on-read flag problem for any non-shared context. The flags are cleared by the first reader, so a second context that owns its own textures would never see the update.
- FBOs are per-context objects, even with context sharing. smOpaqFBO, reflectionCubeFBO, shadingFBO and the water FBOs must be used from the context that created them. The shared stage must therefore run with the owning context current.

*Evidence:*
- `rts/Game/Game.cpp:1519` GenerateIBLTextures placed before worldDrawer.Draw
- `rts/Rendering/WorldDrawer.cpp:249` IBL shared work (249-285)
- `rts/Rendering/WorldDrawer.cpp:316` WorldDrawer::Draw begins with camera->Update() on the active camera
- `rts/Rendering/ShadowHandler.cpp:629` comment: unbind and VP restoration deferred to caller
- `rts/Rendering/Env/CubeMapHandler.cpp:139` same deferred-unbind comment; currReflectionFace time slicing (145-175)
- `rts/Map/SMF/SMFReadMap.cpp:755` shading pass unbinds and restores globalRendering viewport
- `rts/Rendering/WorldDrawer.cpp:308` Draw() starts with glClear on the bound framebuffer (308-309)
- `rts/Game/Game.cpp:1454` RmlGui::Update() at the top of Draw (once per frame)
- `rts/Game/Game.cpp:1468` eventHandler.DrawGenesis() Lua texture/shader update (once per frame)
- `rts/Game/Game.cpp:1514` minimap->Update() renders into the minimap FBO before the target is chosen
- `rts/Game/Game.cpp:1523` FBO::Unbind() + camera->LoadViewport() is the only 'select target' step and hardcodes the default framebuffer
- `rts/Game/Game.cpp:1526` single worldDrawer.Draw() + ResetMVPMatrices(), then the screen pass at 1530-1548
- `rts/Game/Game.cpp:1562` avgDrawFrameTime and GL timestamps assume one draw
- `rts/System/SpringApp.cpp:906` single SwapBuffers after activeController->Draw()
- `rts/Rendering/GlobalRendering.cpp:688` RenderBuffer::SwapRenderBuffers + PutBufferLocks + SDL_GL_SwapWindow(sdlWindow) are one fused per-frame/per-window step
- `rts/Rendering/WorldDrawer.cpp:302` Draw assumes pre-set viewport; camera->Update() at 316
- `rts/Rendering/WorldDrawer.cpp:287` ResetMVPMatrices leaves ortho + BLEND + no depth test
- `rts/Game/Game.cpp:1550` post-pass state assumptions (glLoadIdentity)
- `rts/Rendering/WorldDrawer.cpp:437` water->UpdateWater inside the main alpha pass (per view)
- `coding-agents/ENGINE_PERFORMANCE.md:15` many draw iterations run between sim frames

*Direction:* Keep the idea of a view abstraction (BeginView, Draw, EndView; make target current, FBO 0, viewport, active camera), but fix the stage assignment:
- Prototype minimum: loop the existing body (GenerateIBLTextures through worldDrawer.Draw) per view. Do not call UpdateUnsynced, RmlGui::Update, DrawGenesis or minimap->Update per view. Skip the screen/GUI pass. Swap each window separately.
- Shared stage, once per frame, in the context that owns the FBOs: the sun, sky and shading texture updates (already flag-gated) and the cubemap face cycle. Only run the shadow pass once if every view's frustum fits the same shadow projection.
- Shadows are camera-fitted, so the shadow stage must be per-view, or fitted to the union of all view frustums. Make shadow matrices a function of the view instead of the one-per-frame CAMTYPE_PLAYER computation at Game.cpp:1431.
- Water: the CBumpWater::Update part (dynwaves, coast) belongs to the once-per-frame stage. IWater::UpdateWater, which renders the camera-dependent reflection and refraction FBOs (BumpWater.cpp:561-575, 978-1041), must stay per-view. It also hard-resets with globalRendering->LoadViewport (:572, :986), so it needs view-aware viewport handling.
- Wrap sub-passes in a save/restore scope for FBO binding, viewport, active camera and draw mode, rather than relying on the CGame-level unbind.
- Downgrade this from blocker to risk. The true prerequisites are the context-sharing and default-framebuffer decision, and the per-view camera and globalRendering size coupling (Camera.cpp:144-153).

> verifier B (corrected): The structural facts are right, but the failure mechanism is overstated and the fix direction is partly wrong. A split already exists at the function level: Game.cpp:1519 calls GenerateIBLTextures() once, and 1521-1524 (FBO::Unbind + camera->LoadViewport) is the exact hand-off that the deferred-unbind comments in ShadowHandler.cpp:629-633 and CubeMapHandler.cpp:139-143 rely on. A minimal 2-view prototype only needs `for each view { make target current; FBO::Unbind; set active camera; LoadViewpor

> verifier A (corrected): The structural facts hold. CGame::Draw has no shared/per-view split. CreateShadows (ShadowHandler.cpp:633) and UpdateReflectionTexture (CubeMapHandler.cpp:143) leave their FBO bound despite the comments promising an unbind in GenerateIBLTextures. GenerateIBLTextures (WorldDrawer.cpp:249-285) contains no unbind. Only CGame::Draw unbinds, at Game.cpp:1522-1524. Most cited line numbers check out. Three parts of the stated mechanism do not survive reading the code, and the 'blocker' severity does no

### LOP-04 - globalRendering->active is app-wide but driven by any window and gates all drawing
**Severity:** risk - **Effort:** S - **Status:** unverified - **Depends on:** SDL window and input events are not dispatched by windowID; Window lifecycle events are applied to the primary window regardless of which window sent them; SDL window events are not routed by window id; Draw path has no split between once-per-frame work and per-target work; SDL window events are not attributed to a window

`active` is set false on any MINIMIZED/HIDDEN and true on SHOWN/RESTORED/MAXIMIZED. It gates CGame::Draw (sleep and early-out), FBO::IsReady, mouse input, FreeController and the Lua menu.

*Why it breaks:* Minimizing or hiding window 2 deactivates window 1: CGame::Draw returns early after a 10 ms sleep (except a forced draw every 30 s), input is dropped, and in fullscreen FBO::GLContextLost downloads all reload-on-alt-tab FBOs. The reverse also happens. Visibility must be per target, with an app-level any-visible aggregate for sound and FBO readiness.

*Evidence:*
- `rts/System/SpringApp.cpp:1160` SHOWN/RESTORED/MAXIMIZED: active=true, sound, GLContextReinit (1165-1175)
- `rts/Game/Game.cpp:1471` inactive: sleep and render only every 30 s
- `rts/Rendering/GL/FBO.cpp:36` FBO::IsReady() returns globalRendering->active
- `rts/System/Input/MouseInput.cpp:202` mouse code reads active (also MouseHandler.cpp:815, FreeController.cpp:87)
- `rts/System/SpringApp.cpp:1179` MINIMIZED/HIDDEN handling
- `rts/Game/Camera/FreeController.cpp:87` controller state reset when !active
- `rts/System/SpringApp.cpp:1095` MainEventHandler has no windowID filter
- `rts/System/SpringApp.cpp:1165` active=true on SHOWN/RESTORED (and :1185 false)
- `rts/System/SpringApp.cpp:1143` SIZE_CHANGED rewrites global geometry (1135-1148)
- `rts/Rendering/GL/FBO.cpp:38` FBO::IsReady() == globalRendering->active
- `rts/Game/UI/MouseHandler.cpp:815` active read by mouse code (also MouseInput.cpp:202)
- `rts/System/SpringApp.cpp:906` SwapBuffers(swap,false) called once for the primary
- `rts/Rendering/GlobalRendering.cpp:683` swap skipped only if !allow && !forceSwapBuffers; default ForceSwapBuffers=1 (:62)
- `rts/Rendering/Textures/TextureRenderAtlas.cpp:359` atlas update gated on FBO::IsReady
- `rts/Menu/LuaMenuController.cpp:116` menu draw gated on active
- `rts/Game/Game.cpp:1451` UpdateUnsynced early return skips all drawing
- `rts/Rendering/GlobalRendering.cpp:703` SDL_GL_SwapWindow(sdlWindow) only
- `rts/Rendering/GlobalRendering.cpp:1596` SDL_WINDOW_MINIMIZED flag check already used

*Direction:* RenderTarget::visible per target; active = primary.visible for input/sound; skip a target's draw when that target is not visible while still presenting visible ones.

### LOP-05 - VSync is applied once to whichever drawable is current; two blocking swaps per frame and mixed refresh rates are unhandled
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** Present path hard-wired to the single sdlWindow and assumes it is current; CTX-02; Secondary window creation path and pixel-format parity; SwapBuffers fuses frame-boundary buffer bookkeeping with presenting one window; Window and GL context are CGlobalRendering singletons with no per-target MakeCurrent; SwapBuffers fuses window present with ring-buffer and fence bookkeeping; Context ownership hardwired to the single sdlWindow (make-current, swap, destroy, load handoff)

CVerticalSync stores one interval and calls SDL_GL_SetSwapInterval at config change. SDL documents this as applying to the current context, and WGL/GLX scope it to the current drawable. There is also no engine frame limiter, so vsync is the only pacing.

*Why it breaks:* VSync is a single process-global setting (CVerticalSync::interval, VerticalSync.h:21) applied exactly once through SDL_GL_SetSwapInterval (VerticalSync.cpp:76/81/87) while the only window/context is current. That happens at SpringApp.cpp:272 -> GlobalRendering.cpp:1607-1613. SetInterval early-outs when the value is unchanged (VerticalSync.cpp:66), so later UpdateGLConfigs calls (SpringApp.cpp:1117, :1144) and any re-call after switching windows do nothing. A second window or context created later therefore keeps the driver default swap interval and ignores the user's VSync. The setter also writes the single "VSync" config key (:69), so it is not modelled per target. Present is one SDL_GL_SwapWindow(sdlWindow) per frame (GlobalRendering.cpp:703). Adding a second blocking swap of a vsynced window on the same thread serialises two vblank waits, giving about half the refresh rate, or locking to the slower monitor when refresh rates differ. If VSync=0 there is no engine frame limiter (no MaxFPS-style config exists, and the only draw-path sleep is Game.cpp:1472 while inactive), but that is existing single-window behaviour, not a new multi-target defect. lastSwapBuffersEnd (GlobalRendering.cpp:717) feeds camera transition timing (CameraHandler.cpp:348, :406-412). It stays correct only if it is updated after the primary's blocking swap. DwmFlush (GlobalRendering.cpp:694-711, default off) would run once per swap if the swap were looped per window.

*Evidence:*
- `rts/Rendering/VerticalSync.cpp:19` VSync default -1 (adaptive)
- `rts/Rendering/VerticalSync.cpp:66` early return when interval already equals cached value
- `rts/Rendering/VerticalSync.cpp:53` Toggle reads the current drawable's interval
- `rts/Rendering/GlobalRendering.cpp:1607` UpdateGLConfigs -> SetInterval, called on SIZE_CHANGED
- `rts/Rendering/GlobalRendering.cpp:694` DwmFlush before/after swap (to 711)
- `rts/Game/CameraHandler.cpp:348` transition timing from lastSwapBuffersEnd + 1000/FPS (also :406-412)
- `vclibs64/include/SDL2/SDL_video.h:2094` SetSwapInterval is 'for the current OpenGL context'
- `rts/Game/Game.cpp:1472` only other pacing is a 10 ms sleep when inactive; no frame limiter exists
- `rts/Rendering/VerticalSync.cpp:76` SDL_GL_SetSwapInterval affects the current drawable only
- `rts/Rendering/VerticalSync.cpp:62` SetInterval caches by value (66) and calls SDL_GL_SetSwapInterval for the current window (76-87)
- `rts/Rendering/GlobalRendering.cpp:696` DwmFlush before/after each SDL_GL_SwapWindow (696-711)
- `rts/Rendering/GlobalRendering.cpp:683` allowSwapBuffers/forceSwapBuffers gate
- `rts/Rendering/GlobalRendering.cpp:1611` UpdateGLConfigs re-applies vsync only to whatever is current
- `rts/Rendering/GlobalRendering.cpp:703` single swap
- `rts/System/SpringApp.cpp:906` one SwapBuffers per frame

*Direction:* Make the swap interval a per-target property applied in Present right after MakeCurrent of that target, with a path that bypasses CVerticalSync's cached early-out, or invalidates the sentinel when a window is created or made current. Prototype default: the primary keeps the user's VSync and is swapped last so its blocking swap paces the frame. Secondary targets use interval 0 and are swapped first. Update lastSwapBuffersEnd and run DwmFlush only for the primary. A simple frame limiter is optional, needed only when the primary is not vsynced. Do not let a secondary window write the shared "VSync" config key. Independent per-monitor cadence is a longer-term design decision.

> verifier B (confirmed): The core mechanism is real and verified in code. CVerticalSync::SetInterval (rts/Rendering/VerticalSync.cpp:62-89) caches a single `interval` (initial -1000, VerticalSync.h:21) and early-outs on an unchanged value (:66). The SDL_GL_SetSwapInterval calls (:76, :81, :87) apply only to whichever drawable is current. SDL_video.h documents the call as per current context, and WGL/EGL/GLX-EXT scope it to the current drawable. UpdateGLConfigs (GlobalRendering.cpp:1607-1613) calls only verticalSync->Set

> verifier A (corrected): The core mechanism holds in the current tree. VSync is a process-global value applied once to whichever context/window is current at startup, and the cached early-out blocks any re-apply. Verified: (1) VerticalSync.cpp:62-89 stores one `interval` (VerticalSync.h:21, sentinel -1000) and calls SDL_GL_SetSwapInterval only when the value changes (:66). (2) The only initial application is SpringApp.cpp:272 -> GlobalRendering.cpp:1607-1613 UpdateGLConfigs -> verticalSync->SetInterval(), run once when 

### LOP-06 - Draw time feeds sim lag protection and would slow multiplayer games
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** none

The profiler 'Draw' share and gu->avgDrawFrameTime feed both the client CPU-usage report to the server and the sim/draw time balance. A client that draws two windows looks CPU-heavy and can slow the game for everyone.

*Why it breaks:* NetCommands.cpp:83 reads the profiler "Draw" percentage, divides it by max(1, FPS) and multiplies by minDrawFPS. It adds only a smoothed adjustment (lines 103-106) via linearstep(lowThreshold, highThreshold, simProcUsage) * drawProcUsage. That adjustment is near zero unless simProcUsage is already above roughly 0.65-0.9. The total is clamped to 1 and sent at line 112. Doubling the Draw time raises the reported CPU usage only on clients whose sim load is already high. GameServer.cpp:909-911 counts only local players or non-spectators towards refCpuUsage. A spectator or replay second-window client therefore cannot slow the game for others. GetNetMessageProcessingTimeLimit (NetCommands.cpp:252-256) uses gu->avgDrawFrameTime, which Game.cpp:1564 computes over the whole of Draw() from currentTimePreDraw (line 1455). It is clamped to between 5 ms and 1000/minDrawFPS. Its effect is to shrink the per-frame sim burst on the local client. A separate timer for secondary targets (the direction proposed in the claim) is a sound fix.

*Evidence:*
- `rts/Net/NetCommands.cpp:83` drawProcUsage derived from the 'Draw' timer percentage
- `rts/Net/NetCommands.cpp:112` SendCPUUsage(totalProcUsage) to the server
- `rts/Net/NetCommands.cpp:253` sim/draw balance uses gu->avgDrawFrameTime
- `rts/Net/GameServer.cpp:910` refCpuUsage = highest player CPU usage
- `rts/Game/Game.cpp:1562` avgDrawFrameTime covers all of Draw()
- `rts/Game/Game.cpp:1457` SCOPED_SPECIAL_TIMER("Draw") around the whole draw
- `rts/System/TimeProfiler.cpp:416` AddTimeRaw accumulates all scopes of the same name per frame
- `rts/System/TimeProfiler.h:26` special timer registered by literal name

*Direction:* Count secondary-target draw time under a separate timer name. Exclude it from the "Draw" bucket and from avgDrawFrameTime, or normalise both by the number of targets. This is only needed once the feature is used in non-spectator multiplayer games. The prototype can ignore it.

> verifier B (corrected): The mechanism is real. NetCommands.cpp:83 takes the profiler "Draw" percentage and scales it by minDrawFPS/FPS. Lines 103-112 add it to the CPU usage sent to the server, but only once simProcUsage passes a threshold. Line 253 feeds avgDrawFrameTime into the sim burst limit. Both would grow if a second window is drawn inside the same "Draw" scope. It does not threaten the 2-window, 3D-only prototype. It cannot crash, raise GL errors or produce wrong output. The effect is a throttled sim speed in 

> verifier A (corrected): The mechanism is real and the cited lines match the current tree. The severity is overstated for the 2-window prototype. The effect is limited and conditional, and it matters only for non-spectator clients in networked games, which the prototype does not need.

### LOP-07 - Extra window lifecycle versus loading, reload, quit and non-game controllers
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** GL context and drawable topology is hardwired to one window; TGT-01; Secondary window creation path and pixel-format parity

Window creation, the load-thread context handoff, SpringApp::Reload and Kill ordering all assume one window. Extra windows can be created on the wrong thread, outlive the game, or be left undrawn when the active controller is not CGame.

*Why it breaks:* Window creation, destruction and event handling all assume one SDL window and one draw entry point, so secondary windows get three kinds of treatment. (a) Creation thread: the only safe creation point is the main thread. CGame::Draw is main-thread only, so creating targets there needs no QueuedFunction. QueuedFunction is needed only if creation is attempted from CGame::Load on the load thread. During MT loading the primary context alternates between the main and load threads under CLoadLock, one lock per Draw or load step (LoadLock.h:14-35, SpringApp.cpp:901). A secondary context that shares with the primary one must be created while the primary is current on the creating thread. After loading, the lock is a no-op and the context stays on the main thread (LoadScreen.cpp:172-176). (b) Drawing and swapping: SpringApp::Update (SpringApp.cpp:899-906) draws and swaps exactly one window via activeController->Draw(). CLoadScreen, SelectMenu, LuaMenuController, PreGame and CGame's skip/inactive early returns (Game.cpp:1264-1273, 1452, 1471-1479) never touch other windows, so they would be left stale or undefined. Reload (SpringApp.cpp:758-876) keeps the window while deleting game and pregame. Kill (SpringApp.cpp:1025-1057) deletes game state, then CCameraHandler, then PreKill, then the single window. (c) Event routing: SpringApp::MainEventHandler ignores event.window.windowID (no windowID or SDL_GetWindowID use outside rts/lib). SDL_WINDOWEVENT_CLOSE (SpringApp.cpp:1249-1251) calls RequestQuit, so closing window 2 quits the whole game. HIDDEN/MINIMIZED and SHOWN/RESTORED/MAXIMIZED on any window flip the single globalRendering->active flag (SpringApp.cpp:1160-1198), and CGame::Draw returns early after DrawGenesis when it is false (Game.cpp:1471-1479), which starves all targets. SIZE_CHANGED and MOVED on any window run UpdateGLGeometry, InitGLState and UpdateInterfaceGeometry against the main window's state (SpringApp.cpp:1135-1148).

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:1202` SDL calls from non-main thread via QueuedFunction
- `rts/System/LoadLock.h:14` lock/unlock make the context current/clear it for the load thread
- `rts/Game/LoadScreen.cpp:172` context restored to the main thread after loading
- `rts/System/SpringApp.cpp:758` Reload keeps the window while destroying game state
- `rts/System/SpringApp.cpp:1057` CGlobalRendering::KillStatic after game and fonts are gone
- `rts/Rendering/GlobalRendering.cpp:617` DestroyWindowAndContext destroys only sdlWindow
- `rts/System/SpringApp.cpp:899` Update: controller->Draw then SwapBuffers(swap) (899-906)
- `rts/Game/LoadScreen.cpp:313` load screen bumps drawFrame and swaps itself (313-326)
- `rts/Menu/SelectMenu.cpp:188` menu bumps drawFrame
- `rts/Menu/LuaMenuController.cpp:116` menu draw gated by active (116-120)
- `rts/Game/Game.cpp:1264` skipping path returns before the main draw (1264-1273)

*Direction:* Tie extra-target lifetime to CGame scope instead of a CGlobalRendering registry gated by controller type. Create on the main thread after loading finishes (first CGame::Draw, after LoadScreen::Kill at LoadScreen.cpp:172-176). Destroy at the start of ~CGame (Game.cpp:289), before KillRendering. That single hook runs before KillStatic in both Reload (SpringApp.cpp:787) and Kill (SpringApp.cpp:1025), and the context is current on the main thread there. No separate Reload/Kill/controller-gating code is needed, since windows cannot exist outside a CGame. Keep QueuedFunction only for later runtime creation or destruction requested from Lua or the load thread.

Add window-event routing by windowID in SpringApp::MainEventHandler (SpringApp.cpp:1098-1252). Without it, window 2's CLOSE quits the game, its MINIMIZED/HIDDEN sets globalRendering->active=false (which makes CGame::Draw stop drawing both windows), and its FOCUS_LOST releases all keys and ungrabs the mouse. This is probably its own catalogue entry; the prototype can tolerate it.

> verifier A (corrected): The core claim holds in the current tree. Nothing today creates an extra window, so this is a forward-looking constraint rather than an existing bug, but each cited fact checks out. (1) SDL calls from the load thread deadlock, and the code routes around that with QueuedFunction (GlobalRendering.cpp:1204-1213 for the title, 1320-1329 for input grab, WindowManagerHelper.cpp:84 for the icon). Only the main thread drains that queue, in CGlobalRendering::UpdateWindow (GlobalRendering.cpp:1288-1293, c

> verifier B (corrected): The code facts check out, but none of them breaks the 2-window, 3D-only prototype, so "risk" is too high. The claim is real and belongs in the catalogue as "full" (robust reload/quit, runtime-created targets), with only a small subset needed by the prototype. Effort: S for that subset, M for the full lifecycle plus window-event routing.

Verified: SetWindowTitle and SetWindowInputGrabbing use QueuedFunction because SDL deadlocks off the main thread (GlobalRendering.cpp:1202-1214, 1322-1329). CLo

### LOP-08 - Non-game states have their own draw loops that bump drawFrame
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** none

Load screen, select menu, Lua menu and DrawSkip each have separate draw paths and increment drawFrame themselves.

*Why it breaks:* Three non-game draw loops (LoadScreen.cpp:313, SelectMenu.cpp:188, LuaMenuController.cpp:120) each increment the single global drawFrame. The skipping branch (Game.cpp:1264-1273) returns before the drawFrame++ at Game.cpp:1277, so a secondary window would show no updates for most of a fast-forward, apart from 2Hz DrawSkip output on the primary window only. The skip path does not touch drawFrame. Per-frame caches keyed on drawFrame therefore belong to a single draw counter, so a secondary window needs its own draw-state or frame handling.

*Evidence:*
- `rts/Game/LoadScreen.cpp:313` drawFrame++
- `rts/Menu/SelectMenu.cpp:188` drawFrame++
- `rts/Menu/LuaMenuController.cpp:120` drawFrame++ in menu draw
- `rts/Game/Game.cpp:1264` skipping branch draws at 2Hz and returns (1264-1273)

*Direction:* Only draw secondary targets when the game is in the running, non-skipping state. Give each target a defined behaviour (blank or hold the last frame) during load, menu and skip.

> verifier A (corrected): The load screen, SelectMenu and LuaMenuController paths do bump globalRendering->drawFrame outside CGame::Draw (LoadScreen.cpp:313, SelectMenu.cpp:188, LuaMenuController.cpp:120). The skip branch at Game.cpp:1264-1273 does draw at 2Hz via DrawSkip() and returns before the main draw. But DrawSkip does not bump drawFrame: the only drawFrame increment in Game.cpp is line 1277, which is after the skip return. The claim that DrawSkip increments drawFrame itself is wrong. The problem is a prototype-sc

### LOP-09 - CPU and draw cost scale linearly with views (all-object scans per pass)
**Severity:** polish - **Effort:** M - **Status:** corrected - **Depends on:** Draw-flag bitmask encodes visibility for a single player camera

Update scans all objects with 3 frustum tests, every pass scans all bins and rebuilds instance maps, and every uniform slot is dirtied each Update. A second view roughly doubles this at a 10k-17k unit scale target.

*Why it breaks:* Verified: (1) CUnitDrawerData::Update (UnitDrawerData.cpp:214-266) loops all unsortedObjects, MT-chunked at :243-247, calling UpdateDrawPos, icon state and UpdateCommon. UpdateCommon (ModelDrawerData.h:221-231) calls UpdateObjectDrawFlags and UpdateObjectUniforms for every unit. UpdateObjectDrawFlags (UnitDrawerData.cpp:430-487) does up to 3 cam->InView tests (PLAYER, UWREFL if CanDrawReflectionPass, SHADOW if SHADOWGEN_BIT_MODEL; CAMTYPE_ENVMAP=3 is the exclusive bound, Camera.h:16-19), after early-outs for noDraw/icon/void/LOS. (2) UpdateObjectUniforms (ModelDrawerData.h:206) always picks the non-const GetObjUniformsArray, which calls updateList.SetUpdate(offset) (ModelsMemStorage.cpp:97-103). Every slot is dirty every Update, so ModelUniformsUploader::Update (ModelsDataUploader.cpp:96-107) copies one contiguous region, the whole array, each frame. (3) Each unit/feature pass loops every bin and filters by flag (UnitDrawer.cpp:1941-1960 opaque, :1988-2003 alpha), calls AddToSubmission, then S3DModelVAO::Submit (Models/3DModelVAO.cpp:399-445) concatenates instance data, uploads it with instVBO.SetBufferSubData and clears the map. Per view that is up to 7 unit passes and 7 feature passes: opaque 1 (WorldDrawer.cpp:372-373), alpha 2 (WorldDrawer.cpp:413-414 and 453-454), water reflection opaque+alpha (IWater.cpp:163-169), water refraction opaque+alpha (IWater.cpp:206-212). The reflection and refraction passes only run when the water renderer uses them.

Corrections: (a) The Update-side costs in (1) and (2) are per frame, not per view. CGame::UpdateUnsynced calls worldDrawer.Update -> CUnitDrawer::UpdateStatic -> modelDrawer->Update() once (Game.cpp:1433, WorldDrawer.cpp:225-226, ModelDrawer.h:78-82), then transformsUploader.Update() and modelUniformsUploader.Update() (Game.cpp:1434-1435). They only double if the design calls Update per window; the whole-array dirtying exists today with one view. Doubling is only forced for the part that is camera-dependent. (b) The dependency note should be stated as a mechanism: the classification writes a single bitmask on the object (o->drawFlag, ModelDrawerData.h:207, UnitDrawerData.cpp:461-480) from the one global PLAYER/UWREFL/SHADOW camera set (CCameraHandler::GetCamera at :437). Window 2 cannot reuse it, so it needs a second per-view classification, and that is a correctness item rather than a perf item. (c) The shadow pass is not an obvious per-view multiplier: it runs once per frame in GenerateIBLTextures, but the shadow matrix is fitted to the player camera (ShadowHandler.cpp:130-148, 530-564). A second view either shares one shadow map or needs its own.

*Evidence:*
- `rts/Rendering/Units/UnitDrawerData.cpp:243` MT loop over all units with camera-dependent work
- `rts/Rendering/Models/ModelsMemStorage.cpp:97` non-const GetObjUniformsArray dirties the slot on every call
- `rts/Rendering/Units/UnitDrawer.cpp:1941` each pass iterates all object bins; Submit rebuilds instance data (3DModelVAO.cpp:399-445)
- `coding-agents/ENGINE_PERFORMANCE.md:7` scale target about 10k units, 17.7k seen

*Direction:* Keep as a full/scale item and reword: do not call the whole CUnitDrawerData::Update per view. Keep the view-independent work (draw pos, uniforms, SSBO upload) once per frame. Replace the single o->drawFlag with a per-view classification computed for all views in one MT sweep, or cache per-view visible lists, so each view's 7 passes do not re-scan all bins. Measure 2 views at 10k units before optimizing; for the prototype, simply accepting about 2x draw cost is fine.

> verifier B (corrected): Linear-in-views cost is real but inherent to drawing N views, and it cannot make the 2-window 3D-only prototype crash, error or show wrong output. Half the stated mechanism is also wrong. (1) The uniform and transform uploads do not scale with views. UpdateUnsynced runs transformsUploader.Update() and modelUniformsUploader.Update() once per draw frame (Game.cpp:1433-1435), not per window. The per-piece transform work is frame-guarded (ModelDrawerData.h:167, `lastUploadFrameIt->second >= gs->fram

> verifier A (corrected): The cited code does what is claimed, but the scaling attribution is partly wrong. All-object Update scan, dirtying of every uniform slot and the whole-array SSBO upload run once per frame, not once per view, so they only double if someone chooses to call Update per window. The unavoidable per-view cost is the draw passes. For the 2-window 3D-only prototype this is an expected cost, not a crash or visible glitch, so severity is full (needed at the 10k-unit scale target), not risk.

### LOP-10 - Draw-bracketed and draw-frequency-coupled global state assumes one draw per frame
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** Draw path has no split between once-per-frame work and per-target work

Several globals are mutated or advanced inside the draw path: team colours (TeamHighlight Enable/Disable), a static spin timer, avgDrawFrameTime (which feeds sim/draw balance) and GL timer queries.

*Why it breaks:* TeamHighlight bracket: safe if it wraps the whole multi-target phase on one thread. avgDrawFrameTime: total draw time across all targets is the right input for the sim/draw balance in GetNetMessageProcessingTimeLimit. It only shifts the balance, and mainly while reconnecting. GL timer queries (GlobalRendering.cpp:725) are profiling-only and bound to the primary context. DrawMiniMapMarker's static spinTime (GuiHandler.cpp:3967) is already frame-time driven but would run faster if called once per target. It is GUI-only and outside the prototype.

*Evidence:*
- `rts/Rendering/TeamHighlight.cpp:24` Enable mutates team->color; Disable (49) restores
- `rts/Game/Game.cpp:1512` Enable at 1512, Disable at 1560, around the single draw
- `rts/Game/Game.cpp:1564` gu->avgDrawFrameTime mixed from total draw time
- `rts/Net/NetCommands.cpp:253` avgDrawFrameTime drives sim/draw time balance
- `rts/Rendering/GlobalRendering.cpp:725` glQueryCounter indexed by drawFrame&1 in the current context
- `rts/Game/UI/GuiHandler.cpp:3967` static spinTime accumulates per DrawMiniMapMarker call

*Direction:* Keep the Enable/Disable bracket around the whole multi-target phase. Keep avgDrawFrameTime as the total, or add per-target stats for the profiler. Restrict the GL timestamp queries to the primary context. Defer the minimap marker spinTime until GUI rendering is added per target.

> verifier A (corrected): The cited code exists and matches, but most items are not defects for N targets, and the claim overstates them. TeamHighlight Enable/Disable (Game.cpp:1512/1560) wrap the whole draw on one thread, so they are correct for N targets as long as the bracket stays around the multi-target phase. Disable has no early-out path; it runs unconditionally at 1560. avgDrawFrameTime (Game.cpp:1562-1564) measures the total draw time per frame. NetCommands.cpp:253 uses it to balance sim against draw, so includi

### LOP-11 - DWMFlush compositor wait is bolted into the per-swap path and would stack per window
**Severity:** polish - **Effort:** S - **Status:** unverified - **Depends on:** none

On Windows, an optional DwmFlush() is called immediately before or after SDL_GL_SwapWindow inside CGlobalRendering::SwapBuffers. DwmFlush blocks until the next desktop-compositor pass, and it is not tied to any window.

*Why it breaks:* With N windows each presented through SwapBuffers, the DWMFlush config (1 = before swap, 2 = after swap) would block once per window per frame. That adds up to N compositor-pass waits, up to one refresh each, on top of the stacked vsync blocking. The flush does not target a particular window, so it should be issued once per frame after all windows are presented. The config comment already notes it can block for up to a frame with vsync off. The default is 0, so the prototype is only hit by users who enabled it. The setting is also a single global int (forceDWMFlush) with no per-window meaning, and the Tracy zones for it are unnamed by window.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:694` DwmFlush called before SDL_GL_SwapWindow when forceDWMFlush == 1
- `rts/Rendering/GlobalRendering.cpp:703` single SDL_GL_SwapWindow(sdlWindow) with DwmFlush bracketing it
- `rts/Rendering/GlobalRendering.cpp:706` DwmFlush called after the swap when forceDWMFlush == 2
- `rts/Rendering/GlobalRendering.cpp:76` config comment: DWMFlush may block up to one frame and is off by default

*Direction:* Move DwmFlush out of the per-window swap into a once-per-frame present phase that runs after every window has been swapped. Pick the pre or post position once per frame, not once per window. Document that the setting is global.

## Overlays, fonts, GUI & capture

### OVL-01 - No 3D-only pass mask: everything after the world pass runs unconditionally
**Severity:** risk - **Effort:** S - **Status:** corrected - **Depends on:** Draw path has no split between once-per-frame work and per-target work

The prototype needs 'terrain/units/sky/water/projectiles only', but the engine has no switch for that. Ten screen-space steps run unconditionally after the world pass, and hideInterface only skips input receivers and the interface widgets.

*Why it breaks:* If window 2 reuses CWorldDrawer::Draw() unchanged, it renders world-space UI and Lua overlays with the primary's mouse and selection state: selectedUnitsHandler.Draw, DrawMiscObjects (command lines, icons, cursorIcons, selection box, guihandler->DrawMapStuff, map marks) and Lua DrawWorld/DrawWorldPreUnit/DrawPreDecals/DrawWaterPost/DrawGroundPre*/DrawWorldPreParticles. The ray-based ones are wrong under a different camera. The Lua callins may also touch context-bound objects (VAOs, FBOs) from the wrong context. The Game.cpp:1533-1547 screen block is a non-issue as long as window 2 does not call CGame::Draw.

*Evidence:*
- `rts/Game/Game.cpp:1533` unit icons screen pass, DrawScreenEffects, hud, debugDrawerAI, receivers, input text, widgets, Rml, cursor, DrawScreenPost (1533-1547)
- `rts/Game/Game.cpp:1580` hideInterface gates only DrawInputReceivers' receiver loop
- `rts/Game/Game.cpp:1607` hideInterface gates only DrawInterfaceWidgets
- `rts/Game/Game.cpp:1595` minimap is drawn even when the interface is hidden in dualScreenMode
- `rts/Rendering/WorldDrawer.cpp:327` DrawMiscObjects called as one block
- `rts/Rendering/WorldDrawer.cpp:366` selectedUnitsHandler.Draw in DrawOpaqueObjects
- `rts/Rendering/WorldDrawer.cpp:383` DebugColVolDrawer/DebugVisibilityDrawer/pathDrawer (383-385)
- `rts/Rendering/WorldDrawer.cpp:482` unit icons are world-space content inside DrawMiscObjects
- `rts/Rendering/WorldDrawer.cpp:466` DrawMiscObjects: DrawCommands 473, cursorIcons 485, DrawSelectionBox 487, DrawMapStuff 488, marks 490
- `rts/Game/Game.h:179` bool hideInterface global
- `rts/Rendering/GlobalRendering.h:244` draw* toggles are process-global (244-263)
- `rts/Rendering/WorldDrawer.cpp:323` eventHandler.DrawWorld (Lua) inside the same pass
- `rts/Game/UI/MouseHandler.cpp:459` selection box from camera right/up and bp.camPos/dir
- `rts/Game/UI/GuiHandler.cpp:3573` DrawMapStuff traces camera->GetPos() + mouse->dir at draw time (3900 GetDefaultCommand)
- `rts/Rendering/DebugDrawerQuadField.cpp:113` debug draw also pairs camera with mouse->dir

*Direction:* 1) Do not re-enter CGame::Draw for window 2. Add a per-target entry point (for example CGame::DrawWorldTarget(target)) that binds the target's context, FBO 0 and viewport, sets the camera, and calls only the world pass. Keep the screen block in the primary-only path. This is the same split as the dependsOn problem and is effort S once that split exists.
2) Add a lightweight per-target flag (a bool or small enum in a RenderTarget/DrawContext struct, effort S-M), not a 12-bit mask. Consult it inside CWorldDrawer::Draw and its sub-drawers to skip: eventHandler world callins (ideally by an early-out in EventHandler so the scattered call sites are covered), selectedUnitsHandler.Draw, DebugColVolDrawer/DebugVisibilityDrawer/pathDrawer, and all of DrawMiscObjects (or at least the mouse/ray-dependent parts: DrawSelectionBox, guihandler->DrawMapStuff, cursorIcons, lineDrawer/command drawing).
3) Defer the full DrawPassMask until per-target GUI or Lua is actually in scope (classify as 'full', effort M).

> verifier A (corrected): The core mechanism holds in the current tree. CGame::Draw (Game.cpp:1530-1548) runs the whole screen block with no per-target or pass switch, and the only existing switch, game->hideInterface, is far narrower than the claim implies. No pass mask or render-target concept exists anywhere in rts/ (grep for RenderTarget, DrawPassMask and similar found nothing). Several details in the claim are inaccurate and are corrected below. I keep blocker because the prototype spec says both windows are 3D-only

> verifier B (corrected): The facts are accurate. Game.cpp:1533-1547 does run ten unconditional screen steps, and the WorldDrawer.cpp evidence lines check out. But "blocker" is wrong, and the proposed fix is aimed at the wrong place.

1. The screen block is not a prototype blocker. CGame::Draw (Game.cpp:1448-1572) is called once per frame by SpringApp.cpp:902. The 2-window prototype never needs to run the screen block for window 2. The second window only needs a new entry point that makes its context current, sets the ca

### OVL-02 - Overlay-style drawer entry points called from the 3D pass use ambient camera and viewport
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** RenderView definition: per-target parameters consumed by model drawers

Selection volumes, world and screen icons, build squares, build icons and minimap icons are drawer functions invoked from world or screen passes, using the ambient camera and global viewport.

*Why it breaks:* Split into two groups. (A) True overlays: selectedUnitsHandler.Draw (WorldDrawer.cpp:366; SelectedUnitsHandler.cpp:605-724 uses ambient GL state and the rb shader, since the matrix push/pop lines 673-680 are commented out; it also skips units via GetIsIcon at 641 and calls commandDrawer->DrawQuedBuildingSquares at 705/713); DrawUnitIconsScreen (Game.cpp:1533-1534; only active when UseScreenIcons is true, default false; divides by globalRendering->viewSizeX/Y at UnitDrawer.cpp:845-848 and calls camera->CalcViewPortCoordinates at 918/950); DrawBuildIcons (CursorIcons.cpp:182; camera->InView at UnitDrawer.cpp:1801 and 1847); ShowUnitBuildSquare (GuiHandler.cpp:3874, via guihandler->DrawMapStuff at WorldDrawer.cpp:488); DrawUnitMiniMapIcons (MiniMap.cpp:1921 and LuaOpenGL.cpp:1405; takes MiniMapIconDrawParams). These are skipped in window 2 for the prototype. Only the first three read the ambient camera or viewport, and ShowUnitBuildSquare and DrawUnitMiniMapIcons read neither. (B) World unit icons, DrawUnitIcons (WorldDrawer.cpp:481-482, enabled whenever !UseScreenIcons, the default): this is the LOD and radar-blip representation of units, not an overlay. It reads the ambient camera for billboard orientation (UnitDrawer.cpp:690, 700-701, 773) and for icons3DShader's gl_ModelViewProjectionMatrix (Icons3DVS.glsl:17). Icon eligibility is decided once per update from the main camera and main-window state: camera->GetPos() and camera->InView (UnitDrawerData.cpp:350-352), iconZoomDist from camHandler->GetCurrentController (UnitDrawerData.cpp:224-230), and iconSizeBase from globalRendering->viewSizeX/Y (UnitDrawerData.cpp:217). Skipping DrawUnitIcons in window 2 hides units that the main camera classifies as icons or radar blips, and calling it unchanged makes window 2 draw icons for units that are close to window 2's camera but far from camera 1. DrawUnitIcons is therefore a per-view LOD classification problem, not a skip-the-overlay problem.

*Evidence:*
- `rts/Rendering/WorldDrawer.cpp:366` selectedUnitsHandler.Draw inside the opaque pass; icons at 481-482
- `rts/Game/Game.cpp:1533` screen icons drawn in the screen pass
- `rts/Rendering/Units/UnitDrawer.cpp:845` screen icon quad built from globalRendering->viewSize
- `rts/Rendering/Units/UnitDrawer.cpp:1820` GL4 DrawBuildIcons uses ambient camera->InView (1847)

*Direction:* Do not put DrawUnitIcons behind a skip-overlays pass mask. Keep it in the world pass and drive it from a per-view classification (per-RenderView icon/LOD state or a view-aware UpdateUnitIconState), with the billboard and sort code reading the view's camera. Only selection volumes, screen icons, build squares, build icons, minimap icons, cursor and GUI-driven draws go behind a pass mask, and those are 'full'. For the prototype, either accept that radar and far-LOD units are invisible in window 2 and document it, or implement per-view icon flags together with the per-camera draw-flag problem (UpdateObjectDrawFlags loops fixed camera types, UnitDrawerData.cpp:430-452).

> verifier A (corrected): The citations and line numbers are real, and the overlay callers (selection volumes, build squares, build icons, minimap icons, screen icons) are not part of a 3D-only window. But the claim lumps together things with different mechanisms, and "window 2 simply must not call them" is wrong for the default 3D unit icons. By default (UnitIconsAsUI=false, UnitDrawer.cpp:70) icons are the unit's LOD and radar-blip representation, not an overlay. CUnitDrawerData::UpdateUnitIconState (UnitDrawerData.cpp

### OVL-03 - GUI receivers, minimap and hit-testing assume a single screen
**Severity:** full - **Effort:** XL - **Status:** confirmed - **Depends on:** Draw path has no split between once-per-frame work and per-target work

All 2D UI input (15 CInputReceiver subclasses, minimap, GuiHandler, Rml, aGui, LuaUI) hit-tests against one static receiver list in one coordinate space and shares one activeReceiver. This is not needed for the prototype but defines the model for GUI in B.

*Why it breaks:* CInputReceiver::GetReceivers() is a function-static deque and activeReceiver a static (InputReceiver.h:54-62). GetReceiverAt (InputReceiver.cpp:76-97) checks Rml, Lua, then the list with global coordinates. MouseHandler dispatch iterates the receivers and sets activeReceiver (MouseHandler.cpp:382-425). The minimap has 79 camera/globalRendering references plus 21 dual-screen references and uses PLAYER camera directly (MiniMap.cpp:1449). aGui::Gui::HandleEvent (Gui.cpp:121-138) and Rml contexts (RmlUi_Backend.cpp:586-599) consume every window's events with global state. The existing dualScreenMode path remaps coordinates within one window and is the only precedent.

*Evidence:*
- `rts/Game/UI/InputReceiver.h:54` static std::deque<CInputReceiver*> sInputReceivers
- `rts/Game/UI/InputReceiver.cpp:76` GetReceiverAt in global coordinates
- `rts/Game/UI/MouseHandler.cpp:382` press dispatch over all receivers, single activeReceiver
- `rts/Game/UI/MiniMap.cpp:1449` minimap draws the PLAYER camera frustum and uses global view geometry
- `rts/aGui/Gui.cpp:121` aGui consumes every event, no window filter
- `rts/Rml/Backends/RmlUi_Backend.cpp:586` Rml ProcessEvent over all contexts, no target filter
- `rts/Game/UI/InputReceiver.cpp:60` DrawReceivers draws every receiver; GetReceiverAt (76) hit-tests all of them
- `rts/Game/Game.cpp:1575` DrawInputReceivers: receivers then luaInputReceiver->Draw() (eventHandler.DrawScreen)
- `rts/Game/UI/InfoConsole.cpp:147` console layout computed from viewSizeX/viewSizeY in Update
- `rts/Game/GameControllerTextInput.cpp:31` text input geometry recomputed in ViewResize from view size
- `rts/Lua/LuaInputReceiver.cpp:71` Draw() forwards to eventHandler.DrawScreen (also drives ProfileDrawer)

*Direction:* Group receivers per target (a per-target receiver list and activeReceiver), selected by the router's target. Give Rml/aGui/LuaUI a per-target context. For the prototype B simply has no receivers, so its events can be dropped.

> verifier A (confirmed): Core mechanism verified in the current tree. InputReceiver.h:54-62 has a function-static receivers deque and a static activeReceiver. InputReceiver.cpp:76-97 hit-tests Rml, then Lua, then the global list, with no target notion. MouseHandler.cpp:382-425 and 567-571 use the single static activeReceiver. aGui Gui.cpp:121-138 iterates every element with no window filter. RmlGui::ProcessEvent (RmlUi_Backend.cpp:586-599) loops over all contexts with no target filter. MiniMap.cpp:1449 uses CAMTYPE_PLAY

### OVL-04 - Minimap FBO and RmlUi layer framebuffers are window-sized singletons updated every frame
**Severity:** full - **Effort:** L - **Status:** corrected - **Depends on:** DSN-17

The minimap renders to its own FBO (with an MSAA resolve FBO) every frame, and RmlUi allocates layer and post-process framebuffers sized to the window. Both are outside the 3D-only prototype but still run for the primary window.

*Why it breaks:* CMiniMap::Update is throttled by a static nextDrawScreen timer (about 15/25/60 Hz, MiniMap.cpp:1093-1114), not run every frame. When it does run, it binds the single member FBO and recreates the multisampled RBO from the global msaaLevel on a size change (1116-1146). It then draws into the window-agnostic minimapTex with its own glViewport (1203) and a resolve blit (1220). It leaves the minimap FBO bound and the viewport changed; CGame restores them at Game.cpp:1522-1524, after GenerateIBLTextures at 1519. RmlUi owns one RenderLayerStack. Its framebuffers are sized by viewport_width/height (RmlUi_Renderer_GL3_Recoil.cpp:790, 1897-1906) and use a fixed 2-sample MSAA (51, 1861). EndFrame binds FBO 0 and the viewport (814-815), which is whichever drawable is current. The viewport is set once in Initialize from globalRendering->winSizeX/Y (RmlUi_Backend.cpp:150-159) and afterwards on every SDL_WINDOWEVENT_SIZE_CHANGED from any window (568-576). The same unfiltered event also calls context->SetDimensions on every RmlUi context (RmlUi_SystemInterface.cpp:206-211). Nothing under rts/ outside lib/ filters on windowID, so window 2's creation or resize would also resize window 1's RmlUi viewport, context dimensions and layer FBOs.

*Evidence:*
- `rts/Game/Game.cpp:1514` minimap->Update before world draw; RmlGui::RenderFrame at 1544
- `rts/Game/UI/MiniMap.h:199` fbo + fboResolve members (199-200)
- `rts/Game/UI/MiniMap.cpp:1116` MSAA RBO and resolve texture creation (1116-1146), viewport at 1203, resolve blit 1217-1220
- `rts/Rml/Backends/RmlUi_Renderer_GL3_Recoil.cpp:790` render_layers.BeginFrame(viewport_width, viewport_height); SetViewport at 709
- `rts/Rml/Backends/RmlUi_Backend.cpp:569` viewport updated on SDL SIZE_CHANGED without a window filter (569-573)

*Direction:* For the prototype, skip minimap->Update and RmlGui::Update/RenderFrame for secondary targets. If overlays stay enabled on window 1, filter SDL window events by windowID (RmlUi_Backend.cpp:568 and RmlUi_SystemInterface.cpp:206) so window 2's resize cannot alter RmlUi state. For full support, give each target its own RmlUi render interface with its own layer stack, and its own contexts and per-target minimap. Both are GL container-object FBOs, which are not shared across GL contexts, so they must be created in the owning target's context. Make the minimap's MSAA policy per target rather than reading the global msaaLevel.

> verifier A (corrected): The core mechanism holds in the current tree. The minimap FBO, resolve FBO and texture are single CMiniMap members (MiniMap.h:199-201). CMiniMap::Update recreates the MSAA renderbuffer from globalRendering->msaaLevel when curDim changes (MiniMap.cpp:1116-1146), then sets glViewport and blits to the resolve FBO (1203, 1220). Update runs at Game.cpp:1514, ahead of the world draw. RmlUi sizes its layer and postprocess framebuffers from one viewport_width/height (RmlUi_Renderer_GL3_Recoil.cpp:790, R

### OVL-05 - Minimap is a singleton overlay tied to primary geometry, camera and input
**Severity:** full - **Effort:** L - **Status:** confirmed - **Depends on:** OVL-15

The minimap has one position/size in primary-window pixels, one camera frustum overlay, one notification list and one input-receiver identity. Per-target minimaps, or a minimap on only one window, need explicit ownership.

*Why it breaks:* Layout comes from globalRendering->viewSize (ParseGeometry, UpdateGeometry clamps). The frustum overlay is drawn from the global `camera` and shows only that camera. The minimap is registered in the global receiver deque, so hit-testing assumes primary coordinates. Update refresh-rate heuristics use viewSizeX*viewSizeY.

*Evidence:*
- `rts/Game/UI/MiniMap.cpp:240` curPos.y derived from globalRendering->viewSizeY
- `rts/Game/UI/MiniMap.cpp:504` UpdateGeometry clamps against viewSizeX/Y
- `rts/Game/UI/MiniMap.cpp:1110` refresh rate chosen from curDim vs viewSize area
- `rts/Game/UI/MiniMap.cpp:1412` DrawCameraFrustumAndMouseSelection draws only the global camera's frustum
- `rts/Game/UI/InputReceiver.cpp:76` GetReceiverAt hit-tests minimap in primary window coordinates

*Direction:* Pick an owner target for the minimap. Optionally extend the frustum overlay to draw every target's camera. Later make geometry relative to the owner target's size.

> verifier A (confirmed): The mechanism holds. MiniMap.cpp derives layout and clamps from globalRendering->viewSizeX/Y (lines 240, 339-346, 504-509, 763-801). Line 1110 compares curDim against the viewSize area to choose the refresh rate. DrawCameraFrustumAndMouseSelection (line 1412) draws only CCameraHandler::GetCamera(CAMTYPE_PLAYER), so there is a single frustum. GetReceiverAt (InputReceiver.cpp:76-97) loops over one global receiver list with window-agnostic x,y, so the minimap is hit-tested in one coordinate space. 

### OVL-06 - Mouse-driven world overlays are bound to the single mouse and primary camera
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** OVL-01

DrawMapStuff, the selection box, the centroid cursor and the tooltip trace from mouse->dir and camera->GetPos(), and hit-test GUI receivers in primary-window pixel coordinates, from inside the draw path.

*Why it breaks:* WORLD-PASS PART (verified): CWorldDrawer::Draw calls DrawMiscObjects at WorldDrawer.cpp:327. DrawMiscObjects (466-493) runs mouse->DrawSelectionBox() at :487 and guihandler->DrawMapStuff(false) at :488, on every world pass. DrawMapStuff (GuiHandler.cpp:3543) does the following:
- It hit-tests GUI receivers at mouse->lastx/lasty (:3559), via CInputReceiver::GetReceiverAt/IsAbove in primary-window pixel coordinates (InputReceiver.cpp:76-97).
- If minimapCoors is true (:3560, minimap->ProxyMode() or !hideInterface && minimapInput), it traces straight down from the minimap position (:3566-3567). Otherwise tracePos = camera->GetPos() and traceDir = mouse->dir (:3573-3574).
- That ray feeds the drag previews (:3631-3635, :3693-3697), the GuiTraceRay unit-range circles (:3738), the build previews (:3819-3832) and the default-command lookup (:3900).
mouse->dir is computed from whichever camera is global at the time: once per frame in UpdateUnsynced (Game.cpp:1438 -> MouseHandler.cpp:965-968 -> :708 camera->CalcPixelDir(lastx,lasty)) and in the input handlers (:359, :537).

Imprecision in the original claim: in window 2 this is not simply 'window 1's ray'. If the global camera is swapped to window 2's camera for the second pass, tracePos becomes camera 2's position while traceDir stays camera 1's mouse ray. That is a hybrid ray matching neither window. If the camera is not swapped, window 2 redraws window 1's overlay.

Selection box: GetSelectionBoxVertices (MouseHandler.cpp:432-475) builds the quad from camera->GetPos/GetRight/GetUp/GetForward (:459-472) combined with bp.camPos/bp.dir captured on window 1 (:359). In window 2 that gives a skewed or off-screen box whenever the user drag-selects in window 1.

Minimap marker spin: spinTime is a function-local static (GuiHandler.cpp:3967-3968) advanced by lastFrameTime on every call. DrawMiniMapMarker is reached only from DrawMapStuff(false) at :3569-3570. That needs miniMapMarker, minimap->FullProxy() (default true, MiniMap.cpp:61 and :101), minimapCoors, and the minimap not minimized, so in practice only while the mouse hovers the minimap. With N passes per frame it spins N times as fast.

MIS-BUCKETED ITEMS: 
(a) The centroid cursor is not in the world pass. DrawCentroidCursor (GuiHandler.cpp:3999-4050, camera->CalcViewPortCoordinates at :4041, viewSizeY at :4046) is called from CMouseHandler::DrawCursor (MouseHandler.cpp:1049-1055), which runs in the Draw::Screen block (Game.cpp:1545). 
(b) The tooltip is not in the world pass either. GetCurrentTooltip (MouseHandler.cpp:725-781) is called by CTooltipConsole::Draw (a GUI receiver drawn via DrawInputReceivers, Game.cpp:1541) and by Lua (LuaUnsyncedRead). It uses camera->GetPos() and dir (:759-778), so it is primary-bound, but it is a GUI-pass and Lua concern. 
(c) CursorIcons (CursorIcons.cpp:120) is not mouse-driven. It projects command-queue and Lua icons with camera->CalcViewPortCoordinates and globalRendering->pixelX/pixelY (MouseCursor.cpp:233-238). It is drawn in DrawMiscObjects at :485 and Clear()s its icon list at the end of Draw (CursorIcons.cpp:66). That makes it a global-camera and viewport-size dependency (covered by the camera/viewport problems), not an input-ownership one. Icon producers must also run once per target pass, because Draw() empties the list.

*Evidence:*
- `rts/Game/UI/GuiHandler.cpp:3559` GetReceiverAt(mouse->lastx, mouse->lasty) hit-test inside a draw function
- `rts/Game/UI/GuiHandler.cpp:3573` trace origin and direction from the active camera and mouse
- `rts/Game/UI/GuiHandler.cpp:3967` static float spinTime advanced per call
- `rts/Game/UI/GuiHandler.cpp:4041` centroid cursor uses camera->CalcViewPortCoordinates and viewSizeY
- `rts/Game/UI/MouseHandler.cpp:669` selection box from the single mouse state
- `rts/Game/UI/MouseHandler.cpp:725` GetCurrentTooltip traces from the global camera and dir
- `rts/Game/Game.cpp:1438` mouse->UpdateCursorCameraDir() once per frame against the primary camera
- `rts/Game/UI/CursorIcons.cpp:120` projection uses the active camera and global pixel size

*Direction:* Split DrawMiscObjects into world-intrinsic items (command queues, line drawer, map marks) and pointer/selection overlays (DrawSelectionBox, DrawMapStuff). Gate the overlays on an 'input-owning target' flag. The 3-line gate at WorldDrawer.cpp:487-488 is enough for the prototype and is effectively covered by the 3D-only pass-mask dependency. Make the minimap marker's spinTime a per-frame value (derived from the draw-frame clock), not a per-call static. Keep the centroid cursor and tooltip in the later GUI-per-target work: per-target pointer state (lastx/lasty/dir, per-target camera for CalcPixelDir) and a per-target pick context. Move CursorIcons under the camera/viewport-global problem.

> verifier A (corrected): The core mechanism is real and verified in the current tree: the world pass calls the mouse-bound overlays, and they use the primary mouse and the global camera. Nothing in the tree gates them (no render-target or primary flag anywhere in rts/Game). Two of the cited pieces are mis-bucketed, and the 'window 1's ray' description is imprecise. Severity 'full' stands: none of this crashes or raises GL errors, and in idle play these draw nothing. Glitches appear only while the user interacts in windo

### OVL-07 - Normalized font layout bakes global pixel size into geometry at call time
**Severity:** full - **Effort:** M - **Status:** confirmed - **Depends on:** none

FONT_NORM text multiplies the size by globalRendering->pixelX/pixelY when glPrint emits the quads. Buffered text is emitted at call time and drawn later. Console wrap width and many widgets are computed from the primary view size.

*Why it breaks:* Quads are generated in normalized coordinates sized for the primary view. Drawing them into a target with a different size or aspect gives distorted glyphs. FONT_BUFFERED batches are emitted early and drawn later, so a target switch in between uses stale pixel scale. The font's own viewMatrix/projMatrix members are dead state that is never wired.

*Evidence:*
- `rts/Rendering/Fonts/glFont.cpp:863` sizeX *= globalRendering->pixelX; sizeY *= pixelY under FONT_NORM (also 1058-1059)
- `rts/Rendering/Fonts/glFont.cpp:904` buffered path accumulates quads, drawn later by DrawBuffered
- `rts/Rendering/Fonts/glFont.cpp:191` viewMatrix/projMatrix set but never used
- `rts/Rendering/GlobalRendering.cpp:1537` pixelX = 1/viewSizeX, pixelY = 1/viewSizeY
- `rts/Game/Game.cpp:1613` clock/FPS/speed text uses FONT_NORM
- `rts/Game/UI/InfoConsole.cpp:147` Wrap width computed from viewSizeX at Update; drawn with pixelX at 118-125

*Direction:* Add a per-target pixel scale (and ortho projection) read at emission time. Wire the dead CglFont view/proj matrices to it. Forbid FONT_BUFFERED batches that span a target switch.

> verifier A (confirmed): The code matches the claim. glFont.cpp:863-865 and 1057-1059 multiply sizeX/sizeY by globalRendering->pixelX/pixelY under FONT_NORM when the text is emitted. GlobalRendering.cpp:1539-1540 sets pixelX = 1/viewSizeX and pixelY = 1/viewSizeY. CglFont's viewMatrix/projMatrix are only assigned (glFont.cpp:191-192) and otherwise only declared or given setters (glFont.h:67-68, 161-162). Nothing in rts/Rendering/Fonts reads them. InfoConsole.cpp:117/120/147 uses FONT_NORM, pixelX and a viewSizeX-based w

### OVL-08 - RmlUi is initialised against the primary window and context
**Severity:** full - **Effort:** L - **Status:** corrected - **Depends on:** OVL-01

The in-engine RmlUi backend takes its viewport from the primary window and updates it from any window's SIZE_CHANGED. Its GL3 renderer owns per-context objects. Window 2 resizes would corrupt the primary's UI viewport, and a per-target GUI would need its own renderer state.

*Why it breaks:* As claimed, plus a second leakage path. processContextEvent (RmlUi_Backend.cpp:583) forwards SIZE_CHANGED to RmlSDLRecoil::InputEventHandler, which for each context calls context->SetDimensions(data1, data2) (RmlUi_SystemInterface.cpp:206-211). It also calls context->ProcessMouseLeave() on SDL_WINDOWEVENT_LEAVE (:212-213). Both run for events from any window. For SIZE_CHANGED, InputEventHandler returns true (:176 default), so processContextEvent returns true and the loop in ProcessEvent (:593-596) visits every context. A resize or leave on window 2, or the SIZE_CHANGED that SDL most likely emits when window 2 is shown, therefore resizes the render_interface viewport and every Rml context's layout to window 2's size, and drops mouse-hover on the primary UI. A smaller pre-existing wrinkle: winX/winY and the viewport are only updated inside processContextEvent, so a primary resize while zero contexts exist is missed. Geometry compiled during RmlGui::Initialize (Game.cpp:854) lives in the primary context's VAOs/FBOs (non-shared GL objects), so drawing it with window 2's context current would draw nothing or raise GL errors. Shaders and textures are only usable if the contexts share lists.

*Evidence:*
- `rts/Rml/Backends/RmlUi_Backend.cpp:150` winX/winY from globalRendering->winSizeX/Y at init; SetViewport(157)
- `rts/Rml/Backends/RmlUi_Backend.cpp:568` SIZE_CHANGED from any window rewrites render_interface viewport and winX/winY
- `rts/Rml/Backends/RmlUi_Renderer_GL3_Recoil.cpp:724` GL state backup against current framebuffer
- `rts/lib/RmlUi/CMakeLists.txt:128` SDL sample backends (own windows/contexts) only with RMLUI_SAMPLES/TESTS
- `rts/Rml/Backends/RmlUi_Backend.cpp:316` every new Rml context gets dimensions {winX, winY}
- `rts/Rml/Backends/RmlUi_Backend.cpp:399` RenderFrame renders all contexts in one pass
- `rts/Rml/Backends/RmlUi_Renderer_GL3_Recoil.cpp:790` BeginFrame binds render_layers FBOs (context-local)
- `rts/Rml/Backends/RmlUi_Renderer_GL3_Recoil.cpp:814` EndFrame binds framebuffer 0 and draws a fullscreen quad
- `rts/Game/Game.cpp:1544` RmlGui::RenderFrame in the screen pass (Update at 1454)

*Direction:* Put the window-ID guard at the top of RmlGui::ProcessEvent (RmlUi_Backend.cpp:586), before the per-context loop, rather than only around the SetViewport call. Compare event.window.windowID against SDL_GetWindowID of the primary window, and apply it to every SDL_WINDOWEVENT. A guard inside processContextEvent's viewport branch alone would still let RmlSDLRecoil::InputEventHandler resize contexts and fire mouse-leave. For the prototype, skip RmlGui::Update and RenderFrame for secondary targets, which falls out of the pass mask. For a full product, choose between one Rml context set and render interface per target (VAOs, FBOs and the viewport are per GL context) and drawing one shared UI into each window with per-context geometry. Either way, key winX/winY, the viewport and the dimensions on a render-target id instead of the single BackendState.

> verifier A (corrected): The core mechanism holds in the current tree. There is a single global BackendState (RmlUi_Backend.cpp:123) with one render_interface and one winX/winY. RmlGui::Initialize (:150-159) seeds these from globalRendering->winSizeX/Y, and OnContextCreate (:316) gives every new context those dimensions. processContextEvent (:568-577) applies any SDL_WINDOWEVENT_SIZE_CHANGED data1/data2 to render_interface.SetViewport and winX/winY. A grep for windowID across rts/ outside lib finds no window-ID filterin

### OVL-09 - Screen-space layers tied to the projectile subsystem: HUD, minimap projectiles, AI debug
**Severity:** full - **Effort:** L - **Status:** corrected - **Depends on:** none

Screen-pass drawers that belong to the GUI layer, not the 3D world. Each needs a per-target layering decision for the full product.

*Why it breaks:* Screen-space HUD, AI-debug and minimap layers run only in CGame::Draw's screen pass (Game.cpp:1530-1548). Each reads global per-window state. HUDDrawer reads globalRendering->aspectRatio and the global camera (HUDDrawer.cpp:56-69, 85, 107). DebugDrawerAI divides by globalRendering->viewSizeX/Y (DebugDrawerAI.cpp:268-269, 329, 469-470). CMiniMap::DrawWorldStuff builds its projectile layer from the shared renderProjectiles list and the CProjectileDrawer-owned RBs (MiniMap.cpp:2003-2007, ProjectileDrawer.cpp:727-773). These layers only matter for a full product, where each target needs its own GUI layer. Two corrections follow. First, some overlays are inside CWorldDrawer::DrawMiscObjects (WorldDrawer.cpp:466-493) and will appear in window 2 unless gated: unit icons when UnitIconsAsUI=false, lineDrawer, cursorIcons, mouse->DrawSelectionBox, guihandler->DrawMapStuff and inMapDrawerView. Second, the real multi-target coupling of icons is not the screen pass but the icon state. isIcon and iconZoomDist are computed once per frame from camHandler->GetCurrentController (UnitDrawerData.cpp:217-238, 401), so with UnitIconsAsUI=true a secondary window shows neither icons nor LOD'd model units, and with it false the icon choice is wrong for window 2's camera. A related minimap coupling: the minimap reuses NanoParticles transientVertices, which GatherVisibleEnemies frustum-culls against the current global camera (NanoParticleRenderer.cpp:455, 613-636). That cache is keyed only on drawFrame and camType (NanoParticleRenderer.cpp:668-675), so window 2 would reuse window 1's cull set.

*Evidence:*
- `rts/Game/Game.cpp:1538` HUD and AI debug in the screen pass
- `rts/Rendering/HUDDrawer.cpp:85` screen-space HUD uses global aspect ratio
- `rts/Game/UI/MiniMap.cpp:2006` minimap projectile layer uses shared RBs

*Direction:* Keep this as a full-product layering item (world, world overlays, screen GUI) and drop the "shared RBs" framing, which is not a problem for sequential draws. For the prototype, skipping the screen pass for secondary windows is enough for HUD, AI debug and minimap. It must also gate the overlays inside CWorldDrawer::DrawMiscObjects (icons, selection box, cursor icons, map markers, DrawMapStuff, lineDrawer). Per-camera icon state (isIcon, iconZoomDist) and the nano cull cache keyed on drawFrame and camType belong to separate world-pass per-camera-state problems.

> verifier A (corrected): The core is true: HUD, AI debug and minimap are screen-pass or GUI layers, none of which the 2-window 3D-only prototype needs, so full is the right severity. Every cited line checks out. Game.cpp:1538-1539 has the hudDrawer and debugDrawerAI calls. HUDDrawer.cpp:85 and :107 use globalRendering->aspectRatio, and DrawModel at :56-69 uses the global camera matrices and basis vectors. DebugDrawerAI.cpp:268, 269, 329, 469 and 470 divide by viewSizeX/Y. MiniMap.cpp:2003-2007 calls NanoParticles::DrawO

### OVL-10 - Screen-space Lua callins and screen matrices assume one GUI target
**Severity:** full - **Effort:** L - **Status:** confirmed - **Depends on:** LUA-11; OVL-01

DrawScreen, DrawScreenEffects, DrawScreenPost and DrawInMiniMap* set up screen matrices from globalRendering and the single minimap. A per-view GUI needs per-target screen matrices, view sizes and minimap/input receivers.

*Why it breaks:* EnableDrawScreenCommon loads globalRendering->screenProjMatrix/screenViewMatrix and pushes viewSizeX/Y to the callin. Screen callins fire once from CGame::Draw after the world pass. DrawInMiniMap uses the single global minimap (ResetMiniMapMatrices). gl.DrawMiniMap scales by globalRendering->viewSize. LuaUI receives DrawScreen through the engine input receivers (one list). Out of prototype scope (the prototype fires none of these), but a complete product needs per-view GUI content and routing.

*Evidence:*
- `rts/Lua/LuaOpenGL.cpp:985` SetupScreenMatrices uses globalRendering->screenProj/screenViewMatrix (985-994); EnableDrawScreenCommon 832-841
- `rts/Lua/LuaOpenGL.cpp:1092` ResetMiniMapMatrices asserts and uses the global minimap (1092-1100)
- `rts/Lua/LuaOpenGL.cpp:1316` gl.DrawMiniMap scales by globalRendering->viewSize (1316-1321)
- `rts/Game/Game.cpp:1530` screen pass (ScreenEffects, HUD, input receivers, RmlGui, cursor, ScreenPost) once per frame after the world
- `rts/Lua/LuaHandle.cpp:2980` DrawScreenCommon pushes the primary view size
- `rts/Lua/LuaHandle.cpp:2986` DrawScreenCommon pushes viewSizeX/viewSizeY (DrawScreen 3003, Effects 3019, Post 3039)
- `rts/Lua/LuaOpenGL.cpp:990` SetupScreenMatrices uses globalRendering->screenProjMatrix/screenViewMatrix
- `rts/Lua/LuaOpenGL.cpp:1234` gl.GetViewSizes returns globalRendering->viewSize
- `rts/Lua/LuaUnsyncedRead.cpp:894` Spring.GetViewGeometry returns viewSize/viewPos (window geometry at 939)
- `rts/Rendering/GlobalRendering.cpp:1615` UpdateScreenMatrices from primary window and desktop geometry
- `rts/Game/Game.cpp:1536` DrawScreenEffects and DrawScreenPost (1547) bracket the UI draw

*Direction:* Prototype: do not fire these callins for any view. Full product: make screen matrices, GUI geometry and the minimap per-view, fire DrawScreen* once per GUI-enabled view with that view's size and a view identity, and decide which widgets target which view.

> verifier A (confirmed): Checked the current tree. EnableDrawScreenCommon (LuaOpenGL.cpp:832) calls SetupScreenMatrices, which loads globalRendering->screenProjMatrix and screenViewMatrix (985-994). DrawScreenCommon in LuaHandle.cpp pushes globalRendering->viewSizeX/Y (2986-2987). Game.cpp 1530-1548 runs the screen pass once per frame after worldDrawer.Draw, with DrawScreenEffects at 1536 and DrawScreenPost at 1547. ResetMiniMapMatrices is registered at 876, 928 and defined at 1092. All are per-process rather than per-t

### OVL-11 - Screen-space unit icons depend on the screen pass and view size
**Severity:** full - **Effort:** M - **Status:** confirmed - **Depends on:** Unit icon state is computed once per frame from the primary camera; OVL-01

With UnitIconsAsUI=1, icons are drawn in the screen pass using globalRendering->viewSize, and the world pass skips icons. Models are hidden via the screen-based isIcon test. A 3D-only secondary target would show neither.

*Why it breaks:* WorldDrawer skips DrawUnitIcons when UseScreenIcons(). The icons are then drawn only by DrawUnitIconsScreen in the screen pass, normalised by globalRendering->viewSizeX/Y. UpdateUnitIconStateScreen computes isIcon from camera->CalcViewPortCoordinates and iconSizeBase (viewSize), and hides models when the icon is large. If the screen pass is not run for window 2, zoomed-out units vanish there. The config default is false, so the default prototype is unaffected.

*Evidence:*
- `rts/Rendering/WorldDrawer.cpp:481` world icons skipped when UseScreenIcons()
- `rts/Game/Game.cpp:1533` DrawUnitIconsScreen only in the screen pass
- `rts/Rendering/Units/UnitDrawer.cpp:845` icon quad normalised by globalRendering->viewSizeX/Y (845-848)
- `rts/Rendering/Units/UnitDrawerData.cpp:217` iconSizeBase from max(viewSizeX, viewSizeY)
- `rts/Rendering/Units/UnitDrawerData.cpp:388` CalcViewPortCoordinates and iconRadius for the screen icon test
- `rts/Game/Camera.cpp:581` CalcViewPortCoordinates scales by the camera's own viewport[] (primary-sized)
- `rts/Rendering/Units/UnitDrawer.cpp:70` UnitIconsAsUI default false

*Direction:* Per target: if a target has no overlay pass, force the world-space icon path and do not hide models by screen-space icon decisions. Longer term, run the screen icon pass per target with that target's viewport and size.

> verifier A (confirmed): Verified in the current tree. With useScreenIcons set, CUnitDrawerData::Update calls UpdateUnitIconStateScreen (UnitDrawerData.cpp:235-236). That function sets isIcon from camera->CalcViewPortCoordinates and iconSizeBase, where iconSizeBase comes from globalRendering->viewSize (UnitDrawerData.cpp:217, 382-401). The world pass skips icons when UseScreenIcons() is true (WorldDrawer.cpp:481). DrawUnitIconsScreen runs only in the Draw::Screen block (Game.cpp:1533-1534). A 3D-only secondary target wo

### OVL-12 - UBO update depends on GUI singletons (minimap, mouse)
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** none

UpdateMatricesImpl dereferences the global minimap and UpdateParamsImpl the global mouse unconditionally, so a render path with GUI objects absent or per-window would crash or read the wrong object.

*Why it breaks:* UpdateMatricesImpl and UpdateParamsImpl fill one global UBO per frame. The mm* matrices come from the global minimap (UniformConstants.cpp:200-210). The mouse fields and the mouseWorldPos ray trace (250-293) come from the global mouse and the CAMTYPE_PLAYER camera. The data is per-process and tied to window 1, so it is wrong for any other render target. The global objects always exist while the game runs, so a null deref is not realistic for the 2-window prototype.

*Evidence:*
- `rts/Rendering/UniformConstants.cpp:200` minimap->GetViewMat/GetProjMat dereferenced without null check (200-210)
- `rts/Rendering/UniformConstants.cpp:250` mouse-> fields dereferenced (250-262, 266-267)

*Direction:* Do not fix this on its own. Fold it into the per-view UBO design. Split the UBO into frame-global data (time, wind, sun, map) and per-view data (camera matrices, viewGeometry, fog scaled by that view's far plane). Rewrite the per-view block before each window's draw. Guard the minimap and mouse reads with null checks, or move them into a GUI-only block, as a small tidy-up when the GUI is added per window. The mouse-pick GuiTraceRay (266-279) should be skipped, or run only for the focused window.

> verifier B (corrected): The crash mechanism does not hold for the 2-window 3D-only prototype. Both globals are non-null for the whole game session. `mouse` is allocated at app start (CMouseHandler::InitStatic, rts/System/SpringApp.cpp:291, MouseHandler.cpp:133) and freed only at shutdown (SpringApp.cpp:1049). `minimap` is allocated in the CGame constructor (rts/Game/Game.cpp:837) and freed in teardown (Game.cpp:1029). The only UBO update call site is CGame::UpdateUnsynced (Game.cpp:1441), which runs after both exist. "

> verifier A (corrected): The cited derefs are real (UniformConstants.cpp:200-210 minimap->GetViewMat/GetProjMat, 250-267 mouse->lastx/lasty/buttons/offscreen/mmbScroll/locked, no null checks). The stated consequence does not hold for the prototype. mouse is created in CMouseHandler::InitStatic (MouseHandler.cpp:133, called from SpringApp.cpp:291) before any game code and lives until KillStatic. minimap is created in CGame::LoadGame (Game.cpp:837) and freed at Game.cpp:1029, so it always exists during UpdateUnsynced. Gam

> verifier A (corrected): The code matches the citations. UpdateMatricesImpl (UniformConstants.cpp:200-210) dereferences minimap without a null check. UpdateParamsImpl (250-267) dereferences mouse without a null check. The "crash" part does not hold for the 2-window prototype. Both windows come from one game instance, so the global minimap and mouse objects still exist and are valid, and nothing crashes. In the prototype the mm* matrices are only consumed by minimap and GUI draws, which are out of scope. The mouse fields

> verifier B (corrected): The code is as claimed. UpdateMatricesImpl dereferences `minimap` at UniformConstants.cpp:200-210, and UpdateParamsImpl dereferences `mouse` at lines 250-267. Neither is a prototype blocker or risk. The prototype runs inside the one CGame, and the minimap and mouse singletons are created with the game, so they exist and the dereferences do not crash. The claim that "a 3D-only secondary view has no minimap/mouse of its own" does not matter, because UpdateConstants runs once per frame, not once pe

### OVL-13 - Video capture reads the primary default framebuffer and rewrites global frame timing
**Severity:** full - **Effort:** M - **Status:** unverified - **Depends on:** Draw path has no split between once-per-frame work and per-target work

AVI capture (Windows-only) sizes the video from the primary viewSize, reads framebuffer 0 at the end of CGame::Draw, and when recording switches global time stepping to a fixed offset and forces lastFrameTime.

*Why it breaks:* Capture size is fixed at StartCapturing from globalRendering->viewSizeX/Y. readOpenglPixelDataThreaded does glReadPixels(0,0,w,h) of whichever read framebuffer is bound. RenderFrame is placed after the screen pass, so it must stay in the primary context before its swap. AllowRecord() changes timeOffset computation for the entire game (shared by all targets).

*Evidence:*
- `rts/Game/AviVideoCapturing.cpp:62` video size from globalRendering->viewSizeX/Y
- `rts/Rendering/AVIGenerator.cpp:402` glReadPixels from the current framebuffer
- `rts/Game/Game.cpp:1553` RenderFrame after DrawScreenPost; also overrides globalRendering->lastFrameTime
- `rts/Game/Game.cpp:1280` AllowRecord() disables real-time timeOffset (also 1356)
- `rts/Net/GameServer.cpp:2654` server pacing depends on videoCapturing->AllowRecord()

*Direction:* Capture only the primary target initially, with an explicit target and size. Keep the readback in the capture phase of the draw loop. Decide whether recording while multiple windows are open is supported at all.

### OVL-14 - Icon drawing writes unit->iconRadius that click picking reads
**Severity:** polish - **Effort:** S - **Status:** confirmed - **Depends on:** none

The world-icon draw function returns a scale that is stored back into unit->iconRadius, which GuiTraceRay uses as the sphere to hit-test iconified units.

*Why it breaks:* Draw output feeds input state. Drawing icons from a second camera overwrites iconRadius with a value computed for the second camera's distance, so clicks in window 1 use the wrong radius until it is redrawn.

*Evidence:*
- `rts/Rendering/Units/UnitDrawer.cpp:770` unit->iconRadius = DrawUnitIcon(...)
- `rts/Rendering/Units/UnitDrawer.cpp:788` same write in the sorted path
- `rts/Game/TraceRay.cpp:461` cv.InitSphere(u->iconRadius) for picking
- `rts/Rendering/Units/UnitDrawerData.cpp:391` screen-icon variant writes iconRadius from Update

*Direction:* Make DrawUnitIcon pure by returning void or ignoring its result. Compute iconRadius only in the Update path (UnitDrawerData.cpp:391), using the input-owning camera. Then clicking is independent of how many windows draw icons.

> verifier A (confirmed): The cited code does what is claimed. UnitDrawer.cpp:770 and :788 store the return value of DrawUnitIcon into unit->iconRadius. DrawUnitIcon (677-720) returns `scale`, which is computed from camera->GetPos() distance at lines 690-692. TraceRay.cpp:461 uses u->iconRadius for cv.InitSphere to hit-test iconified units. UnitDrawerData.cpp:391 also writes iconRadius, from camera->right and CalcViewPortCoordinates, so that write depends on the camera too. A second window drawing icons with its own came

### OVL-15 - minimap->Update runs in the draw path on the primary context and leaves its FBO bound
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** Draw path has no split between once-per-frame work and per-target work

CMiniMap::Update renders the whole minimap (icons, projectiles, Lua DrawInMiniMap callins) into its own FBO inside CGame::Draw and leaves that FBO bound with a changed viewport. Game.cpp then repairs the state with FBO::Unbind and camera->LoadViewport.

*Why it breaks:* The minimap FBO, its resolve FBO and minimapTex are created in the primary context. If a per-target loop runs Update with another context current, the FBO ids are invalid there, giving GL_INVALID_OPERATION or incomplete-framebuffer errors. If it runs in the primary context after the target was bound, it clobbers the framebuffer and viewport. It also fires Lua callins (DrawInMiniMapBackground, DrawInMiniMap) and MiniMapGeometryChanged. It is wasted work when no target shows a minimap, as in the prototype.

*Evidence:*
- `rts/Game/Game.cpp:1514` minimap->Update() before GenerateIBLTextures
- `rts/Game/Game.cpp:1521` comment 'restore back to the default FBO / Viewport' is the only repair
- `rts/Game/UI/MiniMap.cpp:1116` fbo.Bind() twice, then UpdateTextureCache; never unbinds
- `rts/Game/UI/MiniMap.cpp:1131` comment 'gets done in CGame' for fbo.Unbind
- `rts/Game/UI/MiniMap.cpp:1203` glViewport(0,0,minimapTexSize) inside UpdateTextureCache
- `rts/Game/UI/MiniMap.cpp:1373` Lua DrawInMiniMapBackground/DrawInMiniMap run from DrawForReal during Update (1373, 1386)
- `rts/Game/UI/MiniMap.cpp:1125` eventHandler.MiniMapGeometryChanged fired from Update

*Direction:* Leave the minimap in the once-per-frame primary-context phase, before the per-target loop. Make the per-target code bind its own FBO and viewport explicitly, so it never relies on the Game.cpp:1522-1524 repair. Optionally skip minimap->Update() when no target presents a minimap. Do not call Update per target.

> verifier A (confirmed): Verified in current tree. Game.cpp:1514 calls minimap->Update() unconditionally. MiniMap.cpp:1116 and 1120 call fbo.Bind() twice, then UpdateTextureCache, which sets glViewport to minimapTexSize at 1203. The unbind is commented out at 1131-1132 ("gets done in CGame"). Game.cpp:1521-1524 then repairs with FBO::Unbind and camera->LoadViewport. DrawInMiniMapBackground and DrawInMiniMap fire at 1373 and 1386 during DrawForReal. MiniMapGeometryChanged fires at 1126. Update has early-outs (renderToTex

> verifier B (corrected): The mechanism is real. MiniMap.cpp:1116 and :1120 bind the minimap FBO. The viewport is changed inside UpdateTextureCache. Update never unbinds, with the comment "gets done in CGame" at :1131. Game.cpp:1514 calls Update, then 1522-1524 repair the state with FBO::Unbind and camera->LoadViewport. But the prototype does not fail without a fix. minimap->Update() already runs once per frame, before the world draw, in the primary context, and the existing repair restores the default FBO and viewport b

## Config, compat, performance, build & test

### CFG-01 - Camera state persistence, view save/load and config are single-instance
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** none

Controller mode is a global config (CamMode/CamModeName), saved views are an in-memory map on the single handler, and per-camera config (edge scroll, move multipliers, transitions) is read once per camera or handler.

*Why it breaks:* The state is single-instance, but config is not read once; it is observer-driven. CCameraHandler::Init (CameraHandler.cpp:177-187) registers NotifyOnChange for CamModeName, WindowedEdgeMove, FullscreenEdgeMove, CamTimeFactor, CamTimeExponent, CamTransitionMode and CamSpringHalflife. It then calls ConfigNotify("","") and SetCameraMode(CamModeName). With an empty name, SetCameraMode falls back to the CamMode int (:588-591, CONFIG at :45-58). Later changes to CamModeName call SetCameraMode on this one handler (:242-243), so a CamModeName change would flip every handler instance, not just one view.

The handler is a singleton: `camHandler` (:88, placement-new into camHandlerMem at :97 and :112). It holds currCamCtrlNum, controllerStack, camTransState, viewDataMap (CameraHandler.h:148-160) and the camControllers array (:157). Controller instances are static byte storage (CameraHandler.cpp:96).

SaveView and LoadView (:660-701) key the unordered_map `viewDataMap` by name, with a special "__old_view" slot. The map is in-memory only. The only readers and writers are the viewsave and viewload actions in PushAction (:780-788); there is no config or file persistence.

CCamera config is global per camera slot. `static CCamera cameras[CAMTYPE_COUNT]` (:94) is initialised in InitStatic (:103-108), and each slot registers its own observer via InitConfigNotify (Camera.cpp:67-71). ConfigUpdate (:79-87) copies CameraMoveFastMult, CameraMoveSlowMult, CamFrameTimeCorrection, EdgeMoveDynamic and EdgeMoveWidth into moveFastMult, moveSlowMult, useInterpolate, edgeMoveDynamic and edgeMoveWidth (Camera.h:279-289). A second set of cameras would add duplicate observers that all track the same keys, so per-view overrides would need explicit fields.

Per-controller settings are also global config. SpringController.cpp:55-81, FreeController.cpp:53-67, FPSController.cpp:29-43, RotOverheadController.cpp:27-31, DollyController.cpp:32-45 and OverviewController.cpp:53,121 each read and observe their own FOV, scrollSpeed and enabled keys. A per-view FOV or speed therefore needs a per-view override layer, not just a second handler.

Edge-move and fullscreen decisions in UpdateController read the single globalRendering->fullScreen (CameraHandler.cpp:264-265). That overlaps the window-state problems and should be cross-referenced rather than duplicated here.

*Evidence:*
- `rts/Game/CameraHandler.cpp:187` SetCameraMode(configHandler CamModeName) once at Init; CamMode config at :47
- `rts/Game/CameraHandler.cpp:660` SaveView/LoadView use viewDataMap (CameraHandler.h:160), in-memory only
- `rts/Game/Camera.cpp:79` ConfigUpdate reads move/edge/interp config into each camera (Camera.h:287-289)

*Direction:* Keep severity "full": a clone-the-primary pose and mode is enough for the prototype. Decide per-view config policy up front: either all views share the existing global keys, which works today because every observer sees the same values, or add per-view overrides for mode, FOV and speed. Move the handler state into a per-view CameraView object, with controllers and viewDataMap per view and the CamModeName observer scoped to the primary view or a view id. Register the shared camera and controller config observers once, not per camera and controller instance. Route viewsave and viewload to the focused view. Only add persistence, for example Cam2* keys or a layout config, if per-view state must survive sessions.

> verifier A (corrected): The core claim holds: view and mode state lives in one in-memory handler and is not persisted, so a second view has no home for its own pose, mode or settings. It does not block the prototype. One part is wrong: per-camera and per-handler config is not "read once". It is read at init and re-read through ConfigNotify observers on every change, so a second camera or handler would already inherit the same values. The real coupling is that the handler is a singleton and that the CamModeName observer

### CFG-02 - No per-target verification and observability tooling
**Severity:** full - **Effort:** M - **Status:** confirmed - **Depends on:** none

Existing diagnostics (GL debug output, /debugglerrors, Tracy, screenshots, RenderDoc detection) are all frame- or primary-window-scoped, so multi-target defects would be attributed to the wrong target or hidden.

*Why it breaks:* glClearErrors runs once per SwapBuffers, so errors cannot be tied to a target. The GL debug callback is registered per context. FrameMark fires per swap. SCOPED_TIMER zone names are static strings. TakeScreenshot reads winSizeX/Y of the primary from the current read framebuffer. GL_STATE_CHECKER/DEBUG_GLSTATE exists but has no call sites. tools/benchmark/benchmark.sh passes --benchmark flags that SpringApp no longer defines.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:680` error clearing once per swap
- `rts/Rendering/GlobalRendering.cpp:1985` ToggleGLDebugOutput registers the callback on the current context
- `rts/Game/UnsyncedGameCommands.cpp:1663` /DebugGLErrors toggles verbose per-swap error logging
- `rts/Rendering/Screenshot.cpp:47` screenshot size from primary winSize
- `rts/Rendering/GL/glStateDebug.h:15` GL_STATE_CHECKER, enabled with DEBUG_GLSTATE
- `rts/CMakeLists.txt:32` DEBUG_GLSTATE option
- `rts/System/TimeProfiler.h:23` SCOPED_TIMER creates a Tracy zone with a static name
- `tools/benchmark/benchmark.sh:7` stale --benchmark flags

*Direction:* Use oracles: flag off equals baseline screenshot; same camera and size gives pixel-identical window 1 and window 2. Run with DebugGL=1, DebugGLReportGroups=1 and /debugglerrors expecting zero [OPENGL_DEBUG]/[GL] lines. Wrap each target draw in GL_STATE_CHECKER on a DEBUG_GLSTATE build. Tracy build with per-target zone names and a named secondary frame mark. Give TakeScreenshot a target parameter. Note that RenderDoc frame boundaries are per swap.

> verifier A (confirmed): Spot-checked claims hold. glClearErrors is called in the swap path (GlobalRendering.cpp:681). Screenshot.cpp:47 uses globalRendering->winSizeX, then glReadPixels at line 58. GL_STATE_CHECKER and DEBUG_GLSTATE exist (glStateDebug.h:15, rts/CMakeLists.txt:32) with no call sites outside the header and cpp. SCOPED_TIMER uses a static name for the Tracy zone (TimeProfiler.h:23). benchmark.sh passes --benchmark flags (lines 7-9) and no "benchmark" match turned up in rts/Game. The sources I grepped are

### CFG-03 - Per-view draw policy (icon distance, LOD scale, draw distance, draw masks) is global
**Severity:** full - **Effort:** M - **Status:** confirmed - **Depends on:** RenderView definition: per-target parameters consumed by model drawers

Icon distance and fade, LOD scales, feature draw/fade distance, wireframe, forward/deferred toggles, per-object noDraw and engineDrawMask are all process-wide or per-object.

*Why it breaks:* UnitIconDist, iconFadeStart/Vanish and iconScale are CUnitDrawerData members (UnitDrawerData.h:197-212). LODScale* are static (LuaObjectDrawer.h:115-118). FeatureDrawDistance/FadeDistance are in CFeatureDrawerData (FeatureDrawerData.cpp:60-61). wireFrameMode, drawForward and drawDeferred are static (ModelDrawer.h:50, 174-175). engineDrawMask and noDraw are per-object (SolidObject.h:361; UnitDrawerData.cpp:439). A secondary view can neither have its own detail policy nor hide a unit that only the primary view shows.

*Evidence:*
- `rts/Rendering/Units/UnitDrawerData.h:197` icon distances and fades as drawer-data members
- `rts/Rendering/LuaObjectDrawer.h:115` static LODScale arrays
- `rts/Rendering/Common/ModelDrawer.h:174` static drawForward/drawDeferred toggles; wireframe at 50
- `rts/Sim/Objects/SolidObject.h:361` per-object engineDrawMask shared by all views

*Direction:* Put a ViewPolicy struct in RenderView (icon distance, LOD scales, draw distances, pass mask, wireframe) initialised from the global config. Decide whether per-object masks (engineDrawMask, noDraw) are per-view in the Lua API.

> verifier A (confirmed): Every cited location checks out in the current tree. UnitDrawerData.h:198-212 holds unitIconDist, iconLength, iconScale, iconFadeStart and iconFadeVanish as members of the single drawer-data instance. They are written from config in UnitDrawerData.cpp:152-156 and through the static CUnitDrawer::SetUnitIconDist (UnitDrawer.h:68). LuaObjectDrawer.h:115-118 declares static LODScale, LODScaleShadow, LODScaleReflection and LODScaleRefraction arrays, filled from config in LuaObjectDrawer.cpp:299-304 a

### CFG-04 - Per-view feature toggles and renderer selection (sky, water, shadows) are global
**Severity:** full - **Effort:** M - **Status:** confirmed - **Depends on:** none

Water renderer, sky implementation, shadow config and draw toggles are global singletons, so views cannot differ.

*Why it breaks:* IWater::water (IWater.h:55) and ISky::sky (ISky.h:71) are static unique_ptrs. globalRendering->drawSky/drawWater/drawGround/drawFog gate passes (ModernSky.cpp:46; WorldDrawer.cpp:217,338,398,500). CSkyBox::Init writes globalRendering->drawFog from the map's fogStart (SkyBox.cpp:202). Shadow bits are global (ShadowHandler.h:116). A secondary window may want cheap water, no shadows or no sky.

*Evidence:*
- `rts/Rendering/Env/IWater.h:55` single static water renderer
- `rts/Rendering/Env/SkyBox.cpp:202` sky init writes global drawFog
- `rts/Rendering/WorldDrawer.cpp:398` drawWater/drawGround gates are global

*Direction:* Add per-view settings {drawSky, drawWater, shadow mode, water quality}. Support a per-view water renderer variant or capability flags (e.g. skip RTT for secondary views).

> verifier A (confirmed): I opened every cited line and the code does what the claim says. IWater.h:55 is `static std::unique_ptr<IWater> water`, reached through the static `GetWater()` at line 47. ISky.h:71 is `static std::unique_ptr<ISky> sky`, defined at ISky.cpp:45. The draw toggles are plain bools on the single globalRendering object (GlobalRendering.h:244-254). They gate passes at ModernSky.cpp:46, SkyBox.cpp:233, WorldDrawer.cpp:217, 338, 398 and 500, and at SMFGroundDrawer.cpp:395 and 481. Lua and chat commands f

### CFG-05 - Per-window config keys, fullscreen defaults and geometry persistence are singletons
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** Secondary window creation path and pixel-format parity; No opt-in request surface for extra render targets

Window attributes come from a fixed set of config keys read by name, and Fullscreen defaults to true. A second window has no config, its geometry would overwrite the primary's saved geometry, and config changes only ever re-apply to the primary.

*Why it breaks:* Keep the singleton-key, fullscreen-default and primary-only re-apply claims, and replace the "overwrites primary geometry" claim with the following. (1) The Read/Save pair is hard-wired to the primary sdlWindow, so there is currently no path that persists the secondary's geometry at all. (2) SpringApp::MainEventHandler never filters SDL_WINDOWEVENT by windowID (no windowID or SDL_GetWindowFromID use anywhere in rts/ outside lib/). Move and resize events from a secondary window therefore run SaveWindowPosAndSize() (SpringApp.cpp:1111, 1143) against the primary, then UpdateGLGeometry, InitGLState, UpdateInterfaceGeometry and ResizeEvent (:1143-1152). SDL_WINDOWEVENT_CLOSE on any window calls RequestQuit() (:1249-1251), which quits the whole game. HIDDEN/MINIMIZED and SHOWN toggle the global globalRendering->active (:1160-1198). This is an event-routing problem that is related to this one but distinct. (3) A naive second CreateSDLWindow() call (:398-460) would read Fullscreen (default true), WindowPosX/Y (default 0,32) and GetCfgWinRes() (:1374-1387). The fallback in GetCfgWinRes, when XResolution(Windowed) is 0, is GetMaxWinRes(), which uses GetCurrentDisplayIndex(), and that returns the primary's display index (:1368-1372, 1389-1392). The result is a desktop-sized window at the primary's position (or an exclusive-fullscreen request on the same display), exactly covering or fighting the primary. A per-target geometry source (explicit size, position and display, defaulting to windowed) is needed, plus a decision on whether to persist it.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:73` Fullscreen defaults to true
- `rts/Rendering/GlobalRendering.cpp:1374` GetCfgWinRes keyed by global fullScreen
- `rts/Rendering/GlobalRendering.cpp:1600` SaveWindowPosAndSize overwrites primary keys on every event
- `rts/Rendering/GlobalRendering.cpp:1259` ConfigNotify -> winChgFrame -> SetWindowAttributes(primary only)
- `rts/Lua/LuaUnsyncedCtrl.cpp:5425` Lua writes the same global keys through SetWindowPosHelper
- `rts/Rendering/GlobalRendering.cpp:243` ctor reads WindowPosX/Y; ConfigNotify keys registered at 358-371
- `rts/Rendering/GlobalRendering.cpp:1583` SaveWindowPosAndSize fixed keys, notify=false
- `rts/Rendering/GlobalRendering.cpp:1342` SetWindowPosHelper writes config for the primary
- `rts/Rendering/GlobalRendering.cpp:1546` ReadWindowPosAndSize reads sdlWindow only
- `rts/Rendering/GlobalRendering.cpp:1281` single winChgFrame deferral
- `rts/System/Config/ConfigHandler.cpp:308` persisted Set locks and rewrites the config file
- `rts/System/SpringApp.cpp:1111` SaveWindowPosAndSize on every MOVED event

*Direction:* Add a per-target window-spec struct (display index, position, size, fullscreen, borderless) and pass it into CreateSDLWindow and SetWindowAttributes instead of reading global keys inside them. For the prototype, create the secondary windowed from an explicit spec or new Window2* keys, with no persistence and no ConfigNotify wiring. Separately (event-routing problem), filter SDL_WINDOWEVENT by event.window.windowID in SpringApp::MainEventHandler so secondary-window move, resize, close and focus events do not drive the primary's Save/UpdateGLGeometry/RequestQuit paths. Add per-target Save/Read routines and namespaced keys only for the full product.

> verifier A (corrected): The core mechanism holds in the current tree and every cited line checks out. The window config keys are fixed process-wide singletons read by name (GlobalRendering.cpp:73-84, 243-244, 341-342, 412-416, 1221-1224, 1374-1376). Fullscreen defaults to true (:73). ConfigNotify only sets the single winChgFrame (:1281), and UpdateWindow then calls SetWindowAttributes(sdlWindow) for the primary only (:1298-1304). SetWindowPosHelper, which backs Spring.SetWindowGeometry, writes the same global keys (:13

### CFG-06 - UI visibility toggles and view-owner state are global
**Severity:** full - **Effort:** S - **Status:** confirmed - **Depends on:** OVL-01

hideInterface, drawMapMarks, iconHideWithUI, drawDebug and the first-person HUD/controllee are global. They affect world-pass content and cannot differ per target.

*Why it breaks:* game->hideInterface is a single bool consulted in WorldDrawer (map marks), the unit-icon code and the input/interface draw. The FPS-mode HUD draws for the player's controllee, and unit drawing hides that controllee in every pass, so a second window would also lack the controlled unit. Any target-specific 'hide UI' needs per-target flags.

*Evidence:*
- `rts/Game/Game.h:179` bool hideInterface (single global)
- `rts/Rendering/WorldDrawer.cpp:490` drawMapMarks && !game->hideInterface gates inMapDrawerView
- `rts/Rendering/Units/UnitDrawerData.cpp:361` hideInterface && iconHideWithUI hides icons (also UnitDrawer.cpp:726, 868)
- `rts/Game/Game.cpp:1538` hudDrawer->Draw(controllee) uses the player's fpsController
- `rts/Rendering/HudDrawer.cpp:56` HUD draws with camera->GetProjectionMatrix() and aspectRatio (85, 107)
- `rts/Rendering/Units/UnitDrawer.cpp:190` the controllee is excluded from all non-reflection passes

*Direction:* Move these into per-target settings: a target-local hideInterface and overlay flags, defaulting secondary targets to 'UI off'. Decide whether the first-person controllee is hidden in all targets or only the owner.

> verifier A (confirmed): Verified in the current tree. Game.h:179 declares a single `hideInterface`. It is read at WorldDrawer.cpp:490 (map marks together with globalRendering->drawMapMarks), UnitDrawerData.cpp:361 and UnitDrawer.cpp:726 and 868 (icons). It is also read in Game.cpp (1580, 1607) and in GuiHandler/MouseHandler. Game.cpp:1538 draws the HUD for `gu->GetMyPlayer()->fpsController.GetControllee()` in the screen pass. UnitDrawer.cpp:190 skips the controllee in every pass except reflection. All of it is per-proc

### CFG-07 - Window configuration, creg state and headless stubs are single-window
**Severity:** full - **Effort:** S - **Status:** confirmed - **Depends on:** none

Window position, size and attributes come from fixed config keys. CGlobalRendering is a creg struct with timing members. The headless SDL stubs return one static window and lack window-id and current-window queries.

*Why it breaks:* SetWindowAttributes and SaveWindowPosAndSize read and write one set of keys (WindowPosX, XResolutionWindowed, Fullscreen, ...), so window 2 has nowhere to persist. ConfigNotify keys schedule a window rebuild via winChgFrame, which only touches the primary. New per-target state added to CGlobalRendering needs CR_IGNORED/CR_MEMBER treatment. In headless builds SDL_CreateWindow returns the same static pointer for every call and SDL_GL_GetCurrentWindow/SDL_GetWindowID are not stubbed, so shared code using them will not link or will conflate windows.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:1221` SetWindowAttributes reads fixed config keys (1221-1224)
- `rts/Rendering/GlobalRendering.cpp:1583` SaveWindowPosAndSize writes fixed keys (1600-1603)
- `rts/Rendering/GlobalRendering.cpp:358` ConfigNotify key list
- `rts/Rendering/GlobalRendering.cpp:127` creg members lastSwapBuffersEnd/drawFrame (:129); sdlWindow/glContext CR_IGNORED (:216-217)
- `rts/lib/headlessStubs/sdlstub.c:55` SDL_CreateWindow returns one static pointer

*Direction:* Add per-target config (e.g. SecondaryWindow* keys) or CLI/Lua-driven creation with no persistence for the prototype. Mark new CGlobalRendering state as creg-ignored and add the missing SDL stubs.

> verifier A (confirmed): Verified in the current tree. SetWindowAttributes (GlobalRendering.cpp:1223) and SaveWindowPosAndSize (:1600-1602) use the fixed WindowPosX and XResolutionWindowed keys. winChgFrame is set at :1281 and consumed at :1298-1304, where UpdateWindow only calls SetWindowAttributes(sdlWindow) on the primary. The headless SDL_CreateWindow (sdlstub.c:55-58) returns the address of one static int for every call. No SDL_GL_GetCurrentWindow or SDL_GetWindowID stub exists anywhere under headlessStubs. This do

### CFG-08 - Window pixel format is a process-wide assumption that offscreen copies must match
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** FBOs are per-GL-context objects but the engine has one context

MSAA samples and depth format are global values (msaaLevel, supportDepthBufferBitDepth) used to size every offscreen copy, while the window's real format comes from a per-window fallback chain.

*Why it breaks:* Latent global-versus-actual mismatch: msaaLevel is the requested sample count, not the achieved one. CreateSDLWindow (GlobalRendering.cpp:398-460) discards the achieved samples and depth bits, and CheckGLMultiSampling (:1852-1866) only validates non-zero. Only two consumers truly need the sample count to equal the window default framebuffer: (a) DepthBufferCopy's MS path, which blits from the default FB (DepthBufferCopy.cpp:83-84) into an MS texture created with msaaLevel samples (:140); (b) Lua MS textures and RBOs, which the engine explicitly forces to msaaLevel because the default FB's count must match "or blitting breaks" (LuaOpenGL.cpp:4270-4272, LuaFBOs.cpp:767-800 can blit to or from the default FB). Both break only when MSAALevel > 0 and the driver silently gave fewer samples than requested. The depth-format mismatch between the window (a 24/32/16 fallback) and the FBO-probed supportDepthBufferBitDepth already exists at default config and is tolerated by drivers. For multi-window, the extra constraint is that a single GL context can only be made current on a drawable with a compatible pixel format (WGL and GLX), so window 2 must get exactly window 1's achieved format. A mismatch most likely shows up as an SDL_GL_MakeCurrent failure, not a silent blit error.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:402` AA x depth fallback chain per window (402-449), msaaLevel never updated to the achieved level
- `rts/Rendering/GlobalRendering.cpp:1721` msaaLevel *= CheckGLMultiSampling(); the check at 1852-1866 reads GL_SAMPLES of the current framebuffer
- `rts/Rendering/GlobalRendering.cpp:914` supportDepthBufferBitDepth probed with 16x16 FBO renderbuffers (914-932), independent of window depth bits
- `rts/Rendering/DepthBufferCopy.cpp:140` glTexImage2DMultisample(msaaLevel); blit from default FB at 84
- `rts/Rendering/GL/GeometryBuffer.cpp:144` MS depth/colour use global msaaLevel (151)
- `rts/Game/UI/MiniMap.cpp:1145` CreateRenderBufferMultisample(msaaLevel); same global in Lua: LuaOpenGL.cpp:4272, LuaTextures.cpp:123
- `rts/Rendering/Units/UnitDrawer.cpp:800` alpha-to-coverage keyed on global msaaLevel >= 4
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:1730` HighQualityDecals requires msaaLevel > 0 (also 106)

*Direction:* Keep the claim's direction, but frame it as a robustness fix. After creating window 1, read back the achieved samples and depth bits (SDL_GL_GetAttribute, or GL_SAMPLES on the default FB), store them in a per-target format struct, and size the MS DepthBufferCopy and Lua MS textures from the actual samples. Create window 2 with window 1's achieved attributes instead of re-running the fallback chain from the requested msaaLevel. Fail loudly if its pixel format differs from the context's. Optionally assert equal samples in FBO::Blit before blitting. Not needed for the 2-window 3D-only prototype at MSAALevel 0, so severity is full, not risk.

> verifier B (corrected): The mechanism is real, but it is a latent single-window bug that multi-window does not worsen, and it does not hit the 2-window 3D-only prototype with default settings, so "risk" is too high. Verified facts:

(1) CreateSDLWindow (rts/Rendering/GlobalRendering.cpp:402-449) runs the aaLvls x zbBits fallback chain and only logs the achieved format (line 447). msaaLevel is never lowered to the achieved level.

(2) msaaLevel *= CheckGLMultiSampling() (line 1721) is a boolean multiply. The check at 18

> verifier A (corrected): The core mechanism holds in the current tree, but the claim overstates which consumers are affected and over-rates severity for the 2-window 3D-only prototype. What holds: msaaLevel has only three writers, which are the config init (GlobalRendering.cpp:287), the odd-to-even bump (:566) and `msaaLevel *= CheckGLMultiSampling()` (:1721). CheckGLMultiSampling (:1852-1866) only tests that GL_SAMPLES is non-zero on the bound framebuffer. CreateSDLWindow (:398-460) is const, runs the AA x depth fallba

### CFG-09 - creg registration and HEADLESS build for new target state
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** TGT-01

CGlobalRendering is a creg struct placed into static memory. New RenderTarget state must stay out of creg or be ignored, and must compile and behave in HEADLESS builds.

*Why it breaks:* CGlobalRendering is creg-registered and placement-new'd into static storage (GlobalRendering.cpp:100, :106-221, :224), but it is never part of the creg save: CGameStateCollector::Serialize (CregLoadSaveHandler.cpp:78-116) does not serialize globalRendering. The existing CR_MEMBER/CR_IGNORED split therefore constrains nothing, and RenderTarget state can live in plain non-creg members, including a std::vector inside CGlobalRendering. The real HEADLESS constraint is different. HEADLESS builds still call the SDL window/context API and link against rts/lib/headlessStubs/sdlstub.c. That stub implements only a subset: SDL_CreateWindow and SDL_GL_CreateContext return one shared static &foo, and SDL_GL_MakeCurrent and SDL_GL_SwapWindow are no-ops. The stub does not define SDL_GetWindowSize, SDL_GetWindowPosition, SDL_GetWindowID, SDL_GL_GetCurrentContext or SDL_GL_GetDrawableSize, which is why ReadWindowPosAndSize has its own #ifdef HEADLESS branch. Two consequences follow. First, any multi-target code that calls one of the unstubbed SDL functions outside an #ifdef HEADLESS guard (for example window-id based event routing or per-window size queries) breaks the link of engine-headless and engine-dedicated, which are also part of the `check` target. Second, in HEADLESS every target's SDL_Window* and SDL_GLContext compare equal, so a map from SDL window pointer to target collapses to a single entry. Targets must be keyed by their own id, not by the SDL pointer.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:108` CR_REG_METADATA: only the per-frame timing and draw flags are CR_MEMBER
- `rts/Rendering/GlobalRendering.cpp:1548` HEADLESS stub of ReadWindowPosAndSize (1548-1556)
- `rts/Rendering/GlobalRendering.cpp:100` static placement storage globalRenderingMem

*Direction:* Drop the creg-related direction; RenderTarget does not need to avoid or mimic creg. Instead: (a) do not use unstubbed SDL functions without an #ifdef HEADLESS guard or an added stub in rts/lib/headlessStubs/sdlstub.c; (b) do not key targets by SDL_Window* or SDL_GLContext pointer, because the HEADLESS stubs return identical pointers; (c) make the HEADLESS path create only a single stub target of fixed size (the existing 8x8 fallback), or make the additional target a no-op, so headless sim/test runs behave as before; (d) build engine-headless once after the prototype lands to catch link regressions.

> verifier A (corrected): The cited lines are real (CR_REG_METADATA at GlobalRendering.cpp:108-221 with CR_MEMBER for drawFrame/timeOffset/grTime and CR_IGNORED for geometry; globalRenderingMem at :100; HEADLESS 8x8 branch at :1548-1556). Two parts of the mechanism are wrong. (1) The creg half is vacuous. CGlobalRendering is never serialized: CGameStateCollector::Serialize (rts/System/LoadSave/CregLoadSaveHandler.cpp:78-116) lists gs, gu, game, readMap, unitHandler and others, and does not include globalRendering. No tes

### CFG-10 - Exclusive fullscreen default and DualScreenMode conflict with extra windows
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** No opt-in request surface for extra render targets

Fullscreen defaults to true (exclusive SDL_WINDOW_FULLSCREEN unless borderless), and DualScreenMode splits one window into game and minimap viewports. Both change what 'the view' means and interact badly with a second window.

*Why it breaks:* Fullscreen defaults to true (GlobalRendering.cpp:73), and the exclusive flag SDL_WINDOW_FULLSCREEN is used unless borderless (line 1248). An exclusive-fullscreen primary window on the same display would plausibly hide or flicker a second window. I did not verify this at runtime. MinimizeOnFocusLoss defaults to 0 (line 71), and the hint is only cleared when it is 0 (lines 573-574). So the claim that it would minimize the second window is false by default; it only matters if the user sets it to 1. DualScreenMode is read in SetDualScreenParams (line 1425) and starts as false (line 339). When it is on, UpdateViewPortGeometry (line 1433) halves viewSizeX, so the per-target viewport source is ambiguous. That is a real design point, but the setting is opt-in. The prototype can sidestep all of this by running windowed with DualScreenMode=0, so it is not a risk to the prototype.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:73` Fullscreen default true
- `rts/Rendering/GlobalRendering.cpp:428` FULLSCREEN vs FULLSCREEN_DESKTOP flags
- `rts/Rendering/GlobalRendering.cpp:573` MinimizeOnFocusLoss hint
- `rts/Rendering/GlobalRendering.cpp:1433` UpdateViewPortGeometry dual-screen split
- `rts/Game/Game.cpp:1595` minimap drawn on the dual viewport
- `rts/System/SpringApp.cpp:1113` MOVED reruns geometry only when dualScreenMode

*Direction:* When extra render targets are requested, validate that Fullscreen=0 (windowed or borderless) and DualScreenMode=0, and log a notice otherwise. Warn about MinimizeOnFocusLoss=1 only as a minor note. Each render target should own its own view geometry, so that dual-screen does not leak into the per-target viewport. Effort S.

> verifier B (corrected): The claim is partly wrong and overrated for the prototype. MinimizeOnFocusLoss defaults to 0 (GlobalRendering.cpp:71) and line 573 only sets the SDL hint to "0" when the config is 0, so the minimize-on-focus-loss risk exists only if a user opts in. Exclusive fullscreen on one display is a legitimate setup constraint. But the prototype can simply run windowed (Fullscreen=0 or borderless), which is a user or config choice, not an engineering blocker. DualScreenMode defaults to false (GlobalRenderi

> verifier A (corrected): The Fullscreen and DualScreenMode facts hold, but the MinimizeOnFocusLoss part is wrong, and the whole item is a validation or config concern rather than a prototype risk.

### CFG-11 - Global ground-drawer flags, info-texture mode and debug overlays are not per target
**Severity:** polish - **Effort:** S - **Status:** confirmed - **Depends on:** none

Wireframe, advanced shading, ground detail, map border, drawGround, the info-texture overlay mode and the SmoothHeightMesh debug mesh are single global values applied to every target.

*Why it breaks:* CBaseGroundDrawer holds wireframe/advShading/drawMapEdges (BaseGroundDrawer.h:85-92) and groundDetail (SMFGroundDrawer.h:96), applied inside the shared Draw path (SMFGroundDrawer.cpp:325-328, :422-423, :593-620). CInfoTextureHandler has one current mode and one texture (InfoTextureHandler.cpp:137-147), consumed by terrain, grass and decals via GetCurrentInfoTexture. SmoothHeightMeshDrawer::Draw rebuilds the whole mesh into the shared RenderBuffer<VA_TYPE_0> each call when enabled (SmoothHeightMeshDrawer.cpp:82-124, default off). Per-target overrides (e.g. a debug wireframe window, different overlay) are not possible without moving these into a per-view options struct.

*Evidence:*
- `rts/Map/BaseGroundDrawer.h:91` wireframe/advShading global flags
- `rts/Map/SMF/SMFGroundDrawer.cpp:593` GetGroundDetail is a single global value scaled per pass
- `rts/Rendering/Map/InfoTexture/Modern/InfoTextureHandler.cpp:137` one current info texture for all consumers
- `rts/Rendering/SmoothHeightMeshDrawer.cpp:82` debug mesh rebuilt into shared RenderBuffer per draw

*Direction:* Move these into a per-view options struct (defaulting to the global values) after the prototype; no action needed for the first two-window test.

> verifier A (confirmed): I opened every cited location and the mechanism holds in the current tree. These are single process-wide values with no per-target indirection, and none of them blocks or corrupts the 2-window 3D-only prototype, so polish is the right severity.

- **Ground flags:** BaseGroundDrawer.h:85-92 holds drawMapEdges (:87), wireframe (:91) and advShading (:92) as plain members. BaseGroundDrawer.h:61-62 hands out mutable refs to the last two. The WireMap and map-shaders console commands toggle them (Unsyn

### CFG-12 - Window chrome and OS hints (title, icon, compositing bypass, Win32 style, screensaver) are applied to a single sdlWindow
**Severity:** polish - **Effort:** S - **Status:** confirmed - **Depends on:** none

Title, icon, compositor-bypass and resizable-style helpers are keyed to the one sdlWindow or to a single static icon surface. A second window would get no title or icon and no compositing block, and the queued main-thread setters capture a raw SDL_Window* that dangles if that window closes first.

*Why it breaks:* SetWindowTitle only ever targets the member sdlWindow, and the cross-thread path enqueues a lambda holding the raw pointer. SetIconSurface keeps one static WindowIcon (single surface and bitmap) and is torn down in DestroyWindowAndContext only for the primary window. BlockCompositing runs once at creation and SetWindowResizable runs from SetWindowAttributes; a window #2 has to repeat both explicitly. SDL_DisableScreenSaver and EnableScreenSaver are process-wide and not a problem. A closed window #2 with a pending queued title or icon call would hand SDL a freed window.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:1202` SetWindowTitle uses the member sdlWindow; the non-main-thread path enqueues the raw pointer
- `rts/System/Platform/WindowManagerHelper.cpp:50` static WindowIcon holds one surface and bitmap shared by all callers
- `rts/System/Platform/WindowManagerHelper.cpp:81` queued SetWindowIconImpl captures win
- `rts/Rendering/GlobalRendering.cpp:576` SetWindowAttributes and BlockCompositing are applied once at creation of the primary window only
- `rts/Rendering/GlobalRendering.cpp:621` icon is released on exit via the primary sdlWindow only

*Direction:* Make title, icon and window-style setup per-window (a helper taking SDL_Window*) and share the icon bitmap but not the surface. Cancel or validate queued window operations by windowID instead of raw pointer when a window is destroyed.

> verifier A (confirmed): Verified in the current tree. SetWindowTitle (GlobalRendering.cpp:1202-1214) uses the member sdlWindow and enqueues the raw SDL_Window* when called off the main thread. SetIconSurface (WindowManagerHelper.cpp:49-87) keeps one function-static WindowIcon. The queued SetWindowIconImpl captures win (line 84). The icon is cleared only through the primary sdlWindow in DestroyWindowAndContext (GlobalRendering.cpp:621). BlockCompositing runs once for sdlWindow at creation (lines 583-584). Minor refineme

## Design decisions

### DSN-01 - Define the per-render-target view context and split the frame into global update vs per-view prepare/draw
**Severity:** blocker - **Effort:** M - **Status:** corrected - **Depends on:** none

Nothing in the engine represents 'a view'. The frame is UpdateUnsynced (global, camera-dependent) then GenerateIBLTextures then one WorldDrawer::Draw. A decision is needed on what a view owns and where per-view work runs.

*Why it breaks:* Nothing in the engine represents a view. All view-dependent state is keyed to the single global camera pool (static CCamera cameras[CAMTYPE_COUNT], CameraHandler.cpp:94) and to globalRendering. The frame runs as one global UpdateUnsynced, then one GenerateIBLTextures, then one WorldDrawer::Draw. Concretely:

1. CCamera::Update derives its aspect ratio, pixel scale and viewport from the global globalRendering (pixelY, aspectRatio, viewSizeX/Y, viewPosX/Y) at Camera.cpp:144-155. A camera does not own its viewport, so a second window cannot get a correct projection without a view-owned viewport.
2. The frame uniform buffer reads only CAMTYPE_PLAYER (UniformConstants.cpp:167-179). It also holds UWREFL matrices (186-193), viewGeometry from globalRendering (222), fogParams scaled by the PLAYER far plane (230-231), and mouse and minimap state (250). It is updated once at Game.cpp:1441 and bound once at Game.cpp:1464. A second view needs its own upload and bind. The existing second camera->Update() at WorldDrawer.cpp:316 refreshes the camera but not this buffer.
3. Draw flags are computed once per frame for every camera type in one unit-owned loop (UnitDrawerData.cpp:430-452, camType PLAYER..<ENVMAP). They are reached via worldDrawer.Update -> CUnitDrawer::UpdateStatic (WorldDrawer.cpp:225), so there is one flag set for one camera set.
4. The grass visibility cache (oldCamPos, oldCamDir, blockDrawer) reads CAMTYPE_PLAYER (GrassDrawer.cpp:518-533) from an EventClient Update.
5. The shadow fit is derived from the PLAYER camera (ShadowHandler.cpp:128-134, SetShadowMatrix). It runs once in Update and again in CreateShadows.
6. Ground Update runs once, before any target exists (WorldDrawer.cpp:217-220).
7. Water reflection and refraction render via the global camera (IWater.cpp:139-222).

So the per-view work (camera set, viewport, flags, caches, buffer, shadow and water results) is spread over UpdateUnsynced, eventHandler.Update and WorldDrawer::Draw. Without an explicit view context, each fix would invent its own "which window" answer.

*Evidence:*
- `rts/Game/Game.cpp:1376` camHandler->UpdateController runs in UpdateUnsynced and drives the only camera
- `rts/Game/Game.cpp:1430` camera->Update() then shadowHandler.Update (1431), worldDrawer.Update (1433), UniformConstants Update (1441): all view-dependent, all once per frame
- `rts/Game/Game.cpp:1519` GenerateIBLTextures, FBO::Unbind and camera->LoadViewport (1524) run once, then worldDrawer.Draw (1526)
- `rts/Rendering/GlobalRendering.cpp:1433` existing viewPos/viewSize abstraction (dual-screen mode) is a precedent for view != window, but is global, not per target
- `rts/Rendering/Common/ModelDrawer.h:110` Draw/DrawOpaquePass/DrawAlphaPass/DrawShadowPass signatures have no view parameter (lines 110-116)
- `rts/Rendering/WorldDrawer.cpp:372` unitDrawer->Draw / featureDrawer->Draw; alpha passes at 413-414 and 453-454, icons at 482
- `rts/Rendering/Env/IWater.cpp:163` reflection/refraction passes call the drawers with only bool flags (163-169, 206-212)
- `rts/Rendering/Units/UnitDrawer.h:22` MiniMapIconDrawParams: existing precedent for an explicit viewer/cull parameter struct
- `rts/Rendering/Common/ModelDrawerState.cpp:317` pass kind read from game->GetDrawMode() (also 345)
- `rts/Map/BaseGroundDrawer.h:33` Draw(DrawPass) with no target/camera argument
- `rts/Game/Camera.h:316` #define camera (CCamera::GetActive())
- `rts/Game/CameraHandler.cpp:94` static camera pool indexed by type
- `rts/Map/SMF/SMFReadMap.cpp:1029` single ground drawer per map
- `rts/Rendering/WorldDrawer.cpp:219` ground Update once per frame, before any target exists

*Direction:* Split the work into two phases.

Phase A (prototype, effort M). Do not build a RenderView that owns caches or a UBO. Add a small struct per extra window with its own CCamera state and viewport.
- Global stage, once per frame. After camHandler->UpdateController, update the secondary camera. Make the draw-flag and visibility passes use the union of both views. The passes are CUnitDrawerData/CFeatureDrawerData::UpdateObjectDrawFlags, ProjectileDrawer's camPlayer-based flags, and grass/ground LOD. For example, add a CAMTYPE_PLAYER2 entry to the camType loop at UnitDrawerData.cpp:430, or OR the flags in a second pass. This must happen before worldDrawer.Update (Game.cpp:1433). Run GenerateIBLTextures (shadows, cubemap, sky) once, fitted to the primary camera.
- Per-view stage, once per window. Wrap it in a scoped swap that follows the existing GetSetActiveCamera/restore convention. The scope copies the view's camera state into the PLAYER slot (CopyState) and sets globalRendering view geometry (the dual-screen UpdateViewPortGeometry/UpdatePixelGeometry precedent at GlobalRendering.cpp:1433-1543), or alternatively gives CCamera::Update an explicit viewport parameter. Then call camera->Update, UniformConstants::UpdateMatrices and glViewport, clear, and run only DrawOpaqueObjects, DrawAlphaObjects and the 3D parts of DrawMiscObjects in that window's context. Finally restore the primary state. Skip eventHandler.DrawWorld and the GUI for secondary views.
- Not covered by this item: GL context sharing, shadow-map fit to a non-primary camera, and the per-view camera controller. Those are separate problems.

Phase B (full product, effort L to XL). Promote the scoped swap into a real RenderView owning its camera set, per-view caches, viewport and UBO, once GUI, Lua and input per target need it.

> verifier A (corrected): The cited code does what is claimed, and no view abstraction exists in the current tree (grep for RenderView, RenderTarget and ViewContext finds nothing in rts/ outside rts/lib). I checked each citation.

Game.cpp: UpdateController is at 1376, camera->Update at 1430, shadowHandler.Update at 1431, worldDrawer.Update at 1433 and UniformConstants::Update at 1441. IBL generation is at 1519, FBO::Unbind plus LoadViewport at 1522-1524, and worldDrawer.Draw at 1526.

Other citations that hold:
- ModelD

> verifier B (corrected): The problem is real and the premise holds. The frame is a single global stage (Game.cpp:1376 UpdateController, 1430 camera->Update, 1431 shadowHandler.Update, 1433 worldDrawer.Update, 1441 UniformConstants::Update) followed by one Draw (1519-1526). All of it is keyed to the PLAYER camera, so a second window cannot be drawn without some per-view prepare step. Without one, window 2 would reuse window 1's matrices or draw nothing. I checked the code and confirmed three concrete failure mechanisms:


### DSN-02 - GL context architecture decision: one context across N windows, N shared contexts, or offscreen render plus present
**Severity:** blocker - **Effort:** S - **Status:** confirmed - **Depends on:** TGT-01

The engine has one window and one context and no sharing precedent. Which of A (one context, MakeCurrent per window), B (N shared contexts) or C (offscreen FBO per target, blit to windows) is chosen decides most of the other problems in this domain.

*Why it breaks:* (A) One context, SDL_GL_MakeCurrent(window_i, ctx) per target. Every non-shared object (VAOs, FBOs, queries, indexed UBO/SSBO bindings, glEnable and fixed-function state) stays valid because only one context exists. Costs: matching pixel formats (window 2 must be compatible), per-target make-current and swap, context state shared across targets, one default framebuffer per window. The engine already hands its single context between the main thread and the load thread with MakeCurrent set/clear, so MakeCurrent churn has precedent. (B) Textures, buffers, shaders, RBOs, syncs and display lists are shared. VAOs (12 owner classes), FBOs (about 19 persistent), queries, indexed bindings, all one-time context state, ARB env params and KHR_debug callbacks are per-context and must be duplicated or re-initialised. In-place streamed buffers also need cross-context sync. This is the only option that allows parallel render threads, and it is an order of magnitude more work. (C) Render each target into an offscreen FBO inside the primary context, then blit to each window. It has A's single-context safety and relaxes window pixel-format needs (no depth or MSAA on windows). But about 20 FBO::Unbind() sites rebind FBO 0 and would escape the offscreen target, default-framebuffer readbacks need rework, and an MSAA resolve plus extra memory is added.

*Evidence:*
- `rts/Rendering/GlobalRendering.h:384` single sdlWindow and glContext members
- `rts/Rendering/GlobalRendering.h:385` single SDL_GLContext glContext
- `rts/Rendering/GlobalRendering.cpp:474` single SDL_GL_CreateContext(sdlWindow)
- `rts/System/LoadLock.h:14` context hand-off between threads goes through the same singleton call
- `rts/Rendering/GL/FBO.cpp:322` Unbind is static and binds 0 by design (322-336)
- `rts/Rendering/GL/VAO.cpp:21` glGenVertexArrays: container object not shared across contexts
- `rts/Rendering/GL/RenderBuffers.h:778` every RenderBuffer owns a VAO created in the primary context
- `rts/Rendering/GlobalRendering.cpp:1715` glClipControl is per-context state set once in InitGLState
- `rts/Rendering/UniformConstants.cpp:335` indexed UBO bindings are context state
- `rts/Rendering/GlobalRendering.cpp:703` SDL_GL_SwapWindow(sdlWindow); SDL requires that window to be current
- `rts/Rendering/GL/FBO.cpp:250` FBO object created once per context
- `rts/Rendering/GL/myGL.cpp:466` cached draw/read blit FBOs
- `rts/Rendering/GlobalRendering.cpp:664` glGenQueries once; queries are not shared between contexts
- `rts/Rendering/GlobalRendering.cpp:402` pixel format chosen by an aa/depth fallback loop (402-449)
- `rts/Rendering/Env/ISky.cpp:58` FFP fog parameters are context state
- `rts/Rendering/Common/ModelDrawerState.cpp:1` compatibility-profile FFP state (glPushAttrib) used throughout, so context 2 would also need compat profile
- `rts/Rendering/GlobalRendering.cpp:612` MakeCurrentContext(bool clear) hardwired to the single pair
- `rts/System/SpringApp.cpp:901` CLoadLock unique lock held across activeController->Draw()
- `rts/Rendering/GL/StreamBuffer.cpp:64` unbounded glClientWaitSync spin; cross-context fences need the creator to flush
- `rts/Rendering/GL/RenderBuffers.h:439` RenderBuffer owns vbo/ebo/vao
- `rts/System/Threading/ThreadPool.cpp:89` glThreadSupport = false, no secondary GL contexts
- `rts/Game/LoadScreen.cpp:125` stale 'rebinds primary context to hidden window' comment
- `rts/Rendering/GlobalRendering.cpp:587` one CreateGLContext call; no share-with-current attribute anywhere in rts/
- `rts/Game/Game.cpp:1521` FBO::Unbind + camera->LoadViewport restores 'the default FBO / viewport' before WorldDrawer::Draw (1526)
- `rts/Map/SMF/SMFGroundDrawer.cpp:228` deferred pass skipped for reflection/refraction/cubemap because they own an FBO (228-236)
- `rts/Rendering/Common/ModelDrawer.h:326` model deferred pass conditional because water renderers use their own FBOs
- `rts/Game/UI/MiniMap.cpp:1140` only existing MSAA-FBO plus Blit resolve precedent (1140-1146, 1217-1220)
- `rts/Rendering/GlobalRendering.cpp:692` SwapBuffers binds read FB 0 then SDL_GL_SwapWindow(sdlWindow) (703)

*Direction:* Adopt A (one context, MakeCurrent per window) and record the choice as settled. Treat B and C as rejected for the prototype. The implementation hazards under A are below, in rough order of severity.

1. Present path. GlobalRendering::SwapBuffers (GlobalRendering.cpp:671-718) is called once from SpringApp::Update (SpringApp.cpp:906), after Draw, for a single sdlWindow. SDL2's SDL_GL_SwapWindow only swaps the currently current window and otherwise just sets an SDL error, so a swap issued after drawing window 2 silently does nothing. Split it into two parts.
   - A per-frame part, run exactly once after all views are drawn: RenderBuffer::SwapRenderBuffers (:688), IStreamBufferConcept::PutBufferLocks (:689), FrameMark (:713) and the timers.
   - A per-window present: MakeCurrent(win_i, ctx), glBindFramebuffer(READ, 0), then SDL_GL_SwapWindow(win_i).
   RenderBuffer and StreamBuffer ring slots advance once per swap (StreamBuffer.h:384-387), so calling the per-frame part per window would burn ring slots and add fence waits.

2. Window 2 pixel format must match window 1. CreateSDLWindow (GlobalRendering.cpp:398-460) picks MSAA and depth bits through a fallback loop and leaves SDL_GL_* attributes at the last attempt. Window 2 must reuse the final accepted values and be created after window 1's context exists. A mismatch makes wglMakeCurrent/glXMakeCurrent fail, so every GL call afterwards on that thread is a silent no-op. Check the SDL_GL_MakeCurrent return value and fall back to single-window.

3. Swap interval is per drawable. VerticalSync::SetInterval (VerticalSync.cpp:62-89) calls SDL_GL_SetSwapInterval only for the current drawable. With vsync=1 on both windows, two blocking swaps per frame can halve the frame rate. Set the secondary window's interval to 0 (or only block on the primary) while that window is current.

4. MakeCurrentContext(bool clear) is hard-wired to sdlWindow (:612-613). It is called per window-change frame from UpdateWindow (:1305), on DISPLAY_CHANGED (SpringApp.cpp:1246), and from CLoadLock lock/unlock (LoadLock.h:21,32) and LoadScreen.cpp:175. Give it a target or window parameter, or reset the current window to the primary at the start of Draw, and guarantee that restore happens at the end of each per-window pass.

5. SDL window events in MainEventHandler (SpringApp.cpp:1095-1253) never check event.window.windowID. A CLOSE on window 2 calls RequestQuit (:1249-1251), and SIZE_CHANGED/MOVED run UpdateGLGeometry/InitGLState/SaveWindowPosAndSize against the primary window's geometry. Route events by window ID. This is not a blocker for a fixed-size second window.

Also introduce a minimal RenderTarget or View abstraction (SDL_Window*, size, camera, swap interval) so a later move to C or B does not touch call sites.

> verifier A (confirmed): I opened the cited code in the current tree and the claim holds. The engine has one SDL_Window and one SDL_GLContext (GlobalRendering.h:384-385). The context is created once in CreateWindowAndContext (GlobalRendering.cpp:587), and every SDL_GL_CreateContext call at 474, 490 and 515 targets sdlWindow. A grep of non-lib rts finds no SDL_GL_SHARE_WITH_CURRENT_CONTEXT, so there is no context-sharing precedent. MakeCurrentContext (612-614) is SDL_GL_MakeCurrent(sdlWindow, clear ? nullptr : glContext)

> verifier B (confirmed): The mechanism and evidence check out against the code. There is exactly one SDL_CreateWindow (GlobalRendering.cpp:442), one SDL_GL_CreateContext path (:474/:490/:515, all on sdlWindow), one MakeCurrentContext (:612-613) and one SDL_GL_SwapWindow(sdlWindow) (:703). A repo-wide grep finds no SDL_GL_SHARE_WITH_CURRENT_CONTEXT and no second window or context. Without a second drawable made current and swapped, window 2 stays empty, so this is a true prerequisite and blocker for the prototype. The ot

### DSN-03 - Migration mechanism for ~400 reader lines of per-target globals
**Severity:** risk - **Effort:** M - **Status:** corrected - **Depends on:** TGT-01

A design decision is needed on how roughly 400 reader lines in about 55 files move off the per-target globals, so the prototype does not need a flag-day rewrite.

*Why it breaks:* Every legacy field is a plain int or float member of CGlobalRendering (GlobalRendering.h:161-216). Readers fall into three kinds, and registers copied in by ActivateTarget (option A) only fix the first.

1. Readers inside a per-target draw scope. These work under A. Examples are CCamera::Update (Camera.cpp:144-155), CalcPixelDir (Camera.cpp:571-574), AdvWater.cpp:134-136, DynWater.cpp:340-341, DepthBufferCopy.cpp:72-73 and GeometryBuffer.cpp:210-216.

2. Readers in once-per-frame update phases that run outside any scope.
   - UniformConstants::Update() runs in CGame::UpdateUnsynced (Game.cpp:1441) and is bound once in Draw (Game.cpp:1464).
   - Its viewGeometry (UniformConstants.cpp:222) and camera matrices (UpdateMatricesImpl, UniformConstants.cpp:161) are therefore computed once, from whichever target the registers hold at that moment.
   - CUnitDrawerData::Update bakes iconSizeBase from viewSize (UnitDrawerData.cpp:217).
   - CSMFGroundTextures::DrawUpdate derives its LOD diagonal from viewSize (SMFGroundTextures.cpp:351-353).
   - Window 2 would reuse window 1's values. Shaders read viewGeometry through the UBO in ModelVertProgGL4.glsl:59 and ModelFragProgGL4.glsl:18.

3. Readers that bake state at init or on the ViewResize event.
   - BumpWater.cpp:181-182 and 425-427 compile viewSize and viewPos into GLSL #defines and screenTextureX/Y.
   - GroundDecalHandler.cpp:473-476 and 1693-1702 set the screenSizeInverse uniform once, on init and on ViewResize.
   - DepthBufferCopy.cpp:55-63 and 138-140, and RefractWater.cpp:48-79, size textures from viewSize on init or ViewResize.
   - DynWater.cpp:55 picks refractSize from viewSizeY at init.
   - The existing eventHandler.ViewResize (Game.cpp:1127) fires only on a real resize, so switching targets cannot reuse it. Two windows of different sizes would be wrong without per-target resources.

The fields also mix three notions: window (win*), the viewport sub-rect (view*), and the dual-screen second viewport (dualView*, GlobalRendering.cpp:1433-1470, plus Camera.cpp:740-743). A target abstraction has to subsume the existing DualScreenMode viewport or force it off for secondary targets. "Writes outside a scope silently hit the primary" is only true if the scope restores the primary on exit. Otherwise the registers keep the last-activated target. UpdateViewPortGeometry (GlobalRendering.cpp:1433) is a writer that derives view* from winSize* of the single SDL window.

*Evidence:*
- `rts/Game/Camera.cpp:141` Update reads pixelY, aspectRatio, viewSize, viewPos (141-162); the single funnel for camera view data
- `rts/Rendering/UniformConstants.cpp:222` viewGeometry from registers
- `rts/Game/UI/MiniMap.cpp:97` about 72 globalRendering lines, 28 of them viewSize; GUI, not needed for the prototype
- `rts/Lua/LuaHandle.cpp:2588` Lua view geometry is a bulk reader (28 globalRendering lines in the file)
- `rts/Rendering/Env/BumpWater.cpp:425` example of a 3D-path reader that bakes view state

*Direction:* Keep the phased plan, but state its limits and split out the concrete work.

- Phase 0 uses option A with a RAII RenderTargetScope that saves and restores the previously active target. It must include running UniformConstants.Update (params and matrices) and Bind once per target inside the scope, instead of once per frame in UpdateUnsynced. For the prototype this is the minimum that keeps the second window's view data from being stale.
- For the prototype, constrain both windows to the same size. That lets the init-time bakes (BumpWater #defines, GroundDecalHandler uniform, DepthBufferCopy and RefractWater textures, DynWater refractSize) survive until Phase 1.
- Phase 1 moves the caching and baking consumers to explicit per-target data.
  - Pass a RenderView& into CCamera::Update and UpdateMatrices.
  - Remove CubeMapHandler.cpp:222's direct global read.
  - Move per-target view state out of baked shader constants and into the UBO viewGeometry.
  - Make DepthBufferCopy, GeometryBuffer, RefractWater and BumpWater screen textures per-target.
  - Fix UnitDrawerData::Update and SMFGroundTextures::DrawUpdate to run per target or take the view as a parameter.
- Add a debug assert that the legacy per-target registers are only read while a scope is active.
- Phase 2 moves GUI, Lua and input readers (MiniMap, LuaHandle, MouseHandler, camera edge-scroll at Camera.cpp:736-745) to their own target's view.
- Downgrade this item from blocker to risk. List the UBO per-target update and the baked per-target resources as separate items, and keep the real blocker on the singleton-state item.

> verifier B (corrected): The problem is real but overstated as a blocker. The counts are largely right: viewSize 170, viewPos 69 and win* 88 all reproduce. The "reader" label is wrong, though. Many of those lines are the definitions and writers inside the singleton itself. In GlobalRendering.{h,cpp} that is viewSize 26, viewPos 18, win* 54 and dual* 42, and dual* totals 84, not about 60. The aspect/pixel regex also hits rts/lib/fastgltf (10 lines) and ProfileDrawer (47 GUI lines). Real external readers are nearer 300, a

> verifier A (corrected): The counts and the A/B/C framing reproduce. viewSize is 170 lines and viewPos is 69, MiniMap.cpp has 72 globalRendering lines (28 viewSize), LuaHandle.cpp has 28, and win* is 88. Excluding GlobalRendering.{h,cpp} and rts/lib, a combined grep gives 392 lines in 50 files, so "~400 in ~55" holds. The 3D-path files total about 62 lines, which fits "65-70 in ~17 files". Three things need correcting. (1) The per-field counts include the definitions and writers in GlobalRendering.{h,cpp} (26 viewSize, 

### DSN-04 - Per-view firing policy for Lua draw callins
**Severity:** risk - **Effort:** S - **Status:** corrected - **Depends on:** Overlay draws are interleaved in CWorldDrawer::Draw; only DrawMiscObjects is cleanly skippable

Every Lua draw callin is embedded in an engine pass, so rendering a second view through the same passes fires all of them again. They include LuaUI/LuaRules overlay callins and engine-correctness callins. The prototype must decide which fire for which view.

*Why it breaks:* Every Lua draw callin is fired from inside an engine pass, and CEventHandler's DRAW_CALLIN / DRAW_ENTITY_CALLIN macros and the 8 hand-written entry points carry no view or pass-context parameter. Re-running a pass for window 2 therefore re-fires everything in that pass, with window-1 state in Lua (gl.GetViewSizes, Spring.GetDrawFrame, no target identity). The real classes depend on pass context, not only on view.

(a) The shadow callins (DrawWorldShadow, DrawShadowPassTransparent, DrawShadow{Units,Features}Lua) fire once per frame from CreateShadows, which GenerateIBLTextures calls (WorldDrawer.cpp:257). They are not view-independent: the shadow matrix and projection scale are fitted to the CAMTYPE_PLAYER camera. ShadowHandler.cpp:130/133 and 532-533 do this, and the default mode, CAM_CENTER (:34, :701), uses GetOrthoProjectedFrustumRadius(playerCam). Once-per-frame firing is only valid if the single shadow map stays shared between windows, which would leave window 2 with shadows fitted to window 1's camera. If shadow maps become per-view, these callins must fire per view. Their firing cadence is coupled to the shadow-ownership decision.

(c) The ground callins DrawGroundPre/PostForward are not only view-bound. Ground::Draw(drawPass) reaches DrawForwardPass (SMFGroundDrawer.cpp:416-417), which fires them at :336/:346. It is called from several passes:
- The once-per-frame, view-independent cubemap pass, Draw(DrawPass::TerrainReflection) at CubeMapHandler.cpp:232 from WorldDrawer.cpp:264.
- The per-view water reflection and refraction FBO passes (IWater.cpp:158/201). DrawDeferredPass returns early for these at SMFGroundDrawer.cpp:229-235, so only the forward events fire there.
- The main Normal pass.
The Lua callins take no argument saying which pass they are in. A gate keyed only on a 'current view policy' would therefore either suppress the cubemap-pass call or fire it with whatever view was last active.

Class (c) as a whole is an optional hook for the engine, not a hard requirement. The docs (LuaHandle.cpp:2860/2871) describe PreForward/PreDeferred as 'convenient for setting uniforms' when a Lua map shader is set. DrawGroundPostDeferred, DrawUnitsPostDeferred and DrawFeaturesPostDeferred exist so Lua can gl.CopyToTexture the G-buffer (LuaHandle.cpp:2886-2894). LuaUI widgets also register for these, so 'which client type' is a separate axis from the class.

Also unguarded: Ground*, PreDecals, WorldPreParticles and WaterPost are NOOP_STATE_FUNCS (LuaOpenGL.h:66-68, 85-91). They do no push or reset of GL state, so Lua leaks from view 1 straight into view 2.

*Evidence:*
- `rts/System/EventHandler.cpp:680` DRAW_CALLIN macro (680-700): no view/target parameter; 21 instantiations at 703-723
- `rts/System/EventHandler.cpp:726` DRAW_ENTITY_CALLIN macro for Unit/Feature/Shield/Projectile/Material; the single choke point for gating
- `rts/Rendering/WorldDrawer.cpp:323` eventHandler.DrawWorld() inside the world draw
- `rts/Map/SMF/SMFGroundDrawer.cpp:268` class (c) map-shader callins at 269/286/304/313/336/346, gated by alwaysDispatchEvents || HaveLuaRenderState()
- `rts/Game/Game.cpp:1468` eventHandler.DrawGenesis() once per Draw
- `rts/Rendering/ShadowHandler.cpp:444` DrawWorldShadow fires once per frame from DrawShadowPasses, outside any view
- `rts/Lua/LuaHandle.cpp:2949` DrawWorldPreParticles is documented as called multiple times per draw frame
- `rts/Lua/LuaOpenGL.cpp:604` EnableCommon: AttribBits push only when safeMode
- `rts/Lua/LuaOpenGL.cpp:528` FIXME glViewport depends on mode
- `rts/Lua/LuaOpenGL.cpp:1234` gl.GetViewSizes returns globalRendering view size
- `rts/Rendering/Env/Particles/ProjectileDrawer.cpp:945` DrawWorldPreParticles per particle pass
- `rts/Lua/LuaHandle.cpp:2959` callin has no target identity
- `rts/Rendering/WorldDrawer.cpp:346` DrawPreDecals; DrawWorldPreUnit :367
- `rts/Game/Game.cpp:1536` DrawScreenEffects/DrawScreenPost (:1547) are screen-space callins
- `rts/Lua/LuaUnsyncedRead.cpp:1183` Spring.GetDrawFrame exposes drawFrame

*Direction:* Keep the idea of a per-view Lua draw policy, but gate on pass context plus view, not view alone. Introduce an explicit render-pass context set by the frame driver, with values such as FrameLevel (genesis, shadow, cubemap/IBL), ViewMain, ViewWaterReflection/Refraction and Screen. Check it at the top of the DRAW_CALLIN and DRAW_ENTITY_CALLIN macros and the 8 hand-written entry points.

Prototype defaults:
- FrameLevel stays once per frame, including ground callins fired from the cubemap pass. Fire DrawGenesis once.
- Class (c) callins fire in every view's main and reflection/refraction passes, otherwise Lua map shaders and G-buffer composites break.
- Class (b) overlays (DrawWorld, DrawPreDecals, DrawWorldPreUnit, DrawWaterPost, DrawWorldPreParticles, DrawWorldReflection/Refraction, DrawBuildSquare) default to off for secondary views, and off for all views if the prototype must be pure 3D. Make this a per-view flag.
- Class (d) screen callins are not reached by the 3D-only view driver, because they sit in CGame::Draw outside worldDrawer.Draw (Game.cpp:1536-1547). They need an explicit no-fire rule only if window 1 keeps the screen pass.
- Record the shadow-map ownership decision (shared vs per-view) as a hard dependency. It decides whether the shadow callins stay once per frame.
- Decision for the user: whether LuaUI and LuaRules/LuaGaia are gated separately, since both register for class (b) and (c) callins.
- Optional later: pass a view or target id to the callins so Lua can tell views apart.

Severity: risk (a prototype-time decision with a small gating change), not blocker.

> verifier B (corrected): The mechanism is real and the line evidence checks out. DRAW_CALLIN is at EventHandler.cpp:680-700 with 21 instantiations at 703-723, and the 8 hand-written entry points are at 1016-1062. The call sites in WorldDrawer, SMFGroundDrawer, Game.cpp and ShadowHandler are also correct. But "blocker" is overstated for the 2-window, 3D-only prototype.

1. Doing nothing is a working default. The same-sim second view is just another CWorldDrawer::Draw (WorldDrawer.cpp:302), so every callin fires again, in

> verifier A (corrected): The core mechanism holds, so the claim is not refuted. The line numbers checked match the current tree. The DRAW_CALLIN macro (EventHandler.cpp:680-700) has no view parameter and has 21 instantiations at 703-723. DRAW_ENTITY_CALLIN is at 726-740. There are exactly 8 hand-written entry points: DrawWorldPreParticles, Draw{Opaque,Alpha}{Units,Features}Lua, DrawShadow{Units,Features}Lua and DrawBuildSquare (EventHandler.cpp:1016-1062). Every callsite cited sits inside a pass. WorldDrawer.cpp:323/346

### DSN-05 - Prototype needs a decision on how the second window's camera is driven
**Severity:** risk - **Effort:** S - **Status:** corrected - **Depends on:** none

All camera input (keys, edge scroll, wheel, MMB drag, Lua, minimap) flows into the single active camera via one CCameraHandler. The prototype requires an independent camera for B, but there is no defined source of motion for it.

*Why it breaks:* CameraHandler.cpp:94 holds one static CCamera per type. CCameraHandler has one currCamCtrlNum and one controller set (CameraHandler.h:148,157). UpdateController (CameraHandler.cpp:258-297) drives `camera`, and the Game.cpp:1376 call site is the only one. Key bindings write movState onto the active camera (UnsyncedGameCommands.cpp:771-780). B has no controller, so without a decision B either mirrors A or is driven through the global mouse and keyboard path, which would move A. Options: (a) fixed pose captured when B opens; (b) follow A with an offset, mirror or orbit; (c) set by console or Lua command, for example /cam2 pos rot fov; (d) keyboard-driven only while B has keyboard focus; (e) full mouse-driven per-target controller.

*Evidence:*
- `rts/Game/CameraHandler.cpp:94` static cameras[CAMTYPE_COUNT]: one camera object per type
- `rts/Game/CameraHandler.h:148` single currCamCtrlNum and controllers array (157)
- `rts/Game/CameraHandler.cpp:258` UpdateController drives only the active camera
- `rts/Game/Game.cpp:1376` the only per-frame controller update call site
- `rts/Game/UnsyncedGameCommands.cpp:771` camera movement keys set movState on the active camera

*Direction:* Make B a separate CCamera instance, not an entry in the static cameras[] array, with no controller. Each frame set its pose from one of these: a fixed pose captured when B opens; A's pose plus a fixed offset; or a console/Lua-set pose. Swallow all input events for B's window so nothing moves A. This is effort S. Per-target controllers driven by pointer input go in phase 2. The multi-camera design must still be compatible with submitting several draw frames at once, as for VR. Per-view camera state, such as the pose, projection and frustum, should therefore be self-contained and not read from the global `camera`. Treat that as the design decision. The choice of motion source is a small detail.

> verifier A (confirmed): Cited code matches the current tree. CameraHandler.cpp:94 has one static cameras[CAMTYPE_COUNT], and cameras[ACTIVE] only records which type is active (comment on line 93, SetActiveCamera on line 100). CameraHandler.h has a single currCamCtrlNum (the getter is at line 113). UpdateController(CPlayer*, bool) is declared in the header and defined at CameraHandler.cpp:258. Its only call site is Game.cpp:1376, which drives the global camera. The movement key executors in UnsyncedGameCommands.cpp:771-

> verifier B (corrected): The code facts are right: there is one static camera per type (CameraHandler.cpp:94) and one currCamCtrlNum (CameraHandler.h:148). UpdateController (CameraHandler.cpp:258) drives only the active camera. The decision is real, but it is not a blocker. A 2-window 3D-only prototype can render B from a fixed or derived pose with no controller at all. Without a decision B does not crash or corrupt output. It just has no defined motion, which is a scoping choice rather than a failure. The worst plausib

### DSN-06 - Render target ownership, lifetime and creation trigger are undefined
**Severity:** risk - **Effort:** S - **Status:** corrected - **Depends on:** none

Code cannot answer who creates a second window, when, and who owns it. Without this the prototype has no entry point.

*Why it breaks:* The claim stands: nothing creates, owns or tracks a second window, so a creation trigger, owner and destruction point have to be chosen before any rendering work. The corrections below affect the design.

1. The per-target render loop cannot live in SpringApp::Update, as the direction says. SpringApp::Update (SpringApp.cpp:881-908) calls `activeController->Draw()` once (902) and `globalRendering->SwapBuffers` once (906). The 3D pass is inside CGame::Draw (Game.cpp:1448-1572), which runs UpdateUnsynced, DrawGenesis and GenerateIBLTextures once per frame. It then does `FBO::Unbind(); camera->LoadViewport()` and `worldDrawer.Draw()` (1519-1527), followed by the screen/GUI pass (1530-1548). So the owner must be consulted from CGame::Draw (or CWorldDrawer) for the per-target 3D pass. SpringApp::Update only needs the list for presentation.

2. CGlobalRendering::SwapBuffers cannot simply be called per target. It bundles once-per-frame work with the per-window present: `RenderBuffer::SwapRenderBuffers()` and `IStreamBufferConcept::PutBufferLocks()` (GlobalRendering.cpp:688-689), `FrameMark` (713), `lastSwapBuffersEnd` (717), and a hard-coded `SDL_GL_SwapWindow(sdlWindow)` (703). The prototype needs a split into frame-end work and a per-window present. Calling it twice would double-swap the render buffers.

3. The lifetime question includes SDL window-event routing, which the claim missed. SpringApp::MainEventHandler (SpringApp.cpp:1095-1252) never looks at `event.window.windowID`:
 - SDL_WINDOWEVENT_CLOSE calls RequestQuit() (1249-1251), so closing the second window would quit the whole game, or be vetoed by Lua AllowQuit.
 - MINIMIZED and HIDDEN set `globalRendering->active = false` (1179-1185). CGame::Draw then sleeps and returns early (Game.cpp:1471-1479), so hiding or minimizing window 2 stops drawing window 1.
 - SIZE_CHANGED and MOVED re-run UpdateGLGeometry and SaveWindowPosAndSize, which only know the primary (1111, 1135-1147).
 - FOCUS_LOST releases all keys and the mouse grab (1204-1241), so focusing window 2 does this to window 1. That is an input item for later, not a prototype blocker.
 The target owner must therefore own window-ID to target routing and decide what closing a secondary window means.

4. Creation timing is constrained by loading. During multithreaded load the single GL context is handed back and forth between the main thread and the game-load thread. CLoadLockMtx::lock/unlock call `MakeCurrentContext(false/true)` on the one sdlWindow/glContext (LoadLock.h:14-34, GlobalRendering.cpp:612-614). CLoadScreen::Kill makes it permanently current on main only after the load thread joins (LoadScreen.cpp:157-177). SDL window creation must be on the main thread, and QueuedFunction only covers title, grab and icon (GlobalRendering.cpp:1206-1213, 1322-1329, WindowManagerHelper.cpp:84; drained in UpdateWindow 1288-1292, called from SpringApp::Update:887). So the target should be created after loading completes, on the main thread, and destroyed before the game is deleted (SpringApp.cpp:787 in Reload, 1025 in Kill). Per-game recreation on Reload then follows naturally.

5. activeController is not always CGame: it can be CPreGame, the menu or CLoadScreen (SpringApp.cpp:735-753, 870-873). Their Draw calls render only to the primary window. The design must say what the secondary window shows then (hidden, cleared or last frame).

6. The GflagsExt.h:29 evidence is true but not an obstacle. DEFINE_int32_EX references an undeclared `external_name` and is unused, but SpringApp.cpp:128-157 shows plain DEFINE_bool, DEFINE_string and DEFINE_bool_EX all work. Adding a flag is a one-liner. The ConfigHandler.cpp:289 throw only affects undeclared keys. A declared `CONFIG()` is served by DefaultConfigSource (ConfigHandler.cpp:118). The per-window config problem is therefore not a blocker for the prototype.

7. GlobalRendering.cpp:1204 is a comment line inside SetWindowTitle. The real marshalling precedent is lines 1206-1213 and the UpdateWindow drain.

*Evidence:*
- `rts/System/SpringApp.cpp:758` Reload keeps the primary window across games
- `rts/Rendering/GlobalRendering.cpp:1204` main-thread-only SDL calls already marshalled via QueuedFunction
- `rts/Game/UnsyncedGameCommands.cpp:1629` precedent: engine commands that mutate global rendering state at runtime (/debuggl)
- `rts/System/Config/ConfigHandler.cpp:289` throws for keys not declared with CONFIG
- `rts/Rendering/GlobalRendering.cpp:73` Fullscreen/WindowBorderless/XResolution/WindowPos keys, all primary
- `rts/Rendering/GlobalRendering.cpp:358` NotifyOnChange keys re-apply the primary window
- `rts/System/SpringApp.cpp:248` --window/--fullscreen only reach the primary via SetFullScreen
- `rts/System/GflagsExt.h:29` DEFINE_int32_EX uses undeclared external_name
- `rts/Lua/LuaUnsyncedCtrl.cpp:5415` SetWindowGeometry has no target parameter

*Direction:* Prototype scope (effort S):
1. Add one declared CONFIG key (e.g. SecondaryWindow) plus a DEFINE_bool_EX flag. Do not touch the primary's Fullscreen/XResolution/WindowPos keys.
2. Create the secondary SDL_Window on the main thread from SpringApp::Update once game != nullptr && game->IsDoneLoading(). Do not create it in CGame::Load or from Lua.
3. Use the same GL attributes and pixel format as the primary. CreateSDLWindow's AA/depth fallback loop at GlobalRendering.cpp:432-449 picks the effective values. Bind the existing glContext to it with SDL_GL_MakeCurrent(win2, glContext) rather than creating a second context. A second context would need SDL_GL_SHARE_WITH_CURRENT_CONTEXT and would not share VAOs, FBOs or default-framebuffer state, which the engine uses heavily. Alternative: render to an offscreen FBO and blit it to window 2.
4. Keep the window list in CGlobalRendering as a small struct (SDL_Window*, windowID, size). Destroy it in DestroyWindowAndContext, which already runs after game deletion (SpringApp.cpp:1025 then :1057). It can persist across Reload because it is a bare drawable.
5. Drive the extra 3D pass from CGame::Draw, in a per-target loop around Game.cpp:1519-1526. UpdateUnsynced and GenerateIBLTextures need per-frame state, so SpringApp::Update is the wrong place for rendering.
6. Use SpringApp::Update only for presenting. Split CGlobalRendering::SwapBuffers (:671-718) into per-frame work (SwapRenderBuffers, PutBufferLocks, FrameMark, lastSwapBuffersEnd), which must run once per frame, and per-window SDL_GL_SwapWindow plus make-current. Also set the swap interval to 0 on the secondary window so two vsync'd swaps don't halve the frame rate.
7. Gate SpringApp::MainEventHandler on event.window.windowID so window 2 events do not quit the game, flip globalRendering->active or release input.

Defer Lua and chat triggers, per-window config and Reload semantics.

> verifier A (corrected): The core claim holds. The tree has no second-window or render-target scaffold. SDL_CreateWindow is called once (GlobalRendering.cpp:442) and SDL_GL_CreateContext only for the single sdlWindow (474/490/515). CGlobalRendering holds exactly one `sdlWindow` and one `glContext` (GlobalRendering.h:384-385). No window-ID or RenderTarget code exists anywhere in rts/. Creation sits in SpringApp::Init -> InitWindow (SpringApp.cpp:263,427-433). Teardown is only in SpringApp::Kill -> CGlobalRendering::KillS

> verifier B (corrected): The gap is real but overstated as a blocker. Nothing in the code creates a second window (CreateWindowAndContext at GlobalRendering.cpp:518 makes the one sdlWindow/glContext, called once from SpringApp::InitWindow at SpringApp.cpp:433). But the decision is cheap and any answer works for the prototype. The Reload-lifetime, Lua/chat trigger and per-window-config parts of the claim are 'full' scope, not prototype scope. Several cited items do not bite for the prototype. ConfigHandler.cpp:289 only t

### DSN-07 - Swap pacing and vsync telemetry are single-stream; a second vsynced swap can silently halve the frame rate
**Severity:** risk - **Effort:** S - **Status:** corrected - **Depends on:** Context topology (one context with N drawables vs one per window) must be decided before validation can be specified

lastSwapBuffersEnd, the TIMING_SWAP event, FPS and the VSync interval all describe one swap. A second swap per frame overwrites them, and CVerticalSync would not apply the interval to the second window.

*Why it breaks:* SwapBuffers stores globalRendering->lastSwapBuffersEnd and emits DbgTimingInfo(TIMING_SWAP) per call (GlobalRendering.cpp:716-717). CameraHandler uses lastSwapBuffersEnd for camera transition timing (CameraHandler.cpp:348 and 405-412), so a second swap perturbs it. DbgTimingInfo is a public unsynced Lua callin (Events.def:250, EventClient.h:48-52) and has no target parameter. SDL_GL_SetSwapInterval applies to the current drawable, and CVerticalSync returns early when the interval is unchanged (VerticalSync.cpp:66), so a second window never gets the configured interval and runs on the driver default. Two vsynced swaps per game frame can hold the main thread for two vblanks, roughly halving the frame rate, which looks like a sim or render regression with no per-swap timing to prove it.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:716` TIMING_SWAP event and lastSwapBuffersEnd written once per swap
- `rts/Game/CameraHandler.cpp:348` camera transitions read lastSwapBuffersEnd
- `rts/Rendering/VerticalSync.cpp:66` early-out prevents re-applying the interval per drawable
- `rts/System/Events.def:250` DbgTimingInfo is a public Lua-visible callin

*Direction:* For the prototype, make the per-target swap path set the swap interval explicitly. Use the configured VSync on the primary and 0 on secondaries, applied once after MakeCurrent and tracked per window rather than via the single CVerticalSync::interval. Gate the TIMING_SWAP event and lastSwapBuffersEnd writes to the primary target so the callin signature and camera timing stay unchanged. Add per-target Tracy plots later. Per-target blocked-time telemetry is full or polish. If the design uses one context with N drawables, the swap interval is per-context state in some drivers, so the policy of vsyncing only the primary swap and leaving secondaries unvsynced may need to be revisited. Alternatively, present all windows in one batch at the end of the frame, with only the last swap vsynced.

> verifier A (confirmed): I checked the cited code and it matches. SwapBuffers calls eventHandler.DbgTimingInfo(TIMING_SWAP, ...) and then writes globalRendering->lastSwapBuffersEnd once per call (GlobalRendering.cpp:716-717). lastSwapBuffersEnd is a single spring_time member (GlobalRendering.h:142). CameraHandler.cpp:348, 406 and 412 read it for camera transition timing, so a second swap in the same frame would shift it. CVerticalSync::SetInterval returns early when the clamped interval equals the cached one (VerticalSy

> verifier B (corrected): The mechanism is real. SwapBuffers writes lastSwapBuffersEnd and emits TIMING_SWAP once per call (GlobalRendering.cpp:703-717). VerticalSync::SetInterval has an early-out on an unchanged interval and calls SDL_GL_SetSwapInterval once (VerticalSync.cpp:66, 76-87). That call applies only to the current context/drawable, so a second window would run on the driver default. If that default is vsync-on, two blocking swaps per frame can halve the frame rate. It is not a blocker. Nothing crashes and not

### DSN-08 - Decal visibility and ghost dimming are per-viewer state baked into the shared instance buffer
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** none

Decal alpha (LOS, ghost, spectator rules) is evaluated against gu->myAllyTeam and written into the single decal instance VBO that every target draws.

*Why it breaks:* Decal visMult has three sources of per-target state baked into the one shared instVBO, not one.

(a) Viewer identity: gu->myAllyTeam, gu->spectatingFullView and gu->myTeam are read at GroundDecalHandler.cpp:1561-1566, 1575, 1585 and 1599. Track decals are also filtered at creation time with the same global viewer (AddTrack, :1259: `if (!gu->spectatingFullView && !unit->IsInLosForAllyTeam(gu->myAllyTeam)) return;`). A second viewer with a different allyteam therefore cannot get the decal at all, which is deeper than a visMult toggle. This part is not prototype-blocking.

(b) Per-camera unit state: the iconOnly input (`unit->GetDrawFlag() == DrawFlags::SO_DRICON_FLAG`, :1564) comes from CUnitDrawerData::Update. That computes iconZoomDist from camHandler->GetCurrentController() (UnitDrawerData.cpp:224-230) and iconSizeBase from globalRendering->viewSize* (:217). UpdateUnitIconState then uses the global `camera` for DrawAsIconByDistance and camera->InView (UnitDrawerData.cpp:350-352). Icon-state-by-screen-size uses camera->CalcViewPortCoordinates (:388-401). The flag is written once per Update. Window 2's camera therefore inherits window 1's icon decision, and a building that is an icon in a zoomed-out window 1 has its plate decal hidden in a zoomed-in window 2 as well.

(c) Per-camera feature fade: `std::max(0.0f, feature->drawAlpha)` (:1588) is set in FeatureDrawerData.cpp:145-176 from the CAMTYPE_PLAYER camera distance (featureDrawDistance and featureFadeDistance). Feature decals fade according to window 1's camera only.

Parts (b) and (c) are fed by the single-camera unit and feature draw-flag computation, which is probably a separate problem. If that is not fixed, they give a visible glitch (missing or faded decals) when the windows differ in zoom, which is the normal case for independent cameras. Even once the draw flags are made per camera, the decal instance buffer still bakes visMult once, so per-target visibility needs a per-target visMult stream (a second instance attribute or SSBO) or the visibility test moved into the vertex shader.

Draw() is also called once per target. UpdateDecalsVisibility is O(decalOwners) per call, and the second call mostly finds no changes, so only the upload dedup is affected. The cost of the second pass is small.

*Evidence:*
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:1561` visibility from gu->myAllyTeam LOS status
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:1566` gu->spectatingFullView branch
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:777` instance VBO uploaded once and shared

*Direction:* Prototype: keep one viewer for both windows (the same gu->my*). Treat the iconOnly and feature-fade inputs as subordinate to the per-camera unit/feature draw-flag problem. Until that is solved, either ignore the iconOnly hide for decals when more than one target is active, or drive visMult only from sim-side LOS, buildProgress and ghost state and leave icon and distance hiding to the shader. Full product: make visMult per-target. Options are a per-target instance attribute or SSBO stream, passing a viewer allyteam bitmask (LOS bits per allyteam) to the shader and computing visibility on the GPU, or per-viewer-class instance buffers. Move the AddTrack LOS gate (:1259) out of creation, for example by always creating the decal and gating at visibility time.

> verifier A (corrected): The cited mechanism is real in the current tree. CGroundDecalHandler::Draw (GroundDecalHandler.cpp:761) calls UpdateDecalsVisibility, which mutates decals[pos].visMult from gu->myAllyTeam, gu->spectatingFullView and gu->myTeam (:1561-1566, :1575, :1585, :1599). It marks the shared decalsUpdateList (:1593, :1601). Draw uploads once into the single instVBO (:777-789) and draws all decals with one glDrawArraysInstanced (:830). It is not a blocker for the 2-window prototype, because both windows use

### DSN-09 - Drawing the world N times has no cost model or sharing plan
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** Visibility flags and transform uploads come from a single camera

Draw is variable-rate and runs after sim on one thread with a 10k-unit target. Each extra target repeats the camera-dependent work, so the prototype risks roughly halving frame rate unless sharing is decided up front.

*Why it breaks:* Same as claimed. One correction: the cubemap is positioned from whichever camera was active at the time (prvCam, via GetSetActiveCamera at CubeMapHandler.cpp:204), so with multiple targets it depends on which camera is active when GenerateIBLTextures runs. The shared shadow map and cubemap will be visually wrong for the secondary view, which is a fidelity issue and not a crash.

*Evidence:*
- `coding-agents/ENGINE_PERFORMANCE.md:11` sim and draw serial on one thread; ~10k units target
- `rts/Game/Game.cpp:1519` GenerateIBLTextures (shadows, cubemap, sky) once per frame
- `rts/Rendering/UnitDrawer.cpp:1941` per-bin per-object walk with ShouldDrawOpaqueUnit and AddToSubmission
- `rts/Rendering/WorldDrawer.cpp:437` water->UpdateWater pre-pass inside the alpha pass
- `rts/Rendering/ShadowHandler.cpp:530` shadow matrix fitted to playerCam
- `rts/Rendering/Env/CubeMapHandler.cpp:218` env cube drawn from the player camera position
- `rts/Rendering/DepthBufferCopy.cpp:136` depth copy texture sized to current viewSize

*Direction:* Treat as a design/perf note. Measure first with Tracy. Prototype shortcuts: reuse the primary shadow map and cubemap, and require equal window sizes. Per-target shadow, water and depth resources are a later step. Downgrade from risk to full: the prototype only gets slower, and wrong shadows or reflections on the second window are tolerable.

> verifier B (corrected): The facts hold. ENGINE_PERFORMANCE.md says sim and draw run serially on one thread. Game.cpp:1519 calls GenerateIBLTextures once per frame. ShadowHandler::SetShadowMatrix takes playerCam. But this is a performance and design concern, not a prototype failure. Drawing the world twice will cost roughly 2x draw time, and nothing crashes, raises GL errors or shows empty output because of it. The prototype should take the naive double draw first and measure it. The shadow-map and cubemap staleness for

> verifier A (corrected): The mechanism is real and the cited code checks out, but this is a performance and design-decision item, not a prototype blocker or likely-corruption risk. Verified: ENGINE_PERFORMANCE.md:11-13 (sim and draw serial on one thread, ~10k units); Game.cpp:1519 (GenerateIBLTextures once per frame); WorldDrawer.cpp:436-437 (UpdateWater inside the world draw); ShadowHandler.cpp:493-533 (light matrix fitted to the player camera); CubeMapHandler.cpp:204-218 (env cubemap positioned from the previously act

### DSN-10 - Frame pacing and cost: draw cost scales with views, no per-target cadence, shared interpolation time
**Severity:** full - **Effort:** S - **Status:** unverified - **Depends on:** none

All timing state (timeOffset, FPS, lastFrameTime, avgFrameTime, avgDrawFrameTime) is computed once per frame in UpdateUnsynced and kept as singletons. The sim/draw time balance consumes avgDrawFrameTime. There is no frame limiter.

*Why it breaks:* Keeping timeOffset per frame is right (both views show the same sim instant) and FPS stays 'frames per second'. N views multiply draw cost, which the net-message time budget absorbs through avgDrawFrameTime. That average is measured around Draw excluding the swap, so moving a vsync-blocking present inside the timed region would inflate it and starve sim catch-up. A secondary at a lower rate or on a slower monitor has no cadence mechanism. Views cannot be skipped independently because timeOffset and the Update phase are shared.

*Evidence:*
- `rts/Game/Game.cpp:1564` avgDrawFrameTime from pre/post draw (swap excluded)
- `rts/Net/NetCommands.cpp:252` sim time budget uses avgDrawFrameTime (245-257)
- `rts/System/GlobalConfig.cpp:71` MinSimDrawBalance default 0.15
- `rts/Game/Game.cpp:1283` timeOffset computed once per frame; FPS at :1362
- `rts/Rendering/GlobalRendering.h:130` timing state are singleton members of CGlobalRendering (130-151)

*Direction:* Keep timing per frame and keep the present out of the timed draw region. Decide on a per-target policy (every frame, every Nth frame, or a max rate) and a limiter for the non-vsynced case. Optionally record per-view CPU draw time for diagnostics.

### DSN-11 - Granularity of per-view data in persistent ring buffers must be decided
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** none

Ring slots advance per frame, and Map() only waits on a slot's fence once; per-view rewrites of the same slot inside one frame would race with in-flight reads.

*Why it breaks:* Confirmed as a latent constraint, not a present defect. Because WaitBuffer clears the fence on first use, a slot is safe to write only once per SwapBuffer cycle. Today the model transform and uniform uploads are view-independent and written before any view draws, so N views can read the same slot. The condition is that the fence is queued after the last view's draws. SwapBuffer queues it in lockList, and the fence is created from lockList later (StreamBuffer.cpp:25-34). If an N-view frame swaps before all views are submitted, the fence would be placed too early.

*Evidence:*
- `rts/Rendering/GL/StreamBuffer.h:447` PersistentMapImpl::Map waits on fences[allocIdx] and returns ptr into that slot
- `rts/Rendering/GL/StreamBuffer.cpp:58` WaitBuffer deletes and zeroes the sync after the first wait
- `rts/Rendering/ModelsDataUploader.cpp:109` BindBufferRange then SwapBuffer once per UpdateCommon (frame cadence); numBuffers=3 at line 188

*Direction:* Record it as a design rule: per-view data goes in BufferSubData-style buffers, or in a (frames x views) ring with one fence per slice. Make sure the uploader Update and SwapBuffer run once per frame, with the fence created after all views have been submitted. Not needed for the prototype.

> verifier B (corrected): The mechanism is accurate, but the prototype does not fail without this decision. I checked each claim against the code.

Confirmed:
- PersistentMapImpl::Map waits on fences[allocIdx] (StreamBuffer.h:451).
- WaitBuffer deletes the sync and zeroes it (StreamBuffer.cpp:72-73). A second Map() on the same slot in one frame therefore does not wait.
- SwapBuffer (StreamBuffer.h:475-478) only queues the lock and advances allocIdx.

Why the prototype does not need it:
- Only two buffers use persistent m

> verifier B (corrected): The mechanism holds for persistent-map buffers. Map() waits on fences[allocIdx] (StreamBuffer.h:451), and SwapBuffer queues the fence and advances the slot (StreamBuffer.h:475-477). ModelsDataUploader calls BindBufferRange and then SwapBuffer once per update (ModelsDataUploader.cpp:109-110). But the problem is only hypothetical for the prototype. The transform and model-uniform data is view-independent, and it is uploaded once per frame before any view draws. Both windows read the same slot, and

> verifier A (corrected): The cited mechanism is accurate. PersistentMapImpl::Map (StreamBuffer.h:447-456) calls WaitBuffer(fences[allocIdx]). WaitBuffer (StreamBuffer.cpp:58-77) deletes the sync and sets it to {}, so a later glIsSync check returns early and a second Map on the same slot never waits. SwapBuffer (StreamBuffer.h:475-478) queues the fence for the current slot and advances. ModelsDataUploader.cpp:109-110 calls BindBufferRange then SwapBuffer once per update, and numBuffers is Impl::PERSISTENT_MAPPING_BUFFERI

### DSN-12 - Lua API compatibility rules and test coverage for multi-view
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** none

Games treat the Lua draw and camera APIs as stable. Multi-view must be additive: existing functions and callins keep their primary-view or current-drawing-view meaning, and per-view information arrives through new functions.

*Why it breaks:* Design rule: Lua callins and Spring.Get*Geometry keep primary-view meaning (or current-drawing-view meaning inside draw callins). Widget handlers such as cont/LuaUI/widgets.lua:1279-1292 call w:DrawGenesis()/w:DrawWorld() with no arguments, so view identity cannot be passed as a callin argument. It must come from new query functions. This matters once Lua world callins (DrawWorld*, DrawGenesis) are run for additional views, which the 3D-only prototype does not do.

*Evidence:*
- `coding-agents/BACKWARDS_COMPATIBILITY.md:12` weigh breakage; prefer contained, mechanical changes
- `cont/LuaUI/widgets.lua:1287` widgetHandler dispatch does not forward engine args (also 1260, 1279, 1295)
- `rts/Lua/LuaHandle.cpp:2573` documented callin contracts (ViewResize, Draw*) that games depend on
- `rts/Lua/LuaUnsyncedRead.cpp:892` GetViewGeometry returns primary view fields
- `rts/Lua/LuaUnsyncedRead.cpp:934` GetWindowGeometry returns primary window fields
- `rts/Lua/LuaUnsyncedRead.cpp:962` GetWindowDisplayMode uses the primary window
- `rts/Rendering/GlobalRendering.cpp:106` CR_REG_METADATA lists every member
- `rts/System/creg/creg_runtime_tests.cpp:97` 'Missing member(s)' check
- `test/CMakeLists.txt:368` testCreg runs spring-headless --test-creg

*Direction:* Keep as a full-product design decision (category dsn). Defer until Lua draw callins are invoked per view. For the prototype, simply do not call Lua callins for the secondary view, and add the test widget when that is enabled.

> verifier B (corrected): The facts check out: BACKWARDS_COMPATIBILITY.md lines 15-16 say to prefer contained blast radius and mechanical adaptation, and GetViewGeometry (LuaUnsyncedRead.cpp:892) and GetWindowGeometry (:934) return primary-window fields. But this is a policy and test-coverage item, not a prototype risk. The 2-window, 3D-only prototype draws no Lua, GUI or overlay callins for the secondary view. Existing Lua behaviour stays unchanged by construction when the secondary view skips Lua, so no code in the pro

> verifier A (corrected): Cited facts check out: BACKWARDS_COMPATIBILITY.md:16 (contained blast radius / mechanical adaptation), widgets.lua:1287 DrawWorld dispatch forwards no args, LuaHandle.cpp has the DRAW_CALLIN and ViewResize callins, LuaUnsyncedRead has GetViewGeometry/GetWindowGeometry/GetWindowDisplayMode, AGENTS.md:434 asks for a test widget on Lua changes. But this is a policy/design-rule item, not a mechanism that breaks the 2-window 3D-only prototype. In the prototype, Lua draw callins are not invoked for th

### DSN-13 - Multi-display and DualScreenMode overlap with N-window output
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** TGT-01

DualScreenMode already splits one window across displays for a minimap, with its own viewport arithmetic. It conflicts conceptually with N independent windows, and display selection and fullscreen modes assume the primary's display.

*Why it breaks:* DualScreenMode is an opt-in mode (CONFIG DualScreenMode, default false, GlobalRendering.cpp:67) that carves ONE window into a game half and a minimap half. SetDualScreenParams (:1423-1427) sets the flags, and UpdateViewPortGeometry (:1433-1535) fills viewPos/viewSize and dualViewPos/dualViewSize. Under dual mode with multiple monitors it reads SDL display rects (:1466-1482) and stores a y-offset (viewWindowOffsetY, dualWindowOffsetY, :1525-1526) used by the mouse (MouseHandler.cpp:255, :342, :535; MouseInput.cpp:105), the camera edge-scroll (Camera.cpp:740-746), AGUI (SpringApp.cpp:460) and the Lua API (LuaUnsyncedRead.cpp:911-918, :1078-1093). It conflicts conceptually with N independent windows because it assumes one window, one display index (GetCurrentDisplayIndex, :1389-1392, which defaults to sdlWindow) and one set of singleton viewport globals. The minimap code also restores the single global viewport after a dual draw (MiniMap.cpp:1402-1403, :1815-1816). None of this runs when DualScreenMode is off, which is the default and the prototype configuration. The real prototype-adjacent hazards sit elsewhere. Fullscreen defaults to true (GlobalRendering.cpp:73), a single global fullScreen flag drives SDL_WINDOW_FULLSCREEN exclusive mode (:429, :1248), and size and position come from singleton config keys (GetCfgWinRes :1374-1387, SetWindowAttributes :1218-1257). The prototype must force windowed (or borderless) mode for its windows; this belongs under the singleton window-state problem rather than DualScreenMode. The process-wide SDL hint only matters if MinimizeOnFocusLoss=1.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:1433` UpdateViewPortGeometry computes viewPos/viewSize and dualView* from the window
- `rts/Rendering/GlobalRendering.cpp:1389` GetCurrentDisplayIndex only for sdlWindow
- `rts/Rendering/GlobalRendering.cpp:573` MinimizeOnFocusLoss hint is a process-wide SDL hint
- `rts/Game/Camera.cpp:740` edge scroll picks the dual view by mouse x
- `rts/Rendering/GlobalRendering.cpp:1423` SetDualScreenParams; UpdateViewPortGeometry splits the window (1433-1535)
- `rts/Rendering/GlobalRendering.h:351` dualScreenMode/MiniMapOnLeft docs: viewSizeX is half the GL viewport
- `rts/Game/UI/MiniMap.cpp:324` LoadDualViewport + scissor on dualView*
- `rts/Game/UI/MouseHandler.cpp:255` y minus viewWindowOffsetY (also 342, 535)
- `rts/Game/Camera/OverviewController.cpp:69` dual-specific camera logic (69, 92)
- `rts/System/SpringApp.cpp:1113` window-move handling re-runs GL geometry only for dualScreenMode
- `rts/Lua/LuaUnsyncedRead.cpp:906` GetDualViewGeometry (906-916, 1083-1086)
- `rts/Rendering/GlobalRendering.cpp:2018` LoadViewport() / LoadDualViewport() glViewport from those globals
- `rts/Game/UI/MiniMap.cpp:1402` restores via globalRendering->LoadViewport() after dual draw (also 1815-1816)
- `rts/Game/Game.cpp:1595` minimap drawn when hideInterface and dualScreenMode

*Direction:* Downgrade to full (S for the prototype guard, M for the full decision). For the prototype, force DualScreenMode off or assert and log an error when multi-window output is enabled, and run the extra windows windowed or borderless; do not touch UpdateViewPortGeometry yet. Separately decide whether N-window output supersedes DualScreenMode, for example by making the dual half a minimap target with its own window and viewport and then retiring viewPos/dualViewPos and the y-offset hacks. Provide per-window display index and window geometry by replacing the single-sdlWindow defaults in GetCurrentDisplayIndex, GetMaxWinRes and GetDisplayBounds. Document exclusive fullscreen across multiple windows as unsupported at first. Drop the refresh-rate-pacing rationale unless a per-window pacing design item is added elsewhere. The swap interval and vsync per window (VerticalSync.cpp:76-87 against the one SDL_GL_SwapWindow at GlobalRendering.cpp:703) is a separate pacing problem and should be tracked there.

> verifier B (corrected): The facts are mostly accurate, but the severity is too high. For the 2-window, 3D-only prototype this is a policy decision that the default config already settles, so it is not a "risk". (1) DualScreenMode defaults to false (GlobalRendering.cpp:67, constructor :339). With it off, UpdateViewPortGeometry returns early with viewPos=0 and viewSize=winSize (:1435-1443), so nothing in the 3D path is affected. A prototype that ignores or forces off DualScreenMode loses nothing. (2) The 22 non-GlobalRen

> verifier A (corrected): The code facts check out, but the severity is wrong and two supporting claims are weak. Verified true: UpdateViewPortGeometry at GlobalRendering.cpp:1433-1535 builds the dual layout from per-display SDL rects. There are exactly 22 dualScreenMode uses outside GlobalRendering (MiniMap 16, OverviewController 2, Camera.cpp:740, Game.cpp:1595, LuaUnsyncedRead.cpp:1083, SpringApp.cpp:1113). GetMaxWinRes, GetCurrentDisplayIndex and GetDisplayBounds default to sdlWindow (:1368-1398). LoadViewport and Lo

### DSN-14 - No automated or GPU-backed test coverage for rendering
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** Headless stubs alias every window and context and lack the SDL calls a multi-window layer needs; DSN-15

The 22 Catch2 tests cover System and Sim only, CI never creates a GL context, and tests link explicit source subsets. Target logic needs GL-free seams to be testable at all.

*Why it breaks:* Tests are Catch2 executables linked from explicit source subsets (test/CMakeLists.txt:86-95). Nothing under test/ links rendering code. CI exercises the engine only through spring-headless (testCreg at line 368, synctest at synctest.yml:186) with stubbed GL, so no GL context or multi-window behaviour is covered. The multi-target layer needs GL- and SDL-free seams to be unit-testable. The claim's "22 tests" and "only rendering-adjacent check" are loose, but the conclusion stands.

*Evidence:*
- `test/CMakeLists.txt:86` add_spring_test macro; no Rendering-linked test exists
- `test/CMakeLists.txt:179` Float3/Matrix44f tests link explicit source subsets
- `test/AGENTS.md:52` how to add a test
- `.github/workflows/synctest.yml:185` spring-headless synctest, no GPU
- `test/validation/run.sh:49` headless validation run
- `test/CMakeLists.txt:368` testCreg runs spring-headless, the only engine-level test
- `.github/workflows/synctest.yml:186` only engine run in CI, headless with stubbed GL
- `rts/lib/headlessStubs/gladstub.cpp:548` every GL call becomes a no-op lambda returning R{}

*Direction:* Keep the proposal. Put pure logic (target registry, viewport and aspect math, windowID-to-target routing, config clamping) in GL-free units and add them as Catch2 tests in test/CMakeLists.txt. Treat GPU-backed CI as optional follow-up work.

> verifier A (corrected): The core claim holds. add_spring_test (test/CMakeLists.txt:86-95) builds each test from hand-listed sources with per-test flags. Many tests use -DNOT_USING_CREG or -DBUILDING_AI (lines 176-257). Nothing under test/ references GlobalRendering or Rendering/ except test/headercheck/CMakeLists.txt, which only checks headers. testCreg (line 368) runs spring-headless --test-creg. CI runs spring-headless in synctest.yml:186, and I found no Xvfb or GPU step in the workflows. Not verified: the "22 tests"

### DSN-15 - No current-render-target concept to assert against; existing asserts vanish in default builds
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** Context topology (one context with N drawables vs one per window) must be decided before validation can be specified

Nothing can state which target, context, framebuffer, viewport and camera a draw call belongs to. The few existing guards are assert()s that the default RELWITHDEBINFO/-DNDEBUG build removes. A RenderTargetScope guard with runtime-switchable validators is the core verification tool.

*Why it breaks:* The implicit globals are globalRendering (window, context, view and win sizes; 296 reads in 47 files), the `camera` macro (Camera.h:316, ~505 uses in 85 files), Game::gameDrawMode (Game.h:152) and CCamera::viewport, copied from globalRendering during Update (Camera.cpp:141-159). Context switching is MakeCurrentContext(), hardwired to the single pair. The only guard asserts are FBO-id checks (FBO.cpp:383, 427...) and EnableColorOutput's bound-FBO assert (ShadowHandler.cpp:667). AGENTS.md states the default build is RELWITHDEBINFO with -DNDEBUG, which compiles out all assert(). So no check exists that GL's current context, draw framebuffer, viewport, matrix stack or debug-group depth match the target being drawn, and none that survives in the builds people actually run.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:612` MakeCurrentContext: no target parameter, nothing records the current target
- `rts/Game/Camera.cpp:153` camera viewport and matrices derived from the global viewSize/viewPos
- `rts/Rendering/GL/FBO.cpp:383` existing guard style: assert on bound FBO id, removed under NDEBUG
- `rts/Game/Camera.h:316` #define camera (CCamera::GetActive()) is the only 'current view' notion

*Direction:* Split the finding in two. (1) A minimal RenderTargetScope that makes the context current, sets a thread-local active target, swaps the globalRendering view/win size source and camera, and opens a debug group. The prototype needs this anyway, so treat it as a basic part of the core multi-target design (effort S/M) and fold it into the finding that fixes globalRendering and camera globals. (2) Runtime-switchable validation (L1 context check, L2 glGet checks) as an opt-in debug aid. Make it a config or console toggle rather than NDEBUG-dependent. Add it after the prototype renders, as polish.

> verifier B (corrected): The facts hold: the camera macro is at Camera.h:316 and MakeCurrentContext (GlobalRendering.cpp:612) takes no target. The absence of a current-target concept is real. But it is a verification and tooling aid, not a requirement. The 2-window prototype does not crash or corrupt because a validator is missing. The actual failures (stale globalRendering sizes, FBO::Unbind, LoadViewport, per-camera caches) are separate problems that the other findings cover. A target-scope concept is nearly free, sin

> verifier A (confirmed): I checked these against the current tree. FBO.cpp has assert(GetCurrentBoundFBO() == fboId) at lines 383, 427, 440 and others. Camera.h:316 defines `camera` as CCamera::GetActive(). Camera.cpp:141-159 derives aspect, matrices and viewport from the globalRendering view sizes. MakeCurrentContext(bool clear) at GlobalRendering.cpp:612 takes no target parameter. NDEBUG is set for RELWITHDEBINFO, RELEASE and PROFILE at CMakeLists.txt:524-526, so assert() is compiled out in the default build. I did no

### DSN-16 - No GUI-free per-target draw statistics or identity markers to diagnose an empty or wrong window
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** DSN-15

The prototype windows have no GUI, console or profiler overlay, and the engine exposes almost no draw statistics outside ProfileDrawer. A blank or partly drawn second window would be hard to diagnose.

*Why it breaks:* Only RenderBuffer element and submit counters exist, shown by the GUI-only ProfileDrawer (ProfileDrawer.cpp:128-131). There are no per-target or per-pass drawn-object counters. Diagnosing a blank second window therefore needs external tools (RenderDoc, Tracy zones, GL debug output) until counters are added. This is developer tooling, not a prototype blocker.

*Evidence:*
- `rts/Game/UI/ProfileDrawer.cpp:129` render-buffer stats visible only through the GUI overlay
- `rts/Rendering/GL/RenderBuffers.h:70` GetSubmitNum is the only per-frame submit counter
- `rts/Sim/Misc/GlobalSynced.cpp:72` precedent: TracyPlot for non-GUI telemetry
- `rts/Rendering/WorldDrawer.cpp:308` clear colour comes from the sky fog colour, identical for both targets

*Direction:* Treat it as a nice-to-have debug aid. Add per-target RenderBuffer submit counters exported as TracyPlots, plus an optional per-target clear-colour tint. Schedule it after the prototype works.

> verifier B (corrected): The observation is accurate: draw statistics exist mainly in the ProfileDrawer GUI overlay, and TracyPlot is used only sparingly. It is a diagnostics and tooling gap, though, not a defect in the prototype. Blank or wrong output is already visible in the window itself. The developer can diagnose it with Tracy, a debugger, RenderDoc, or temporary log lines. Nothing crashes, raises GL errors, or corrupts state because counters are missing. The proposed direction is also over-scoped for a prototype:

> verifier A (corrected): The factual base holds. ProfileDrawer.cpp:128-131 prints RenderBuffer SumElems/SumIndcs/NumSubmits in a GUI font overlay. GetSubmitNum at RenderBuffers.h:70 is the only submit counter. TracyPlot appears in GlobalSynced.cpp:72-78 only for speed factors. ShadowHandler.cpp:143 DrawFrustumDebug exists and is called from MiniMap.cpp:2009. A grep of rts/Rendering for drawn-count or draw-call counters found nothing. This is a diagnosability and tooling gap, though, not something that makes the prototyp

### DSN-17 - No ownership model for shared, per-target and per-camera render resources
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** none

Every window-sized or camera-dependent resource is a process-wide singleton reached through globals. There is no place where per-target resources can be created, sized, found and destroyed.

*Why it breaks:* Same core claim, with three corrections. (1) The cited IWater.h:47 is the accessor GetWater(); the static storage is `static std::unique_ptr<IWater> water` at IWater.h:55. (2) ModelDrawer::geomBuffer (ModelDrawer.h:54) is a non-owning alias of LuaObjectDrawer's buffer (ModelDrawer.cpp:25), so only two G-buffers exist: the map one in CSMFGroundDrawer and the LuaObjectDrawer one. (3) The "destroy with the owning context current" and shared/per-target taxonomy depends on a GL context model the claim never states. The engine has exactly one glContext, and MakeCurrentContext is hard-wired to the single sdlWindow (GlobalRendering.cpp:612-614; context creation at 474-515, no SDL_GL_SHARE_WITH_CURRENT_CONTEXT anywhere in rts/). If windows share one context by re-making it current on each window, the FBO-teardown issue and the container-object issue below mostly disappear. If each window gets its own shared-list context, textures and buffers can be shared, but container objects cannot: FBOs (DepthBufferCopy's depthFBOs, GeometryBuffer::buffer, shadow FBO, BumpWater refractFBO) and VAOs (rts/Rendering/GL/VAO.cpp:21 does glGenVertexArrays) are per-context. So a "shared" map-sized texture still needs a per-context FBO or VAO wrapper. The taxonomy needs a fourth axis, per-context container objects, and the design must first decide between one context with N windows and N shared contexts.

*Evidence:*
- `rts/Rendering/DepthBufferCopy.h:50` global depthBufferCopy singleton
- `rts/Rendering/LuaObjectDrawer.h:95` static geomBuffer, accessor at 70; SMFGroundDrawer.h:72-73 for the map one
- `rts/Rendering/Env/IWater.h:47` static IWater::water accessed through GetWater()
- `rts/Rendering/ShadowHandler.cpp:37` global shadowHandler instance
- `rts/Rendering/GL/FBO.cpp:265` FBO::Kill issues GL calls, so teardown needs the owning context current (265-296)

*Direction:* Prototype: keep the singletons and add a RenderTarget switch step. Before each target's pass, set the globalRendering view geometry and the active camera, then fire the existing ViewResize path only if the size differs from the last rendered target (or require equal sizes at first). Re-run shadowHandler.CreateShadows and the water update per target, since both depend on the active camera. Use one shared GL context and SDL_GL_MakeCurrent per window.

Later: introduce RenderTargetResources, taking these viability points into account.
- DepthBufferCopy uses ScopedDepthBufferCopy refcounting by GroundDecalHandler (GroundDecalHandler.cpp:107,1731) and ProjectileDrawer (ProjectileDrawer.cpp:440). That consumer model has to be keyed per target.
- The G-buffer pointer is cached in three places (ModelDrawer.cpp:25, LuaObjectDrawer's static, the CSMFGroundDrawer member), and Lua textures reference it by name. Forwarding via RenderTarget::Current() must be swapped before any Lua draw call.
- The water renderers bake window size into the object and the shader source. CBumpWater initialises screenTextureX/Y from viewSize (BumpWater.cpp:181-182) and bakes ScreenInverse and ViewPos as GLSL compile-time constants (BumpWater.cpp:425-427). Moving the screen copy into a bundle therefore needs those turned into uniforms, otherwise each size change means a shader recompile.
- Classify each resource as shared, per-target or per-camera next to its declaration, as proposed.

> verifier A (corrected): The core claim holds in the current tree. Window-sized and camera-dependent resources are process-global singletons, sized from globalRendering->viewSize*, with no per-target owner. Checked in code: depthBufferCopy is `extern std::unique_ptr<DepthBufferCopy>` (DepthBufferCopy.h:50). It is created in WorldDrawer.cpp:131 and sized from globalRendering->viewSizeX/Y at DepthBufferCopy.cpp:138 and 140. It is always consumed, because ProjectileDrawer.cpp:440 holds a `ScopedDepthBufferCopy(false)`. It 

> verifier B (corrected): The facts are accurate. depthBufferCopy is a global unique_ptr (DepthBufferCopy.h:50), LuaObjectDrawer::geomBuffer is a static leaked pointer (LuaObjectDrawer.cpp:42,255), the map G-buffer is a CSMFGroundDrawer member (SMFGroundDrawer.h:104), IWater::water is static (IWater.h:55), shadowHandler is a global (ShadowHandler.cpp:37), and FBO::Kill issues GL calls (FBO.cpp:265-296). But the 2-window, 3D-only prototype does not need an ownership model.

1. Every resource listed is transient within a s

### DSN-18 - No regression guard against new single-target assumptions during the long refactor
**Severity:** full - **Effort:** S - **Status:** corrected - **Depends on:** none

The refactor will take many PRs while other contributors keep adding FBO::Unbind(), glViewport, LoadViewport and globalRendering view-size reads. Nothing detects when the number of single-target assumptions grows.

*Why it breaks:* Same as claimed, with corrected counts: 19 globalRendering->LoadViewport sites in 15 files, 1 camera->LoadViewport in Game.cpp:1524, and about 36 to 40 glViewport hits outside rts/lib.

*Evidence:*
- `rts/Game/Game.cpp:1522` canonical reset pair: FBO::Unbind() + camera->LoadViewport()
- `rts/Rendering/Env/BumpWater.cpp:572` representative globalRendering->LoadViewport() reset (4 sites in this file)
- `rts/Rendering/Map/InfoTexture/Modern/Combiner.cpp:144` representative reset in the info-texture passes (6 files)
- `rts/Rendering/GL/GeometryBuffer.cpp:216` glViewport(0,0,viewSizeX,viewSizeY) from global size

*Direction:* Add a no-GL CTest or CMake-script lint that counts FBO::Unbind(), glViewport, LoadViewport and UpdateLoadViewport per file against a checked-in allowlist. Fail when a count grows or a new file appears, and ratchet the allowlist down as sites migrate. Keep it out of the prototype's critical path; it can land alongside or after the first migration PR.

> verifier B (corrected): The counts are real (FBO::Unbind() is 23 occurrences in 14 files, which I confirmed by grep). But this is a process and tooling concern, not a mechanism in the code. The 2-window 3D-only prototype does not fail without it. A prototype is built on a branch by a small team, so it does not need a ratchet. At most it is a safeguard for a long-lived multi-PR refactor, which comes after the prototype. 'risk' is overstated because the prototype cannot be corrupted by a missing lint. It belongs in 'full

> verifier A (corrected): The counts check out against the current tree. FBO::Unbind() has 23 occurrences in 14 files, including FBO.cpp's own 3 definitions. globalRendering->LoadViewport( has 19 occurrences, but they sit in 15 files, and none are in GlobalRendering.cpp. glViewport has about 36 to 40 hits outside rts/lib, including the Rml renderer and the Lua files, which is close to the claimed 39. The canonical reset pair is real: Game.cpp has a single camera->LoadViewport() at line 1524, two lines from the cited 1522

### DSN-19 - Per-view cost multiplication of water and shadow passes
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** none

Each view re-renders the scene for water render-to-texture (and shadows if per-view), so N windows can cost about N times the env budget.

*Why it breaks:* For the 2-window prototype, water RTT correctly runs once per view because UpdateWater is invoked inside each CWorldDrawer::DrawAlphaObjects (WorldDrawer.cpp:437). The cost is O(views) by construction, and the reflection pass cannot be shared because it is derived from the active camera (AdvWater.cpp:339-343, BumpWater.cpp:1026-1030). Shadows and the env cubemap are generated once per CGame::Draw (Game.cpp:1519) and already shared. Their correctness (shadow fit to one player camera under CAM_CENTER, cubemap centred on one camera) is the real issue and belongs in separate entries. The only camera-independent waste is AdvWater's 128x128 bump FBO update (AdvWater.cpp:263-326, which depends only on gs->frameNum), which is redone per view.

*Evidence:*
- `rts/Rendering/Env/IWater.cpp:139` each reflection/refraction pass redraws most of the world
- `rts/Rendering/Env/BumpWater.cpp:45` defaults: reflection=1, refraction=1 (screencopy), depth copy on (48)
- `rts/Rendering/ShadowHandler.cpp:436` shadow pass draws grass, projectiles, units, features, terrain

*Direction:* Reframe as a per-view environment quality and budget policy, classified as non-blocking (full, effort S to M).
- Keep per-view UpdateWater for correctness.
- Add a per-view quality knob: scale the reflection texture size (reflTexSize at BumpWater.cpp:1033, the hardcoded 512 at AdvWater.cpp:345) and use BumpWaterReflection=1 skip-terrain for secondary views. Alternatively allow a per-view water renderer, with BasicWater for secondary views that need no RTT.
- Hoist camera-independent work out of the per-view path: the AdvWater bump FBO (AdvWater.cpp:263-326) and BumpWater::Update, which is already per sim frame, so run them once per frame.
- Do not offer 'primary only' reflection as a skip option for AdvWater or BumpWater, because the secondary view would show a wrong reflection.
- Keep shadows and the cubemap generated once per frame (shared). Make the shadow fit camera-independent for the prototype (ShadowProjectionMode=MAP_CENTER, or a fit to the union of view frusta). Track that, and the cubemap centring, as separate correctness problems.
- Add per-view timer and tracy names, because the current SCOPED_TIMER labels such as 'Draw::Water::DrawReflections' and 'Draw::World' are global and would silently aggregate views.

> verifier A (corrected): The mechanism holds, but the severity is overstated. Water render-to-texture does run once per CWorldDrawer::Draw call, so each view repeats it. The code is as cited: IWater::DrawReflections/DrawRefractions (IWater.cpp:139-181, 183-219) redraw sky, ground, units, features, projectiles, alpha, nano particles and Lua events. The default renderer is AdvWater (IWater.cpp:28 sets WATER_RENDERER_REFLECTIVE=1). Its UpdateWater clears and draws a 128x128 bump FBO and a 512x512 reflection FBO (AdvWater.c

> verifier B (corrected): The evidence is accurate: IWater.cpp:139-181 redraws sky, ground, units, features, projectiles and alpha. AdvWater is the default (IWater.cpp:27-28 uses WATER_RENDERER_REFLECTIVE), with a 512x512 reflection at AdvWater.cpp:345 and a 128x128 bump pass at AdvWater.cpp:265. BumpWater screencopies are at BumpWater.cpp:903-913. But the claimed 'risk' does not hold for a 2-window, 3D-only prototype, for three reasons.

1. The per-view water render is required for correctness, not just a cost. CWorldDr

### DSN-20 - Primary-view-only consumers read the implicit camera: sound listener, FPS control, AI callback, sim camera write
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** none

Several non-rendering consumers read or write the active camera implicitly. Once the active camera can be a view's camera, each needs an explicit 'primary view' policy.

*Why it breaks:* Every listed consumer reads or writes the implicit global `camera` (CCamera::GetActive(), which returns cameras[cameras[ACTIVE].camType]) from a non-draw context, where the active type is PLAYER only because every temporary SetActiveCamera restores it. Today the PLAYER slot is also the only camera that UpdateController and camera->Update() maintain (Game.cpp:1376, :1430). With N views, exactly one view must own that slot and the primary-view policy must be explicit. Otherwise sound listener, FPS direct control, SetRotY and AI callbacks silently follow whichever view last wrote the slot, or whichever camera was left active after Draw.

Corrections to the claimed evidence:
1. UniformConstants.cpp:266-276 does not use the implicit camera. It already reads CCameraHandler::GetCamera(CAMTYPE_PLAYER) explicitly (:167, :230). It is already explicitly primary-view-bound, and the real concern is that the per-frame UBO (cameraView/Proj at :169-179) is single-view.
2. UnitDrawer.cpp:190 reads no camera. It reads gu->GetMyPlayer()->fpsController.GetControllee(). This is a per-view draw-policy decision: hiding the controllee is only right for the view that is in first-person mode.
3. SpringApp.cpp:1167 and :1187 are window-event routing, not a camera read. MainEventHandler never filters on event.window.windowID (grep for windowID/SDL_GetWindowID in rts outside lib finds nothing). Any window's HIDDEN/MINIMIZED event therefore sets globalRendering->active=false (:1185) and calls sound->Iconified(true) for the whole engine. CGame::Draw then sleeps and returns early (Game.cpp:1471-1478), so both windows stop rendering. This is a separate problem and a plausible prototype risk. It belongs in a window-event-routing or per-window-state entry, not this one.
4. Game.cpp:1390 is gated by `newSimFrame || forceUpdate` (<=30Hz, Game.cpp:1384-1391), not once per draw.

*Evidence:*
- `rts/Game/Game.cpp:1390` sound->UpdateListener(camera...) in the 30Hz block (1384-1391)
- `rts/Game/FPSUnitController.cpp:126` GetHAndPFromVector(camera->GetDir()) sent via SendDirectControlUpdate (:133); move state :115
- `rts/Sim/MoveTypes/GroundMoveType.cpp:3331` sim code writes camera->SetRotY(...)
- `rts/ExternalAI/AICallback.cpp:231` PosInCamera -> camera->InView(pos, radius); far plane at :1351
- `rts/Rendering/Units/UnitDrawer.cpp:190` FPS controllee skipped in all non-reflection passes
- `rts/Game/Game.cpp:1376` camHandler->UpdateController (single camera)
- `rts/Game/Game.cpp:1427` unitTracker.SetCam
- `rts/Game/Game.cpp:1438` mouse->UpdateCursorCameraDir
- `rts/Rendering/UniformConstants.cpp:266` mouse ray traced from player camera
- `rts/System/SpringApp.cpp:1167` sound->Iconified on window events

*Direction:* Keep this as a dsn item at full severity. Define one explicit primary view (the one that owns CAMTYPE_PLAYER, or a new CCameraHandler::GetPrimaryCamera()). Bind sound listener, FPS state/SetRotY, AICallback (PosInCamera, AIVAL_GUI_VIEW_RANGE), unit tracker, selection-in-view and mouse ray to it. Guarantee the draw loop leaves the active camera as primary after each view. Move the window-event audio and active-flag issue (SpringApp.cpp:1160-1190) into a separate per-window event routing problem and rate it risk for the prototype. Decide per-view FPS-controllee hiding in UnitDrawer.cpp:190.

> verifier A (corrected): The code facts check out and nothing in the tree already handles the case. There is no GetPrimaryCamera or primary-view concept anywhere in rts/. The `camera` macro is CCamera::GetActive() (Camera.h:316, Camera.cpp:96), which resolves to cameras[CAMTYPE_ACTIVE]'s stored type (CameraHandler.cpp:100,126). Each consumer reads the camera as stated: Game.cpp:1390 (UpdateListener), FPSUnitController.cpp:115 and :126, GroundMoveType.cpp:3331 (a write), AICallback.cpp:231 and :1351. The mechanism needs 

### DSN-21 - RenderDoc/NSight capture boundaries are single-swap and the engine has no in-app capture integration
**Severity:** full - **Effort:** M - **Status:** unverified - **Depends on:** Context topology (one context with N drawables vs one per window) must be decided before validation can be specified

External tools delimit a frame by SwapBuffers. With two presents per game frame a capture contains one window's work plus a slice of the other's. The engine only detects RenderDoc by flag and has no in-app trigger.

*Why it breaks:* CheckGLExtensions detects RenderDoc through GL_EXT_debug_tool and sets a process-wide underExternalDebug flag (GlobalRendering.cpp:767-779, 782), which changes extension checks (:819) but nothing about frame framing. The engine includes no RenderDoc in-app API (0 hits for renderdoc_app). SwapBuffers is both the present and the frame delimiter, and it binds GL_READ_FRAMEBUFFER 0 for third-party capture (:691-692). With two swaps per game frame, a capture covers one target and part of the other, and in-flight resources from the other window appear in the wrong frame. Debug groups (see the debug-group problem) would also be split across the capture boundary.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:767` RenderDoc detection by GL_EXT_debug_tool flag only
- `rts/Rendering/GlobalRendering.cpp:691` swap binds READ_FRAMEBUFFER 0 for external capture
- `rts/Rendering/GlobalRendering.h:371` underExternalDebug is a single process-wide bool

*Direction:* Add an optional RenderDoc in-app API integration (StartFrameCapture and EndFrameCapture around 'update plus all targets plus all presents') behind a console command and config. Provide a debug mode that sequences targets deterministically, and one that renders only a chosen target. Document the RenderDoc multi-window workflow.

### DSN-22 - Shadow, cubemap and water textures are singleton resources with hardwired accessors
**Severity:** full - **Effort:** M - **Status:** corrected - **Depends on:** none

Shadow map, shadow matrices/params, cubemaps and water textures are exposed through singleton getters used by about 100 call sites; per-view variants need a 'current view' concept.

*Why it breaks:* CShadowHandler is a process-wide singleton (ShadowHandler.h:157) holding one depth texture, one color texture and one viewMatrix[2]/projMatrix[2] pair (ShadowHandler.h:138-142). It is read through about 26 getter refs in 9 files, plus sampler setup, plus the Lua `$shadow` texture and shadow-parameter accessors. The shadow state is produced in two places, both keyed to a single camera. CShadowHandler::Update() (ShadowHandler.cpp:128-135, from Game.cpp:1431) builds the matrices from CAMTYPE_PLAYER. CreateShadows() (ShadowHandler.cpp:627-663, from WorldDrawer.cpp:257) then renders with that precomputed shadow camera. In the default CAM_CENTER mode (ShadowHandler.cpp:34, :700) the projection depends on the player frustum. The GL4 path reads shadowView/proj from the UniformConstants UBO, filled once per draw-frame (UniformConstants.cpp:181-183, Game.cpp:1441). If views are rendered one after another, the singleton works as an implicit "current view" provided that for each view you re-run Update(viewCam), CreateShadows() and the UBO update before drawing. Introducing a ShadowView struct is required only if several views' shadow maps must exist at once, or if the contexts do not share textures. The water part of the claim is unsupported: water textures are private to the water class. The cubemap part is weak: only the env-reflection cubemap is camera-position dependent (CubeMapHandler.cpp:218), and it is time-sliced.

*Evidence:*
- `rts/Rendering/ShadowHandler.h:83` single GetShadowTextureID/GetColorTextureID; extern singleton at 157
- `rts/Map/SMF/SMFRenderState.cpp:296` per-Enable shadowMat uniform from the global handler
- `rts/Rendering/Common/ModelDrawerState.cpp:208` per-Enable shadowMatrix uniform
- `rts/Rendering/Env/GrassDrawer.cpp:370` shadowMatrix/shadowParams uniforms (339-340 at init); samplers 663-664, 736-737
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:827` shadowMatrix uniform; sampler 508-511
- `rts/Rendering/Env/BumpWater.cpp:936` shadowMatrix + shadow samplers bound per Draw

*Direction:* Do not start with a ShadowView/EnvView refactor for the prototype. First make shadow generation per-view with the singleton unchanged: for each view, set the active player camera, then call shadowHandler.Update(), CreateShadows(), the UniformConstants matrix/UBO refresh and the world draw. Alternatively, use SHADOWPROMODE_MAP_CENTER so one camera-independent shadow map serves both windows. Split this claim into a separate problem covering Update() being bound to CAMTYPE_PLAYER (ShadowHandler.cpp:130) and UniformConstants being filled once per drawFrame from the player camera (UniformConstants.cpp:167-183, Game.cpp:1441). Keep a ShadowView struct plus a Current() accessor as a later optimisation, to cache per-view maps or avoid regeneration. Drop water from the title. Mention cubemaps only as "env reflection is shared, acceptable".

> verifier A (corrected): The core fact holds: CShadowHandler is one extern instance (ShadowHandler.h:157) with one depth texture, one color texture and one pair of matrices (:83-84, :138-142). The cited call sites are real and the line numbers match: SMFRenderState.cpp:296, ModelDrawerState.cpp:208, GrassDrawer.cpp:339-340/370-371/663-664/736-737, GroundDecalHandler.cpp:508-511/827, BumpWater.cpp:936-940, DynWater.cpp:328, LuaOpenGLUtils.cpp:557/560. Four things are overstated or wrong. (1) The numbers: "59 refs in 13 f

### DSN-23 - Tracy frame marks, zone naming and GPU zones assume one target
**Severity:** full - **Effort:** S - **Status:** confirmed - **Depends on:** DSN-15

Tracy's frame boundary is the single FrameMark in SwapBuffers. Zone names are compile-time literals, and the Tracy OpenGL GPU context is a single object per thread. Two swaps per draw frame would double the frame count and make per-target profiles indistinguishable.

*Why it breaks:* FrameMark runs once per SwapBuffers (GlobalRendering.cpp:713), so two swaps give two unnamed frames per game draw frame and distort Tracy's FPS and frame-time statistics. The codebase already uses a named frame set for the sim (FrameMarkStart/End 'SimFrame', Game.cpp:1705 and 1823), which is the pattern to follow. SCOPED_TIMER hashes and registers literal names and states 'names are assumed to be compile-time literals' (TimeProfiler.h:22-23), so 'Draw::World' gives identical zones for both targets. The engine uses no GPU zones today (0 TracyGpu uses). Tracy's TracyGpuContext stores one GpuCtx pointer in per-thread profiler data (TracyOpenGL.hpp:52, TracyProfiler.cpp:1308), so profiling two GL contexts on one thread would need manual switching.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:713` single unnamed FrameMark per swap
- `rts/Game/Game.cpp:1705` precedent: named frame set via FrameMarkStart
- `rts/System/TimeProfiler.h:22` timer names must be literals
- `rts/lib/tracy/public/client/TracyProfiler.cpp:1308` GPU context is a single per-thread wrapper

*Direction:* Keep FrameMark for the primary target only. Add named frame sets ('Draw Target N') with FrameMarkStart and FrameMarkEnd inside the guard. Annotate zones with ZoneText or a color per target. Defer GPU zones, and if added, swap GetGpuCtx on target switch or use one per target.

> verifier A (confirmed): Verified in the current tree. FrameMark is emitted once, unnamed, right after SDL_GL_SwapWindow in CGlobalRendering::SwapBuffers (GlobalRendering.cpp:713), and it is the only FrameMark in rts/. A second swap per draw frame would therefore double the frame count. The sim already uses FrameMarkStart/End with a named set (Game.cpp:1705 and 1823), so the precedent holds. TimeProfiler.h:22 says "names are assumed to be compile-time literals". A grep for TracyGpu/TracyOpenGL in rts/ (excluding the ven

### DSN-24 - Viewer perspective (ally team, spectator, controllee, selection) is global inside drawers
**Severity:** full - **Effort:** L - **Status:** confirmed - **Depends on:** RenderView definition: per-target parameters consumed by model drawers; No per-view identity to key per-frame caches, flags and tokens on

LOS filtering, ghost buildings, icon selection, uniform visibility, FPS-controllee hiding and selection highlight all use gu->myAllyTeam, gu->spectatingFullView and the local player's selection.

*Why it breaks:* UpdateObjectDrawFlags drops objects not in LOS of gu->myAllyTeam (UnitDrawerData.cpp:449). ShouldDrawOpaqueUnit hides the local player's FPS controllee (UnitDrawer.cpp:190). DrawGhostedBuildings uses gu->myAllyTeam (UnitDrawer.cpp:1118-1119, 2051, 2076). Live ghost transform slots are only maintained for the local allyTeam (UnitDrawerData.cpp:748-777). currentIconIndex is a per-unit field for the local team, updated by events filtered on gu->myAllyTeam (324-328, 703-746). Model uniforms are only filled for LOS-visible objects (ModelDrawerData.h:209). isSelected is the local player's selection. All windows are forced to the same viewer. The prototype uses one viewer for both windows, so this does not block it. GetUnitIconIndex(unit, allyTeam, fullView) (UnitDrawerData.cpp:300-322) and MiniMapIconDrawParams show the precedent for viewer-parametric calls.

*Evidence:*
- `rts/Rendering/Units/UnitDrawerData.cpp:449` LOS gate uses gu->myAllyTeam / spectatingFullView
- `rts/Rendering/Units/UnitDrawerData.cpp:757` live ghost transforms only for gu->myAllyTeam
- `rts/Rendering/Units/UnitDrawer.cpp:190` FPS controllee of the local player excluded from drawing
- `rts/Rendering/Units/UnitDrawer.cpp:2051` ghost buildings per gu->myAllyTeam
- `rts/Rendering/Common/ModelDrawerData.h:209` uniform fields LOS-gated on gu->myAllyTeam
- `rts/Rendering/Units/UnitDrawerData.cpp:300` viewer-parametric icon lookup already exists
- `rts/Rendering/Units/UnitDrawerData.cpp:327` currentIconIndex from gu->myAllyTeam/spectatingFullView
- `rts/Map/ReadMap.cpp:818` unsynced heightmap updated from LOS of gu->myAllyTeam (878 UpdateLOS)
- `rts/Rendering/Env/NanoParticles/NanoParticleRenderer.cpp:511` viewChanged rebuild of persistent buffers (505-546)
- `rts/Rendering/Env/Decals/GroundDecalHandler.cpp:1561` decal visibility from gu->myAllyTeam (1561-1599)
- `rts/Sim/Units/Unit.cpp:1437` GetTransformMatrix adds GetErrorVector(gu->myAllyTeam)
- `rts/Game/UnsyncedGameCommands.cpp:1153` SpecTeam sets gu team, UpdateTeams, PlayerChanged, CopySyncedToUnsynced (1140-1169)
- `rts/Rendering/Units/UnitDrawer.cpp:511` existing viewAllyTeam/fullView parameters (511-515)
- `rts/Game/TraceRay.cpp:427` viewer filter via gu->myAllyTeam (438-441, 487)
- `rts/Rendering/UniformConstants.cpp:279` GuiTraceRay for the player's allyteam
- `rts/Game/Game.cpp:1538` HUD bound to gu->GetMyPlayer()

*Direction:* Decision first: same viewer for all targets (prototype) or per-target viewer. If per-target: add ViewerContext {allyTeam, fullView, controllee, selection ref} to RenderView; make ghost slots, currentIconIndex and uniform LOS gating per viewer or computed per view at draw time.

> verifier A (confirmed): Every cited site was opened and matches the claim in the current tree. UnitDrawerData.cpp:449 in UpdateObjectDrawFlags gates draw flags on losStatus[gu->myAllyTeam] / spectatingFullView, and does so inside the per-camType loop, so the viewer is not a parameter. UnitDrawer.cpp:190 excludes gu->GetMyPlayer()->fpsController.GetControllee(). DrawGhostedBuildings (GLSL 1118-1119, GL4 2051/2076/2102) and the callers at 1082/2009 use gu->myAllyTeam / spectatingFullView. UpdateLiveGhostTransforms (UnitD

### DSN-25 - GL debug messages are unattributed and unthrottled
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** GL debug-output callback is installed only on the primary context

Callback output has no target, context, frame or pass identity and no de-duplication. One systematic per-draw error in window 2 would flood infolog.txt and still not say which window caused it.

*Why it breaks:* As claimed, but the flood only occurs when GL debug output is explicitly enabled (glDebug / ToggleGLDebugOutput, GlobalRendering.cpp:1985-2003). With it on, there is no per-target attribution, no de-duplication and no throttling.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:1950` callback: no target id, only two ids suppressed, no rate limit
- `rts/Rendering/GlobalRendering.cpp:1996` static shared GLDebugOptions as userParam
- `rts/System/Log/FramePrefixer.cpp:31` prefix = time + sim frame only
- `rts/Rendering/GL/myGL.cpp:738` verbose error drain logs drawFrame but not target

*Direction:* Downgrade to polish, a dev-tooling item that is useful once window 2 exists. Keep the direction: carry a target id in the callback userParam, de-duplicate by (target, id, message) with counters, and add a current-target tag to the log prefix.

> verifier B (corrected): The mechanism is real. GlobalRendering.cpp:1950-1982 logs every message at L_WARNING with no target id. Only ids 131169 and 131185 are filtered, and there is no rate limit. A static GLDebugOptions is the userParam (:1996), and synchronous output is enabled (:2001). But this is only a diagnostics aid, and it is gated behind the glDebug option and ToggleGLDebugOutput (:1985-1991). In the default configuration the callback is not installed, so there is no flood. The prototype does not fail or corru

> verifier A (corrected): The code facts check out in the current tree. The callback at GlobalRendering.cpp:1950-1982 suppresses only ids 131169 and 131185 and has no rate limit or target identity. It logs at L_WARNING (:1974). userParam is a single static GLDebugOptions (:1996). GL_DEBUG_OUTPUT_SYNCHRONOUS is enabled (:2001). FramePrefixer.cpp:46-49 emits only time and sim frame. myGL.cpp:738 logs drawFrame with no target. The severity is overstated, though. The callback is installed only through ToggleGLDebugOutput, wh

### DSN-26 - Per-target draw toggles and debug flags
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** DSN-03; DSN-15

drawSky, drawWater, drawGround, drawFog, drawMapMarks, drawDebug* are process-wide flags (about 39 reader lines in 15 files) toggled by console and Lua.

*Why it breaks:* They affect both windows equally. It is a design choice whether window 2 should be able to hide water or sky independently. Note drawGround also gates the per-frame GroundDrawer::Update.

*Evidence:*
- `rts/Rendering/GlobalRendering.h:244` flags (244-262)
- `rts/Rendering/WorldDrawer.cpp:217` drawGround gates Update; also 338, 398, 490, 500
- `rts/Game/UnsyncedGameCommands.cpp:1619` console toggle (drawSky)
- `rts/Lua/LuaUnsyncedCtrl.cpp:4511` Lua setters for drawSky/drawWater/drawGround (4511-4533)
- `rts/Rendering/GlobalRendering.h:261` drawDebug, drawDebugCubeMap, glDebug, glDebugErrors as process globals
- `rts/Game/UnsyncedGameCommands.cpp:1594` debug cubemap toggle replaces the global sky object
- `rts/Rendering/WorldDrawer.cpp:338` world draw reads global drawGround

*Direction:* Keep (a) global for the prototype. Later add a per-target override mask, resolved at target activation, applying only to target-specific passes (terrain, sky, units, overlays) and not to the shared shadow/water/IBL generation passes. Use an OR across targets for the Update gate at WorldDrawer.cpp:217. Treat drawFog as sky-derived, with the per-camera far plane as the per-target input. Move glDebug/glDebugErrors into the GL-context-setup problem: register the debug callback on every context at creation.

> verifier A (corrected): Core claim holds and the polish severity is right. These flags are plain process-wide bools in CGlobalRendering (GlobalRendering.h:244-266) that all targets would read identically. Writers include console actions and Lua setters (UnsyncedGameCommands.cpp:1594/1619/2878/3536, LuaUnsyncedCtrl.cpp:4509-4535). WorldDrawer.cpp:217 really gates CGroundDrawer::Update, and 338/398/490/500 are the cited readers. A two-window 3D-only prototype can leave them global, since both windows share one sim and eq

### DSN-27 - Single-window split-viewport stepping stone
**Severity:** polish - **Effort:** S - **Status:** confirmed - **Depends on:** none

DualScreenMode already shows one window with two viewports. Two viewports of the same window would validate per-target camera, UBO, culling and shadow isolation without multi-context, vsync, event-routing or lifecycle risks.

*Why it breaks:* UpdateViewPortGeometry computes separate view and dual viewports, and LoadDualViewport and the minimap draw already use the second region. A world draw into a scissored half would exercise every shared-state problem except platform window plumbing.

*Evidence:*
- `rts/Rendering/GlobalRendering.cpp:1433` view/dual viewport computation
- `rts/Rendering/GlobalRendering.cpp:2023` LoadDualViewport
- `rts/Game/Game.cpp:1595` second region drawn separately

*Direction:* Optional step 0: draw the second camera into half the window (scissor plus viewport) behind the same opt-in key. Then promote that region to a real window once state isolation is proven.

> verifier A (confirmed): The cited code does what is claimed. UpdateViewPortGeometry (GlobalRendering.cpp:1433-1461) splits the window into view and dual regions, halved when there is one display or fullscreen. LoadDualViewport (2023-2026) only calls glViewport on the dual rectangle. Game.cpp:1595-1597 draws only the minimap into that region, so today no world camera uses it. The claim is framed as a proposal, so this is not a defect. Caveat: a split viewport shares one GL context, one default framebuffer and one vsync,

### DSN-28 - UBO block layout is a public, hand-duplicated contract
**Severity:** polish - **Effort:** S - **Status:** corrected - **Depends on:** none

The two blocks' layout and bindings are copied by hand into three engine GLSL files, published in docs, and consumed by Lua GL4 shaders, so adding per-view members or splitting blocks silently breaks shaders.

*Why it breaks:* The two std140 UBO blocks (binding 0 matrices, binding 1 params) exist in three forms. First, the C++ structs UniformMatricesBuffer and UniformParamsBuffer (rts/Rendering/UniformConstants.h:21-93), registered with creg (UniformConstants.cpp:29-108). Second, hand-written GLSL copies in ModelVertProgGL4.glsl:16-84, ShadowGenVertProgGL4.glsl:16-84 and ModelFragProgGL4.glsl:13-43 (the frag program carries only the params block), plus the docs listing at making-shaders.md:75 and :113. Third, a runtime-generated definition for Lua (UniformConstants.cpp:141-142 into LuaShaders.cpp:1359-1369). The hand copies are not generated. They even use the block name UniformMatrixBuffer, while the generated text uses the creg class name UniformMatricesBuffer. Each GLSL shader declares its own block, so a layout or binding change in C++ makes those shaders read wrong offsets with no compile or link error. That is a maintenance guardrail, not a prototype blocker. The current blocks already carry per-view data (camera matrices at UniformConstants.cpp:167-179 and viewGeometry at :222). A multi-window design can keep layout and bindings bit-identical and refill the same buffers per view. A layout change (such as a view index) only matters if the design chooses it. Appending after teamColor[2048] would be tolerated by shaders declaring the shorter block, as long as the buffer is at least as large as the declared block.

*Evidence:*
- `cont/base/springcontent/shaders/GLSL/ModelVertProgGL4.glsl:16` hand-copied block definitions at 16-84; ModelFragProgGL4.glsl:13-42; ShadowGenVertProgGL4.glsl:16-84
- `doc/site/content/docs/guides/rendering/making-shaders.md:75` documented UniformMatrixBuffer (binding 0) and UniformParamsBuffer at 113
- `rts/Lua/LuaShaders.cpp:1354` GetGLSLDefinition exposed to Lua
- `coding-agents/BACKWARDS_COMPATIBILITY.md:3` compat is a constraint weighed against benefit

*Direction:* Keep UniformMatrixBuffer (binding 0) and UniformParamsBuffer (binding 1) bit-identical, and refill them per view at the start of each view's draw. Treat this as a constraint note attached to the per-view uniform refill problem, not as a standalone problem. Optionally add a unit test or a generated include that checks the hand-copied GLSL blocks against UniformConstants::GetGLSLDefinition.

> verifier B (corrected): The facts are accurate. Three hand-copied std140 definitions exist (ModelVertProgGL4.glsl:16-84, ShadowGenVertProgGL4.glsl:16-84, ModelFragProgGL4.glsl:13-43), plus the docs (making-shaders.md:75,113) and the generated Lua definition (LuaShaders.cpp:1354-1368, UniformConstants.h:131-142). A diff shows the three copies are currently identical to each other. The documented block name is UniformMatrixBuffer, while the Lua-generated name is UniformMatricesBuffer. That does not matter, because the bi

> verifier A (corrected): The code facts are true. The block layout is hand-copied into ModelVertProgGL4.glsl:16-84 (both blocks), ShadowGenVertProgGL4.glsl:16-84 (both blocks) and ModelFragProgGL4.glsl:13-43 (params block only, not both). The docs show it at making-shaders.md:75 and :113. UniformConstants::SetGLSLDefinition (UniformConstants.h:136-182) generates GLSL from creg metadata, and LuaShaders.cpp:1359-1369 exposes it. The copies currently match UniformConstants.h:21-93 member for member. But 'risk' is overstate

> verifier A (corrected): The duplication is real. ModelVertProgGL4.glsl:16-84 and ShadowGenVertProgGL4.glsl:16 and 54 carry hand-copied std140 blocks at bindings 0 and 1, and ModelFragProgGL4.glsl:13 copies the params block. The docs show both blocks (making-shaders.md:75, 113), and LuaShaders.cpp:1368 exposes the generated definition to Lua. But this is a constraint on how the fix is designed, not a defect that makes the prototype crash or render wrongly. If the prototype refills the same two blocks at bindings 0 and 1

> verifier B (corrected): The duplication is real. ModelVertProgGL4.glsl:16 hard-codes a std140 copy of UniformMatrixBuffer at binding 0. The same appears in ModelFragProgGL4, ShadowGenVertProgGL4 and ShadowGenFragProgGL4. The engine also exposes generated definitions to Lua through LuaShaders.cpp:1368 and :1386. Nothing in the 2-window prototype requires touching the layout. The claim's own recommendation is to keep both blocks bit-identical and refill them per view, which leaves every shader unchanged. The layout only 

## Appendix: refuted / duplicate

- Transform and uniform uploads gated by single-camera draw flag (duplicate): The gating facts are real. UpdateCommon (ModelDrawerData.h:220-231) calls UpdateObjectDrawFlags and then uploads the transform only if alwaysUpdateMat or 0 < drawFlag < SO_DRICON_FLAG (:227). The feature unsynced matrix is gated the same way (FeatureDrawerData.cpp:207-209). The draw flags come only 
- Worker threads read the global active camera during particle fill and flag updates (refuted): The code facts cited are real, but they do not form a problem. The race exists only in a hypothetical design (per-view threads, or view N+1 update overlapping view N fill) that is not required for the 2-window prototype or a full product.

What holds up:
- ProjectileDrawer.cpp:120 runs `getParticle(
- Update-phase offscreen bakes finish by restoring main-window viewport on whatever drawable is current (refuted): The observation is accurate but the harm is not. The update-phase bakes do end with FBO::Unbind() plus globalRendering->LoadViewport() (ProjectileDrawer.cpp:1358-1359, SMFReadMap.cpp:756, Radar:109, Path:208, Los:94, AirLos:95, Combiner:144, ModernInfoTexture:35, BumpWater:883). But the claimed cond
- Per-view above/below-water state and wasted reflection passes (refuted): The cited lines say what the claim says, but they describe a single-camera quirk that exists today. They do not show a multi-target problem, and the claim's own text concedes "nothing new breaks".

What I checked:
- WorldDrawer.cpp:504 gates the overlay on camera->GetPos().y. The overlay uses only t
- World pass embeds overlay draws that drain per-frame queues (refuted): The central mechanism (a second WorldDrawer::Draw finds drained queues, so overlays are missing or wrong) is false, because every one of these queues is refilled from retained sim/UI state inside the same call that drains it.

- lineDrawer: DrawMiscObjects refills it itself before DrawAll(). WorldDr
