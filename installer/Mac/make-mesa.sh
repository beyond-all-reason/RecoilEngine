#!/bin/bash
# Builds Mesa's EGL, Zink and KosmicKrisp at a fixed Mesa version.
#
# The version is pinned on purpose: KosmicKrisp from Mesa 26.3 on unrolls every
# draw whose fragment shader has flat inputs unless the engine asks for
# first-vertex provoking, so the engine and Mesa have to move together.
#
# Usage: make-mesa.sh <work dir> <install prefix>
# Env:
#   MESA_VERSION   default: 26.3.0-rc1
#   MOLTENVK_DIR   default: $(brew --prefix molten-vk), only its headers are used
#   LLVM_PREFIX    default: $(brew --prefix llvm), for KosmicKrisp's kernels
set -euo pipefail

WORK_DIR="$(mkdir -p "$1" && cd "$1" && pwd)"
PREFIX="$(mkdir -p "$2" && cd "$2" && pwd)"
MESA_VERSION="${MESA_VERSION:-26.3.0-rc1}"
MOLTENVK_DIR="${MOLTENVK_DIR:-$(brew --prefix molten-vk)}"
LLVM_PREFIX="${LLVM_PREFIX:-$(brew --prefix llvm)}"
MESA_SRC="$WORK_DIR/mesa-$MESA_VERSION"
LIBCLC_VERSION=22.1.8.3
LIBCLC_SRC="$WORK_DIR/mesa-libclc-$LIBCLC_VERSION"
LIBCLC_PREFIX="$WORK_DIR/libclc-install"

# archive.mesa3d.org is down now and then, the tag on Mesa's GitLab has the same sources
download() {
	curl -fL --retry 5 --retry-all-errors --retry-delay 10 --connect-timeout 30 -o "$@"
}

if [ ! -d "$MESA_SRC" ]; then
	MESA_TARBALL="$WORK_DIR/mesa-$MESA_VERSION.tar"
	[ -f "$MESA_TARBALL" ] ||
		download "$MESA_TARBALL" "https://archive.mesa3d.org/mesa-$MESA_VERSION.tar.xz" ||
		download "$MESA_TARBALL" "https://gitlab.freedesktop.org/mesa/mesa/-/archive/mesa-$MESA_VERSION/mesa-mesa-$MESA_VERSION.tar.gz"
	mkdir -p "$MESA_SRC"
	tar -xf "$MESA_TARBALL" -C "$MESA_SRC" --strip-components 1
fi

# LLVM 23+ libclc lacks the SPIR-V target Mesa compiles KosmicKrisp's kernels for,
# Homebrew's mesa uses this fork for the same reason
if [ ! -f "$LIBCLC_PREFIX/share/pkgconfig/mesa-libclc.pc" ]; then
	[ -f "$WORK_DIR/mesa-libclc-$LIBCLC_VERSION.tar.bz2" ] ||
		download "$WORK_DIR/mesa-libclc-$LIBCLC_VERSION.tar.bz2" \
			"https://gitlab.freedesktop.org/karolherbst/mesa-libclc/-/archive/$LIBCLC_VERSION/mesa-libclc-$LIBCLC_VERSION.tar.bz2"
	tar -xf "$WORK_DIR/mesa-libclc-$LIBCLC_VERSION.tar.bz2" -C "$WORK_DIR"
	cmake -S "$LIBCLC_SRC" -B "$LIBCLC_SRC/build" -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX="$LIBCLC_PREFIX" \
		-DCMAKE_PREFIX_PATH="$LLVM_PREFIX" \
		-DLLVM_DIR="$LLVM_PREFIX/lib/cmake/llvm"
	cmake --build "$LIBCLC_SRC/build"
	cmake --install "$LIBCLC_SRC/build"
fi

# Mesa's code generators need mako and pyyaml, the build needs a newer bison
python3 -m venv "$WORK_DIR/venv"
"$WORK_DIR/venv/bin/pip" install -q mako pyyaml packaging
export PATH="$WORK_DIR/venv/bin:$(brew --prefix bison)/bin:$PATH"

COMMON_ARGS=(
	--buildtype=release
	--prefix="$PREFIX"
	-Db_ndebug=true
	-Dstrip=true
	-Dplatforms=macos
	-Dglx=disabled
	-Dgles1=disabled
	-Dvulkan-layers=
	-Dgallium-rusticl=false
	-Dvideo-codecs=
	-Dtools=
)

RECONFIGURE=
[ -d "$WORK_DIR/build/meson-private" ] && RECONFIGURE=--reconfigure
meson setup $RECONFIGURE "$WORK_DIR/build" "$MESA_SRC" "${COMMON_ARGS[@]}" \
	-Dopengl=true \
	-Degl=enabled \
	-Dgles2=enabled \
	-Dgallium-drivers=zink \
	-Dvulkan-drivers= \
	-Dllvm=disabled \
	-Dmoltenvk-dir="$MOLTENVK_DIR" \
	-Dvulkan-loader-rpath=@loader_path/../lib
ninja -C "$WORK_DIR/build" install

RECONFIGURE=
[ -d "$WORK_DIR/build-kk/meson-private" ] && RECONFIGURE=--reconfigure
PATH="$LLVM_PREFIX/bin:$PATH" PKG_CONFIG_PATH="$LIBCLC_PREFIX/share/pkgconfig:${PKG_CONFIG_PATH:-}" \
	meson setup $RECONFIGURE "$WORK_DIR/build-kk" "$MESA_SRC" "${COMMON_ARGS[@]}" \
	-Dopengl=false \
	-Degl=disabled \
	-Dgles2=disabled \
	-Dgallium-drivers= \
	-Dvulkan-drivers=kosmickrisp \
	-Dllvm=enabled
PATH="$LLVM_PREFIX/bin:$PATH" ninja -C "$WORK_DIR/build-kk" install
