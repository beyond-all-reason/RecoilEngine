#!/bin/bash
# Build the Mesa Zink + KosmicKrisp stack the macOS package ships.
#
# Clones upstream Mesa at a pinned commit, applies patches/mesa/*.patch, and
# builds. It owns the pin: nothing else in the repo should name a Mesa commit.
#
# A provenance stamp at $MESA_PREFIX/.driver-provenance records the pin and the
# sha256 of every patch, so a prefix that already matches is left alone and the
# whole build is skipped.
#
# Env:
#   MESA_PREFIX         install prefix        (default ~/dev/mesa-install-premtl4)
#   MESA_SRC            source clone          (default ~/dev/mesa)
#   MESA_PATCH_DIR      patch series          (default <repo>/patches/mesa)
#   MESA_FORCE_REBUILD  1 to rebuild anyway
#
# The two-pass build and the traps around it are explained in
# coding-agents/MACOS_BUILD.md. Do not duplicate the meson lines anywhere else.
set -euo pipefail

REPO=$(cd "$(dirname "$0")/.." && pwd)

# The commit before c08dba83025 "kk: Move to Metal4 command encoding", which
# never returns the memory a render pass allocates and leaks about 5 GiB a
# second under the engine. See patches/README.md before moving this.
MESA_COMMIT="56588ef0665ab81fa16248489542c1747fdf99ef"

MESA_PREFIX="${MESA_PREFIX:-$HOME/dev/mesa-install-premtl4}"
MESA_SRC="${MESA_SRC:-$HOME/dev/mesa}"
PATCH_DIR="${MESA_PATCH_DIR:-$REPO/patches/mesa}"
FORCE="${MESA_FORCE_REBUILD:-0}"

# The trailing "|| true" is load-bearing. An empty patches/mesa is the normal
# case, and under "set -o pipefail" a failing ls aborts the whole build.
patch_list() { ls "$PATCH_DIR"/*.patch 2>/dev/null | sort || true; }

# What SHOULD be installed at the prefix. Any difference means rebuild.
want_stamp() {
	echo "mesa_commit=$MESA_COMMIT"
	local p
	for p in $(patch_list); do
		echo "patch=$(basename "$p"):$(shasum -a 256 "$p" | cut -d' ' -f1)"
	done
	# recorded for transparency, not part of the cache key
	echo "# built=$(sw_vers -productVersion) $(uname -m)"
}

STAMP_FILE="$MESA_PREFIX/.driver-provenance"
WANT="$(want_stamp)"
key() { grep -v '^#'; }
NPATCH=$(patch_list | wc -l | tr -d ' ')

if [ "$FORCE" != 1 ] && [ -f "$STAMP_FILE" ]; then
	if [ "$(echo "$WANT" | key)" = "$(key < "$STAMP_FILE")" ]; then
		echo "mesa: $MESA_PREFIX is current ($MESA_COMMIT + $NPATCH patches), skipping"
		exit 0
	fi
	echo "mesa: $MESA_PREFIX is stale, pin or patches changed, rebuilding"
fi

# An existing prefix with no stamp was built by hand, before this script, and it
# is very likely the driver the engine is running on right now. Building over it
# means a failed build leaves no working driver at all. Refuse and make the
# choice explicit.
if [ "$FORCE" != 1 ] && [ -d "$MESA_PREFIX" ] && [ ! -f "$STAMP_FILE" ]; then
	echo "mesa: $MESA_PREFIX exists but carries no provenance stamp, so it was"
	echo "      built by hand and this script cannot tell what is in it."
	echo "      Refusing to build over a working driver."
	echo
	echo "      Build somewhere else:   MESA_PREFIX=~/dev/mesa-install-verify $0"
	echo "      Or overwrite it:        MESA_FORCE_REBUILD=1 $0"
	exit 1
fi

# Measured 2026-08-25 on an M1 Pro: build-clc 113 MB, build-zink 247 MB, the
# installed prefix 37 MB. Call it 400 MB, and refuse below 1 GB so a build
# cannot fail halfway and leave a half-installed prefix.
FREE_MB=$(df -m "$(dirname "$MESA_PREFIX")" | awk 'NR==2 {print $4}')
if [ "${FREE_MB:-0}" -lt 1024 ]; then
	echo "FATAL: only ${FREE_MB} MB free. A two-pass build needs about 400 MB of"
	echo "       object files plus a 37 MB prefix, and this machine has filled up"
	echo "       before. Stale build-* dirs in the Mesa clone are the usual win."
	exit 1
fi

# ---- source at the pin, patches applied ------------------------------------

if [ ! -d "$MESA_SRC/.git" ]; then
	echo "=== cloning Mesa into $MESA_SRC ==="
	git clone --filter=blob:none https://gitlab.freedesktop.org/mesa/mesa.git "$MESA_SRC"
fi

cd "$MESA_SRC"

if ! git cat-file -e "${MESA_COMMIT}^{commit}" 2>/dev/null; then
	git fetch origin "$MESA_COMMIT" || git fetch origin
fi

# Refusing beats silently building an unpinned driver, which is indistinguishable
# from a pinned one once it is installed.
git cat-file -e "${MESA_COMMIT}^{commit}" 2>/dev/null || {
	echo "FATAL: pinned commit $MESA_COMMIT is unreachable, refusing to build"
	exit 1
}

echo "=== [1/4] Mesa at $MESA_COMMIT + $NPATCH patches ==="
git checkout -f "$MESA_COMMIT"
git clean -fd
echo "mesa: version $(cat VERSION)"

# git am first so authorship and message survive and the result is a branch that
# can be rebased when the pin moves. git apply is the fallback for foreign
# patches with no From: header.
for p in $(patch_list); do
	echo "  applying $(basename "$p")"
	if ! git am --keep-non-patch "$p" 2>/dev/null; then
		git am --abort 2>/dev/null || true
		git apply --whitespace=nowarn "$p" || {
			echo "FATAL: $(basename "$p") did not apply to $MESA_COMMIT"
			exit 1
		}
	fi
done

# ---- build -----------------------------------------------------------------

echo "=== [2/4] python env ==="
[ -d .venv ] || python3 -m venv .venv
.venv/bin/pip install --quiet mako packaging setuptools 'meson==1.11.2'

export PATH="$PWD/.venv/bin:/opt/homebrew/opt/bison/bin:/opt/homebrew/opt/llvm/bin:$PATH"
export PKG_CONFIG_PATH="/opt/homebrew/opt/libclc/share/pkgconfig:/opt/homebrew/opt/spirv-llvm-translator/lib/pkgconfig"

COMMON="-Dplatforms=macos -Degl=enabled -Degl-native-platform=surfaceless
  -Dgallium-drivers=zink -Dvulkan-drivers=kosmickrisp
  -Dmoltenvk-dir=/opt/homebrew/opt/molten-vk
  -Dgallium-rusticl=false -Dtools= -Dglx=disabled -Dgbm=disabled
  -Dvideo-codecs= -Dopengl=true -Dgles1=disabled -Dgles2=disabled"

# Pass 1 exists because Mesa forces LLVM on whenever CLC is enabled and
# KosmicKrisp needs CLC, so -Dllvm=disabled alone fails at configure. Building
# the CLC tools first lets pass 2 take them from PATH with -Dmesa-clc=system,
# and the shipped libgallium then carries no LLVM at all.
echo "=== [3/4] pass 1, CLC tools ==="
meson setup build-clc --buildtype=release $COMMON -Dllvm=enabled
ninja -C build-clc src/compiler/clc/mesa_clc src/compiler/spirv/vtn_bindgen2

echo "=== [4/4] pass 2, the shipped driver ==="
export PATH="$PWD/build-clc/src/compiler/clc:$PWD/build-clc/src/compiler/spirv:$PATH"
meson setup build-zink --buildtype=release --prefix="$MESA_PREFIX" \
	$COMMON -Dllvm=disabled -Dmesa-clc=system
ninja -C build-zink install

# Only stamp once install has actually happened, so a failed build never leaves
# a prefix claiming to be current.
echo "$WANT" > "$STAMP_FILE"
echo "mesa: installed to $MESA_PREFIX ($MESA_COMMIT + $NPATCH patches)"
