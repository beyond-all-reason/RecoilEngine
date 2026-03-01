# Shared Linux build image for amd64 and arm64.
# Parameterized via build args; invoked by per-platform build.sh wrappers.
#
# We pick ubuntu 18 instead of newer one because we are dynamically linking
# to glibc and a few other libraries and we want to keep compatibility for as
# many users as possible.
FROM docker.io/ubuntu:18.04

ARG ENGINE_PLATFORM
ARG STATIC_LIBS_BRANCH=18.04

ENV ENGINE_PLATFORM=${ENGINE_PLATFORM}

ARG TARGETARCH

RUN apt-get update \
 && apt-get install --no-install-recommends --yes software-properties-common \
 && add-apt-repository ppa:ubuntu-toolchain-r/test --yes \
 && apt-get update \
 && apt-get install --no-install-recommends --yes \
    binutils ca-certificates curl gcc-13 g++-13 git libc6-dev ninja-build \
    p7zip-full pkg-config \
    libsdl2-dev libopenal-dev libfreetype6-dev libfontconfig1-dev \
 && apt-get remove --purge --yes software-properties-common \
 && apt-get autoremove --purge --yes \
 && apt-get upgrade --yes \
 && rm -rf /var/lib/apt/lists/* \
 && rm -f /usr/bin/ld.gold  # gold from binutils 2.30 can't link pthread with gcc-13

# Install cmake from official binary — pip wheels have hash issues on aarch64
ARG CMAKE_VERSION="3.27.9"
RUN if [ "$TARGETARCH" = "arm64" ]; then ARCH_SUFFIX="aarch64"; else ARCH_SUFFIX="x86_64"; fi \
 && curl -L https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-${ARCH_SUFFIX}.tar.gz \
    | tar xz --strip-components=1 -C /usr/local

# Build zstd from source (needed by packaging scripts)
RUN git clone --depth 1 --branch v1.5.6 https://github.com/facebook/zstd.git \
 && cd zstd \
 && CC=gcc-13 make -j$(nproc) \
 && mv programs/zstd /usr/local/bin \
 && cd .. \
 && rm -rf zstd

# Build ccache from source for multi-arch support (prebuilt binaries are x86_64-only)
ARG CCACHE_VERSION="4.11.3"
RUN git clone --depth 1 --branch v${CCACHE_VERSION} https://github.com/ccache/ccache.git \
 && cd ccache \
 && mkdir build && cd build \
 && cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=gcc-13 \
    -DCMAKE_CXX_COMPILER=g++-13 \
    -DREDIS_STORAGE_BACKEND=OFF \
    -DZSTD_FROM_INTERNET=ON \
    .. \
 && ninja \
 && ninja install \
 && cd ../.. \
 && rm -rf ccache

WORKDIR /build
RUN mkdir src cache out artifacts && chmod a+rwx cache out artifacts

# Fetch library dependencies and configure resolution
RUN git clone --depth=1 https://github.com/beyond-all-reason/spring-static-libs.git -b ${STATIC_LIBS_BRANCH} spring-static-libs
ENV PKG_CONFIG_LIBDIR=/build/spring-static-libs/lib/pkgconfig
ENV PKG_CONFIG="pkg-config --define-prefix --static"
ENV CMAKE_PREFIX_PATH=/build/spring-static-libs/
ENV PREFER_STATIC_LIBS=TRUE

# Set up default cmake toolchain
COPY all-linux/toolchain.cmake .
ENV CMAKE_TOOLCHAIN_FILE=/build/toolchain.cmake

# Configure ccache caching
COPY all/ccache.conf .
ENV CCACHE_CONFIGPATH=/build/ccache.conf
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
ENV CMAKE_C_COMPILER_LAUNCHER=ccache
