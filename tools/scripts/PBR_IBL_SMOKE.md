# Portable PBR IBL smoke launch

`run-pbr-ibl-smoke.sh` is a Windows Git Bash launcher for the PBR IBL branch. It does not require fixed checkout names or a shared parent directory when explicit paths are supplied.

## Required inputs

- A Recoil runtime containing `spring.exe`, installed runtime files, fonts, and base content.
- A BAR checkout containing `dbg_pbr_ibl_viewer.lua` and `dbg_pbr_ibl_selftest.lua`.
- A directory containing the test map named by `pbr-ibl-startscript.txt`.

For an in-tree Recoil build, build and install the runtime first:

```bash
cmake --build build/window --target spring
cmake -P build/window/cont/cmake_install.cmake
```

Debug executables larger than 2 GiB must have their symbols split before Windows can run them. Use the tracked helper (or an equivalent `objcopy` flow):

```bash
tools/scripts/split-debug-symbols.sh --runtime-directory build/window
```

The helper discovers `objcopy`, `objdump`, and `strip` from `PATH`, falls back to `C:/mingw64/bin`, and accepts `PBR_OBJCOPY`, `PBR_OBJDUMP`, and `PBR_STRIP` overrides.

## Location-independent launch

```bash
tools/scripts/run-pbr-ibl-smoke.sh \
  --engine /c/path/to/recoil-runtime/spring.exe \
  --runtime-directory /c/path/to/recoil-runtime \
  --game-directory /d/checkouts/Beyond-All-Reason-Private.sdd \
  --maps-directory /e/games/BAR/maps \
  --write-directory /c/temp/pbr-ibl-user
```

The equivalent environment variables are:

```bash
export PBR_ENGINE=/c/path/to/recoil-runtime/spring.exe
export PBR_RUNTIME_DIRECTORY=/c/path/to/recoil-runtime
export PBR_GAME_DIRECTORY=/d/checkouts/Beyond-All-Reason-Private.sdd
export PBR_MAPS_DIRECTORY=/e/games/BAR/maps
export PBR_WRITE_DIRECTORY=/c/temp/pbr-ibl-user
tools/scripts/run-pbr-ibl-smoke.sh --self-test
```

`PBR_BUILD_DIRECTORY` remains a compatibility alias for `PBR_RUNTIME_DIRECTORY`. `PBR_START_SCRIPT`, `PBR_CONFIG_TEMPLATE`, and `PBR_GAME_LINK_NAME` expose the remaining launch inputs. Run `tools/scripts/run-pbr-ibl-smoke.sh --help` for the complete list.

When no BAR path is supplied, the launcher first checks the expected experimental sibling name and then accepts exactly one sibling `Beyond-All-Reason-Private*.sdd` checkout containing both PBR IBL widgets. Ambiguous discovery fails with an actionable message instead of choosing an arbitrary checkout.

`--prepare-only` validates the paths, creates the managed game/maps junctions, and writes isolated settings without starting the engine. The stable managed game mount is named `Beyond-All-Reason-Private-pbr-test.sdd`, so renaming the source checkout does not change the runtime archive path.

The launcher never edits the BAR checkout, installed maps, or a normal BAR user directory. Runtime settings and reports stay under the selected write directory.
