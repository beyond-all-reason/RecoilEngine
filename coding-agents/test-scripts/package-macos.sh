#!/bin/bash
# Build a portable macOS engine archive in the shape of a Recoil release.
#
# The layout comes from `cmake --install`, which is what CI uses in
# docker-build-v2/scripts/compile.sh, so the archive carries AI/, LuaUI/,
# fonts/, doc/, examples/, base/, spring-headless, spring-dedicated,
# libunitsync.dylib and pr-downloader, not just the graphical engine. Compare
# against a real release with:
#
#   diff <(cd <release>  && find . -type f | sort) \
#        <(cd <this out> && find . -type f | sort)
#
# It differs from a Linux release in exactly two ways, both forced by macOS:
#
#   - lib/ holds every non-system dylib, because a target machine has no
#     Homebrew. CI deletes lib/ after installing because it is GNUInstallDirs
#     leftovers from vendored subprojects. Here it is deleted and then recreated
#     for the bundled copies, so the order matters.
#   - mesa/ holds Mesa's EGL, gallium and KosmicKrisp, plus the Vulkan ICD file.
#     MacEGLBridge loads Mesa from <exe dir>/mesa and sets the driver
#     environment itself, so spring is the binary and needs no launcher.
#
# Bundled dylibs are reached three different ways, because @executable_path
# means whichever process loaded the file, and that is not always the engine:
#
#   root executables    @executable_path/lib            spring, headless,
#                                                       dedicated, pr-downloader
#   libunitsync.dylib   @loader_path/lib                a lobby dlopens it, so
#                                                       @executable_path would be
#                                                       the lobby's own directory
#   AI/*/*/*/*.dylib    @loader_path/../../../../lib    the engine dlopens these
#                                                       from four levels down
#   mesa/lib/*.dylib    @loader_path/../../lib          MacEGLBridge dlopens
#                                                       libEGL by path
#
# Mesa is dlopened rather than linked, so dylibbundler cannot find it on its
# own. Its dylibs are passed to it explicitly below, then moved into mesa/.
#
#   package-macos.sh [outdir]
#
# Build everything first, tolerating the two Linux-only test targets that
# cannot compile here:
#
#   ninja -C build-macos -k 0
#
# Mesa is 26.3.0-rc1, unpatched, built with installer/Mac/make-mesa.sh, see
# coding-agents/MACOS_BUILD.md. 26.2.5 leaks during load and must not be packaged.
set -euo pipefail

REPO=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
BUILD=${BUILD_DIR:-$REPO/build-macos}
PLATFORM=arm64-macos
# a build configured with VERSION_OVERRIDE reports that version, so name the archive after it
VERSION=$(sed -n 's/^VERSION_OVERRIDE:STRING=//p' "$BUILD/CMakeCache.txt" 2>/dev/null)
VERSION=${VERSION:-$(cd "$REPO" && git describe --abbrev=7)}
OUT=${1:-$REPO/dist/recoil_${VERSION}_${PLATFORM}}
MESA=${MESA_PREFIX:-$HOME/dev/mesa-install-26.3.0-rc1}
VKLIB=${VK_LOADER_LIB:-/opt/homebrew/opt/vulkan-loader/lib}

command -v dylibbundler > /dev/null || { echo "need dylibbundler (brew install dylibbundler)"; exit 1; }
command -v 7zz          > /dev/null || { echo "need 7zz (brew install sevenzip)"; exit 1; }
[ -x "$BUILD/spring" ] || { echo "no engine at $BUILD/spring, run: ninja -C $BUILD -k 0"; exit 1; }
[ -d "$MESA/lib" ]     || { echo "no Mesa prefix at $MESA"; exit 1; }

echo "==> installing the portable layout into $OUT"
rm -rf "$OUT"
mkdir -p "$(dirname "$OUT")"
cmake --install "$BUILD" --prefix "$OUT" > "$(dirname "$OUT")/.install.log" 2>&1 \
	|| { grep -m5 -A4 "CMake Error" "$(dirname "$OUT")/.install.log"; exit 1; }

# GNUInstallDirs leftovers from vendored subprojects. CI drops these for the
# same reason, see docker-build-v2/scripts/compile.sh. lib/ is recreated below
# for the bundled dylibs, so this has to happen first.
rm -rf "$OUT/include" "$OUT/lib" "$OUT/share"
mkdir -p "$OUT/lib"

# -L so the versioned target is copied rather than a symlink into Homebrew
cp -L "$MESA"/lib/libEGL.1.dylib "$OUT/lib/"
cp -L "$MESA"/lib/libgallium-*.dylib "$OUT/lib/"
cp -L "$MESA"/lib/libvulkan_kosmickrisp.dylib "$OUT/lib/"
cp -L "$VKLIB"/libvulkan.1.dylib "$OUT/lib/"

# The engine links sdl2-compat, which is a shim that dlopens SDL3 rather than
# linking it, so dylibbundler cannot see it. It looks for @loader_path first,
# and the loader here is libSDL2 in lib/, so SDL3 has to sit beside it. Without
# this the packaged engine aborts before it logs anything.
SDL3=${SDL3_LIB:-/opt/homebrew/opt/sdl3/lib/libSDL3.dylib}
if [ -f "$SDL3" ]; then
	cp -L "$SDL3" "$OUT/lib/libSDL3.dylib"
else
	echo "!! no libSDL3 at $SDL3, the engine will abort on start"
fi

# dylibbundler rewrites what a file depends on, not what a file calls itself.
# The ones copied in by hand still carry their build location as an install
# name, which makes the archive non relocatable.
echo "==> rewriting install names"
for f in "$OUT/lib"/*.dylib; do
	install_name_tool -id "@executable_path/lib/$(basename "$f")" "$f" 2>/dev/null || true
done

AI_LIBS=$(cd "$OUT" && find AI -name '*.dylib' 2>/dev/null | sed "s|^|$OUT/|")

# When dylibbundler cannot resolve a dependency it asks on stdin, and on EOF it
# asks again, forever, writing the prompt to the log until the disk is full. It
# reached 6.8GB once. Answering "quit" makes it stop at the first one it cannot
# resolve and say which one that was.
echo "==> bundling dependencies"
# shellcheck disable=SC2046,SC2086
dylibbundler --overwrite-files --bundle-deps \
	--fix-file "$OUT/spring" \
	--fix-file "$OUT/spring-headless" \
	--fix-file "$OUT/spring-dedicated" \
	--fix-file "$OUT/pr-downloader" \
	--fix-file "$OUT/libunitsync.dylib" \
	$(for f in $AI_LIBS; do printf -- '--fix-file %s ' "$f"; done) \
	--fix-file "$OUT/lib/libEGL.1.dylib" \
	--fix-file "$OUT/lib"/libgallium-*.dylib \
	--fix-file "$OUT/lib/libvulkan_kosmickrisp.dylib" \
	--fix-file "$OUT/lib/libvulkan.1.dylib" \
	--fix-file "$OUT/lib/libSDL3.dylib" \
	--dest-dir "$OUT/lib" \
	--install-path "@executable_path/lib" \
	< <(yes quit) > "$OUT/.bundle.log" 2>&1 \
	|| { grep -m3 -B2 "incomplete name" "$OUT/.bundle.log" || tail -20 "$OUT/.bundle.log"; exit 1; }

# dylibbundler writes @executable_path into everything it touches. That is right
# only for the executables at the root. See the header for why each of the other
# two groups needs a different prefix.
echo "==> repointing dylib references at @loader_path"
relink() { # relink <file> <path to lib/ as seen from that file>
	local f=$1 prefix=$2 dep
	otool -L "$f" | awk '/@executable_path\/lib\//{print $1}' | while read -r dep; do
		install_name_tool -change "$dep" "$prefix/$(basename "$dep")" "$f"
	done
	codesign --force -s - "$f" 2>/dev/null || true
}
for f in "$OUT/lib"/*.dylib; do
	relink "$f" "@loader_path"
done
relink "$OUT/libunitsync.dylib" "@loader_path/lib"
for f in $AI_LIBS; do
	relink "$f" "@loader_path/../../../../lib"
done

# zink reaches the Vulkan loader by name, so the binary needs somewhere to look
# that is not Homebrew. DYLD_LIBRARY_PATH would also work and is the wrong tool:
# it is stripped for signed and hardened binaries later on.
# dylibbundler adds this rpath once per dependency it rewrites, and dyld refuses
# to start a process carrying a duplicate LC_RPATH. Strip every copy, then add
# exactly one.
strip_rpaths() {
	while otool -l "$1" | grep -q "@executable_path/lib"; do
		install_name_tool -delete_rpath "@executable_path/lib/" "$1" 2>/dev/null \
			|| install_name_tool -delete_rpath "@executable_path/lib" "$1" 2>/dev/null \
			|| break
	done
}
for f in "$OUT/spring" "$OUT/spring-headless" "$OUT/spring-dedicated" "$OUT/pr-downloader"; do
	strip_rpaths "$f"
	install_name_tool -add_rpath "@executable_path/lib" "$f"
	codesign --force -s - "$f" 2>/dev/null || true
done
strip_rpaths "$OUT/libunitsync.dylib"
codesign --force -s - "$OUT/libunitsync.dylib" 2>/dev/null || true

# The AI libraries need no rpath, because every dependency they have is already
# an explicit @loader_path above. They still collect dylibbundler's duplicates,
# and dyld refuses to load a file carrying a duplicate LC_RPATH, so strip
# without adding one back. Missing this makes every AI fail to load, including
# NullAI, which links nothing but libSystem.
for f in $AI_LIBS; do
	strip_rpaths "$f"
	codesign --force -s - "$f" 2>/dev/null || true
done

# Where MacEGLBridge looks: <exe dir>/mesa/lib/libEGL.1.dylib, and the ICD at
# the path a Mesa install prefix puts it. Mesa's own dylibs reach each other as
# siblings, everything else they need stays in lib/.
echo "==> moving Mesa into mesa/"
mkdir -p "$OUT/mesa/lib" "$OUT/mesa/share/vulkan/icd.d"
mv "$OUT/lib/libEGL.1.dylib" "$OUT/lib"/libgallium-*.dylib "$OUT/lib/libvulkan_kosmickrisp.dylib" "$OUT/mesa/lib/"
for f in "$OUT/mesa/lib"/*.dylib; do
	install_name_tool -id "@loader_path/$(basename "$f")" "$f"
	otool -L "$f" | awk '/@loader_path\//{print $1}' | while read -r dep; do
		[ -e "$OUT/mesa/lib/$(basename "$dep")" ] && continue
		install_name_tool -change "$dep" "@loader_path/../../lib/$(basename "$dep")" "$f"
	done
	codesign --force -s - "$f" 2>/dev/null || true
done
# library_path is relative to this file, which keeps the archive relocatable
cat > "$OUT/mesa/share/vulkan/icd.d/kosmickrisp_mesa_icd.aarch64.json" <<'JSON'
{
    "file_format_version": "1.0.0",
    "ICD": {
        "library_path": "../../../lib/libvulkan_kosmickrisp.dylib",
        "api_version": "1.3.0"
    }
}
JSON

echo "==> checking nothing still points outside the archive"
BINARIES="$OUT/spring $OUT/spring-headless $OUT/spring-dedicated $OUT/pr-downloader $OUT/libunitsync.dylib $AI_LIBS"
# shellcheck disable=SC2086
LEAKS=$( (otool -L $BINARIES "$OUT/lib"/*.dylib "$OUT/mesa/lib"/*.dylib || true) \
	| grep -v ":$" | grep -E "/opt/homebrew|/Users/" || true)
if [ -n "$LEAKS" ]; then
	echo "!! these still reference paths outside the archive:"
	echo "$LEAKS" | sed 's/^/   /'
else
	echo "   clean, no Homebrew or home directory references"
fi

# The failure this catches is silent: unitsync loads fine from a shell and fails
# only once a lobby dlopens it, because by then @executable_path is the lobby.
# The same applies to every AI the engine dlopens. otool -L prints the file's own
# install name first, which dyld never resolves, so drop it and look only at what
# each file actually loads.
# shellcheck disable=SC2086
HOSTED=$(for f in "$OUT/libunitsync.dylib" $AI_LIBS "$OUT/lib"/*.dylib; do
	otool -L "$f" | tail -n +3 | grep "@executable_path" | sed "s|^|$(basename "$f"): |" || true
done)
if [ -n "$HOSTED" ]; then
	echo "!! these break when something other than the engine loads them:"
	echo "$HOSTED" | sed 's/^/   /'
else
	echo "   clean, nothing dlopened depends on @executable_path"
fi

# Same list a release carries, generated the same way. gzip adds its own
# checksum, so the list validates itself. Check with:
#   gunzip -c files.md5.gz | md5sum -c -
# CI documents this as `zcat`, which on macOS looks for a .Z file and fails.
echo "==> writing files.md5.gz"
( cd "$OUT" && find . -type f ! -name '*.dbg' ! -name files.md5.gz ! -name '.bundle.log' \
	-exec md5sum {} \; | gzip > files.md5.gz )

echo "==> $(du -sh "$OUT" | cut -f1) in $OUT"

ARCHIVE="$(dirname "$OUT")/recoil_${VERSION}_${PLATFORM}.7z"
echo "==> compressing as $(basename "$ARCHIVE")"
rm -f "$ARCHIVE"
( cd "$OUT" && 7zz a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on "$ARCHIVE" ./* -xr\!.bundle.log > /dev/null )

echo "==> $(du -sh "$ARCHIVE" | cut -f1) in $ARCHIVE"

# pr-downloader unpacks an engine flat into engine/<platform>/<version>/ and
# replaces \/:?"<>| with _ in the version first, see CFileSystem::EscapeFilename
# and CFileSystem::extractEngine. That version is the engine's own sync version,
# which is not the same string as the archive name.
SYNC=$("$OUT/spring" --sync-version 2>/dev/null | head -1)
ESCAPED=$(echo "$SYNC" | tr '\\/:?"<>|' '_')

echo
echo "    archive version: $VERSION"
echo "    sync version:    $SYNC"
echo "    install:         7zz x '$ARCHIVE' -o\$HOME/.spring/engine/macos_arm64/'$ESCAPED'"
