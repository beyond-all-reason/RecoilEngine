FROM docker.io/ubuntu:24.04
ENV ENGINE_PLATFORM=amd64-windows

RUN apt-get update \
 && apt-get upgrade --yes \
 && apt-get install --no-install-recommends --yes \
    g++-mingw-w64-x86-64-posix git ninja-build ccache pipx p7zip-full binutils zstd \
    pkg-config gcc-13 g++-13 build-essential \
 && rm -rf /var/lib/apt/lists/*

# Using `pipx --global` would be better but it's available only since pipx 1.5
RUN PIPX_HOME=/opt/pipx PIPX_BIN_DIR=/usr/local/bin pipx install cmake==3.27.* conan

WORKDIR /build
RUN mkdir src cache out artifacts && chmod a+rwx cache out artifacts

# Set up default cmake toolchain
COPY amd64-windows/toolchain.cmake .
ENV CMAKE_TOOLCHAIN_FILE=/build/toolchain.cmake
ENV CONAN_HOME=/build/.conan2

# Configure ccache caching
COPY all/ccache.conf .
ENV CCACHE_CONFIGPATH=/build/ccache.conf
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
ENV CMAKE_C_COMPILER_LAUNCHER=ccache

