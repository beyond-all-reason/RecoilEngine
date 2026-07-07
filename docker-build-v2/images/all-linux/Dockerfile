# We pick ubuntu 18 instead of newer one because we are dynamically linking
# to glibc and a few other libraries and we want to keep compatibility for as
# many users as possible.
FROM docker.io/ubuntu:18.04
ARG ENGINE_PLATFORM
ARG STATIC_LIBS_BRANCH=18.04
ENV ENGINE_PLATFORM=${ENGINE_PLATFORM}

RUN apt-get update \
 && apt-get install --no-install-recommends --yes software-properties-common \
 && add-apt-repository ppa:ubuntu-toolchain-r/test --yes \
 && apt-get update \
 && apt-get install --no-install-recommends --yes \
    curl gcc-13 g++-13 git ninja-build curl p7zip-full python3-pip python3-setuptools \
    libopenal-dev libfreetype6-dev libfontconfig1-dev \
    libdevil-dev libdevil1c2 \
    zlib1g-dev libogg-dev libvorbis-dev libexpat1-dev \
    libcurl4-openssl-dev \
    xorg-dev libasound2-dev libpulse-dev libdbus-1-dev libudev-dev libpci-dev \
    libgl1-mesa-dev libglu1-mesa-dev \
 && apt-get remove --purge --yes software-properties-common \
 && apt-get autoremove --purge --yes \
 && apt-get upgrade --yes \
 && rm -rf /var/lib/apt/lists/*

# We need a newer ccache for compression support.
ARG CCACHE_VERSION="4.13.1"
# We use musl version because glibc in ubuntu18 is too old.
RUN export CCACHE_ARCHIVE="ccache-${CCACHE_VERSION}-linux-$(uname -m)-musl-static" \
 && curl -L -O https://github.com/ccache/ccache/releases/download/v${CCACHE_VERSION}/${CCACHE_ARCHIVE}.tar.xz \
 && tar -xf ${CCACHE_ARCHIVE}.tar.xz \
 && mv ${CCACHE_ARCHIVE}/ccache /usr/bin \
 && rm -rf ${CCACHE_ARCHIVE}*

# And need newer zstd :(, fortunately it's quick build
RUN git clone --depth 1 --branch v1.5.7 https://github.com/facebook/zstd.git \
 && cd zstd \
 && CC=g++-13 make -j$(nproc) \
 && mv programs/zstd /usr/local/bin \
 && cd .. \
 && rm -rf zstd

RUN pip3 install --upgrade pip \
 && pip3 install scikit-build \
 && pip3 install cmake==3.27.*

# Build SDL3 from source (Ubuntu 18.04 has no SDL3 package)
ARG SDL3_VERSION="3.2.10"
RUN curl -fsSL "https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VERSION}/SDL3-${SDL3_VERSION}.tar.gz" -o SDL3.tar.gz \
 && tar xzf SDL3.tar.gz \
 && mkdir -p SDL3-${SDL3_VERSION}/build \
 && cd SDL3-${SDL3_VERSION}/build \
 && /usr/local/bin/cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=gcc-13 -DCMAKE_CXX_COMPILER=g++-13 \
    -DSDL_TEST=OFF -DSDL_TESTS=OFF -DSDL_INSTALL_CMAKE_CONFIG=ON \
    -G "Unix Makefiles" \
 && /usr/local/bin/cmake --build . -j$(nproc) \
 && /usr/local/bin/cmake --install . \
 && cd / && rm -rf SDL3.tar.gz SDL3-${SDL3_VERSION}

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
