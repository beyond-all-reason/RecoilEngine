# Building and running Recoil on macOS

Apple Silicon only, macOS 15 or newer. Expect an afternoon the first time, most of it compiling Mesa.

macOS has no OpenGL 4.6. Apple froze its driver at 4.1 core, and the engine needs 4.6 compatibility. So the graphical engine runs on Mesa's Zink, which translates OpenGL to Vulkan, on top of KosmicKrisp, which is Mesa's Vulkan driver for Metal. That is why step 2 exists and why it takes so long. `MACOS_RENDERER_PLAN.md` explains what was tried before settling on this.

The headless, dedicated and unitsync builds need none of that. If you only want a server or content tools, do step 1 and step 3 and stop.

## 1. Dependencies

```bash
brew bundle --file=Brewfile
brew install meson ninja bison llvm libclc spirv-tools spirv-llvm-translator molten-vk vulkan-loader vulkan-headers
```

The engine builds with Homebrew GCC, not Apple Clang, to keep floating point behaviour consistent with the Linux and Windows builds. Simulation determinism depends on that. The one Objective-C file is compiled with clang regardless, because GCC cannot parse it.

## 2. Mesa

No released Mesa works. Build it from source, pinned, in two passes. The second pass is what you keep, and it needs no LLVM at runtime, which is worth 180 MB in the packaged engine.

```bash
git clone --filter=blob:none https://gitlab.freedesktop.org/mesa/mesa.git
cd mesa && git checkout 56588ef0665
python3 -m venv .venv && .venv/bin/pip install mako packaging setuptools 'meson==1.11.2'

export PATH="$PWD/.venv/bin:/opt/homebrew/opt/bison/bin:/opt/homebrew/opt/llvm/bin:$PATH"
export PKG_CONFIG_PATH="/opt/homebrew/opt/libclc/share/pkgconfig:/opt/homebrew/opt/spirv-llvm-translator/lib/pkgconfig"

COMMON="-Dplatforms=macos -Degl=enabled -Degl-native-platform=surfaceless
  -Dgallium-drivers=zink -Dvulkan-drivers=kosmickrisp
  -Dmoltenvk-dir=/opt/homebrew/opt/molten-vk
  -Dgallium-rusticl=false -Dtools= -Dglx=disabled -Dgbm=disabled
  -Dvideo-codecs= -Dopengl=true -Dgles1=disabled -Dgles2=disabled"

# Pass 1: build only the two CLC tools. Nothing from this build ships.
meson setup build-clc --buildtype=release $COMMON -Dllvm=enabled
ninja -C build-clc src/compiler/clc/mesa_clc src/compiler/spirv/vtn_bindgen2

# Pass 2: the real build, with those tools on PATH and no LLVM.
export PATH="$PWD/build-clc/src/compiler/clc:$PWD/build-clc/src/compiler/spirv:$PATH"
meson setup build-zink --buildtype=release --prefix="$HOME/dev/mesa-install-premtl4" \
  $COMMON -Dllvm=disabled -Dmesa-clc=system
ninja -C build-zink install
```

The pin matters. `56588ef0665` is the commit before `kk: Move to Metal4 command encoding`, which gives every render pass its own MTL4 command buffer and never returns the memory. Anything after it leaks about 5 GiB a second under the engine and is unplayable. That is filed upstream as <https://gitlab.freedesktop.org/mesa/mesa/-/work_items/15998>.

Why two passes. Mesa forces LLVM on whenever CLC is enabled, and KosmicKrisp needs CLC, so `-Dllvm=disabled` on its own fails at configure with "CLC requires LLVM". Under `-Dmesa-clc=system` that requirement collapses to rusticl alone, which is off, and CLC becomes a build-time tool. The shipped `libgallium` then has zero LLVM symbols. LLVM was only ever serving gallivm, the JIT behind the `draw` module's geometry and tessellation fallback, which Zink over a real Vulkan driver does not need.

Five things that will otherwise waste your afternoon:

- Mesa needs Python `mako` and `meson`, and no Homebrew formula provides `mako`. Use the venv above rather than touching Homebrew's Python.
- Homebrew's `llvm` and `bison` are both keg-only, so they need to be on `PATH` explicitly. Apple's bison is 2.3 and cannot parse Mesa's grammars.
- Meson bakes the bison path into `build.ninja` at configure time, so putting it on `PATH` only for the build step does nothing. If Homebrew's bison is missing, configure silently picks Apple's and the failure only appears later as a syntax error in a `.y` file.
- Zink on macOS wants `-Dmoltenvk-dir` at configure time even though the target is KosmicKrisp.
- Pass 1 still needs Homebrew's `llvm` installed, even though nothing from it ships.

## 3. The engine

```bash
cmake -B build-macos-legacy -G Ninja \
  -DCMAKE_C_COMPILER=/opt/homebrew/bin/gcc-16 \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/g++-16 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-macos-legacy -j8 --target engine-legacy
```

Swap `engine-legacy` for `engine-headless`, `engine-dedicated` or `unitsync` if that is all you want. `ninja -C build-macos-legacy -k 0` builds everything, tolerating the two Linux-only test targets that cannot compile here.

Never reuse a CMake cache across a Homebrew upgrade. Cellar paths are absolute and versioned, so configure fails on a package that moved. Delete `CMakeCache.txt` and keep `CMakeFiles/`, which preserves the object files.

## 4. Running it

The engine finds Mesa entirely through environment variables, none of which can be baked into the binary:

```bash
export EGL_PLATFORM=surfaceless
export MESA_LOADER_DRIVER_OVERRIDE=zink
export GALLIUM_DRIVER=zink
export LIBGL_DRIVERS_PATH=$HOME/dev/mesa-install-premtl4/lib
export VK_DRIVER_FILES=$HOME/dev/mesa-install-premtl4/share/vulkan/icd.d/kosmickrisp_mesa_icd.aarch64.json
export MESA_GL_VERSION_OVERRIDE=4.6
export MESA_GLSL_VERSION_OVERRIDE=460

./build-macos-legacy/spring --isolation-dir ~/spring-testdata --window
```

Use `--isolation-dir`. Without it the engine picks up whatever is in `~/.spring`, and a `base/springcontent.sdz` older than this engine is missing shaders it asks for, which draws the terrain black. That is a stale content problem wearing a renderer costume, and it has fooled people before.

`coding-agents/test-scripts/` holds start scripts that launch a real game without a lobby, and its README covers the measurement harness.

## 5. Sharing a build

```bash
coding-agents/test-scripts/package-macos.sh
```

Produces `dist/recoil_<version>_arm64-macos/`, laid out like a Recoil release and carrying every non-system dylib including Mesa, so it runs on a machine with no Homebrew. `spring` in that archive is a launcher that sets the variables above and calls `spring-bin`.

About 143 MB unpacked and 28 MB as a `.7z`. If yours is roughly double that, the Mesa it bundled was built with LLVM: `libLLVM.dylib` is 164 MB on its own and drags in `libz3` at another 16 MB. `MESA_PREFIX=<prefix>` picks which Mesa gets bundled.

## What works

The engine runs real games. Terrain, units, the menu and LuaUI all draw, mouse input lands where you click, and the window resizes.

Known rough edges are all in `MACOS_RENDERER_PLAN.md`. The short version: performance is bounded by presenting through a readback rather than 1:1, `glPolygonMode(GL_LINE)` does nothing on this driver so wireframe stages are skipped, and immediate-mode batches need a flush between them because the driver merges batches it should not.
