# mingw-w64 GCC version is dictated by the base distro, and Ubuntu/Debian lag
# badly (Ubuntu 24.04 = GCC 13, Debian trixie = 14). Arch ships current GCC, so
# we use it and pin for reproducibility on two axes:
#   1. the base image, by date-stamped tag (an immutable snapshot of the rootfs)
#   2. the packages, by pointing pacman at the Arch Linux Archive for a fixed day
# A full `-Syu` against that snapshot reconciles base + packages to one date,
# avoiding Arch's partial-upgrade breakage. Keep the two dates close together.
#
# Snapshot 2026-05-01 => mingw-w64-gcc 15.2.0 (16.1.0 landed 2026-05-04).
# To bump GCC: pick a newer snapshot date that carries the version you want
# (browse https://archive.archlinux.org/packages/m/mingw-w64-gcc/) and move the
# base tag to a matching/earlier date from https://hub.docker.com/_/archlinux.
FROM archlinux:base-20260419.0.517065
ENV ENGINE_PLATFORM=amd64-windows

ARG ARCH_SNAPSHOT=2026/05/01
# The base tag is close to the snapshot, so its keyring already trusts these
# packages; -Syu upgrades archlinux-keyring from the snapshot as part of the set.
RUN echo "Server=https://archive.archlinux.org/repos/${ARCH_SNAPSHOT}/\$repo/os/\$arch" \
      > /etc/pacman.d/mirrorlist \
 && pacman -Syu --noconfirm --needed \
      mingw-w64-gcc git ninja ccache python-pipx 7zip binutils zstd \
 && pacman -Scc --noconfirm

# cmake is pinned to 3.27 (matching the linux image) via pipx rather than the
# distro package, which would otherwise drift with the snapshot date.
RUN PIPX_HOME=/opt/pipx PIPX_BIN_DIR=/usr/local/bin pipx install cmake==3.27.*

WORKDIR /build
RUN mkdir src cache out artifacts && chmod a+rwx cache out artifacts

# Fetch library dependencies and configure resolution
RUN git clone --depth=1 https://github.com/beyond-all-reason/mingwlibs64.git
ENV MINGWLIBS=/build/mingwlibs64

# Set up default cmake toolchain
COPY amd64-windows/toolchain.cmake .
ENV CMAKE_TOOLCHAIN_FILE=/build/toolchain.cmake

# Configure ccache caching
COPY all/ccache.conf .
ENV CCACHE_CONFIGPATH=/build/ccache.conf
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
ENV CMAKE_C_COMPILER_LAUNCHER=ccache
