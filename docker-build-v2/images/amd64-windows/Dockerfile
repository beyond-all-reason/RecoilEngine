FROM docker.io/ubuntu:24.04
ENV ENGINE_PLATFORM=amd64-windows

RUN apt-get update \
 && apt-get upgrade --yes \
 && apt-get install --no-install-recommends --yes \
    g++-mingw-w64-x86-64-posix git ninja-build ccache pipx p7zip-full binutils zstd curl \
 && rm -rf /var/lib/apt/lists/*

# Using `pipx --global` would be better but it's available only since pipx 1.5
RUN PIPX_HOME=/opt/pipx PIPX_BIN_DIR=/usr/local/bin pipx install cmake==3.27.*

WORKDIR /build
RUN mkdir src cache out artifacts && chmod a+rwx cache out artifacts

# Fetch library dependencies and configure resolution
ARG LOCAL_TARBALL=""
ARG RECOIL_LIBS_VERSION
RUN mkdir recoil-libs \
  && if [ -f "/build/local_tarball.tar.gz" ]; then \
         echo "Using local tarball at /build/local_tarball.tar.gz"; \
         tar -xzf "/build/local_tarball.tar.gz" -C recoil-libs; \
     else \
         echo "Downloading tarball"; \
         curl -fsSL "https://github.com/RecoilEngine/RecoilEngineLibs/releases/download/${RECOIL_LIBS_VERSION}/recoil-libs-x64-mingw.tar.gz" \
             | tar -xz -C recoil-libs; \
     fi
ENV MINGWLIBS=/build/recoil-libs

# Set up default cmake toolchain
COPY amd64-windows/toolchain.cmake .
ENV CMAKE_TOOLCHAIN_FILE=/build/toolchain.cmake

# Configure ccache caching
COPY all/ccache.conf .
ENV CCACHE_CONFIGPATH=/build/ccache.conf
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
ENV CMAKE_C_COMPILER_LAUNCHER=ccache
