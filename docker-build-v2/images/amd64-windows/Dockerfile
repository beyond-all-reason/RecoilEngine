# Arch Linux rolling-release image pinned to a fixed snapshot for reproducibility.
# mingw-w64-gcc on Arch tracks GCC 15 and uses POSIX thread model by default.
# Update the tag when a newer snapshot is desired.
FROM docker.io/archlinux:base-20260301.0.494762
ENV ENGINE_PLATFORM=amd64-windows

# Point pacman to a frozen ALA snapshot date for reproducible package installs.
# The date should match the base image snapshot date.
RUN echo 'Server=https://archive.archlinux.org/repos/2026/03/01/$repo/os/$arch' \
    > /etc/pacman.d/mirrorlist

RUN pacman -Syu --noconfirm \
 && pacman -S --noconfirm --needed \
    mingw-w64-gcc git ninja ccache p7zip binutils zstd cmake \
 && pacman -Scc --noconfirm

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
