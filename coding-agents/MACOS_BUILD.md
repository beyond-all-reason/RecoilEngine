# Building and running Recoil on macOS

Apple Silicon only, macOS 15 or newer. Most of the first build is compiling Mesa.

macOS has no OpenGL 4.6. Apple froze its driver at 4.1 core, and the engine needs 4.6 compatibility. So the graphical engine runs on Mesa's Zink, which translates OpenGL to Vulkan, on top of KosmicKrisp, which is Mesa's Vulkan driver for Metal.

Leon Seemann's `MacEGLBridge` (`rts/Rendering/GL/MacEGLBridge.cpp`) is the present path. It creates the GL context through Mesa EGL, reads each finished frame back through two pixel buffers and presents it through SDL's Metal renderer.

The headless, dedicated and unitsync builds need none of that. If you only want a server or content tools, do step 1 and step 3 and stop.

## 1. Dependencies

```bash
brew bundle --file=Brewfile
brew install meson ninja bison llvm libclc spirv-tools spirv-llvm-translator molten-vk vulkan-loader vulkan-headers
```

The engine builds with Homebrew GCC, not Apple Clang, to keep floating point behaviour consistent with the Linux and Windows builds. Simulation determinism depends on that.

## 2. Mesa

Use Mesa 26.3.0-rc1, unpatched, until 26.3 ships in Homebrew. Do not use 26.2.5: it leaks during load and reached a 9.7 GiB footprint 15 s into SplinterFaction.

Build it with Leon's `make-mesa.sh`, which is `installer/Mac/make-mesa.sh` on the `macos-port-2026.07.04` branch of `github.com/leonseemann/RecoilEngine`. If you have a `leon` remote, `git show leon/macos-port-2026.07.04:installer/Mac/make-mesa.sh` prints it. The script builds the EGL and Zink half and the KosmicKrisp half as two Meson builds into one prefix. Its prerequisites are the Homebrew packages above, `python3`, `cmake` and `curl`. It downloads the Mesa tarball and builds `mesa-libclc` itself.

The script finds its patches in `installer/Mac/mesa-patches` next to it. Put a copy of the script in a directory with an empty `installer/Mac/mesa-patches` and run it from there:

```bash
MESA_VERSION=26.3.0-rc1 installer/Mac/make-mesa.sh ~/dev/mesa-work/work-26.3.0-rc1 ~/dev/mesa-install-26.3.0-rc1
```

That produced a 34 MB prefix. Its header says it pins 26.2 because 26.3 unrolls flat-varying draws. The engine now avoids that path by switching to first-vertex provoking on KosmicKrisp (`preferFirstProvokingVertex` in `rts/Rendering/GlobalRendering.cpp`), so 26.3 is the right choice.

Homebrew's `llvm` and `bison` are keg-only. The script puts them on `PATH` itself. Apple's bison is 2.3 and cannot parse Mesa's grammars.

## 3. The engine

```bash
cmake -B build-macos -G Ninja \
  -DCMAKE_C_COMPILER=/opt/homebrew/bin/gcc-16 \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/g++-16 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-macos -j8 --target engine-legacy
```

That is the configuration in `build-macos/CMakeCache.txt`: GCC 16, Ninja, RelWithDebInfo. `MAC_EGL_ZINK` is ON by default on macOS. Swap `engine-legacy` for `engine-headless`, `engine-dedicated` or `unitsync` if that is all you want. `ninja -C build-macos -k 0` builds everything, tolerating the two Linux-only test targets that cannot compile here.

Never reuse a CMake cache across a Homebrew upgrade. Cellar paths are absolute and versioned, so configure fails on a package that moved. Delete `CMakeCache.txt` and keep `CMakeFiles/`, which preserves the object files.

## 4. Choosing the Mesa build

The engine loads Mesa itself, so no launcher or environment variables are needed. `MacEGLBridge` looks for Mesa in this order:

1. `<exe dir>/mesa`, if it contains `lib/libEGL.1.dylib`.
2. The `MAC_MESA_PREFIX` CMake cache value compiled into the binary. It defaults to `/opt/homebrew/opt/mesa`. Pass `-DMAC_MESA_PREFIX=<prefix>` at configure time to change it. The current `build-macos` cache points at `~/dev/mesa-install-26.2.5`, which leaks, so rely on the symlink.

Make `build-macos/mesa` a symlink to a Mesa install prefix and that is the Mesa the engine runs:

```bash
ln -sfn ~/dev/mesa-install-26.3.0-rc1 build-macos/mesa
```

To switch Mesa builds, repoint the symlink. No relink is needed.

The bridge sets these variables itself, and only when they are not already set:

- `GALLIUM_DRIVER=zink`
- `VK_DRIVER_FILES=<prefix>/share/vulkan/icd.d/kosmickrisp_mesa_icd.aarch64.json`, unless `VK_ICD_FILENAMES` or `VK_DRIVER_FILES` is set. This picks KosmicKrisp because Zink needs `robustness2.nullDescriptor`, which MoltenVK lacks.
- `MESA_GL_VERSION_OVERRIDE=4.6COMPAT`
- `MESA_GLSL_VERSION_OVERRIDE=460`

Export any of them first to override.

## 5. Running it

Use `--isolation-dir`. Without it the engine picks up whatever is in `~/.spring`, and a `base/springcontent.sdz` older than this engine is missing shaders it asks for, which draws the terrain black. It looks like a renderer bug but is stale content.

Check `memory_pressure` first. Then use the capped runner, which kills the engine above a memory ceiling and after a time limit:

```bash
coding-agents/test-scripts/run-capped.sh <seconds> <logfile> [engine args]
```

It runs `./spring` in `build-macos` with `--isolation-dir ~/dev/spring-testdata --window`. Environment variables:

- `SCRIPT=` sets the start script. The default is `splinterfaction-fixed-start.tdf`.
- `BUILD=` sets the build directory.
- `MAX_RSS_KB=` sets the kill ceiling on phys_footprint. The default is 11010048 (10.5 GiB). The machine has 16 GiB and GPU memory is the same RAM.

`coding-agents/test-scripts/README.md` covers the start scripts and the measurement harness.

## 6. Sharing a build

```bash
coding-agents/test-scripts/package-macos.sh
```

Produces `dist/recoil_<version>_arm64-macos/`, laid out like a Recoil release and carrying every non-system dylib including Mesa, so it runs on a machine with no Homebrew. The Mesa prefix defaults to `~/dev/mesa-install-26.3.0-rc1` and `MESA_PREFIX=` overrides it. Mesa goes in the archive's `mesa/` directory, which is where the bridge looks, so `spring` is the engine binary itself with no launcher.

For a build that should join games of a release, configure with `-DVERSION_OVERRIDE=<release>` on a branch made from that release tag. The engine then reports the release version, and the archive is named after it. The 2026.09.01 package built this way completed the BAR benchmark at a 10 GiB peak footprint.

## What works

The engine runs real games. Terrain, units, the menu and LuaUI all draw, mouse input lands where you click, and the window resizes. SplinterFaction and Metal Factions render correctly.

Measured on an M1 Pro with 16 GiB, Mesa 26.3.0-rc1, a 1400x850 window and vsync on, the BAR benchmark (`coding-agents/test-scripts/bar-benchmark.tdf`) completes all 2000 frames:

- footprint: about 10 GB
- 17.1 fps
- mean Sim 16.8 ms, Draw 9.7 ms, Update 11.4 ms
- SwapBuffers, the SDL present, costs 1.47 ms a frame

## Lua immediate mode

`gl.BeginEnd` blocks draw with one `glDrawArrays`. The config `LuaImmediateModeBuffering` controls it, default 1, and `/set LuaImmediateModeBuffering 0` switches back to `glBegin`/`glEnd` at runtime.

This avoids a Zink and KosmicKrisp defect that joins consecutive `glBegin` `LINE_LOOP` batches. With the 2000-loop amplifier, 0 of 8 frames showed stray lines buffered against 7 of 8 unbuffered.

Lua lines now get the wide-line emulation too, so `gl.LineWidth(2)` draws about 3 px where it used to draw 1 px.

## Known rough edges

`glPolygonMode(GL_LINE)` does nothing on this driver, so wireframe stages are skipped.
