# macOS renderer: decomposition plan

Status: S0 through S4b, S6 and S6b are built and on branches, see each layer. I4 is open as PR #3163. S5, S7 and S8 are plan only, and no code, branches or PRs exist for those unless a PR number is given.

Written 2026-08-01 against `macos/test-integration` at 32c28394a5 (17 commits ahead of `origin/master`, 0 behind).

## 1. The actual delta, measured

Everything in this section was checked on this machine today, not read off the old PRs.

### The engine already builds

`engine-legacy` configures and compiles clean on `macos/test-integration` with Homebrew GCC 16, Ninja and ccache. No source changes. Output is a 33 MB Mach-O arm64 binary at `build-macos-legacy/spring`.

```
cmake -S . -B build-macos-legacy -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_COMPILER=/opt/homebrew/bin/gcc-16 -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/g++-16
cmake --build build-macos-legacy -j 10 --target engine-legacy
```

Zero compile errors, zero link errors. One benign linker warning about duplicate static libraries.

This is the single biggest correction to the old picture. #2991 carried around 25 build fixes. The great majority were AppleClang and libc++ problems that do not exist under Homebrew GCC, and most of the rest have since merged upstream. There is no "make it compile" layer to write.

`otool -L` on the binary shows what it links:

- `/opt/homebrew/opt/sdl2-compat/lib/libSDL2-2.0.0.dylib`
- `/System/Library/Frameworks/OpenGL.framework`

### The engine dies at GL context creation

Running `./spring --window --isolation` gets through streflop init (NEON, FPCR verified), watchdog, video mode enumeration and window creation, then aborts:

```
[GR::CreateGLContext] created main GL2.0 core-context
[GR::CreateGLContext] created main GL2.0 compatibility-context
[GR::CreateGLContext] created main GL2.1 core-context
[GR::CreateGLContext] created main GL2.1 compatibility-context
[GR::CreateGLContext] created main GL3.0 core-context
[GR::CreateGLContext] error ("Failed creating OpenGL context at version requested") creating main GL3.0 compatibility-context
... core succeeds to 4.1, compatibility fails at every version >= 3.0 ...
Fatal: [ExitSpringProcess] errorMsg="[GR::CreateGLContext] error ... creating main GL3.0 compatibility-context"
```

`CGlobalRendering::CreateGLContext` (rts/Rendering/GlobalRendering.cpp:462) sweeps every version in both profiles and keeps `cmpCtx`, the lowest compatibility context at or above `minCtx`. Apple's GL offers compatibility only at 2.0 and 2.1, so `cmpCtx.x` stays 0 and the function calls `handleerror` and returns null.

Two things follow. Apple's OpenGL.framework can never satisfy the engine, so the Mesa route is not an optimisation, it is the only route. And #2991's stated reason for unlinking OpenGL.framework, a bus error inside `+[NSOpenGLContext currentContext]` on macOS 26, did not reproduce here. The failure is a clean, handled context-creation failure. Treat that rationale as unverified.

### The Mesa substrate is present but does not work

`brew install mesa` (26.1.4) ships everything needed, verified in the keg:

- `lib/libEGL.dylib`
- `lib/dri/zink_dri.dylib`
- `lib/libvulkan_kosmickrisp.dylib`
- `share/vulkan/icd.d/kosmickrisp_mesa_icd.aarch64.json`
- `include/EGL`, `include/GL`

Homebrew's formula builds macOS with `-Dgallium-drivers=llvmpipe,zink` and `-Dvulkan-drivers=kosmickrisp,swrast` on Sequoia and newer. So no from-source Mesa build is needed just to get the pieces.

A standalone EGL probe compiled against that keg gives a working baseline:

```
EGL 1.5 vendor='Mesa Project'
[COMPAT] GL_RENDERER = llvmpipe (LLVM 22.1.8, 128 bits)
[COMPAT] GL_VERSION  = 4.6 (Compatibility Profile) Mesa 26.1.4
[CORE]   GL_VERSION  = 4.6 (Core Profile) Mesa 26.1.4
```

GL 4.6 compatibility profile, which is exactly what the engine needs and what Apple refuses to give.

Switching to hardware fails:

```
MESA: error: Zink requires the nullDescriptor feature of KHR/EXT robustness2.
libEGL warning: egl: failed to create dri2 screen
eglInitialize FAILED 0x3001
```

Identical failure against KosmicKrisp and against MoltenVK 1.4.2. Not bypassable by `MESA_KK_EXPERIMENTAL=1`, `MESA_KK_DEBUG=force_robustness`, `MESA_KK_DISABLE_WORKAROUNDS=all`, or either `MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS` mode. Zink on lavapipe fails differently ("failed to choose pdev").

Mesa MR 41313 is `kk: Support nullDescriptor` by squidbus, dated 2026-05-11, reviewed by the KosmicKrisp lead. It implements the feature in KosmicKrisp rather than working around it in Zink. `git tag --contains` puts it in `26.2-branchpoint`, `mesa-26.2.0-rc1`, `rc2` and `rc3`. It is not in 26.1.

`brew install --HEAD mesa` cannot deliver it. Homebrew's formula sets `-Dgallium-rusticl=true` and `-Dtools=etnaviv,...`, both of which trigger Mesa's Rust bindings layer, which needs the `syn` crate through pkg-config. There is no Homebrew formula providing it.

### Building 26.2.0-rc3 from source does work

Verified on this machine. Mesa `mesa-26.2.0-rc3` built with a minimal configuration gives a hardware GL context:

```
GL_VENDOR   = Mesa
GL_RENDERER = zink Vulkan 1.4(Apple M1 Pro (MESA_KOSMICKRISP))
GL_VERSION  = 4.6 (Compatibility Profile) Mesa 26.2.0-rc3 (git-5db7111ffb)
GLSL        = 4.60
```

That is the compatibility profile the engine needs, on the GPU. See S0 for the recipe.

Three caveats, all of which matter downstream.

**The GL version is forced, not native.** Without `MESA_GL_VERSION_OVERRIDE=4.6` and `MESA_GLSL_VERSION_OVERRIDE=460`, Zink on KosmicKrisp reports `GL_VERSION = 2.1` and `eglCreateContext` returns `EGL_BAD_MATCH` for every request at 3.0 or above. The overrides make Mesa claim 4.6 rather than making it true. Anything Zink cannot actually back will now fail at draw time instead of at query time. This is a far wider correctness caveat than the geometry-shader one, because it applies to the whole GL surface rather than one stage.

**Zink warns about the device on startup:**

```
WARNING: Some incorrect rendering might occur because the selected Vulkan device
(Apple M1 Pro) doesn't support base Zink requirements: have_EXT_custom_border_color
```

**The geometry-shader lie is confirmed on hardware.** Under the override, `GL_MAX_GEOMETRY_OUTPUT_VERTICES` returns 1024 on a device with no geometry stage. This was previously inferred from #2991's comments. It is now measured.

### Mesa's macOS EGL is surfaceless only

The probe reports the full EGL extension string. It contains `EGL_KHR_surfaceless_context` and no platform extension at all. No `EGL_EXT_platform_base`, no macOS platform, nothing that would let `eglCreateWindowSurface` target a CAMetalLayer.

This independently confirms #2991's core design decision. With stock Mesa there is no window surface, so the engine must render into a pbuffer and get those pixels onto the screen itself. #3060's direct-present design needs the patched "surfaceless platform that creates window surfaces" hack from the Minecraft gist, which is a Mesa fork, not a Mesa build.

### SDL

Homebrew's `sdl2` is an alias for `sdl2-compat` 2.32.70, which is the SDL2 API on top of SDL3 3.4.12. The real SDL2 formula is gone. Per your decision we build on that.

It exports `SDL_GetWindowWMInfo` and contains the `SDL.window.cocoa.window` SDL3 property string, so the NSView and CAMetalLayer grab that #2991 does through `SDL_SysWMinfo` should work. Not yet exercised end to end.

### Dependency declaration gap

`rts/builds/legacy/CMakeLists.txt` calls `find_package_static(Fontconfig 2.11 REQUIRED)`. Neither the `Brewfile` (PR #3038) nor the macOS CI gate (PR #3134) installs fontconfig. The build here only succeeded because fontconfig is installed on this machine for unrelated reasons. Both lists need it before `engine-legacy` can be a CI target.

### Summary of what is missing

| Gap | Nature |
|---|---|
| GL 3.0+ compatibility context | Runtime. Needs Mesa EGL, not Apple GL |
| Working Zink on Metal | External. Needs Mesa past MR 41313 |
| Getting pbuffer pixels to screen | Code. Readback plus Metal present |
| Viewport and window-size contract | Code. Backing pixels vs logical points |
| Mouse coordinate space | Code. Follows from the above |
| Geometry shaders | Capability lie. Needs a guard plus game content |
| fontconfig not declared | Build metadata |
| Shipping Mesa to users | Packaging |

## 2. The layers

### Bucket 1: independent PRs off `origin/master`

These are not part of the stack. Each stands on its own merits.

Most of what could be extracted from #2991 already has been. Merged: #3071 glad GLX, #3065 SpringApp X11 include, #3045 legacy X11 guard, #3034 SDLMain removal, #3047 CUtils scandir, #3026 smmalloc INLINE, #3025 SafeUtil, #3024 SolLua, #3023 LuaTextures logging, #3074 `<algorithm>` includes, #3076 archive narrowing, #3078 LuaMenu LuaSocket. Open and already yours: #3160, #3161, #3073, #3072, #3041, #3044, #3046, #3042, #3038, #3035, #3036.

That leaves four.

**I1. Core-profile ARB extension check, PR #3022, already open**

Purpose: `CheckGLExtensions` fails on a core-profile context because ARB extensions folded into GL 1.3/2.0/3.0 are no longer advertised by name. Skip the legacy name check when `glContextIsCore`.

Touches: `rts/Rendering/GlobalRendering.cpp`, one early return.

Reviewer check: run any core-profile context on Linux. The check should no longer reject it.

Risk: **this PR has CHANGES_REQUESTED from sprunk.** The objection is that the `else` branch setting `glContextIsCore` is a heuristic when `GL_CONTEXT_PROFILE_MASK` reports 0. The stack depends on this landing or on an agreed alternative. Resolve it before building on top.

**I2. `CTextureAtlas::GetTexID` null guard**

Purpose: `atlasTex` stays null when `Finalize`/`Allocate` fails, so `GetTexID()` and `DisOwnTexture()` dereference null. A failed atlas should degrade to no texture, not crash.

Touches: `rts/Rendering/Textures/TextureAtlas.h`, two accessors.

Reviewer check: purely defensive, readable from the diff. No behaviour change on a healthy atlas.

Risk: low. A reviewer may reasonably ask whether hiding the failure is right, and whether a log line belongs there. Have that answer ready.

**I3. fontconfig in the Brewfile and the CI gate**

Purpose: `engine-legacy` requires Fontconfig 2.11 and neither dependency list declares it.

Touches: `Brewfile` (currently PR #3038), the brew install step in the macOS CI workflow (currently PR #3134).

Reviewer check: a clean machine following the Brewfile can configure `engine-legacy`.

Risk: low. Best folded into #3038 and #3134 as extra commits rather than opened as a fourth PR.

**I4. `--window` cannot turn fullscreen off. DONE, PR #3163**

Purpose: `CGlobalRendering::SetFullScreen` computes the windowed case and then throws it away.

```cpp
fullScreen = (cfgFullScreen && !cliWindowed  );
fullScreen = (cfgFullScreen ||  cliFullScreen);
```

The second line is a plain assignment, so `--window` does nothing whenever the config says fullscreen. Not macOS specific, found here because the display mode change that exclusive fullscreen forces is very visible on a Retina Mac: the desktop drops out of its 2x mode and every other window's text goes tiny.

**The date in the earlier draft was wrong.** `git log -S` puts the two-line form in `d1eec3d177`, April 2017, not `acc116b1a0`. That commit rewrote a working if/else chain into two assignments and dropped the first result. The chain gave `--window` priority over `--fullscreen`, so the fix is `!cliWindowed && (cfgFullScreen || cliFullScreen)` rather than the `(cfg && !cliWindowed) || cliFullScreen` suggested here before. The two differ only when both flags are passed.

Branch `fix/window-cli-flag` off `origin/master`, one commit, `9cae4335c6`. Also cherry-picked to the front of the borderless branch below, since that layer needs it to test anything.

Touches: `rts/Rendering/GlobalRendering.cpp`, one line.

Verified: with `Fullscreen = 1` in the config, `--window` gives a decorated window and the log reads `windowed::decorated`.

Risk: low, but it changes behaviour that has been wrong for nine years, so somebody may be relying on it. `SetFullScreen` writes its result back to the config, so `--window` now persists `Fullscreen = 0`, the same way `--fullscreen` has always persisted `Fullscreen = 1`.

**Deliberately not extracted:**

The HiDPI window-size persistence fix from #2991. That was PR #3021 and you closed it as a no-op on master, correctly. Nothing on master reassigns `winSizeX` to backing pixels, so re-querying SDL in `SaveWindowPosAndSize` returns the same value. It only becomes a real fix once layer S3 makes `winSize` mean pbuffer pixels. It belongs in the stack, in the same commit that creates the divergence.

### Bucket 2: the stack on `macos/test-integration`

Suggested integration branch `macos/renderer-integration`, maintained the same way. Rebased onto upstream, self-emptying as pieces land.

Be blunt with reviewers about this: **layers S1 through S3 produce nothing visible.** S1 gets a GL context and then a black window. S2 adds a file nothing calls. Only at the end of S3 does a pixel reach the screen. Anyone reviewing S1 in isolation is reviewing plumbing.

---

**S0. Substrate: a Mesa build where Zink initialises. DONE, with caveats**

Not engine code. A pinned recipe plus a probe, kept in `coding-agents/` or `docs/`.

Purpose: prove that Zink on KosmicKrisp gives a hardware GL 4.6 compatibility context on Apple Silicon, and pin the exact Mesa revision that does it.

Result: yes, on `mesa-26.2.0-rc3`. Recipe below, verified 2026-08-01.

```bash
git clone --filter=blob:none https://gitlab.freedesktop.org/mesa/mesa.git
cd mesa && git checkout mesa-26.2.0-rc3
python3 -m venv .venv && .venv/bin/pip install mako packaging setuptools

PATH="$PWD/.venv/bin:/opt/homebrew/opt/bison/bin:/opt/homebrew/opt/llvm/bin:$PATH" \
PKG_CONFIG_PATH="/opt/homebrew/opt/libclc/share/pkgconfig:/opt/homebrew/opt/spirv-llvm-translator/lib/pkgconfig" \
meson setup build-zink \
  --buildtype=release --prefix="$HOME/dev/mesa-install" \
  -Dplatforms=macos -Degl=enabled -Degl-native-platform=surfaceless \
  -Dgallium-drivers=zink -Dvulkan-drivers=kosmickrisp \
  -Dmoltenvk-dir=/opt/homebrew/opt/molten-vk \
  -Dgallium-rusticl=false -Dtools= -Dglx=disabled -Dgbm=disabled \
  -Dllvm=enabled -Dvideo-codecs= -Dopengl=true -Dgles1=disabled -Dgles2=disabled

PATH="$PWD/.venv/bin:/opt/homebrew/opt/bison/bin:/opt/homebrew/opt/llvm/bin:$PATH" \
  ninja -C build-zink install
```

Homebrew build prerequisites: `meson ninja bison llvm libclc spirv-tools spirv-llvm-translator molten-vk vulkan-loader vulkan-headers`.

Five things had to be found the hard way, all of which belong in the recipe rather than in anyone's memory.

- Mesa needs the Python `mako`, `packaging` and `setuptools` modules. No Homebrew formula provides `mako`, so use a venv rather than touching the Homebrew Python.
- Homebrew's `llvm` is keg-only, so `llvm-config` must be put on `PATH` explicitly.
- `libclc` and `spirv-llvm-translator` need their pkg-config directories on `PKG_CONFIG_PATH`. KosmicKrisp pulls in CLC, so `-Dllvm=disabled` is not an option.
- Zink on macOS requires `-Dmoltenvk-dir` at configure time even when the target is KosmicKrisp.
- Apple's `/usr/bin/bison` is 2.3 and cannot parse Mesa's grammar files. Homebrew's bison 3.8.2 is keg-only. Meson bakes the bison path into `build.ninja` at configure time, so putting it on `PATH` only for the build step does nothing. It must be set for `meson setup`.

Runtime environment:

```bash
EGL_PLATFORM=surfaceless
MESA_LOADER_DRIVER_OVERRIDE=zink
GALLIUM_DRIVER=zink
LIBGL_DRIVERS_PATH=$PREFIX/lib
VK_DRIVER_FILES=$PREFIX/share/vulkan/icd.d/kosmickrisp_mesa_icd.aarch64.json
MESA_GL_VERSION_OVERRIDE=4.6
MESA_GLSL_VERSION_OVERRIDE=460
```

Note Mesa 26.2 uses a single `libgallium-<version>.dylib` megadriver, so there is no `lib/dri/zink_dri.dylib` any more and `LIBGL_DRIVERS_PATH` points at `lib` itself. Also note the loader resolves `@rpath/libvulkan.1.dylib`, which `DYLD_FALLBACK_LIBRARY_PATH` does not satisfy. The consumer binary needs an rpath entry for wherever `libvulkan.1.dylib` lives. That is a preview of the S7 packaging problem.

What a reviewer can verify: run the probe, get `zink Vulkan 1.4(Apple M1 Pro (MESA_KOSMICKRISP))`.

Remaining risk, and it is now the largest in the plan: **the GL 4.6 context is a forced claim, not a measured capability.** Without the overrides Zink reports GL 2.1. We are telling the engine that 4.6 is available and finding out at draw time where that is untrue. The geometry-shader guard in S5 is the first known instance of this. There will very likely be others, and they will surface as rendering bugs rather than as clean failures.

Secondary risk: 26.2.0-rc3 is a release candidate, so the pin will need moving to 26.2.0 final and then tracking.

---

**S1. EGL context creation**

Purpose: on macOS, create the GL context through Mesa EGL instead of SDL, and stop linking Apple's OpenGL.framework.

Touches:
- `rts/builds/legacy/CMakeLists.txt`, locate Mesa EGL, link `Metal`, `QuartzCore`, `IOSurface`, `objc`, drop `OpenGL::GL` on Apple
- `rts/Rendering/GlobalRendering.cpp`, `__APPLE__`-guarded EGL init, destroy, make-current, plus `gladLoadGLLoader(eglGetProcAddress)` and dropping `SDL_WINDOW_OPENGL` from the window flags

Do not copy #2991's `SPRING_MAC_LIBEGL` cache variable verbatim. Write a proper `FindMesaEGL.cmake` that finds the Homebrew keg by default and takes an override, and do not hardcode anyone's home directory the way #3060 does.

Reviewer check: `engine-legacy` builds, `otool -L` shows Mesa's libEGL and no OpenGL.framework, and the engine logs an EGL 1.5 context at GL 4.6 compatibility and gets past `CreateGLContext` into `PostInit`. The window stays black.

Risk: medium. Two places assume an `SDL_GLContext`. `glContext` is typed as `SDL_GLContext` and #2991 casts the `EGLContext` into it, which works but is a lie the code tells itself. Consider a small `__APPLE__` branch instead. Shutdown must call `eglDestroyContext`, not `SDL_GL_DeleteContext`. ExaDev hit a crash on exactly this.

---

**S2. `MetalPresent.mm`**

Purpose: the entire Objective-C surface of the port, in one file. Attach a CAMetalLayer to the SDL window's NSView, own an IOSurface-backed MTLTexture, and blit it to the drawable with a one-triangle render pass.

Touches: `rts/System/Platform/Mac/MetalPresent.{h,mm}`, new, plus source lists in the legacy build. The ObjC and clang split from #3073 already handles compiling it.

Interface, kept C so the GCC side never sees Objective-C:

```c
bool  MacMetalPresent_Init(void* caMetalLayer);
void* MacMetalPresent_AcquireIOSurfaceBuffer(int w, int h, size_t* outRowBytes);
void  MacMetalPresent_PresentIOSurface(bool flipY);
void  MacMetalPresent_PresentBGRA(int w, int h, const void* pixels, bool flipY);
```

Reviewer check: it compiles under clang while the rest builds under GCC, and a `SPRING_MAC_PRESENT_TEST`-style flash of solid colours puts visible frames on screen. That test hook is worth keeping for this layer and deleting later.

Risk: medium. #2991 reaches the NSView through raw `objc_msgSend` casts from C++ in `GlobalRendering.cpp`, including a hand-rolled `CGRect` struct. Since S2 already exists as a `.mm` file, move that code into it and write it as normal Objective-C. That removes the fragile ABI casting and keeps `GlobalRendering.cpp` free of `objc/runtime.h`.

---

**S3. Present path and the size contract. DONE**

Purpose: wire the present into `SwapBuffers` and fix what the engine means by window size.

Branch `macos/renderer-s3-present`, three commits off S2. The main menu draws, windowed and fullscreen, at full backing resolution.

- `c25bfb210a` the size contract. The Metal layer goes up before the EGL context and reports its drawable size, and the pbuffer is created that size. `ReadWindowPosAndSize` takes `winSize` from the pbuffer. `SaveWindowPosAndSize` writes the resolution config from `SDL_GetWindowSize`, which is the #3021 change and is now a real fix.
- `ef41a47ab3` the present. New `rts/System/Platform/Mac/MacGLPresent.{h,cpp}`, called from `SwapBuffers` in place of `SDL_GL_SwapWindow`. `glReadPixels` writes straight into the IOSurface, so no copy sits between GL and the texture the GPU samples.
- `b487bad86a` reverts the S2 test hook.

**The size numbers, measured.** The earlier accounts disagreed because they were different modes, not because anything was wrong.

| | fullscreen (exclusive) | windowed |
|---|---|---|
| `SDL_GetWindowSize` | 3024x1964 | 1200x800 |
| `SDL_GL_GetDrawableSize` | 3024x1964 | 1200x800 |
| `NSWindow.backingScaleFactor` | 1.0 | 2.0 |
| CAMetalLayer drawable | 3024x1964 | 2400x1600 |

`SDL_WINDOW_FULLSCREEN` asks for a display mode change, so macOS leaves its 2x HiDPI mode for a native 1x one and points equal pixels. Windowed keeps 2x. Note that SDL reports the drawable in points in both cases, because the window is not created with `SDL_WINDOW_ALLOW_HIGHDPI` and SDL does not own a GL context for it. That is why the layer, not SDL, is asked for the size.

**The PBO path was dropped, on measurement.** #2991's 40 to 55 ms per frame does not reproduce. At 2400x1600 the present costs about 1.9 ms of real work:

| segment | cost |
|---|---|
| acquiring the IOSurface (waiting for the last present) | 0.01 ms |
| waiting for the GPU to finish the frame | 6.0 ms |
| the readback copy itself, 15.4 MB | 1.4 ms |
| Metal encode and present | 0.5 ms |

The 6 ms is not the readback's cost, it is the frame being drawn, and nothing on the present side removes it. A double-buffered PBO measured 0.7 ms per frame *worse* than reading straight into the IOSurface: `glReadPixels` into a PBO still blocks on this driver, so the PBO buys no run-ahead and only adds a 15 MB copy out of the mapped buffer. Total 8.05 ms against 7.10 ms. Double-buffering the IOSurface was dropped for the same reason: the acquire it would decouple costs 0.01 ms.

If a driver ever makes `glReadPixels` into a PBO asynchronous, revisit. Until then the PBO is cost without benefit.

Reviewer check: **this is the first layer with a visible result.** The main menu draws. Resizing is S6, the pbuffer does not track the window yet.

Deliberately not in this layer: the downsample and non-Retina knobs, the four `SPRING_MAC_*` environment variables and the inline timing harness from #2991. Also `SPRING_FRAME_CAPTURE` and `SPRING_MAC_DUMP_FRAME`, which belong in S8 as CI instrumentation, and one of which is more useful as an engine-wide feature than a macOS one.

---

**The Zink abort, diagnosed, and no longer reproducing**

Found during S2, chased during S3. **It has not been seen since.** On 2026-08-02 it did not fire in eight menu runs, including five instances at once, nor in a three minute game to frame 4668, nor in the user's own longer session. The claim below that it "will block playing anything" is now contradicted by evidence. Treat it as a rare event of unknown cause rather than a blocker, and stop planning around it until it comes back.

The engine takes SIGABRT on a Zink threaded-context worker somewhere between 20 and 60 seconds in, and sometimes not at all in a minute. The abort is inside Metal, not inside Zink:

```
abort
IOGPU        IOGPUMetalCommandBufferStorageAllocResourceAtIndex
AGXMetalG13X AGX::ContextCommon<...>::newCommand(unsigned long, bool)
AGXMetalG13X ...beginComputePass / computeCommandEncoder
kosmickrisp  mtl_new_compute_command_encoder / cs_get_compute / kk_CmdCopyBuffer2
kosmickrisp  vk_common_CmdCopyBuffer / kk_cmd_tramp_CmdCopyBuffer
libgallium   zink_copy_buffer / zink_resource_copy_region / tc_batch_execute
libgallium   util_queue_thread_func
```

So Metal fails to allocate storage for a command buffer and aborts the process rather than returning an error. At the point of the abort the process holds 990 MB of `IOAccelerator` across 1710 regions.

What is ruled out: the watchdog, the pbuffer size mismatch, and the present path. It happens with the present path wired and unwired, and it happened before any present existed. `zink_copy_buffer` is ordinary buffer traffic, not the readback.

What is not yet known: whether the 1710 regions grow without bound, which would make this a resource leak in Zink or KosmicKrisp rather than a burst. The next step is to sample the `IOAccelerator` region count over time and see whether it climbs. If it climbs, the bug is upstream and the pin has to move. If it plateaus, the engine is asking for more GPU allocations than the driver can hold, and the engine side is worth looking at.

Getting past it is not an engine change on current evidence.

---

**S4. Input coordinate space. DONE**

Purpose: SDL reports mouse events in logical points. After S3 the viewport is in backing pixels. Without scaling the cursor reads half its real height, so you have to aim below a button to hover it and the error grows down the screen.

Branch `macos/renderer-s4-input`, two commits off S3.

- `852884ce73` the scale. `CGlobalRendering::PointToPixel` and `PixelToPoint`, backed by `pixelsPerPointX/Y` computed in `ReadWindowPosAndSize` as the ratio of the pbuffer to `SDL_GetWindowSize`. The ratio, not `backingScaleFactor`: exclusive fullscreen drops the display to 1x where the two sizes agree, so the ratio is right in both modes and the backing scale is not.
- `96713bd736` the conversion, in `InputHandler::PushEvents`, plus the two paths that are not events.

**It is not `MouseInput.cpp`, which is where the old plan and #2991 both put it.** The main menu is aGui, and aGui elements register their own `InputHandler` handler and read `ev.button.x` and `ev.motion.x` directly (`Button.cpp`, `List.cpp`, `Window.cpp`), hit-testing against `GuiElement::screensize`, which `SpringApp` sets from `viewSize`. RmlUi does the same against `winSize`. Converting in `MouseInput` alone fixes the game and leaves the menu broken, which is the stated reviewer check. Converting where events enter the engine covers all three consumers in one place, with no `#ifdef` anywhere in the event handling.

Two mouse reads do not arrive as events and are converted on their own: `SDL_GetMouseState` in the wheel branch of `aGui/List.cpp`, and `WarpPos`, which hands a position back to SDL and so converts the other way.

Verified on screen. Windowed: hover lands on the button under the cursor, clicking Edit Settings opens the settings list, and the wheel scrolls that list with the row under the pointer highlighted. Fullscreen: confirmed by hand.

**Testing this without a human needs care.** `CGWarpMouseCursorPosition` teleports the pointer without delivering a motion event, so SDL keeps reporting the old position until something really moves. A synthetic click after a warp lands wherever the pointer was before. Post an explicit `.mouseMoved` after the warp, or the harness will tell you the coordinate space is broken when it is not.

**Exclusive fullscreen makes the pointer feel slow, and that is not this layer.** The mode change takes the desktop from a 1512x982 point space to a native 3024x1964 one, and macOS moves the pointer in points, so the same hand movement crosses half the screen. Nothing in the engine can fix it. Borderless fullscreen avoids it by not changing the mode, which is S6b.

---

**S4b. Render resolution knob. DONE**

Purpose: let a machine draw at the window's point size instead of its backing size, trading sharpness for a large saving on the S3 present path.

Branch `macos/renderer-s4b-resolution`, one commit off S4. `b2cec98be7`.

`CONFIG(bool, MacHiDPIRendering)`, default on, reaching `MacMetalPresent_Init` as an argument. It sets `layer.contentsScale` rather than `layer.drawableSize`, because AppKit derives the drawable from the scale when it lays a hosted layer out, so a drawable set behind its back would not survive a resize. Nothing else in the size contract changes: the pbuffer follows the layer, `winSize` follows the pbuffer, and `pixelsPerPoint` becomes 1 so S4's conversion goes back to the identity.

Measured with it off, windowed at 1210x800: `1210x800 points at 1.0x, 1210x800 drawable`, `winSize=<1210,800>`, the menu visibly upscaled, and hover still lands on the button under the cursor. The saving itself is arithmetic, not measured: the readback drops from 15.4 MB to 3.8 MB per frame, so roughly 1.4 ms to 0.35 ms at S3's measured rate, and the fragment work drops to a quarter.

The trap to avoid is treating this as a fix for the fullscreen pointer speed. It is not: exclusive fullscreen is already 1x, so the knob only makes it blurrier. What it buys is making borderless fullscreen affordable, since borderless keeps the 2x desktop and would otherwise always pay for the full backing resolution.

This is #2991's non-Retina knob, deliberately dropped from S3 and taken up here on its own merits.

---

**The first real game run, 2026-08-02**

A game runs. SplinterFaction 0.1.78 on AcidicQuarry, single player, reached frame 4668 at 31 to 38 fps. Terrain, detail textures, normal mapping, unit models, the minimap, the build menu, icons and every UI panel all draw correctly. This is the first evidence that the port is more than a menu.

**The data dir matters more than anything else here.** Dropping `--isolation` lets `~/.spring` supply `base/springcontent.sdz`, which on this machine is from January 2025 and is missing the shaders this engine asks for: `SMFShadingTextureVertProg.glsl`, `Icons2DVS.glsl`, `IconsFS.glsl` and `groundDecals.lua`. The ground shading shader then fails to validate and the terrain draws black. That is a stale content problem and not a renderer one, and it wasted the first run. Use `--isolation-dir ~/dev/spring-testdata`, which holds this build's `base/`, the repo's fonts and LuaUI, and symlinks to one game and one map.

**`fillModeNonSolid` is the second capability lie, and it is worse than the geometry-shader one.** Mesa says it plainly at startup:

```
MESA: warning: Incorrect rendering will happen because the Vulkan device
doesn't support the 'fillModeNonSolid' feature
```

Metal has no wireframe polygon fill mode, so `VK_POLYGON_MODE_LINE` does not exist and Zink cannot honour `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)`. Every outline the engine draws that way comes out as a filled triangle instead. On screen that is large black and grey wedges over the terrain, red triangles over the minimap, and filled polygons over the resource bar.

There are 35 `glPolygonMode` call sites outside `rts/lib`. The ones that hurt are `Game/UI/StartPosSelecter.cpp` (the placement phase), `Game/UI/GuiHandler.cpp` (build placement ghosts), `Game/SelectedUnitsHandler.cpp`, `Rendering/CommandDrawer.cpp` and `Rendering/Units/UnitDrawer.cpp`. `Lua/LuaOpenGL.cpp` passes the mode straight through from Lua, so game content can reach it too.

Unlike the geometry-shader case this one **is** detectable, because the driver knows and says so, and a filled triangle where an outline belongs is far more visible than a missing effect. Worth its own layer.

**Done, branch `macos/renderer-polygonmode-line`, three commits off `macos/renderer-borderless`.** The capability has no GL query, so `CGlobalRendering::ProbePolygonModeLine` measures it: rasterise a triangle into a 32x32 FBO in `GL_LINE` mode and count lit pixels, since a driver that filled it lights about four times as many. The result is `supportPolygonModeLine`, logged next to the other capability flags and exposed as `Platform.glSupportPolygonModeLine`. The rectangle-outline sites (`CommandDrawer`, `GuiHandler`'s option LEDs, `StartPosSelecter`'s ready box, `SelectedUnitsHandler`) now draw their four edges as line primitives on every platform rather than asking for a filled rect in line mode, which also fixes `StartPosSelecter` drawing a second filled quad over the first everywhere. The nanoframe wireframe stage is skipped when the capability is absent, because a solid model there reads as a finished unit, and the GL4 path submits its index list as `GL_LINES` instead. Verified on screen.

`VK_EXT_depth_clip_enable` is missing too, with the same "incorrect rendering will happen" warning and no symptom yet identified.

**One shader still fails to validate:** `FX Shader is not valid: active samplers with a different type refer to the same texture image unit`. Cause not yet chased.

**Two input problems, neither investigated:**

- The mouse cursor is not visible in game. Note that `screencapture` never records the cursor, so no screenshot can confirm or deny this. It needs a human or a different capture method.
- Cmd-Tab does not switch away. The overlay appears and focus stays put. `SDL_SetWindowGrab` through `CGlobalRendering::SetWindowInputGrabbing` is the obvious suspect, and the log shows `Input grabbing is enabled!` repeatedly, which comes from the `GrabInput` action executor, so something is issuing it.

**SplinterFaction cannot test S5.** It uses no geometry shaders at all, so a different game is needed to exercise that path.

---

**The Lua drawing artefacts, diagnosed 2026-08-02**

The stray skewed geometry over everything Lua draws is a Zink bug, reproduced standalone outside the engine. **Consecutive `glBegin`/`glEnd` batches that use different vertex formats corrupt the second format's vertices.** No GL error is raised.

`~/dev/macos-probes/fmt_probe.c` is the reproducer. On `zink Vulkan 1.4(Apple M1 Pro (MESA_KOSMICKRISP))`, Mesa 26.2.0-rc3:

| test | result |
|---|---|
| F1 to F8, vertex format varying *inside* one batch | all OK |
| G1, a run of no-texcoord batches then a run of texcoord batches | **WRONG**, 19875 stray pixels |
| G2, format alternating every single batch | OK |
| G3, the load screen's exact batch shape | **WRONG**, 715 stray pixels |
| G4, a shader draw issued after the transition | OK |
| G5, G1 again with every batch leading with a texcoord | OK |
| G6, the same with colour, texcoord and normal | OK |

**It is Zink or KosmicKrisp, not Mesa's shared immediate-mode code, and not a stale pin.** Measured properly by building `origin/staging/26.2` with `-Dgallium-drivers=zink,llvmpipe`, so one binary drives both. llvmpipe passes all fifteen, Zink on KosmicKrisp fails the same three. Same source, same build, same probe. The earlier llvmpipe comparison confounded driver with version and that caveat is now retired.

**Moving the pin does not help.** `staging/26.2`, 30 commits past rc3, fails identically. That includes `05d1d627f79 zink: Fix zink_bo_unmap synchronization for client pointer support`, which looked like a strong candidate because immediate mode is the client-pointer path. It is not this bug.

**ExaDev has not solved it either.** Their shipped release `engine-macos-arm64-2026.06.08-gaefcef0` bundles `libgallium-26.2.0-devel.dylib` with KosmicKrisp. Running the probe against their bundled Mesa fails the same three tests with byte-identical numbers. So the difference between their working build and our artefacts is not the driver.

Three independently built Mesas, from revisions weeks apart, produce identical stray-pixel counts. The defect is fully deterministic and depends only on the sequence of batches the application submits. That rules out the race theory and means the trigger is what the game draws, not what the driver was built from. Beyond All Reason evidently does not produce the triggering sequence and SplinterFaction does.

G2 passing and G1 failing is the shape of the bug. Alternating every batch is fine, a *run* of one format followed by a run of another is not. That matches Mesa accumulating consecutive same-format batches into one buffer and mishandling the boundary.

**Only the batch at the boundary is corrupted.** Measured with `empty_mod.sdz` as the game, so no content but the diagnostic load screen draws anything. Five bands, all the same 28-vertex rounded rectangle:

| band | batch | result |
|---|---|---|
| A | no texcoords, no texture | clean |
| B | texcoords, no texture | **sprawls across the top half** |
| C | texcoords, texture bound | clean |
| D | no texcoords, texture bound | clean |
| E | texcoords, no texture, identical to B | clean |

B and E are the same construct and only B is wrong, so the trigger is not "texcoords with no texture bound".

**Which batch gets corrupted is not predictable from the drawing order.** A second run, with nothing textured and nothing blended so that texcoord presence was the only variable, drew six bands in the order no, no, yes, yes, no, yes. Only band 6 was corrupted. Band 3 is also the first texcoord batch after a run without them and it is clean, which is the exact opposite of the first run, where the first such batch was the corrupted one and the later one was clean. Bands 1 to 5 draw correctly and only look fragmentary because band 6's sprawl covers them.

Two tests, contradictory under any "first boundary batch" rule. What survives is that the corruption tracks something below the API, most likely the offset or alignment a batch happens to land at inside Mesa's immediate-mode buffer, which moves as the batch count and vertex count change. That also fits the standalone results, where G1 with 18 batches per run scattered heavily, G3 with the load screen's shape scattered lightly, and G2 alternating every batch not at all.

**A sacrificial warm-up batch does not help.** One texcoord-carrying batch drawn first with `gl.ColorMask(false, ...)` leaves the picture bit for bit unchanged. There is no cheap "absorb the bad batch" mitigation.

The `empty_mod.sdz` harness is worth keeping. It has no LuaIntro and an empty `LuaUI/`, so a loose `LuaIntro/main.lua` in the data dir is the only thing drawing, and `~/dev/macos-probes/empty_script.txt` starts it. Note `gametype` must be the modinfo `name` ("Empty Mod"), not its `game` field. For bisecting a real game's Lua rather than replacing it, extract the `.sdz` to a folder ending in `.sdd` and the engine picks it up.

This is what SplinterFaction's load screen hits. `LuaIntro/Addons/main.lua` draws its panel through `RectRound`, whose `DrawRectRound` emits 28 vertices as 7 quads in one `GL.QUADS` batch, with `gl.TexCoord` on some vertices, immediately after batches that emit none.

Verified in-engine by overriding `LuaIntro/main.lua`. `CLuaIntro::LoadFile` uses `SPRING_VFS_RAW_FIRST`, so a loose `LuaIntro/main.lua` in the data dir beats the game archive. That is the cheapest way to get a controlled Lua drawing test inside the real engine with no rebuild. Escalating from a plain rounded rect to the game's exact construct, the band that first emits texcoords after a band that did not is the one that sprawls. The bands after it, which keep the same format, draw clean.

**Normalising the vertex format is not the fix. That reading was wrong, and it is now disproven.** G5 and G6 pass, but they pass by accident of their batch shape, not because a uniform format avoids the bug. Run against batches shaped the way `gl.Shape` and `gl.BeginEnd` really produce them, every normalising variant is *worse* than doing nothing: re-emitting the current colour, texcoord and normal before `glBegin`, after `glBegin`, with fixed values, or with a pinned vertex arity all fail 16 of 17 sweep positions against naive's 15, and they start failing with no preceding batches at all. That is the standalone reproduction of the full-screen rainbow smear the previous engine attempt produced.

**Fixed, 2026-08-03. See "The immediate-mode batching bug" below.**

Seen in game as well as on the load screen: on the start position screen the top-left resource panel draws as a skewed parallelogram with fine hatching, and a purple band of the same hatching runs from it down the full height of the map.

**The invisible cursor is NOT explained by this.** G4 shows a shader draw after the transition is unaffected, and `CMouseCursor::Draw` uses a render buffer and a real shader, not immediate mode. Still open.

Dead ends, all tested this session, do not redo:

- **glthread.** Zink turns it on by default (`driinfo_zink.h` has `DRI_CONF_MESA_GLTHREAD_DRIVER(true)` against gallium's `false`), so it looked promising. It is not on here. `mesa_glthread=false` produces no `ATTENTION: default value ... overridden by environment` line, which Mesa prints only when the override changes the value. The artefact is identical either way.
- **Vertex format varying inside a batch**, which is what `gl.Shape` and `gl.BeginEnd` produce. F1 to F7 all pass.
- **`gl_ModelViewProjectionMatrix` going stale after immediate mode**, the obvious cursor theory. F8 passes.
- **The screen matrices.** `UpdateScreenMatrices` logs `vpx=60, vpy=-804, vsx=2760, vsy=1720, ssx=1512, ssy=982`. The desktop size is in points while the viewport is in backing pixels, so the `ssy >= wsy` invariant on line 1781 is violated and `vpy` goes negative. The arithmetic still cancels out and the projection is self-consistent, so it is not the cause of this. It is worth tidying anyway.

A trap that cost four runs: `Intro:DrawLoadScreen` inherits a **0..1 ortho**, not pixel space. `LuaOpenGL::EnableCommon` does not call `SetupScreenMatrices`. The game's own load screen compensates with `gl.PushMatrix()` then `gl.Scale(1/vsx, 1/vsy, 1)`. A test that draws in pixel coordinates lands far off screen and looks exactly like "Lua draws nothing".

---

**The immediate-mode batching bug, root-caused and fixed 2026-08-03**

The Lua drawing artefacts are fixed. Verified by A/B on one binary against the six-band `empty_mod` harness: with the mitigation on all six bands draw as clean rounded rectangles, with it forced off by config the cyan band sprawls skewed across the whole screen. Both images are phase dumps of the same frame of the same run configuration.

**What it is.** Mesa's `vbo_exec` accumulates consecutive `glBegin`/`glEnd` batches into one persistently mapped buffer and issues them together. Zink on KosmicKrisp renders that wrongly once the buffer already holds another batch and a later batch widens its vertex format part way through, which is exactly what `gl.Shape` and `gl.BeginEnd` produce. No GL error is raised.

**The measurement that settles it.** `src/mesa/vbo/vbo_exec_draw.c` was instrumented to print the buffer offset, stride, vertex size, primitive count and attribute layout of every immediate-mode draw. Two runs of the same probe, one on Zink and one on llvmpipe out of the same Mesa binary, produce a **byte-identical** stream of draws. llvmpipe renders it correctly and Zink does not. So Mesa's state tracker output is not at fault and the defect is wholly inside zink or kosmickrisp.

**What fixes it, and what does not.** Only `glFlush()` immediately *before* `glBegin` works, 17 of 17 sweep positions clean. Everything else fails:

| candidate | result |
|---|---|
| `glFlush()` before `glBegin` | **clean, 17 of 17** |
| `glFlush()` after `glEnd` | fails, same as naive |
| a blend enable/disable before `glBegin` | fails, same as naive |
| normalising the vertex format, four variants | worse than naive |
| a matrix push/pop before `glBegin` | fails, and never flushed anything |
| disabling `ARB_buffer_storage`, `ZINK_DEBUG=sync`, `ZINK_DEBUG=noreorder` | no effect |

The blend toggle is the important one. It makes Mesa issue the accumulated batches, so the trace shows the **same 38 draws as the `glFlush` case, byte for byte**, and it is still wrong. So the fix is not emptying Mesa's buffer. It is submitting the command stream. That points at a hazard on the reused vertex buffer inside the driver rather than anything about vertex layout.

**The engine change.** `CGlobalRendering::ProbeImmediateModeBatching()` reproduces the defect at startup and reports `18658 stray, 13071 unlit`, which is byte-identical to the standalone reproducer. So detection is not impossible after all: the previous probe simply did not reproduce the triggering shape. `supportImmediateModeBatching` is logged with the other capability flags. `glBeginBatch()` in `myGL.h` wraps `glBegin` and submits first when the flag is false, and all 46 non-`rts/lib` `glBegin` call sites now go through it. `ForceImmediateModeFlush` overrides the probe for A/B testing, -1 measures, 0 never submits, 1 always does.

Still open on this: **the cost has not been measured.** `GuiHandler` alone has 28 call sites, so a busy game frame may pay a lot of command-buffer submits. Measure before deciding whether this ships as is or whether `LuaOpenGL` should stop using immediate mode altogether, which is the real fix.

Also still open: the Mesa working tree at `~/dev/mesa` carries the uncommitted `VBO_TRACE` instrumentation in `vbo_exec_draw.c`. Revert it once the upstream bug report is written. `~/dev/macos-probes/off_probe.c` is the reproducer and holds all the candidate mitigations.

---

**What is left in game, and a measurement hazard that invalidates clean dumps**

The load screen is fixed and the in-game artefacts are not. Four things are established about what remains.

**The present path is innocent.** A dump taken at the top of `SwapBuffers` is byte identical to the last dump inside `CGame::Draw`. Whatever is wrong is wrong in the framebuffer before anything Metal touches it.

**The artefact is drawn in the input-receivers phase.** Counting strongly red pixels across phases in a frame that showed the band: 28 at `1-world`, 42 after `CInputReceiver::DrawReceivers()`, then 76030 after `luaInputReceiver->Draw()`. So LuaUI's `DrawScreen` draws it, not the engine's own receivers.

**Two attractive theories are dead, both disproved standalone before touching engine code.** Display lists render correctly on Zink, so `gl.CreateList` plus `gl.CallList` is not it even though the resource bar uses them. `glRectf`, which `gl.Rect` calls and which Mesa expands into `glBegin` internally where the engine's wrapper cannot reach, also renders correctly.

**The instrumentation suppresses the bug.** Two runs of the same binary and the same script, differing only by one extra `DumpFramePhase` call, gave 76030 red pixels in one and at most 2015 across eight sampled frames in the other. A phase dump is a full-framebuffer `glReadPixels`, which forces a command-stream submission, which is exactly the thing that fixes the bug. So **a clean phase dump is not evidence of absence**, and any conclusion drawn from one needs re-checking. This is the likeliest reason earlier dumps looked clean while the screen did not.

That last point sets the method for whoever picks this up. Do not sample eight phases per frame while hunting this. Dump only `7-presented`, which sits next to the present path's own full-framebuffer readback and so costs almost nothing. `SPRING_PHASE_DUMP` now takes a label substring for exactly this.

**A repeatable automated harness now exists.** Three pieces, all in the throwaway SplinterFaction copy at `~/dev/spring-testdata/games/SplinterFaction_0.1.78.sdz`, which is a real copy rather than a symlink so the pristine archive in `~/.spring/games/` stays clean:

- `LuaRules/Gadgets/game_spawn.lua` has its two pre-game deadlines cut from 900 frames to 30. The gadget already force-assigns on timeout, so no new logic was needed. Without this the faction and placement gates eat the first 60 seconds and units never spawn.
- `LuaUI/Widgets/zzz_autoselect.lua` keeps every unit selected. The artefact is far stronger when a selection exists, and this removes the human from the loop.
- `LuaUI/Config/SF_data.lua` `order = 0` disables a widget, so subsets can be driven from a script.

Peak count of strongly red pixels across dumped frames is the detector. A clean frame reads in the hundreds, the band in the hundreds of thousands.

**The artefact scales with how much is drawn, so there is no single culprit widget.**

| widgets enabled | peak red |
|---|---|
| resource bar only | 387 |
| build and order menu only | 18402 |
| four selection-driven | 233357 |
| twelve static | 515424 |

**The mitigation works in game, and is not complete.** Same binary, all 131 widgets, only `ForceImmediateModeFlush` differing:

| | peak red | per frame |
|---|---|---|
| forced on | 19604 | 19604, 9145, 333, 393 |
| forced off | 444902 | 364, 423, 444902, 305372, 288783 |

A 23x reduction in the peak and most frames clean. The load screen also regresses visibly when it is forced off, which independently confirms the override works.

**The residual is the same class of bug.** The band appears at a different depth in the draw order from frame to frame, sometimes above the bottom-left button grid, sometimes between the buttons and their backing panel, sometimes behind all of it. That is what a corrupted batch looks like: it lands wherever that batch sat in the sequence, and which batch is corrupted changes.

The resource bar has its own separate corruption, visible with that widget alone enabled while the red band is absent. It is a different symptom and has not been characterised.

**The shared code is `RectRound`, and it explains the split.** `gui_static_resourcebar.lua` builds its panels through `DrawPanel`, which is two `RectRound` calls plus a `TexRect` accent strip from `LuaUI/Images/staticgui_accent.png`. `RectRound` is `gl.BeginEnd(GL.QUADS, DrawRectRound, ...)`, the same 28-vertex construct the load screen uses. Eleven widgets share it verbatim and 47 use display lists.

The difference is where it runs. The load screen calls it live every frame, so `glBeginBatch` applies and it is fixed. The widgets call it inside `gl.CreateList`, so `gl.BeginEnd` executes at compile time where the flush is useless, and the geometry is replayed later by `glCallList` where nothing flushed.

`glCallListBatch` was added to close that, flushing before `glCallList` in `LuaOpenGL::CallList`. **The result is not a clear win and should not be reported as one.** Peak red 21569 with four of five frames at baseline, against 19604 with two of four before. The sample is too small to separate. It needs a longer run or repeated runs before anyone claims it helps.

Two leads not yet followed. The mitigation may simply be insufficient in a frame with textures, blending and hundreds of batches, which the standalone sweep never modelled. And no probe has yet reproduced a display list replayed among many live immediate-mode batches, which is the case that matters here and is not the case the earlier list probe tested.

**The cursor is not a failed cursor replacement.** `gui_customcursorsets.lua` calls `Spring.ReplaceMouseCursor` for 23 cursor names and ignores every return value, which looked like a strong candidate. It is not: a run with a warning wired into `CMouseHandler::ReplaceMouseCursor` logged no failures at all.

Worth knowing anyway, and separable from macOS: `ReplaceMouseCursor` swaps the new cursor in before checking it, so a replacement that loads no frames destroys the working cursor and leaves one that draws nothing. The function returns validity as a bool, which suggests non-destructive failure was intended. Refusing the swap is a behaviour change though, since content could rely on it to hide the cursor deliberately, so this is left as a warning only and the behaviour question is for upstream.

---

**The resource bar, settled 2026-08-03. It is the batching bug inside display lists**

The missing icons, the missing panel accent strips and the skewed panels are not a second defect. They are the immediate-mode batching bug, compiled into a display list where the mitigation cannot reach.

`glFlush` is executed immediately during list compilation and is never recorded into the list. So `glBeginBatch` flushes at compile time, which is useless, and at replay time the corrupted batches are already baked in. The corruption is therefore deterministic: two screenshots a hundred draws apart are pixel identical, and deleting and rebuilding the lists mid-game reproduces the same corruption exactly.

What separates what renders from what does not is which GL path the call takes, measured by instrumenting the widget:

| widget call | GL path | result |
|---|---|---|
| `gl.Rect` | `glRectf`, expanded inside Mesa | always renders |
| `gl.TexRect`, `gl.BeginEnd` | `glBegin` through `glBeginBatch` | corrupted or lost |

Untextured `gl.Rect` markers injected into the same lists, immediately beside the failing draws and inside the same font `Begin`/`End` block, render on every panel every frame. The textured draws next to them do not.

Four theories died on the way and none should be retried. **Display list churn:** the accents and icons live in lists built once, not in the per-frame rebuilt one. **Textured quads in lists:** a standalone probe renders all six of the widget's textures live, from a list, and colour modulated, on Zink. **PNG versus DDS:** the correlation is real in the screenshots and spurious as a cause, since the DDS draws simply sit at different positions in the batch sequence. **Lists compiled too early:** rebuilding them at draw 240 changes nothing.

This confirms the plan's own conclusion that the real fix is `LuaOpenGL` not using immediate mode. No flush-based mitigation can cover content that compiles immediate mode into display lists, and 47 widgets do.

Two traps, both of which produced confident wrong answers before being caught by a control:

- A probe widget drawing about thirty quads and replaying six display lists made **every other widget's UI vanish entirely**. The instrument destroyed what it measured. Always run the same scene with the probe disabled before believing anything.
- The first probe drew pale textures on a white backdrop, where "did not draw" and "drew correctly" look identical. Use a mid grey backdrop and a saturated colour modulate.

Useful mechanics for the next session. The widget can call `Spring.SendCommands("screenshot")` on a frame counter, which writes full resolution PNGs to `<datadir>/screenshots` unattended, and those are far more readable than the quarter size phase dumps. `sips --cropOffset` is not an absolute top-left offset and silently crops the wrong region, so `coding-agents/test-scripts/crop.py` crops PPM and PNG by absolute coordinates instead.

---

**Deferred Lua display lists, the fix, 2026-08-03**

`gl.CreateList` keeps its recording function instead of compiling, and `gl.CallList` runs it, whenever `supportImmediateModeBatching` is false. That puts the drawing back on the live path where `glBeginBatch`'s flush applies. Gated on the measured capability, not on a platform, so this is a platform-neutral change that can go upstream ahead of the renderer.

Verified on the resource bar: the four panel accents, all four icons and the research readout return, and the skew and hatching go. The user confirmed no red artefacts anywhere in that run.

**Both halves are needed, and neither alone is enough.** With `SPRING_NO_BATCH_FLUSH=1`, deferral on and the flush off, every artefact returns including the load screen. Deferral moves list content to where the flush can act, the flush repairs it there.

**The cost is mostly resolution.** 60 second runs, all three at 1280x720 on the built-in display:

| configuration | frames | peak RSS |
|---|---|---|
| stock, no deferral, no flush | 1251 | 2852 MiB |
| deferral + flush | 747 | 1970 MiB |
| deferral, no flush | 1209 | 3014 MiB |

1.67x slower here against 4.7x measured at 5120x2754, so most of what looked catastrophic was the window size. Run the harness windowed and small.

**There is a severe memory leak, around 5 GiB per second, and RSS cannot see it.**

GPU and system memory are the same silicon on Apple Silicon, so IOAccelerator allocations are real pressure. `ps -o rss` does not count them. Measured on one run, `phys_footprint` against RSS at the same instants:

| t | 0 | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 |
|---|---|---|---|---|---|---|---|---|---|
| RSS | 0 | 487M | 745M | 1504M | 1578M | 1229M | 1394M | 1256M | 1349M |
| footprint | 871M | 1804M | 5079M | 9605M | 12G | 20G | 31G | 42G | 50G |

RSS is flat at about 1.3 GiB the whole time. This is why an earlier claim here that there was no leak was wrong: it was measured on RSS, which is blind to the growth, and on peak-divided-by-frames, which is invalid arithmetic because the peak lands during loading.

**It is not caused by the deferred display lists.** Stock reaches 11 GiB at the same six second mark that the fix run reaches 9.6 GiB. The trajectories are the same, so this predates both the deferral and the flush.

**It explains the KosmicKrisp abort.** `kk_CmdBeginRendering` failing to get a Metal command encoder is memory exhaustion, not a mysterious driver bug. It also explains the 54 GiB freeze, and why the RSS ceiling never fired while it happened.

Use `phys_footprint` for anything memory related here. `run-capped.sh` caps on it at 20 GiB, which clears the roughly 10 GiB that loading alone needs while stopping far short of a freeze, and logs RSS, `top`'s MEM and footprint side by side. `top`'s MEM counts reserved address space and is not a pressure measure either.

This is now the largest open problem in the port. It bounds every run to a few tens of seconds and it is the reason long sessions are not possible.

**Ruled out, each by measurement. Do not retry these.**

| theory | test | result |
|---|---|---|
| the deferred display lists | stock vs fix | identical, 11 GiB vs 9.6 GiB at t=6s |
| the present readback | `SPRING_NO_PRESENT` | growth got *faster* |
| missing synchronisation | `SPRING_FRAME_FINISH` | unchanged, 21.5 GiB at t=10s |
| proportional to frames | `SPRING_FRAME_THROTTLE` at 60fps | unchanged, 21.5 GiB at t=10s |
| general immediate-mode drawing | standalone `leak_probe` | flat, 41 to 52 MiB over 18M batches |
| sound and OpenAL | `Sound = 0` | unchanged, 32 GiB at t=10s |

It is also absent in the main menu, flat over 21 seconds, and absent on llvmpipe, flat over 80 seconds with the same binary and content. So it needs a game loaded and it is driver side.

**One genuine defect the probe did find, separate from the engine's problem.** With no per-frame synchronisation at all, `leak_probe` grows without bound, 466 MiB to 5.3 GiB over 1.5 million frames. Adding one `glFinish` per frame makes it flat. That is a clean standalone reproducer for the upstream report, but its magnitude does not explain the engine, and adding a `glFinish` to the engine does not help.

**Where it points.** It scales with a game being loaded but not with frames drawn, which leaves resource creation at scale: the thousands of texture, buffer and mipmap allocations a map and unit set need. `RecoilBuildMipmaps` allocates every mip level for every texture and then calls `glGenerateMipmap`, which is the shape worth reproducing next. Extend `leak_probe` with texture creation rather than touching the engine.

**Worth keeping anyway:** throttling the present to 60fps raised the in-game frame rate from single digits to about 18, so the memory pressure appears to be what was slowing the engine rather than the reverse.

The abort seen when memory is high is inside KosmicKrisp, `kk_CmdBeginRendering` to `cs_start_render` to `mtl_new_render_command_encoder_with_descriptor`. Metal cannot create a render command encoder and aborts the process, the same family as the compute-encoder abort recorded earlier.

**This froze a machine, so read this before running anything.** An uncapped run reached 54 GB on a 16 GB machine, made the system unresponsive to the point that the out-of-memory dialog would not respond, and cost a hard power cycle. 54 GB was the ceiling it hit, not what it wanted. The likeliest cause is the deferred display list runaway described above, before the 8192 cap existed, since that is genuinely unbounded rather than a slow leak. It has not recurred since the cap. macOS has no cgroup and `ulimit -v` does not constrain Metal allocations, so **always launch through `coding-agents/test-scripts/run-capped.sh`**, which polls RSS and SIGKILLs at 3 GiB by default. A `gl.CreateList` count above 8192 is now refused by the engine for the same reason.

The failure of judgement that led to it is worth recording too. A 50 GB figure was reported, `ps` was checked after the process had exited, nothing was found, and it was called benign virtual memory. The correct response to any report of runaway memory is to cap first and investigate second.

---

**The leak is in KosmicKrisp. Queued mipmap generation costs about 26 MiB a texture and nothing throttles it, measured 2026-08-03**

`coding-agents/test-scripts/kk_mipmap_leak.c` is a self-contained reproducer, EGL and libGL only, a core 3.3 context and no drawing at all. It creates a 512x512 texture, allocates every mip level, calls `glGenerateMipmap`, deletes the texture, and repeats. It reports its own `phys_footprint` through `task_info`, so it needs no external tooling. 100 textures, one binary, switched by environment variable:

| configuration | `phys_footprint` |
|---|---|
| Zink over KosmicKrisp, `glGenerateMipmap`, no sync | 2626 MiB |
| Zink over KosmicKrisp, `NO_GENMIP=1`, no sync | 224 MiB |
| Zink over KosmicKrisp, `glGenerateMipmap`, `SYNC=1` | 161 MiB |
| llvmpipe, `glGenerateMipmap`, no sync | 36 MiB |

Linear with count: 661 MiB at 25 textures, 992 at 50, 2339 at 100. The texture is deleted every iteration, so nothing the probe holds accounts for it.

**The memory is never returned, and `SYNC=1` does not free it.** The 2626 MiB figure is read after a full `glFinish`, so everything has retired, and `HOLD=1` reads 2624 MiB after five more idle seconds. What `SYNC=1` does is hold the high-water mark down to one texture's worth by never letting more than one render pass be in flight.

So the mechanism is: **the IOGPU resource pool grows to the high-water mark of concurrently in-flight render passes and never shrinks.** Every result fits this and nothing contradicts it.

`MESA_GL_VERSION_OVERRIDE=4.6` is needed or the core context fails to make current. `leak_probe.c` carries the same loop under `SPRING_PROBE_MIPMAP=1` with `SPRING_PROBE_NO_GENMIP=1` as its control, but `kk_mipmap_leak.c` is the one to hand upstream.

**Watch out for the in-loop numbers.** Without `SYNC=1` the footprint reads 236 MiB at texture 90 and then jumps to 2626 MiB at the closing `glFinish`, because the cost lands when the queue drains rather than where it was incurred. Sampling the loop and stopping there reads as a much smaller problem than it is.

**The mechanism, in the order it was established.** Each step ruled out the layer above it, so do not re-derive them.

1. Zink's own device memory is not the leak. A patch counting bytes at zink's `vkAllocateMemory` and `vkFreeMemory` tops out at 2.3 GiB over 301 allocations while `phys_footprint` reaches 28 GiB. Zink suballocates, so thousands of GL textures were never going to appear as thousands of allocations.
2. `vmmap -summary` puts the growth in `IOAccelerator (graphics)`, and it is region count that grows: 314, then 18636, then 51476, then 108971, reaching 12.5 GiB. `heap -s` names the classes, `IOGPUMetalPooledResource` going from 672 to 46842 and `MTLResourceList` from 42 to 2691.
3. `malloc_history -allByCount` gives the call site. It is `zink_blit` to `kk_CmdBlitImage2` to `vk_meta_blit_image` to `kk_CmdBeginRendering` to `cs_start_render` to `mtl_new_render_command_encoder_with_descriptor`. `glGenerateMipmap` is one blit per mip level, and kosmickrisp starts a fresh MTL4 command buffer and render command encoder for every render pass.
4. The command buffers are not held. Counters in `kk_cmd_buffer.c` show 102000 created in 12 seconds against 257520 resets, with live count never above 1000. Regions track the cumulative number created, about six regions each, not the number live. So the IOAccelerator allocations a command buffer makes are not returned when it is released.

**Ruled out by measurement, all on one binary with runtime switches. Do not retry.**

| theory | test | result |
|---|---|---|
| zink never flushes during a run of blits | `ZINK_BLIT_FLUSH=64` | no change, 31 GiB at t=12s |
| flushing is not enough, it needs a wait | `ZINK_BLIT_SYNC=32` | no change, and the stall provably fired, 247 stalls over 8000 blits |
| a GPU submission backlog | `ZINK_DEBUG=sync` | no change, 37 GiB at t=10s |
| texture creation itself leaks | probe with `glGenerateMipmap` off | flat, 3 MiB over 300 textures |

The flush and sync theories were both wrong for the same reason: the command buffers were never the thing being held. `zink_batch.c` hands out a brand new batch state whenever none has completed, so flushing harder only spreads the same work over more command pools, but that turned out not to matter either.

**Tooling worth keeping.** `~/dev/mesa` carries four uncommitted diagnostic patches, all inert unless their environment variable is set, so a normal run is unaffected:

- `zink_bo.c`, tracks live device memory and dumps the existing per-shape `ZINK_DEBUG=mem` stats on each step of growth, since upstream only prints them when an allocation fails. `ZINK_DEBUG=mem ZINK_MEM_STEP_MB=256 MESA_LOG_LEVEL=info`.
- `zink_blit.c`, `ZINK_BLIT_FLUSH`, `ZINK_BLIT_SYNC` and `ZINK_BLIT_STATS`.
- `kk_cmd_buffer.c`, `KK_CMD_STATS`, counts Metal command buffers created against released.
- the older `vbo_exec_draw.c` `VBO_TRACE` patch from the batching bug.

**Traps that cost time here.** `footprint -p $!` after `timeout 120 cmd &` measures the `timeout` wrapper, not the program, and reports a flat 1 MiB while the real process is taking gigabytes. `heap` needs `-s` for sort by size, and `malloc_history` needs `MallocStackLogging=1` set before the process starts. A probe that leaks at this rate can freeze the machine exactly as the engine did, so cap it too.

**Both obvious engine mitigations are dead. Do not build either.**

- Skipping `glGenerateMipmap` when the file already carries mips is **already implemented**. `HandleDDSMipmap` in `Bitmap.cpp` only calls it when `numEmbeddedLevels == 0`.
- Skipping it when only one level is requested saves nothing. Mesa's `st_generate_mipmap` already returns early at `lastLevel == 0`, so the many `reqNumLevels = 1` call sites cost nothing today.

**Mipmaps are a minority of the engine's growth anyway.** Splitting the KosmicKrisp counters by encoder type over a load gives 68177 render passes against 33823 compute, and only 8000 of those render passes are blits. So `glGenerateMipmap` is at most 8% of it and ordinary drawing and texture upload are the rest. There is no engine change that avoids render passes.

**Throttling does not work, settled 2026-08-03. Six tests, all negative, do not retry any of them.**

| throttle | what it actually bounded | peak |
|---|---|---|
| baseline | | 32 GiB at 12s |
| `SPRING_MIPMAP_SYNC=1`, `glFinish` per texture built | 500 of 68177 render passes | 33 GiB at 12s |
| `ZINK_BLIT_FLUSH=64` | 8000 of 68177 render passes | 31 GiB at 12s |
| `ZINK_BLIT_SYNC=32`, flush plus timeline wait | fired 247 times over 8000 blits | 30 GiB at 12s |
| `ZINK_DEBUG=sync` | draws and dispatches | 37 GiB at 10s |
| `ZINK_MAX_BATCHES=8`, down from zink's 5000 | live batch states 7 to 18, from 126 to 229 | 22 GiB at 14s |

The reason is the same every time. One batch state holds thousands of render passes, so capping outstanding batches does not cap in-flight render passes, and syncing one class of render pass leaves the rest. There is no knob at the zink level that bounds the thing that matters.

Numbers worth keeping. A load makes 102000 Metal command buffers in 12 seconds over 40000 submits, 68177 render passes against 33823 compute, with never more than about 660 command buffers live. Only 500 textures go through `RecoilBuildMipmaps`, which is why syncing that path cannot help. Zink's own throttle never engages at all, since live batch states peak at 126 to 229 against its threshold of 5000.

Filed upstream as <https://gitlab.freedesktop.org/mesa/mesa/-/work_items/15998>, with `coding-agents/test-scripts/kk_mipmap_leak.c` attached as the reproducer. The tracker is behind Anubis, so read and reply to it by hand.

---

**It is a regression, and the engine is playable on the commit before it. Settled 2026-08-03**

The cause is `kk: Move to Metal4 command encoding`, `c08dba83025`, 2026-06-16. Before it, KosmicKrisp took command buffers from a Metal 3 command queue, which owns their memory and recycles it. After it, every render pass takes its own `MTL4CommandBuffer` from the device plus a `MTL4CommandAllocator` that only reclaims on reset.

**This is why nobody else has hit it.** ExaDev's shipped release is `engine-macos-arm64-2026.06.08-gaefcef0`, eight days before that commit, and it bundles `libgallium-26.2.0-devel.dylib`, the same name our pre-Metal4 build produces. The working window is narrow and worth writing down, because both edges are hard:

- **2026-05-11** MR 41313 adds `nullDescriptor` to KosmicKrisp. Without it Zink refuses to start at all, which is why Homebrew's Mesa 26.1.4 cannot be used as a control. It fails with `Zink requires the nullDescriptor feature of KHR/EXT robustness2`.
- **2026-06-16** Metal4 command encoding lands and the memory behaviour breaks.

Anything built between those two dates works. `26.2-branchpoint` and every `26.2.0-rc` contain the Metal4 commit, so there is no released tag that is usable.

**Measured, one probe, two prefixes.** `kk_mipmap_leak` at 100 textures:

| | peak | after 5s idle |
|---|---|---|
| post-Metal4, `mesa-26.2.0-rc3` | 2626 MiB | 2624 MiB |
| pre-Metal4, `56588ef0665` | 482 MiB | 369 MiB |

Pre-Metal4 also gives the memory back, and scales sublinearly rather than at a flat rate: 482 MiB at 100 textures, 938 at 200, 2060 at 500, 3204 at 1000, so 4.8 MiB a texture falling to 3.2. Post-Metal4 is a flat 26 MiB a texture that is never returned.

**The engine result is the one that matters.** A 60 second run on pre-Metal4 exited cleanly, the first clean run of the session. Footprint climbed to about 7.1 GiB during load and then sat between 7.2 and 7.4 GiB for the remaining 38 seconds. The same run post-Metal4 dies at 12 seconds somewhere between 30 and 41 GiB.

**The setup.**

- `~/dev/mesa-install-premtl4` is the prefix, built from `~/dev/mesa` branch `premtl4` at `56588ef0665` using the recipe in section 2 with the prefix changed.
- `build-macos-legacy/run-macos-premtl4.sh` is the launcher. Pass it to the harness as `LAUNCHER=./run-macos-premtl4.sh coding-agents/test-scripts/run-capped.sh ...`.
- It relinks `spring-premtl4` automatically whenever `spring` is newer, so the stale-copy trap that `spring-staging` has cannot happen here.
- `~/dev/mesa` branch `macos-diagnostics` at `337c87ae087` carries the instrumentation, on top of the post-Metal4 tree it was written against. That is where the counters live now, not in the working tree.

**Pre-Metal4 is not a clean win, 2026-08-03. Three things are worse than the first run suggested.**

The first 60 second run exited cleanly at a 7.3gb plateau and that got called playable. Then a longer session on a second map produced all of the following.

**The red batching artefact is back.** The user reports it during play on pre-Metal4. A screenshot at t=25s on AcidicQuarry showed a wholly correct frame, resource bar icons and all, so it is intermittent or content dependent rather than constant. It was not seen on post-Metal4 in this session. What decides it has not been established, and the obvious suspect is the capability probe behind `ForceImmediateModeFlush` reaching a different answer on this driver. Check what the probe decides before assuming the mitigation is even active.

**The frame rate is 14 to 19 fps.** Measured unattended with the new `SPRING_FPS_LOG=<seconds>` switch, on Valles Marineris at 1280x720, windowed:

| phase | fps |
|---|---|
| load screen | 45.3 |
| first 10s of play | 13.9 |
| steady, sim frames 300 to 5100 | 13.9 to 19.2 |

The simulation runs at a full 30 ticks a second the whole time, 301 sim frames per 10 second window, so this is entirely a draw cost and not the sim. The user reports the same 15 to 20 by eye.

**A run hard froze the machine and cost a second power cycle.** This one is not the memory failure and must not be confused with it. In the 54gb incident the cursor still moved and nothing else responded, which is thrash. This time the machine froze completely, cursor included, as the engine exited. That is the signature of a GPU or WindowServer hang.

It matters because **`run-capped.sh` cannot protect against this**. The ceiling polls `phys_footprint`, and a GPU hang does not show up there. Peak was 7856mb, nowhere near the ceiling, and the harness reported a clean exit. No kernel panic was written, so there is nothing to read afterwards. Several earlier pre-Metal4 runs of 50, 60 and 100 seconds exited without freezing, so it is intermittent.

Nothing here is understood yet. Do not treat a memory ceiling as making engine runs safe.

**Two harness notes from the same session.**

- The scratchpad under `/private/tmp` does not survive a reboot. Every log and screen capture from that session was lost and only the numbers already written down survived. Put anything worth keeping in the repo as it is measured, not at the end.
- `run-capped.sh` reports elapsed time that runs well short of wall clock, because each poll shells out to `footprint` and `top` and those take about a second each. A run billed as 60 seconds took about 114.

**The test map is now Valles Marineris 2.6.1**, symlinked into `~/dev/spring-testdata/maps/`. `mapname` in a start script is the mapinfo `name` plus its `version`, so `Valles Marineris 2.6.1`, not the archive filename. The autoselect test widget has been removed from the SplinterFaction copy.

**The red artefact is not a pre-Metal4 regression, and the mitigation is only about 94% effective, 2026-08-03**

`off_probe` built against both prefixes and run on each gives output that differs in one line, the version string. Every measured row is identical. So the Metal4 migration changed nothing about the immediate-mode batching bug, and the artefact coming back on pre-Metal4 is not that version being worse.

What the probe actually says about the mitigation the engine ships:

| fix | wrong |
|---|---|
| E0 naive, as the engine drew before | 15 of 17 |
| E6 glFlush before every begin, what the engine does now | **1 of 17** |
| everything else tried, E1 to E11 | 15 or 16 of 17 |

**So there is a residual.** E6 is far better than anything else and it is not complete. One warm-up count in seventeen still renders wrong, and which one depends on the byte offset a batch lands at inside the immediate-mode buffer. That makes the artefact deterministic for a given piece of content and its preceding draws, and different between runs where the preceding draws differ.

That matches what was seen exactly. In one session the build menu corrupted, stayed static, and the corruption tracked the content as it scrolled. In the next run, same binary, same map, same Mesa, with the build menu opened, nothing. Both are consistent with a 1-in-17 residual rather than with anything having changed.

Two consequences worth holding on to:

- **Do not read a clean run as a fix.** With about a 6% residual, a short session proves very little. Any claim about the artefact needs several runs and the same content on screen.
- **PR #3169 is still worth having and is honestly described.** It measured 0 of 31 corrupted frames against 11 of 89 stock, which is the improvement this table predicts, not a claim of perfection. The real fix remains LuaOpenGL not using immediate mode.

The probe still does not reproduce the engine's display list case. Rows D0 to D2 are 0 of 17 wrong, so a list replayed among many live immediate-mode batches, which is the case that matters in the engine, is still unmodelled.

**A complete fix exists and it is not a flush, measured 2026-08-03**

Normalising the vertex format of **every** immediate-mode batch in the frame gives 0 of 17 wrong. Identical on both Mesa versions, so this is a property of the bug and not of one build.

| fix | pre-Metal4 | post-Metal4 |
|---|---|---|
| E0 naive, how the engine drew before | 15 of 17 | 15 of 17 |
| E6 glFlush before every begin, what it ships now | 1 of 17 | 1 of 17 |
| E12 grid normalised, warm-up left alone | 15 of 17 | 15 of 17 |
| E17 warm-up normalised, grid left alone | 16 of 17 | 16 of 17 |
| **E16 both normalised** | **0 of 17** | **0 of 17** |

Normalising a batch means emitting the full attribute set once immediately after `glBegin`, a colour, a texcoord and a normal, and never varying the vertex arity inside the batch.

**Partial coverage does not work, and that is the hard part.** E12 normalises the batches being looked at and fails. E17 normalises only the batches drawn before them and fails. Only doing both is clean. So this cannot be applied to one widget or one code path. Every immediate-mode batch in the frame has to be normalised or none of it counts, which is a real constraint on how it can be implemented. `glBeginBatch` in `myGL.cpp` is already the chokepoint every batch passes through, so it is the obvious place to try.

Why this is worth having over the flush, if it holds up in the engine: it is complete rather than 94% effective, and three attribute calls per batch should cost far less than a `glFlush` per batch, which serialises. The measured 1.67x at 1280x720 is the flush's price.

**A methodology warning, because it nearly produced three wrong conclusions.** These fixes were added to `off_probe` with `str.replace`, which silently does nothing when the pattern does not match. Two patches failed that way and the runs still printed confident, wrongly labelled results. E16 read 15 of 17 while not actually doing what its label said, and reads 0 of 17 once it does. Verify a patch landed before believing the numbers, and check every substitution rather than the last one.

The probe now lives at `coding-agents/test-scripts/off_probe.c` rather than only in `~/dev/macos-probes`, so it cannot be lost. E20 runs M3's own calls inside the E harness as a control and agrees at 0 of 17.

**Arity alone is not enough in the probe, whatever the engine seemed to show, 2026-08-03**

The engine looked clean with `SPRING_BATCH_ARITY=1`, one run, watching the load screen. The probe disagrees flatly and has been asked three different ways:

| fix | wrong |
|---|---|
| E8 uniform arity at 2f | 15 of 17 |
| E24 the drawn batches widened to 4f, warm-up left narrow | 15 of 17 |
| E25 every batch in the frame widened to 4f | 16 of 17 |
| E16 and E22, the attribute set after begin plus uniform arity | **0 of 17** |

So neither uniformity nor width fixes it on its own. Both halves are needed.

Against that, one engine observation. The load screen is a good test point, since the naive baseline artefacts there within seconds, but one clean run at a roughly 6% residual is weak evidence. Repeat `SPRING_BATCH_ARITY=1` several times before believing it, and if it stays clean then the probe does not model the engine's Lua path and the probe is what needs fixing.

**Switches for this, all gated on `supportImmediateModeBatching` so no other platform is affected:**

| switch | behaviour |
|---|---|
| default | flush before every begin, narrow arity, what ships today |
| `SPRING_BATCH_NORMALISE=1` | attribute set after begin plus uniform 4f arity, no flush |
| `SPRING_BATCH_ARITY=1` | uniform 4f arity only, no flush |
| `SPRING_NO_BATCH_FLUSH=1` | nothing at all, the true baseline |

**The baseline reproduces reliably on the load screen.** `SPRING_NO_BATCH_FLUSH=1` shows the red artefact within seconds of the load screen appearing. That makes an A/B cheap, roughly 30 seconds a side, and it is a far better test point than building a factory.

**Two traps from this round.** Do not run two engines at once, the second aborts with `std::system_error` at `UDPListener::TryBindSocket` and it looks like a code fault. And every `str.replace` patch to the probe needs asserting, since three separate results in this session were confidently mislabelled by substitutions that silently did not apply.

**Uniform vertex arity replaces the flush, and it is a correctness win rather than a speed one, 2026-08-03**

Three interleaved pairs on the load screen, which the baseline corrupts within seconds of appearing:

| side | result |
|---|---|
| `SPRING_NO_BATCH_FLUSH=1`, nothing | artefacts, 3 of 3 |
| uniform 4f arity in LuaOpenGL, no flush | clean, 3 of 3 |

The difference is obvious rather than marginal, so this is not a 6% residual being read as a fix. The mitigation is now uniform arity, the flush is opt-in behind `SPRING_BATCH_FLUSH=1`, and `SPRING_BATCH_NARROW=1` restores the old vertices. Both are gated on `supportImmediateModeBatching`, so nothing changes on a driver without the defect.

**The speed win did not materialise.** Four runs alternating, 10 second fps windows:

| | run 1 | run 2 |
|---|---|---|
| flush | 16.2 | 16.0 |
| arity | 16.4 | 19.2, not comparable |

Three of those four ran unattended with no input at all, on whatever the opening camera shows. The fourth was noticed and played, a building was placed, and that is the 19.2. So the comparable set is 16.2, 16.0 and 16.4, which is no difference whatsoever. Removing the flush does not change the frame rate.

**The likeliest cause is focus, not interaction.** macOS gives the foreground app a higher scheduling priority and marks fully covered windows as occluded. Every unattended run sat behind an editor, unfocused and probably occluded. The 19.2 is the run that had been clicked into. So the outlier is most plausibly explained by window focus rather than by anything about the frame or the mitigation.

That makes every frame rate figure in this document suspect, not just this comparison. All of them were measured on a background window, so they are lower bounds under background scheduling, and the 14 to 19 range recorded earlier as steady state is not the frame rate of a focused, played game.

**Any future performance work has to control for this.** Bring the window to the front before measuring and keep it unoccluded, for example with

```
osascript -e 'tell application "System Events" to set frontmost of first process whose unix id is <pid> to true'
```

and treat any run where the window was covered as void. Without that, focus changes alone can move the result by 20%, which is larger than most of the differences worth measuring. The 1.67x the flush was once measured at does not show up here. Whatever bounds the frame rate at 16 to 19 fps is not the flush, so that is where the next performance work belongs rather than in this mitigation.

**off_probe now disagrees with the engine and the engine wins.** The probe puts uniform arity at 15 of 17 wrong however it is asked, and it is flatly wrong about this path. Either the probe's synthetic grid does not capture what LuaOpenGL does, or the load screen's corruption has a different cause than the grid's. Worth resolving before the probe is trusted for the next rule, since it has been the cheap way to test candidates without engine runs.

**What this costs.** Pinning here gives up every KosmicKrisp fix since 2026-06-16, which is at least 18 commits, and the reported Vulkan version drops from 1.4 to 1.3. Pre-Metal4 is also not perfectly bounded, just far cheaper and able to return memory. It unblocks the port, it does not fix the bug, so the upstream report still matters.

---

**The glCallList flush does not help, measured 2026-08-03**

Step 1 of the previous session's list is closed. Ten interleaved runs on one binary, `SPRING_NO_LIST_FLUSH` switching the `glCallList` half of the mitigation at runtime so the sides could not be confounded by anything drifting between two builds.

| side | frames corrupted | per run |
|---|---|---|
| flush on | 64/186, 34.4% | 15.0 27.7 28.2 96.4 21.9 |
| flush off | 31/170, 18.2% | 19.0 21.7 9.1 15.6 21.4 |

No significant difference on a rank test of the per run rates, U = 4 with n = 5 a side, two tailed p > 0.05. There is no evidence it helps and a weak trend the other way, so `glCallListBatch` was removed. The display list finding above explains why it could never have worked.

**The detector was blind to half the artefact.** It counted strongly red pixels only. The artefact takes the colour of whichever batch was corrupted, and a full screen lavender variant reads as clean under a red-only rule. Count red, magenta and lavender, and score absolutely rather than against a per run baseline: one run had 27 of 28 frames corrupted, which moved its own median baseline to 752008 and scored a catastrophic run as mild.

---

**The invisible cursor is not an engine bug, on current evidence**

Every candidate is ruled out by measurement rather than reading. `HardwareCursorApple::IsValid()` returns false unconditionally, so macOS always takes the software path and `SetCursor` hides the OS cursor, which is why nothing else can be covering for it. Instrumenting `CMouseCursor::Draw` in a real run gives a wholly healthy draw: `frames=1`, a valid texture, a valid shader, `viewSize=<2760,1720>`, and matrix params that put a 32x32 quad exactly at the pointer.

Reading the framebuffer either side of `rb.Submit` shows the draw changing 185 pixels of a 48x48 box with no GL error, and the phase dumps confirm it survives to the presented frame: `5-cursor` differs from `4-rml` by a 28x28 block at the pointer, and `6-screenpost` is identical to `5-cursor`.

So under `empty_mod` the cursor is drawn correctly and reaches the screen. The next step is the same instrumented run under SplinterFaction, since the report came from there and game content is the only remaining difference. Do not re-derive any of the above.

---

**S5. Geometry-shader capability guard, not upstreamable, see section 3**

Purpose: Zink advertises geometry shader support it does not have, because KosmicKrisp reports `geometryShader = false` (Metal has no geometry stage) while `GL_MAX_GEOMETRY_OUTPUT_VERTICES` still returns a positive number. Any GL query for the capability lies, so shader programs with a geometry stage link and then misbehave.

Touches: `rts/Lua/LuaShaders.cpp` `CreateShader`, and `rts/Rendering/Shaders/LuaShaderContainer.cpp`, which also compiles `GL_GEOMETRY_SHADER` objects for engine-side Lua-defined shaders. #2991 only handled the first. The second is a real gap.

Reviewer check: with a geometry-shader widget loaded, the engine logs the strip and keeps rendering instead of producing garbage.

Risk: high, and mostly outside the engine. Stripping the geometry stage does not make a shader correct, it makes it link. Widgets that expand points into quads need a Lua-side no-geometry-shader path, which lives in Beyond All Reason content, not here. Until that content exists this layer buys a running renderer with visibly wrong effects.

---

**S6. Resize. DONE**

Purpose: the pbuffer is a fixed-size allocation, so a window resize must recreate it.

Branch `macos/renderer-s6-resize`, two commits off S4b.

- `acb5734abd` keeps the layer's drawable in step. **S3's comment was wrong about this.** Hosting the layer does get AppKit to resize it, but a CAMetalLayer sizes its drawable from its bounds only until something sets the drawable itself, and S3 has to set it before AppKit's first layout. So the drawable was stuck at the opening size forever. `MacMetalPresent_GetDrawableSize` now recomputes it from the layer's bounds and scale.
- `6a65b5b53f` swaps the pbuffer. `MacEGL::ResizeSurface` creates a new one, makes it current and destroys the old, keeping the context and everything in it. It is called from `ReadWindowPosAndSize`, so one place decides what the framebuffer is and `pixelsPerPointX/Y` stays consistent for free.

Verified: a hand resize logs `framebuffer is now 2274x800`, `winSize` follows, the menu re-lays out for the new aspect ratio rather than stretching, and hover still lands on the button under the cursor.

**The shutdown half was already done.** `DestroyWindowAndContext` has called `MacEGL::DestroyContext` since S1, and that does `eglMakeCurrent(none)`, `eglDestroyContext`, `eglDestroySurface` and `eglTerminate`. The plan called it missing on the strength of ExaDev's `c709dc2a6`, which fixes it on a branch that did not have it.

Still open here, not done:

- Repeated resizes are unproven. One resize works. Nobody has dragged continuously and watched for leaks, and every resize allocates a new pbuffer.

**A testing note that cost an hour.** `run-macos.sh` ends in `exec ./spring`, so the process appears in `ps` as `./spring --isolation` with no path in it. `pgrep -f build-macos-legacy/spring` matches nothing, which reads as "the engine died" while instances quietly pile up and later tests drive whichever window happens to be on top. Match on `./spring` and count them. Related: synthetic `CGEvent` drags cannot resize a window, because the window server ignores posted events on the frame, so that step needs a human.

---

**S6b. Borderless fullscreen. DONE**

Purpose: be full screen without the display mode change, so the desktop keeps its 2x scale and the pointer keeps its speed.

Branch `macos/renderer-borderless`, two commits off S6: the I4 cherry-pick, then `1291d29b33`.

**Most of this already worked, which is why the layer is one guard and not a feature.** Measured on this tree before writing anything:

| config | what happens today |
|---|---|
| `Fullscreen=0, WindowBorderless=1` | borderless window over the usable screen, 2x kept, menu bar visible |
| `Fullscreen=1, WindowBorderless=1` | macOS fullscreen Space, 3024x1898 drawable at 2x, no mode change |
| `Fullscreen=1, WindowBorderless=0`, the shipped default | exclusive, mode change, desktop drops to 1x |

So the gap was only that the default asks SDL for the mode-changing kind. `GetFullScreenFlag` returns `SDL_WINDOW_FULLSCREEN_DESKTOP` unconditionally under `__APPLE__`, used by `CreateSDLWindow`, `SetWindowAttributes` and `LogDisplayMode`. The log line follows the flag asked for, so it stops claiming `fullscreen::exclusive` when it is not.

Verified on the default config: fills the screen, `fullscreen::non-exclusive` at 3024x1898, hover highlights the button under the cursor, a click opens the settings list, and a 100 point screen region still captures as 200 pixels while the engine runs, so the desktop is still at 2x and nothing else on it moved.

Two things that look like defects and are not, both confirmed with the user:

- The top 33 points stay black. That is the menu bar and notch area, which macOS reserves. Filling it is not wanted.
- SDL's fullscreen-desktop puts the window in its own Space. That is the macOS convention and is wanted. `SDL_VIDEO_MAC_FULLSCREEN_SPACES=0` does avoid it, and does then cover the full 982 points, but it is the wrong behaviour for a Mac app.

A measurement worth reusing: `screencapture -R x,y,100,100` and then reading the PNG's pixel size tells you the desktop's scale factor without looking at anyone's windows. 200x200 means 2x, 100x100 means a mode change happened.

`SaveWindowPosAndSize` returns early when `fullScreen`, so none of this writes anything back to the resolution config.

---

**S7. Packaging**

Purpose: a user who is not building Mesa from source can run this.

Touches: a new `.app` bundle assembly, `install_name_tool` relocation of the Mesa and Vulkan dylibs into the bundle, ICD manifests rewritten to bundle-relative paths, the runtime environment (`MESA_LOADER_DRIVER_OVERRIDE=zink`, `EGL_PLATFORM=surfaceless`, `VK_DRIVER_FILES`), and code signing.

Reviewer check: the `.app` runs on a machine with no Homebrew.

Risk: high, and largely non-technical.

- Licensing and distribution. Shipping Mesa artefacts is a decision for the project, not a build detail.
- The pinned Mesa from S0 is not a release, so someone has to own rebuilding it.
- `DYLD_LIBRARY_PATH` is the obvious way to point Zink at `libvulkan.1.dylib`, and it is the wrong way. It is stripped for signed and hardened binaries, and ExaDev hit a SIGBUS that they fixed by removing it. Use `@rpath` and `install_name_tool` instead.
- Gatekeeper. ExaDev's `zink-probe` branch works around this with a self-signed certificate added as a trusted root, which is fine for CI and not fine for users.

ExaDev's `zink-probe` branch is the best available reference for the mechanics: bundle assembly, dylib install-name rewriting, DMG creation, and a smoke test. Read it, do not fork it.

---

**S8. CI**

Purpose: keep the renderer from silently rotting.

Touches: the macOS workflow from #3134. Add fontconfig, add `engine-legacy` to the target list, and add a headless run that asserts a non-blank frame.

Reviewer check: the workflow goes green and fails if the engine stops drawing.

Risk: medium. GitHub's `macos-26` arm64 runners are real hardware, and ExaDev's `zink-probe` ran there successfully on 2026-06-13 (run 27460811690, all steps green). But **the logs have expired and that workflow prints `GL_RENDERER` without asserting on it**, so a green run does not prove it got hardware rather than llvmpipe. Re-run the probe before designing anything around CI hardware rendering.

---

**The measurement harness, built 2026-08-03**

Four things had to hold before any number was worth recording, and all four now do.

**Focus is acquired and verified, not asked for.** `coding-agents/test-scripts/run-measured.sh <seconds> <logfile>` wraps `run-capped.sh`, so the memory ceiling still applies, and adds the window. The trap: `osascript ... set frontmost` returns success against a process that has no window yet, and the engine has no window for the first six or seven seconds of loading. Trusting the return code declares victory early and the run spends its whole length in the background, which is what happened on the first attempt, 0 of 45 polls with cmux frontmost. So it sets frontmost and then reads frontmost back, retrying for up to 90 seconds until the query confirms the engine's own pid. It re-asserts on a lost poll, and voids the run above one lost poll in twenty. **It cannot check occlusion**, because `NSWindow.occlusionState` is not readable from a shell, so keep the window uncovered as well as focused.

**The scene is frozen, by a widget rather than an engine change.** `coding-agents/test-scripts/widget_perf_probe.lua`, installed into the throwaway game by `install-probe.sh` and removable with `install-probe.sh --remove`. At sim frame 90 it captures the camera state, pauses, and then re-applies that one state every frame, which also holds the camera against edge scroll and a knocked mouse. It also runs `debug 1 0`, which enables the profiler with its on-screen overlay off, and echoes the top timers by share of wall clock every five seconds.

What that buys, measured: **a frozen scene reads 15.55, 15.79, 15.85, 15.96, 15.98, 15.97 and 15.80 fps across seven consecutive 10 second cycles.** The same binary on a live scene in the run before it swung 13.99, 18.02, 19.78, 27.97, 18.52, 15.45, 20.29, 24.49. So the harness turned a factor of two into 2.5%.

**Switches are read per frame, so both sides fit in one run.** `rts/Rendering/GL/DiagSwitches.{h,cpp}`. `SPRING_DIAG_CELLS=<seconds>:<cell>[/<cell>...]` cycles a schedule, where a cell is `-` or a comma list of `flush`, `narrow`, `noflush`, `nopresent`, `finish`, `throttle`. It logs `[diag] cycle=N cell=X fps=Y frames=N over Ts` as each cell ends, and an unknown switch name discards the whole schedule rather than silently measuring the baseline twice. With no schedule set every switch keeps its old meaning as a plain environment variable. The mask is a plain global read inline, because `LuaVertexN` consults it per vertex. Phase dumps get `_cell-<name>` in the filename when a schedule is running, so frames can be scored per side.

**Cells are compared paired.** `cells.py <logfile>` groups by cycle, drops cycle 0 for the loading screen, and runs a two-sided sign test on the paired cycles. It is honest about small samples: five cycles cannot produce a p below 0.0625 whatever the effect.

**Three corrections this forces on the rest of this document.**

- **The resolution is 3024x1832, not 1280x720.** The config is 1512x916 points and the display is 2x, so it is 5.54 Mpixel a frame. Every per-pixel figure written before this was out by a factor of six.
- **Focus is worth less than the 20% claimed, and scene variation was the real cause of the old 14 to 19 range.** Frozen and focused reads 15.9, which sits inside that range rather than above it. The comparison that produced the 20% figure was confounded by the scene, not just by focus.
- **Absolute frame rates still move about 10% between runs**, frozen and focused: one run's baseline was 15.9 and another's was 17.5, same resolution, same map, same freeze frame. **So only interleaved deltas are trustworthy, and absolute levels are not.** One uncontrolled variable is the mouse position, which the camera lock does not pin and which changes what the UI draws by hovering.

**Two engine bugs found on the way, both fixed.** `Spring.GetProfilerTimeRecord` pushed its five results before reading its optional second argument with `luaL_optboolean(L, 2, false)`, so index 2 held the pushed total and **any one-argument call errored** with "bad argument #2 ... boolean expected, got number". And the `frameData` branch called `lua_rawset(L, -3)` without ever creating a table, so index -3 was the pushed `peak_pct`: **the documented sixth return value had never worked either.** Reading the argument first and creating the table fixes both. Verified in a real run: one argument, `false` and `true` all succeed, and `true` returns a table of 128 entries. Platform-neutral and a candidate for a standalone upstream PR.

---

**What bounds the frame rate: render passes, not draw calls and not the present, measured 2026-08-03**

**GPU timing is not available on this driver, so it had to be done indirectly.** `coding-agents/test-scripts/timer_probe.c` settles it without an engine run: Zink on KosmicKrisp does not advertise `GL_ARB_timer_query` or `GL_EXT_disjoint_timer_query`, so `GLAD_GL_ARB_timer_query` is false and `SetGLTimeStamp` does nothing. The engine's debug overlay GPU frame time reads zero here. Worse, `glQueryCounter` still resolves and returns `GL_NO_ERROR` with both timestamps zero, so anything that skips the extension check gets a confident 0.00ms. The probe also confirms an unissued query returns `GL_INVALID_OPERATION`, which means `CalcGLDeltaTime`'s `while (!res)` busy-wait would spin forever if it were ever reached before the first `CGame::Draw`. It is safe today only because the extension flag short-circuits it first.

**The frame is GPU bound and the CPU is idle.** On the frozen scene, `Misc::SwapBuffers` is 88 to 95% of wall clock, about 59ms of a 63ms frame. Every CPU-side draw timer together is about 8%. The CPU is blocked in the present's `glReadPixels`, which waits for the GPU to finish the frame.

**The cost is linear in pixel count.**

| drawable | frozen fps | SwapBuffers | CPU `Draw` |
|---|---|---|---|
| 3024x1832, 5.54 Mpixel | 15.9 | 94% | 5.0ms |
| 1512x916, 1.38 Mpixel | 65.0 | 88.9% | 1.19ms |

4.0x the pixels for 4.09x the time. Both runs frozen and focused, and the quarter-resolution one is stable at 64.3 to 66.0 across thirteen intervals. Per draw call and per render pass overhead do not scale with pixels, so neither of those is the bound. Note that CPU-side `Draw` scaled too, 5.0ms against 1.19ms: that is back pressure, draw calls blocking when the driver's queue is full, not CPU work.

**It is not the present path.** Interleaved, `-` against `nopresent,finish`, which keeps a per-frame sync but drops the readback and the present: the no-readback side was faster in 8 of 8 cycles by about 3%, roughly 2ms of a 67ms frame. That matches S3's 1.4ms to copy plus 0.5ms to present. That run was void on focus, but a delta measured between cells of one run is not exposed to focus, which is constant across them.

**It is not raw fill either.** `coding-agents/test-scripts/fill_probe.c` measures the driver's own fill rate at the engine's resolution: 6.7 Gpixel/s for one opaque full-screen quad, 95 Gpixel/s for 64 blended ones, which is tile-local and never touches DRAM. The engine manages 0.09 Gpixel/s. The driver is between 300 and 1000 times faster than the engine achieves, so overdraw alone cannot explain it.

**It is render pass breaks, and one costs a full attachment store and reload at memory bandwidth.** Same probe, same 64 blended full-screen quads, in one pass and then in 64:

| | 3024x1832 | 1512x916 |
|---|---|---|
| 64 quads, one render pass | 3.72ms | 1.56ms |
| 64 quads, one pass a quad | 20.48ms | 4.24ms |
| implied cost of a pass break | 0.266ms | 0.043ms |

A pass break scales with attachment size, so it is not a fixed overhead. 0.266ms for a 22MB store plus a 22MB reload is about 166 GB/s, which is this machine's memory bandwidth. The pass break is the store and the load.

**That closes the model, and two independent numbers fit it.** The engine's 57ms divided by 0.266ms is roughly 200 render passes a frame, which explains the exact linear scaling in pixels. And the flush A/B agrees: interleaved on the frozen scene, baseline 17.51 against 16.19 with a `glFlush` before every immediate-mode batch, faster in 9 of 9 cycles, median delta 1.25 fps, sign test p = 0.0039. That 7.5% is 4.7ms, which is about 18 pass breaks, so this scene issues about 18 immediate-mode batches a frame. **This also settles the contradiction in this document about the flush: it is neither the 1.67x once claimed nor the nothing later measured, it is 7.5%.**

**`MacHiDPIRendering = 0` is not the win it looks like, measured 2026-08-03.** The config option already exists and its description promises "a quarter of the pixels to draw and read back", which is true and does not help. Three frozen focused runs:

| contentsScale | render target | window on screen | fps | frame |
|---|---|---|---|---|
| 2.0, native | 3024x1832, 5.54 Mpixel | 5.54 Mpixel | 15.9 | 62.9ms |
| 2.0, native | 1512x916, 1.38 Mpixel | 1.38 Mpixel | 65.0 | 15.4ms |
| 1.0, non-native | 1512x916, 1.38 Mpixel | 5.54 Mpixel | 21.3 | 47.0ms |
| 1.0, non-native | 756x458, 0.35 Mpixel | 1.38 Mpixel | 24.9 | 40.2ms |

So turning it off gives 1.34x, not the 4x the pixel count predicts. The last row was run to discriminate three explanations, which predicted 85, 23 and over 150 fps. **It is a fixed cost: fit the two non-native rows and it is 37.9ms a frame plus 6.6ms a Mpixel rendered, so the render size barely matters once you are on that path.** 38ms is not a multiple of the 120Hz refresh interval, and it sits inside `Misc::SwapBuffers` at 91.7%, so it looks like a stall rather than work. The mechanism is not established. The obvious suspect is the compositor having to upscale a layer whose `contentsScale` does not match the display's.

**The native-scale rows fit better than the first write-up of them claimed.** 11.4ms a Mpixel through the origin, no fixed term. But note the confound that the fourth row exposes: those two rows changed the window's size on screen and the render size together, so on their own they could not attribute the 4x to either. The non-native rows are what rule the screen-area explanation out, because shrinking the window there bought only 6.8ms.

**Shrinking the pbuffer alone does not help either, and that is the finding that breaks the model, 2026-08-03.** `MacRenderScale` was added to test it: the layer keeps the display's `contentsScale` and only the pbuffer shrinks, so the present's linear sampler does the upscale into a full size drawable. Verified working from the log, `MacRenderScale draws 1512x916 into a 3024x1832 drawable`, no GL errors. It gives **21.5 fps, against 21.3 for `MacHiDPIRendering = 0` and 15.9 for the default.** So both routes to fewer rendered pixels land in the same place, and neither is the 4x that the native-scale pair predicted.

| render | drawable | window on screen | fps | frame |
|---|---|---|---|---|
| 5.54 | 5.54 | 5.54 | 15.9 | 62.9ms |
| 1.38 | 1.38 | 1.38 | 65.0 | 15.4ms |
| 1.38 | 1.38 | 5.54 | 21.3 | 47.0ms |
| 0.35 | 0.35 | 1.38 | 24.9 | 40.2ms |
| 1.38 | 5.54 | 5.54 | 21.5 | 46.5ms |

Rows 1, 2, 3 and 5 fit `3.9ms x rendered Mpixel + 7.5ms x window Mpixel on screen`. **Row 4 does not fit anything.** It renders four times less than row 2 at the same screen area and is 2.6x slower. No linear model covers all five.

**And the present is not where the missing time is, tested twice.** At render scale 0.5, `-` against `nopresent,finish` gives 21.64 against 21.86 fps, faster in 2 of 8 cycles, p = 0.29. So removing the readback and the present entirely changes nothing, the same answer as the 3% at full resolution.

**That leaves about 30ms a frame that is not our rendering, not our present and not our CPU.** The same 1.38 Mpixel of GL work costs 15.4ms with a physically small window and 46ms with a full size one, present removed in the slower case. Nothing inside the engine's own frame differs between those two.

**Settled: the cost appears whenever the frame is not displayed 1:1, and it is about 38ms flat, 2026-08-04.** The discriminating run was render scale 0.5 with the small window, so 0.35 Mpixel rendered into a 1512x916 drawable at the display's own scale. The screen-area model predicted 85 fps. It measured **25.2 fps**, which is the same as the 24.9 of the row that fitted nothing. Sorting all six runs by whether the rendered image reaches the display 1:1:

| # | render | drawable | window on screen | 1:1 | fps | frame |
|---|---|---|---|---|---|---|
| 1 | 5.54 | 5.54 | 5.54 | yes | 15.9 | 62.9ms |
| 2 | 1.38 | 1.38 | 1.38 | yes | 65.0 | 15.4ms |
| 3 | 1.38 | 1.38 | 5.54 | no | 21.3 | 47.0ms |
| 4 | 0.35 | 0.35 | 1.38 | no | 24.9 | 40.2ms |
| 5 | 1.38 | 5.54 | 5.54 | no | 21.5 | 46.5ms |
| 6 | 0.35 | 1.38 | 1.38 | no | 25.2 | 39.7ms |

- **1:1, rows 1 and 2**: 11.3ms a Mpixel, no fixed term.
- **not 1:1, rows 3 to 6 plus the 0.75 run**: about 38ms flat plus 4.7ms a Mpixel rendered.

**The window's area on screen does not enter it.** Rows 4 and 6 have a quarter the screen area of rows 3 and 5 and sit on the same line. That is what kills the WindowServer contention hypothesis below.

**And it is not about who does the scaling.** Rows 3 and 4 break 1:1 in the compositor, by dropping `contentsScale` with `MacHiDPIRendering = 0`. Rows 5 and 6 break it in our own present shader, by shrinking the pbuffer with `MacRenderScale` while the layer stays at the display's scale. Two unrelated code paths, the same 38ms. So the trigger is the final mapping to the display not being 1:1.

**Consequences.**

- **Render scaling is a dead end on this present path.** Every route to fewer pixels breaks 1:1 and pays the 38ms, which is why none of them delivered the 4x. As the render size goes to zero the frame cost goes to 38ms, hence the 24 fps ceiling. `MacRenderScale` is still a net win at the default window, 62.9ms to 46.5ms, because the render saving exceeds the penalty, but the penalty eats most of it.
- **The 4x is real and only available at 1:1**, which means a physically smaller window. Row 2 is the same engine doing the same work at 65 fps.
- **The target is therefore the 11.3ms a Mpixel at 1:1**, which is the render pass cost above, and the 38ms is a separate penalty that only appears if you try to avoid it.
- **The 38ms is worth attributing and reporting.** It is a flat per-frame cost for presenting a non-1:1 layer on this stack, it shows up through two different paths, and nothing in the engine's own frame explains it. Metal System Trace is the tool. A 1x external display would confirm it from the other direction, since nothing anywhere would need scaling.

**Superseded hypothesis, kept because it was wrong in an instructive way: contention with WindowServer.** Its compositing cost scales with the window's area on screen and it keeps compositing whether or not we present, which would explain why removing our present changes nothing, and why row 4, where the compositor also has to magnify, is worse than its render size predicts. Three ways to test it, none tried. A 1x external display, so no scaling is needed anywhere. Sampling WindowServer's own GPU time with Metal System Trace. Or comparing an occluded window, remembering that macOS throttles occluded apps and that confound is what voided earlier runs.

**`MacRenderScale` is fully characterised, and there is a 41.6ms floor.** Three frozen focused runs at the default 1512x916 point window:

| scale | render | fps | model |
|---|---|---|---|
| 1.0 | 5.54 Mpixel | 15.9 | 15.8 |
| 0.75 | 3.12 Mpixel | 18.9 | 18.6 |
| 0.5 | 1.38 Mpixel | 21.5 | 21.3 |

`3.9ms a Mpixel rendered + 41.6ms` predicts all three within 2%, so this is a line rather than three loose numbers, which matters because the 0.75 gain of 1.19x is smaller than the 10% run-to-run drift and could not have been trusted on its own.

**So 24 fps is the ceiling at this window size** whatever the scale, and the 41.6ms floor is the only thing left worth attacking. On quality, 0.75 was judged clearly better than 0.5 by eye and gives 1.19x, which makes it the usable point if anyone wants the trade. A per-target scale, world reduced and UI at full, would remove the text cost but needs separate render targets.

**Do not leave a non-default `MacRenderScale` in the test config.** A stale non-default in `~/dev/spring-testdata/springsettings.cfg` is exactly the shape of the contaminated negative control that already cost this project a session.

**What ExaDev already has, checked 2026-08-03.** Two performance commits on `macos-layer`, and most of it is already here.

- `82c22270f3` P and E core split plus a QOS hint for sim workers. **Already in this tree**, `CpuTopology.cpp` reads the `hw.perflevel*` keys and `Threading.cpp` calls `pthread_set_qos_class_self_np`. It cannot help a draw bound frame anyway, since the CPU is idle, but it should matter on a busy sim.
- `c990c4e301` viewport must match the FBO, not the drawable. **Already satisfied**, differently and better: S6 made `winSize` come from `MacEGL::GetSurfaceSize()` and derives `pixelsPerPoint` from the ratio.
- `931f677cdd` async PBO readback, downsample and timing knobs. **Not ported.** Its claim is 3x in busy scenes, 41ms sync against 13ms with the PBO, on the strength of the PBO hiding the `glReadPixels` pipeline drain behind a frame of latency. That conflicts with S3's measurement here that `glReadPixels` into a PBO still blocks on this driver, so it buys no run-ahead and costs 0.7ms a frame extra. It also conflicts with the `nopresent,finish` result, which kept the drain and removed only the copy and present for 3%. Both could still be true, since their number is a different scene on June's post-Metal4 Mesa, and the S3 figure was taken on the main menu, which draws almost nothing. **It is one interleaved run to settle, once the knob is ported: `-` against `nopbo`.**

**The map is only a fifth of the frame. Empty Mod on the same map runs at 80 fps, 2026-08-04.** Same map, same 3024x1832 drawable, same engine, same driver:

| game | fps | frame | per Mpixel |
|---|---|---|---|
| SplinterFaction 0.1.78, frozen | 15.9 | 62.9ms | 11.3ms |
| Empty Mod | 80 | 12.5ms | 2.3ms |

The control run is formally void on focus, 5 of 45 polls, but the losses start at poll 27 and cycles 1 to 4 precede them at 80.0, 82.0, 85.1 and 80.9 fps. The worst cycle in the whole run is 64. `coding-agents/test-scripts/empty-mod-valles.tdf` is Empty Mod on the SplinterFaction script's map, and `run-capped.sh` now takes `SCRIPT=` so a run can be pointed at it.

**Empty Mod is a floor, not a fair control.** It has no units and no LuaUI, so it cannot separate "the game's interface and effects" from "drawing units". What it does show is that the map costs 12.5ms of the 62.9ms, and that the renderer's own floor is 2.3ms a Mpixel rather than the 11.3ms attributed to it above. **The 11.3ms figure is SplinterFaction's scene, not this renderer's.**

**The detail that narrows it: the frozen SplinterFaction scene has `units=1`.** One commander. So the missing 50ms is not unit count, and it is not the map. It is the game's LuaUI, shaders, model materials and per-frame effects with a single unit on screen. If render passes are the mechanism, roughly 190 of the inferred 200 a frame come from there.

**A start script cannot narrow it further.** `script.tdf` sets the game, map, teams, start position type and modoptions, which is all `splinterfaction-fixed-start.txt` and `empty-mod-valles.txt` do. It cannot disable a game's LuaUI or its gadgets. Only the console action `LuaUI` can (`UnsyncedGameCommands.cpp:2933`), and there is no config key for it.

**So the bisect that would settle it**, and it is one run: the probe freezes the scene, then issues `luaui disable`. That removes the probe with it, but the freeze survives, because pause is engine state and nothing is left to move the camera, and `SPRING_DIAG_CELLS` logs the frame rate from the engine rather than from Lua. Running Metal Factions or MCL on the same map is the other half, a real game with real units against which SplinterFaction can be judged.

**Settled: it is the game's LuaUI, in two unrelated games, 2026-08-04.** The probe now freezes the scene and then issues `luaui disable` a fixed time later, which removes every widget and the probe with it. The freeze survives, because pause is engine state, and the frame rate keeps coming from `SPRING_DIAG_CELLS`, which does not go through Lua. Each row is one run, with the cycles either side of the switch as the two sides.

| game | LuaUI on | LuaUI off | LuaUI costs | share of frame |
|---|---|---|---|---|
| SplinterFaction 0.1.78 | 15.21 fps, 65.7ms | 59.60 fps, 16.8ms | 48.9ms | 74% |
| Metal Factions v2.58 | 8.97 fps, 111.5ms | 63.17 fps, 15.8ms | 95.7ms | 86% |
| Empty Mod, which has no LuaUI at all | | 59.15 fps, 16.9ms | | |

**Metal Factions is worse than SplinterFaction, not better.** 111.5ms a frame against 65.7ms, and 86% of it is its own widgets. Its LuaUI has little in common with SplinterFaction's and it has its own faction picker, so this is a genuinely independent second measurement rather than a second look at the same content. That removes the weakness noted in `test-scripts/README.md`, that every LuaUI finding rested on one game.

**The three LuaUI-off figures agree at about 16ms**, which is the check that they mean anything. With the widgets gone all three draw the same map and at most one unit, so they should agree, and they do.

**What survives the switch, and why the split is clean.** `cus_gl4.lua` and `api_pbr_enabler.lua` are LuaRules gadgets, so unit materials and PBR are still in place afterwards. What goes is the interface plus the full-screen effect widgets, `gfx_deferred_rendering.lua`, `gfx_bloom_shader_deferred.lua`, `gfx_outline_shader.lua`, `gfx_guishader.lua` and `gfx_light_effects.lua`.

**The scene is now proved rather than inferred.** `install-probe.sh --shots 25,60,90` installs a LuaRules gadget that calls the engine's screenshot action at fixed times. A gadget rather than a widget because the frame that most needs proving is the one after `luaui disable`, and no widget survives it. The Metal Factions shots show the game running properly at its own reported `FPS: 9` with an AVEN commander, and then the same terrain and the same selected commander with the engine's own build menu in place of the game's. That is the check that the false 2.5x of the feature sweep would have failed.

**Driving the screenshot from outside the engine does not work.** A synthetic `Ctrl+Alt+S` through System Events, against a `uikeys.txt` binding, produced no files at all, and the log could not say whether the binding failed to parse, the keystroke never arrived or macOS ate it. It depended on an accessibility grant, the keyboard layout and macOS not claiming the combination. The gadget depends on nothing outside the engine. Do not revisit the keystroke route.

**`VSync` was reaching nothing at all on this path, fixed 2026-08-04.** The setting is applied through `SDL_GL_SetSwapInterval`, and SDL owns neither the context nor the present here, which `GlobalRendering.cpp:892` already recorded. A `CAMetalLayer` synchronises with the display by default, so the engine was pinned to the refresh rate whatever the config said. `CVerticalSync::SetInterval` now calls `MacMetalPresent_SetVSync`, and both `MacMetalPresent_Init` and the setter log the state so it cannot apply silently. This never showed on the built-in panel because nothing ever approached 120 fps.

**A 60Hz display is not a ceiling, and the claim that it was is retracted.** Three workloads reading 59.15, 59.60 and 63.17 fps looked like the refresh interval showing through, and it is not. Empty Mod in a quarter-size window runs at **103.4 fps** on the same display with display sync off. Those three agree because they genuinely cost about 16ms each.

**But the display does change the constant.** Empty Mod costs 16.9ms on the external 5120x2880 panel against 12.5ms on the built-in one, same engine, same map, same window in pixels. Fitting the two window sizes on the external display gives **7.3ms fixed plus 1.74ms a Mpixel**, so most of the engine's own frame at this window size is fixed cost rather than fill. Absolute figures from before 2026-08-04 were taken on the built-in panel and are not comparable with anything after it.

**Immediate mode is the live hypothesis for where the LuaUI time goes, untested.** All Lua drawing is real immediate mode on this path: `LuaOpenGL.cpp:285` puts every vertex through `glVertex4f` and `myGL.cpp:568` wraps `glBegin`. Zink has to turn each batch into a buffer upload and a draw, and the flush A/B above already showed immediate-mode batches costing 7.5% of the frame on SplinterFaction, which was read as about 18 batches a frame. That reading assumed one pass break a batch, so it bounds the batch count and not the cost of immediate mode itself.

**The cheap test is a counter, not another A/B.** Count `glBeginBatch` calls and vertices per frame and log them per cell, the same way the plan already proposes counting `glBindFramebuffer`. Tens of thousands of batches a frame would settle it. Twenty would kill it. Note that "the CPU is idle" is not evidence against this, because a main thread blocked waiting on the GPU looks idle whatever the driver thread is doing, and that has never been checked separately.

**There are two batching defects, not one, and each needs its own mitigation, settled 2026-08-04.** This corrects the conclusion above that uniform vertex arity replaces the flush. It does not. Measured with `SPRING_DIAG_CELLS=10:-/flush/normalise` interleaved in one run, scoring stray pixels per frame:

| cell | frames with the artefact | median stray pixels |
|---|---|---|
| `-` | 31 of 31 | 28 |
| `flush` | **0 of 30** | **0** |
| `normalise` | 27 of 29 | 42 |

The flush suppresses it completely. `normalise` does nothing for it whatsoever. Matching each mitigation to what it actually fixes:

| defect | symptom | fixed by |
|---|---|---|
| consecutive batches merge | the minimap's stray lines, the amplifier's strays | `flush` only |
| a batch inherits the previous one's attributes | the build menu's stray polygon | `normalise` only |
| varying vertex arity | the original load screen corruption | arity, already the default |

**So the engine currently ships one mitigation out of three.** Arity is on by default, the flush is opt-in behind `SPRING_BATCH_FLUSH`, and the attribute set was designed at line 893 and never written until today. Batch merging is therefore unfixed in normal play, which is what the stray line in SplinterFaction's minimap has been all along.

**The arity half was itself incomplete.** It normalised vertices and nothing else. `gl.TexCoord` and `gl.MultiTexCoord` both emit 1f, 2f, 3f or 4f depending on how Lua called them, so texture coordinate arity varied across batches exactly as vertex arity used to. Both now widen to 4f, which is semantics-preserving since `glTexCoord2f(x, y)` is defined as `(x, y, 0, 1)`. Nothing else can vary: colour is always emitted as 4f, normals and secondary colour have no form other than three components, a fog coordinate is one float and an edge flag is a boolean.

**`SPRING_BATCH_NORMALISE` restates colour, texture coordinate and normal immediately after every `glBeginBatch`,** so a batch does not depend on what the previous one left current. Colour was already shadowed in `LuaOpenGL::color`, the other two are new. It is shadowed rather than read back with `glGet`, because a `glGet` per begin would stall the pipeline hundreds of times a frame.

**Amplification is what made the merging defect measurable, and it is the transferable method here.** In real content the artefact appears in about 8% of frames, so a 240 sample run yields nine events and baseline against flush came out 9 of 115 against 4 of 115, p about 0.25. `widget_loop_amp.lua` draws 2000 identical `LINE_LOOP` batches a frame, the same shape `game_metal_spot_minimap_drawer.lua:59` uses one of per metal spot. That puts the artefact in 100% of frames with a graded magnitude, and the question stops being "did it happen" and becomes "how much", which is why 30 samples a side was then overwhelming.

**Two scoring mistakes, both of which produced confident zeroes, do not repeat them.** Scoring by excess over the run's minimum fails if any frame was captured before the widget drew, because that frame becomes the reference and every other frame scores the entire drawing as excess: one run read about 10,000 in every cell. It fails again, in the opposite direction, once the artefact is in every frame, because then there is no clean frame to subtract and the run reads zero everywhere while the screen is visibly full of stray lines. `minimap_score.py` now counts absolutely, using the fact that the grid is regular and known, so every legitimate pixel lies at the circle radius from a node. A histogram over one frame put 79,711 pixels on the ring and 32 off it.

**Compiled display lists are worth 5.1x and are still blocked, 2026-08-04.** `SPRING_COMPILE_LISTS=1` ignores the deferral and compiles as normal. Metal Factions goes from 8.97 to 46.0 fps, 111.5ms to 21.7ms, and Lua vertices a frame fall from 790,748 to 147. SplinterFaction roughly doubles, 15.2 to 30 fps. ~~`normalise` clears the build menu polygon that compilation reintroduces.~~ **Retracted, it does not, see below.** What still blocks it is the resource bar's research readout, which is lost under compilation and is **not** a display list problem: `gui_static_resourcebar.lua:773` draws that panel immediate-mode outside any list, and the numbers that are inside the lists render correctly.

**`SPRING_BATCH_NORMALISE` must not default on. On top of the flush it reintroduces batch merging, measured 2026-08-04.** One interleaved run, four cells, 53 shots, scored absolutely against the known grid:

| cell | mitigations | frames with strays | stray pixels |
|---|---|---|---|
| `-` | arity + flush | **0 of 13** | **0** |
| `normalise` | arity + flush + normalise | **14 of 14** | 12 |
| `noflush` | arity | 7 of 15 | 10 |
| `noflush,normalise` | arity + normalise | 9 of 11 | 19 |

The default is clean and adding `normalise` to it is not. The regression is deterministic, exactly 12 stray pixels in every frame where merging is otherwise random per pair, which is what said to look at the pixels rather than the summary. They are two short segments joining a magenta circle to a cyan one, interpolating between the two colours, which is only possible if vertices from two batches ended up in one primitive. Screenshots of the same coordinates in the two cells confirm it: clean under `-`, joined under `normalise`.

**A hypothesis for why, untested.** `LuaOpenGL::BeginBatch` emits colour, texture coordinate and normal after `glBegin`, which widens that batch's vertex format against a batch that emits none, and a vertex format that varies across batches is the original trigger for this whole family. Engine batches through `glBeginBatch` are not normalised and `gl.Rect` never was, so the mixture is exactly the shape that reproduces.

**The defect `normalise` was written for does not appear on the live path at all.** The amplifier now alternates the batch colour under `--alt`, so a batch that inherits the previous one's attributes draws a whole circle in the wrong colour, about 40 pixels against a threshold of 10. Zero wrong circles in all 53 shots, in every cell including `noflush`. So attribute inheritance is still a compiled-list phenomenon only, and nothing on the live path needs it.

**Scope of that null result.** The amplifier varies colour and nothing else, deliberately, so that geometry and vertex format are identical between adjacent batches. It therefore cannot see an inherited texture coordinate or normal, which need a bound texture or lighting to become visible, and it does not model the build menu's stray polygon, which is geometry. It says colour inheritance does not happen live. It does not say `normalise` is unnecessary under compilation, where the artefact was actually observed.

**The instrument was proved before it was believed.** A scorer that reads zero proves nothing until it has been shown to read non-zero on an image that definitely contains what it counts. Recolouring circles in a real screenshot and rescoring gives 0 unmodified, 40 for one wrong circle, 120 for three and 76,700 for all of them. The first attempt at that control swapped red and blue, which turns cyan into yellow and leaves magenta alone, so every case read zero and the scorer looked broken when the control was.

**The research readout is unblocked. A `glFlush` after `glCallList` restores it, and it costs nothing, 2026-08-04.** `SPRING_LIST_FLUSH=1` adds the flush and the resource bar's research value and rate come back under `SPRING_COMPILE_LISTS=1`, verified against the unmodified widget. Frame rate with the flush is 29.4 fps against 29.5 without, ten cycles each, which is inside run to run drift. So the last artefact blocking compiled lists has a fix that is free.

**How it was narrowed, because the answer is a driver defect and that claim needs its evidence.** Each step is one run:

| step | result |
|---|---|
| the same panel drawn before the lists as well as after | the copy before renders, the copy after does not |
| a copy after each of the three lists | fine after the background list, lost after the static and dynamic ones, which are the two that contain text |
| the widget's other font, which no list has compiled | equally lost, so it is not the font object |
| a string the static list draws every frame, printed live | equally lost, so it is not a missing glyph in the atlas |
| the font's unbuffered path, with no `Begin`/`End` | equally lost, so it is not the shared render buffer |
| every piece of GL state read at the lost draw | framebuffer 0, `GL_BACK`, full viewport, colour mask 1111, scissor test off, no stencil, blend on, depth off, right texture, right program, matrices identical to the working run, `glGetError` clean, 42 indices submitted for 7 characters |

So the engine issues a correct, well-formed draw and the driver does not render it. Everything on the engine side is excluded by measurement rather than by argument. It is the same shape as the immediate-mode batching defect, a state transition the driver mishandles with no GL error raised, and it answers to the same mitigation.

**Two engine theories died on the way, do not retry them.** The atlas upload guard at `glFontRenderer.cpp:214` skips the upload whenever `GL_LIST_INDEX` says a list is being recorded, and that guard is sound here: instrumented against the engine's own compile flag it read `dl=267` while compiling, so the driver reports list state correctly and the atlas is not involved. And the glyphs are present regardless, since a string the lists render every frame is equally invisible when drawn live.

**Method note.** The engine cannot tell which Lua draw is which, so the widget echoed a marker either side of the panel and the markers and the font log were read interleaved by timestamp. That is what identified the lost draw as the one submitting 42 indices, and it is worth reaching for before adding more instrumentation.

**Compiled display lists cannot ship. The build menu's content area is destroyed and no mitigation reaches it, settled 2026-08-04.** This retracts the earlier claim above that `normalise` clears the artefact compilation reintroduces. It does not.

The artefact only appears while the menu is scrolling, because `gui_static_buildordermenu.lua` marks itself dirty on every scroll step and `BakeStaticLayer` deletes and recompiles `buildList`. A frozen scene compiles it once and never again, so a frozen A/B measures two clean frames and reads as "no artefact". Driving the scroll from the widget's own `Update` reproduces it at will.

Recolouring every element of the menu in a distinct opaque colour is what identified it. The panel chrome is correct, red shell, blue fill, yellow accent, pink scrollbar. What goes white is exactly the scissored content region, where the headers, button bodies and info strips should be. White is GL's default current colour, so those batches lose their colour and their bounds together and fill the whole scissor rect, which is why it reads as the back of the panel.

| configuration | frames with the content area destroyed |
|---|---|
| compiled, `-` | 15 of 35, and 5 of 36 in a second run |
| compiled, `normalise` | 6 of 36 |
| compiled, `listflush` | 10 of 37 |
| **deferred, what ships** | **0 of 37** |

Neither mitigation is distinguishable from baseline, and the baseline rate itself swings from 14% to 43% between runs, which is why both sides of a comparison have to sit in one run. Deferral is clean.

**Why no flush can fix this, and why the deferral exists.** `glBeginBatch`'s flush executes during compilation and is never recorded, so a list carries no per-batch separation at all, and nothing at replay can insert any. `SPRING_LIST_FLUSH` flushing either side of the compile and after `glCallList` does not help, because the batches inside the list were already merged when it was built. So the 5.1x on Metal Factions is not available on this driver: compiled lists render the game's own interface wrong.

**What ships is unchanged: uniform arity plus the per-batch flush plus the deferral.** `SPRING_BATCH_NORMALISE` stays off, `SPRING_COMPILE_LISTS` stays off, and `SPRING_LIST_FLUSH` only matters if compiled lists are ever revisited.

**Method notes that cost time today.** A game with factions picks one at random per run unless the start script sets `side`, and two runs then differ in commander, unit set and any number the UI displays, which is not a rendering difference. Fog of war means world-space overlays cannot be compared by brightness unless the scene is identical. And `install-probe.sh --move` sends the unit to the middle of the map and tracks it, which removes the faction from the camera framing entirely and exercises overlays a frozen scene never draws. Frame rates in that mode are void.

**Start here next, in this order.**

1. ~~**Decide whether the flush goes back on by default.**~~ **Done.** The flush ships on. `normalise` stays opt-in, because on top of the flush it reintroduces merging, see above. What ships is arity plus the flush.
2. ~~**Find why compiled lists lose the research readout.**~~ **Done.** `SPRING_LIST_FLUSH=1` restores it for no measurable cost. It does not matter in practice, because compiled lists cannot ship for a separate reason, see above.
3. **Find which widgets cost the time.** `luaui disable` is all or nothing across 167 widgets in SplinterFaction and 69 in Metal Factions. Two smaller leads from watching a live game: the build menu costs about 10 fps reliably when opened, and deselecting the starting unit gains about 10.

**Done, do not repeat.** `luaui disable` after the freeze, twice, in two games. Metal Factions, which now has a start script and runs. Whether a 60Hz display caps the frame rate, which it does not. Whether uniform arity alone fixes batch merging, which it does not. Whether `normalise` fixes batch merging, which it does not, and whether it should default on, which it should not.

Not worth retrying, all measured: the present path, cheaper CPU-side drawing, raw fill, render scaling, `MacHiDPIRendering = 0`, and the engine feature sweep.

**The engine-side feature sweep was negative, which agrees with that.** `AllowDeferredMapRendering = 0`, `AllowDeferredModelRendering = 0`, `CubeTexSizeReflection = 128`, `CubeTexGenerateMipMaps = 0` and `Water = 0` together give 17.0 fps against 15.9, which is inside the run-to-run drift. So the cost is in the game's own drawing rather than in engine features it switches on.

**Two traps this sweep walked into, both worth not repeating.**

- **SplinterFaction overrides these config values at runtime.** `gfx_deferred_rendering.lua:306` sets both `AllowDeferred*` keys to 1 and `api_pbr_enabler.lua:72` sets the cubemap to 1024 with mipmaps. That is why the test config kept reappearing with those values. It is a partial override, which is worse than a clean one, because `ModelDrawer.cpp:22` reads at init while `SMFGroundDrawer.cpp:484` is a `static const bool` read on the first draw, after LuaUI has loaded. Comment out those four lines rather than disabling the files, and the deferred widget then takes its own self-disable path.
- **Disabling `api_pbr_enabler.lua` outright stops units spawning and raises the faction picker.** The first sweep attempt reported 40.4 fps, a 2.5x win, which was an empty map with a menu over it. Nothing in the log said so and only the user's eyes caught it. The probe widget now logs `units=N view=WxH` on the freeze line and on every report, so a frame rate measured on the wrong scene is obviously void instead of quietly wrong.

**What this means for the port.** Performance is a content problem, and BAR-family content is explicitly out of scope in section 2b. The honest headline is 80 fps on an empty game at full Retina resolution and 16 fps with SplinterFaction's full content. If anyone wants to pursue it, the next bisect is within the game: LuaUI off separates its interface and effects from its unit materials.

**So if the remaining cost is render passes, they are the game's.** Two directions, and they are not exclusive:

- Engine side: fewer framebuffer switches per frame. Nothing has counted them yet. A counter on `glBindFramebuffer` logged per cell is the cheap next step and it names the engine's own switches, which is what a fix would have to act on.
- Driver side: a tiler should clear or discard rather than load and store when the previous contents are not needed. If Zink or KosmicKrisp is conservatively using load and store actions on every pass, that is a driver fix with a very large payoff, and it is the same shape of finding as the memory leak. Worth checking against Metal's `MTLLoadAction` and `MTLStoreAction` before reporting anything.

**What is not worth retrying.** The present path, at 3%. Cheaper CPU-side drawing, since the CPU is already idle. Raw fill, since the driver is three orders of magnitude faster than the engine achieves. And the unsynced `nopresent` cell with no `glFinish`, which queues frames for the whole cell and makes the next synced cell absorb the entire backlog in one frame, contaminating exactly the baseline it is being compared against.

## 2b. Scope, set 2026-08-03

**Not being pursued right now: S7 packaging and S5 geometry shaders.** No `.app` bundle, no code signing, no Gatekeeper work, and no catering specifically to BAR content. So distributable here means the engine builds, renders correctly, and the changes are in a state someone else can pick up, not a signed bundle a player double clicks.

That reduces the Mesa problem considerably. Without bundling there is nothing to license or ship, just a documented build recipe and the upstream issue.

What is left on the path, in order:

1. **Settle PR #3169.** It is the only macOS work in front of upstream reviewers and it implements the flush, which uniform vertex arity has now superseded. The two have never been compared on the same Mesa, so test the arity fix on post-Metal4 before touching the PR.
2. **Make the renderer stack reviewable.** S1 to S6 and borderless are local branches and nothing has landed. This is the real barrier to anyone else running the port.
3. **The freeze.** It affects anyone who runs the engine whether or not it is packaged, and a memory ceiling cannot catch it.
4. ~~**A measurement harness that produces claims that survive.**~~ **Done, see "The measurement harness" above.** Focus is acquired and verified, the scene is frozen, switches are read per frame so both sides fit in one run, and cells are compared paired. A frozen scene reads to 2.5%.

**Measurements have to be scored, not eyeballed.** Almost every number produced before the harness had to be retracted: the frame rates were measured on a backgrounded window, the negative control had half the fix compiled into it, and three probe results were mislabelled by patches that silently did not apply. The one artefact result that held up was interleaved, same scene, immediate visual check, and it is still only six samples at about p = 0.016.

`summarise.py` scores red, magenta and lavender pixels per frame, which is how the 11 of 89 against 0 of 31 figure was produced. Phase dumps now carry the cell name, so an interleaved artefact A/B can be scored per side rather than pooled. That has not been run yet: the harness was spent on the frame rate question first.

## 3. What cannot go upstream

**S5, the geometry-shader guard.** Three reasons, and they compound.

The capability cannot be detected. Zink advertises geometry shader support that KosmicKrisp cannot deliver, and `GL_MAX_GEOMETRY_OUTPUT_VERTICES` returns a positive value regardless. A platform-neutral PR would need a runtime probe that does not exist. What is left is "if `__APPLE__`, strip the geometry stage", which is a hardcoded platform assumption in shared shader code. A reviewer would be right to reject it.

Stripping is not fixing. A shader whose geometry stage is removed links and draws the wrong thing. Making it correct needs no-geometry-shader variants of the affected widgets, which live in Beyond All Reason content. The engine change alone has no defensible standalone benefit.

The lie may not be permanent. It is a gap in Zink and KosmicKrisp, both under active development. Upstreaming a workaround for someone else's temporary bug commits Recoil to carrying it after the bug is gone.

Keep it on the integration branch, clearly commented as a workaround for a specific Mesa and KosmicKrisp state, with the version it was observed against.

The nearest thing to a defensible upstream change is unrelated to macOS. `LuaShaderContainer.cpp` and `LuaShaders.cpp` compile geometry shaders through two separate code paths. Unifying that is a real cleanup and could go upstream on its own merits, which would then give S5 one place to guard instead of two. Worth doing first if you want to minimise the unupstreamable surface.

## 4. Made redundant by upstream

Do not port these.

**Already merged upstream.** glad GLX loader guard (#3071), SpringApp X11 include guard (#3065), legacy X11 non-Apple guard (#3045), SDL 1.2 SDLMain removal (#3034), CUtils scandir const (#3047), smmalloc INLINE leak (#3026), SafeUtil includes (#3025), SolLua `sol::lua_nil` (#3024), LuaTextures failure logging (#3023), `<algorithm>` includes for libc++ (#3074), archive narrowing and aggregate init (#3076), LuaMenu LuaSocket from base VFS (#3078), MemPoolTypes thread-id cast (#3040), float3 streflop include (#3028).

**Redundant under Homebrew GCC.** The entire AppleClang and libc++ tail of #2991: `Cpp23Compat.hpp`, the assimp `.inl` edits, `smmalloc_generic.cpp`, `float3.h`, `MemPoolTypes.h`, `testMutex.cpp`, the gflags dual-namespace hack, and `GLTFParser.cpp`'s `INLINE` undef. Proof: `engine-legacy` builds with none of them applied. The same applies to ExaDev's `macos-layer` build fixes, which target AppleClang on macos-15.

**Redundant given current master.** #2991 wraps `glxHandler.{h,cpp}` in `#ifndef __APPLE__` and guards `GetVideoMemInfoMESA` in `myGL.cpp`. Master's `glxHandler.cpp` already compiles to stubs on Apple and glad no longer builds `glad_glx.c` there, so both are unnecessary, again proved by the successful build. It also double-nests `#ifndef __APPLE__` around `GLX::Load` and `GLX::Unload` twice, which is a merge artefact.

**Superseded.** The vendored OpenAL EFX headers under `include/Mac/AL/` are obsoleted by #3072, which uses openal-soft.

**Not applicable.** Everything in #3060's `rts/System/SDLCompat/` shim directory, 18 files existing to present an SDL2 surface over SDL3. sdl2-compat already does that, better and outside our tree. `FindMesaEGL.cmake` and `FindSDL3.cmake` from that PR hardcode `/Users/yeojun/Desktop/...`.

**Reclassified, not redundant.** #2991's HiDPI window-size persistence fix. It went upstream as #3021 and was correctly closed as a no-op on master. It becomes a genuine fix once S3 lands, so it moves into the stack rather than being dropped.

## 5. Open questions

1. **Is a forced GL version acceptable?** S0 passes only with `MESA_GL_VERSION_OVERRIDE=4.6`. Native is 2.1. This is not a macOS quirk we can guard around, it is the engine being told a capability set that is partly untrue, with failures landing at draw time. It may still be the right call, because the alternative is waiting for Zink and KosmicKrisp to close the gap honestly. But it should be a decision someone makes on purpose, not a side effect of an environment variable in a launcher script.

2. **Who owns the Mesa pin?** 26.2.0-rc3 works. It is a release candidate, so the pin moves to 26.2.0 final and then needs tracking. Building it is a documented 15 minute job now (see S0) but it is still a from-source dependency with five non-obvious prerequisites. If nobody wants to own it, S7 has no answer and the port is developer-only.

3. **Do we ship Mesa?** Bundling Mesa and Vulkan dylibs in a `.app` is a project-level distribution and licensing decision, not a build detail. The alternative, telling users to `brew install mesa` and pin a revision, is honest but narrows the audience sharply.

4. **Is the platform floor acceptable?** KosmicKrisp needs Metal 4, so macOS 26 and Apple Silicon. Intel Macs and anything older get nothing, ever, on this path.

5. **Who does the BAR no-geometry-shader content?** Without it S5 gives a running renderer with visibly wrong effects. That may be an acceptable first milestone or may not be, and it is not our call.

6. **Does "playable" include sound?** #3072's openal-soft path is on `macos/test-integration` and compiles, but has not been exercised with audio actually playing. If sound is in scope it needs its own verification pass.

7. **Is a readback present path acceptable to land at all?** It is a stopgap. The zero-copy path needs `VK_EXT_external_memory_metal` to grow IOSurface handle support in KosmicKrisp and a new `MESA_memory_object_metal` consumer in Zink. #2991 estimates roughly a month of upstream work. Landing the readback path means carrying it for a while. Measured in S3, the cost is smaller than #2991 made it look: 1.4 ms to copy a 2400x1600 frame and 0.5 ms to present it. The 6 ms that looks like readback is the GPU drawing the frame, which no present path avoids.

8. **Should PR #3022 be resolved first?** It has CHANGES_REQUESTED and the stack depends on it. Either land it, agree an alternative, or accept that S1 carries the change on the integration branch.

## Reference material

Read these, do not branch from them.

- PR #2991, `iamaperson000:upstream-pr-macos-apple-silicon`. 28 commits, 53 files. The architecture is sound and the commit titles are a good decomposition guide. Most of the diff is dead weight now.
- `ExaDev/RecoilEngine` branch `macos-layer`. #2991 plus real hardening: EGL shutdown, pbuffer resize, headless framework linking. Built with AppleClang.
- `ExaDev/RecoilEngine` branch `zink-probe`. Packaging and CI reference: `.app` assembly, dylib relocation, DMG, signing, smoke test.
- PR #3060, `yeojuny:codex/macos-runtime-substrate`. Different architecture (SDL3, direct present), needs a patched Mesa. Not the chosen path.
- <https://docs.mesa3d.org/drivers/kosmickrisp.html>
- <https://gist.github.com/lucamignatti/5312f5e937de2ba44256ecba6de54cc2>, the Minecraft on Zink plus KosmicKrisp writeup. The practical source on the nullDescriptor problem.
- Mesa MR 41313, makes `nullDescriptor` optional for Zink.
