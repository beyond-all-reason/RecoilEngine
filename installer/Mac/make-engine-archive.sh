#!/bin/bash
# Builds the self-contained engine for the macOS launcher (engine,
# pr-downloader, Mesa/Zink/KosmicKrisp) as recoil_<version>_arm64-macos<suffix>.7z,
# the macOS counterpart of the engine archives the launcher downloads on Linux
# and Windows.
#
# The archive holds Engine.app with spring as its main executable, so macOS
# lists the game under its own name and icon instead of as a bare "spring"
# process. Everything else lives in Contents/Resources, linked from
# Contents/MacOS where the engine looks for its data and libraries.
#
# Usage: make-engine-archive.sh <build dir> <output dir>
# Env:
#   PACKAGE_SUFFIX appended to the archive name, like package.sh in docker-build-v2
#   SIGN_IDENTITY  codesign identity, "-" (default) for ad-hoc
#   MESA_PREFIX    required, install prefix of make-mesa.sh (EGL, Zink, KosmicKrisp)
#   VULKAN_PREFIX  default: $(brew --prefix vulkan-loader)
#   BUNDLE_ID      default: io.github.beyond-all-reason.recoil
set -euo pipefail

BUILD_DIR="$(cd "$1" && pwd)"
OUT_DIR="$(mkdir -p "$2" && cd "$2" && pwd)"
SRC_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
SIGN_IDENTITY="${SIGN_IDENTITY:--}"
MESA_PREFIX="${MESA_PREFIX:?set MESA_PREFIX to the install prefix of make-mesa.sh}"
VULKAN_PREFIX="${VULKAN_PREFIX:-$(brew --prefix vulkan-loader)}"
SDL3_PREFIX="${SDL3_PREFIX:-$(brew --prefix sdl3)}"
BUNDLE_ID="${BUNDLE_ID:-io.github.beyond-all-reason.recoil}"
VERSION="$(head -n1 "$BUILD_DIR/VERSION")"
SEVENZIP="$(command -v 7zz || command -v 7z)"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
G="$WORK/Engine.app"
ENGINE="$G/Contents/Resources"
LICENSES="$ENGINE/licenses"
mkdir -p "$ENGINE/lib" "$ENGINE/mesa/lib" "$ENGINE/mesa/share/vulkan/icd.d" "$LICENSES"

cp "$BUILD_DIR/spring" "$BUILD_DIR/libunitsync.dylib" "$BUILD_DIR/tools/pr-downloader/src/pr-downloader" "$ENGINE/"
cp -R "$BUILD_DIR/base" "$ENGINE/"
cp -R "$SRC_DIR/cont/fonts" "$SRC_DIR/cont/LuaUI" "$SRC_DIR/cont/cmdcolors.txt" "$SRC_DIR/cont/ctrlpanel.txt" "$SRC_DIR/cont/luaui.lua" "$ENGINE/"
cp "$BUILD_DIR/VERSION" "$SRC_DIR/installer/Mac/engine/fonts.conf" "$ENGINE/"

# skirmish AIs, if they were built
for ai in AI/Interfaces/C AI/Skirmish/NullAI AI/Skirmish/BARb; do
	if [ -f "$BUILD_DIR/$ai/cmake_install.cmake" ]; then
		cmake -DCMAKE_INSTALL_PREFIX="$ENGINE" -P "$BUILD_DIR/$ai/cmake_install.cmake"
	fi
done

# Mesa EGL + Zink + KosmicKrisp, loaded by MacEGLBridge from <engine>/mesa
sed 's|"library_path": ".*"|"library_path": "../../../lib/libvulkan_kosmickrisp.dylib"|' \
	"$MESA_PREFIX/share/vulkan/icd.d/kosmickrisp_mesa_icd.aarch64.json" \
	> "$ENGINE/mesa/share/vulkan/icd.d/kosmickrisp_mesa_icd.aarch64.json"

# copy all non-system dylib dependencies and make every reference bundle relative;
# sdl2-compat dlopen()s SDL3 from next to itself
python3 "$SRC_DIR/installer/Mac/bundle-dylibs.py" "$ENGINE" "$LICENSES" \
	"mesa=$MESA_PREFIX/lib/libEGL.1.dylib" \
	"mesa=$MESA_PREFIX/lib/libvulkan_kosmickrisp.dylib" \
	"mesa=$(ls "$MESA_PREFIX"/lib/libgallium-*.dylib)" \
	"mesa=$VULKAN_PREFIX/lib/libvulkan.1.dylib" \
	"lib=$SDL3_PREFIX/lib/libSDL3.0.dylib:libSDL3.dylib"

# licenses of everything shipped
cp "$SRC_DIR/LICENSE" "$SRC_DIR/gpl-2.0.txt" "$SRC_DIR/gpl-3.0.txt" "$LICENSES/"
cp "$SRC_DIR/cont/fonts/COPYING" "$LICENSES/fonts-COPYING"
cp "$SRC_DIR/installer/Mac/engine/README-licenses.txt" "$LICENSES/README.txt"

mkdir -p "$G/Contents/MacOS"
mv "$ENGINE/spring" "$G/Contents/MacOS/spring"
for f in "$ENGINE"/*; do
	ln -s "../Resources/$(basename "$f")" "$G/Contents/MacOS/$(basename "$f")"
done
cp "$SRC_DIR/installer/Mac/spring.icns" "$ENGINE/"
sed -e "s/###VERSION###/$VERSION/g" -e "s/###BUNDLE_ID###/$BUNDLE_ID/g" \
	"$SRC_DIR/installer/Mac/engine/Engine-Info.plist" > "$G/Contents/Info.plist"

# sign inside out
SIGN_ARGS=(--force --timestamp --options runtime -s "$SIGN_IDENTITY")
[ "$SIGN_IDENTITY" = "-" ] && SIGN_ARGS=(--force -s -)
find "$ENGINE" -name '*.dylib' -print0 | xargs -0 codesign "${SIGN_ARGS[@]}"
codesign "${SIGN_ARGS[@]}" "$ENGINE/pr-downloader"
codesign "${SIGN_ARGS[@]}" --entitlements "$SRC_DIR/installer/Mac/engine/entitlements.plist" "$G"
codesign --verify --deep --strict "$G"

ARCHIVE="$OUT_DIR/recoil_$(git -C "$SRC_DIR" describe --abbrev=7)_arm64-macos${PACKAGE_SUFFIX:-}.7z"
rm -f "$ARCHIVE"
# no executable filters: current 7-Zip picks its ARM64 filter, which the 7za
# 16.02 the launcher extracts with does not know ("Unsupported Method")
(cd "$WORK" && "$SEVENZIP" a -snl -mx=9 -mf=off "$ARCHIVE" Engine.app >/dev/null)
# for notarization, which does not take 7z archives
[ "$SIGN_IDENTITY" = "-" ] || ditto -c -k --keepParent "$G" "$OUT_DIR/engine-notarize.zip"
echo "$ARCHIVE"
