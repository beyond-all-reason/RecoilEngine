# macOS ARM64 (M4) Port Status

## ✅ What's Working

| Component | Status | Details |
|-----------|--------|---------|
| **Full ARM64 build** | ✅ | Compiles & links clean (28MB binary) |
| **Game boot** | ✅ | Launches to lobby via `/Applications/Beyond All Reason.app` |
| **Music & sound** | ✅ | OpenAL playing; lobby background music audible |
| **Network** | ✅ | Connects to BAR servers; login window appears |
| **ChiliFX UI widgets** | ✅ | Load successfully (LuaShaders=1, --safemode removed) |
| **Mouse clicks** | ✅ | Fixed: `viewWindowOffsetY=0` on macOS; click targets now align |
| **GL compatibility shims** | ✅ | Software matrix tracking, glBegin/glEnd emulation, ortho backup all functional |
| **Framebuffer capture** | ✅ | Saves to `/tmp/bar_framebuffer.ppm` at frame 10 for debugging |
| **MGL Metal init** | ✅ | Metal device, CAMetalLayer created; GLAD drawing overrides loaded |
| **No crashes** | ✅ | Stable through lobby load, music, and networking |

## ❌ What's Broken

| Issue | Symptom | Root Cause | Fix location |
|-------|---------|------------|--------------|
| **GPU texture bug** | UI renders as grey boxes, no text/images | `GLD_TEXTURE_INDEX_2D is unloadable` — Apple Metal GL wrapper driver bug | **Engine** — needs GLSL workaround or MGL/GLFW switch |
| **Giant cursor** | ~4× normal size | `gui_cursor.lua` computes scale from `GetScreenGeometry()` which returns Retina resolution on Mac | **Game data** — `luaui/Widgets/gui_cursor.lua` line 54 |
| **Framebuffer 92% black** | Only ~8% of pixels have non-black content | Same as GPU texture bug — textures aren't loaded, so nothing renders | **Engine** — same as texture bug |

## 📦 Repo Split

| Repo | Contains | Has our fixes? | Branch |
|------|----------|:---:|--------|
| `beyond-all-reason/RecoilEngine` | C++ engine source | ✅ Engine fixes | PR #3093 |
| `RufflezAU/RecoilEngine` | Fork of engine | ✅ All engine fixes | `macos-experiment` |
| `beyond-all-reason/Beyond-All-Reason` | Game data (Lua, maps, assets) | ❌ Upstream | — |
| `RufflezAU/Beyond-All-Reason` | Fork of game data | 🔧 Game-side fixes | `macos-experiment` |

## 🔧 Engine Fixes Applied (RecoilEngine)

| Fix | File | What was done |
|-----|------|---------------|
| GL matrix tracking | `myGL.h` (537 lines) | Software intercepts for all fixed-function GL matrix ops |
| glBegin/glEnd emulation | `myGL.h` | VBO flush with GLSL 150 shader for immediate mode |
| glGetFloatv ortho backup | `myGL.h` | Saves ortho projection for 2D UI when 3D code overwrites it |
| White fallback texture | `myGL.h` | Creates 1×1 white texture when binding texture 0 |
| Apple GPU detection | `GlobalRendering.cpp` | `haveApple` flag for platform-specific code paths |
| Viewport offset fix | `GlobalRendering.cpp` | `viewWindowOffsetY=0` on macOS (was 57px, caused 135px click misalignment) |
| ChiliFX enable | `GlobalRendering.cpp` | Removed `--safemode`, set `LuaShaders=1` |
| MGL integration | `MGLIntegration.mm` | Metal device, CAMetalLayer, GLAD drawing overrides |
| ARM64 CMake | `legacy/CMakeLists.txt` | macOS ARM64 linking (Metal, QuartzCore, MGL library) |
| **CLAMP_TO_BORDER fix** | `RmlUi_Renderer_GL3_Recoil.cpp` + upstream | Replaced with `GL_CLAMP_TO_EDGE` on `__APPLE__` — border clamp unreliable on Metal GL |
| **ReleaseTexture fix** | Both RML renderers | Fixed stack-pointer bug in `glDeleteTextures()` |
| **Pixel alignment** | `RmlUi_Renderer_GL3_Recoil.cpp` | Added `glPixelStorei(GL_UNPACK_ALIGNMENT, 1)` before RML texture uploads |
| **Mip preallocation** | `myGL.cpp` | Skipped empty mip level glTexImage2D on macOS — confuses Metal state tracking |
| macOS threading | `ThreadAffinityGuard`, `Threading` | Thread naming + Linux-style scheduler |
| ARM CPU topology | `CpuTopology.cpp` | M4 core detection |
| OpenAL EFX | `SpringEFX.h` | macOS-compatible EFX declarations |
| Libunwind | `FindLibunwind.cmake` | Homebrew ARM64 path |

## 🎯 Remaining Work

### Engine side (RecoilEngine)
1. **Fix GPU texture driver bug** — highest priority, blocks all visual rendering
   - Investigate: port Doomsday Engine's GLSL sampler-workaround to RecoilEngine's shader system
   - OR: complete MGL+GLFW integration so GL renders through Metal natively
   - OR: find Apple-specific GL texture format that avoids the bug
2. **MGL full rendering** — long-term solution; needs GLFW windowing (SDL→GLFW migration)

### Game side (Beyond-All-Reason)
1. **Fix giant cursor** — cap `autoCursorSize` formula in `gui_cursor.lua` for high-DPI displays
2. **ChiliFX tuning** — reduce GPU load with simpler shaders on macOS
3. **Engine settings** — macOS-specific defaults in `EngineOptions.lua` or `springsettings.cfg`
