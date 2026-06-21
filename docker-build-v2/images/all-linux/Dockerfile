# Ubuntu 20.04 (glibc 2.31) is the minimum for our static lib builds.
FROM docker.io/ubuntu:20.04
ARG ENGINE_PLATFORM
ENV ENGINE_PLATFORM=${ENGINE_PLATFORM}

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
 && apt-get install --no-install-recommends --yes software-properties-common \
 && add-apt-repository ppa:ubuntu-toolchain-r/test --yes \
 && apt-get update \
 && apt-get install --no-install-recommends --yes \
    curl gcc-13 g++-13 git ninja-build p7zip-full xz-utils zstd python3-pip python3-setuptools \
    libsdl2-dev libopenal-dev libfreetype6-dev libfontconfig1-dev \
 && apt-get remove --purge --yes software-properties-common \
 && apt-get autoremove --purge --yes \
 && apt-get upgrade --yes \
 && rm -rf /var/lib/apt/lists/*

# We need a newer ccache for compression support.
ARG CCACHE_VERSION="4.13.1"
RUN ARCH=$(uname -m) \
 && CCACHE_ARCHIVE="ccache-${CCACHE_VERSION}-linux-${ARCH}-glibc" \
 && curl -fsSL "https://github.com/ccache/ccache/releases/download/v${CCACHE_VERSION}/${CCACHE_ARCHIVE}.tar.xz" \
    | tar -xJ \
 && mv ${CCACHE_ARCHIVE}/ccache /usr/bin \
 && rm -rf ${CCACHE_ARCHIVE}


RUN pip3 install --upgrade pip \
 && pip3 install scikit-build \
 && pip3 install cmake==3.27.*

WORKDIR /build
RUN mkdir src cache out artifacts && chmod a+rwx cache out artifacts

# Fetch library dependencies and configure resolution
ARG RECOIL_LIBS_VERSION
ARG LOCAL_TARBALL=""
RUN case "${ENGINE_PLATFORM}" in \
       amd64-linux) LIBS_TARBALL="recoil-libs-x64-linux.tar.gz" ;; \
       arm64-linux) LIBS_TARBALL="recoil-libs-arm64-linux.tar.gz" ;; \
       *) echo "Unknown ENGINE_PLATFORM: ${ENGINE_PLATFORM}" && exit 1 ;; \
     esac \
  && mkdir recoil-libs \
  && if [ -n "$LOCAL_TARBALL" ]; then \
         echo "Using local tarball: $LOCAL_TARBALL"; \
         tar -xzf "$LOCAL_TARBALL" -C recoil-libs; \
     else \
         curl -fsSL "https://github.com/RecoilEngine/RecoilEngineLibs/releases/download/${RECOIL_LIBS_VERSION}/${LIBS_TARBALL}" \
             | tar -xz -C recoil-libs; \
     fi
ENV PKG_CONFIG_LIBDIR=/build/recoil-libs/lib/pkgconfig
ENV PKG_CONFIG="pkg-config --define-prefix --static"
ENV CMAKE_PREFIX_PATH=/build/recoil-libs/
ENV PREFER_STATIC_LIBS=TRUE

# Set up default cmake toolchain
COPY all-linux/toolchain.cmake .
ENV CMAKE_TOOLCHAIN_FILE=/build/toolchain.cmake

# Configure ccache caching
COPY all/ccache.conf .
ENV CCACHE_CONFIGPATH=/build/ccache.conf
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
ENV CMAKE_C_COMPILER_LAUNCHER=ccache
