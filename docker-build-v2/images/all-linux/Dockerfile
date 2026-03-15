# We pick ubuntu 18 instead of newer one because we are dynamically linking
# to glibc and a few other libraries and we want to keep compatibility for as
# many users as possible.
FROM docker.io/ubuntu:18.04
ARG ENGINE_PLATFORM
ARG STATIC_LIBS_BRANCH
ENV ENGINE_PLATFORM=${ENGINE_PLATFORM}

RUN apt-get update \
 && apt-get install --no-install-recommends --yes software-properties-common \
 && add-apt-repository ppa:ubuntu-toolchain-r/test --yes \
 && apt-get update \
 && apt-get install --no-install-recommends --yes \
    curl gcc-13 g++-13 git ninja-build curl p7zip-full xz-utils build-essential \
    python3.8 python3.8-distutils libx11-dev libx11-xcb-dev libfontenc-dev xkb-data \
    libice-dev libsm-dev libxau-dev libxaw7-dev libegl1-mesa-dev libgl1-mesa-dev \
    libx11-dev libx11-xcb-dev libfontenc-dev libice-dev libsm-dev libxau-dev libxaw7-dev \
    libxcomposite-dev libxcursor-dev libxdamage-dev libxdmcp-dev libxext-dev libxfixes-dev \
    libxi-dev libxinerama-dev libxkbfile-dev libxmu-dev libxmuu-dev \
    libxpm-dev libxrandr-dev libxrender-dev libxres-dev libxss-dev libxt-dev libxtst-dev \
    libxv-dev libxxf86vm-dev libxcb-glx0-dev libxcb-render0-dev \
    libxcb-render-util0-dev libxcb-xkb-dev libxcb-icccm4-dev libxcb-image0-dev \
    libxcb-keysyms1-dev libxcb-randr0-dev libxcb-shape0-dev libxcb-sync-dev libxcb-xfixes0-dev \
    libxcb-xinerama0-dev libxcb-dri3-dev uuid-dev libxcb-cursor-dev libxcb-dri2-0-dev \
    libxcb-dri3-dev libxcb-present-dev libxcb-composite0-dev libxcb-ewmh-dev \
    libxcb-res0-dev libxcb-util-dev pkg-config \
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

RUN curl -L -O https://bootstrap.pypa.io/pip/3.8/get-pip.py && python3.8 get-pip.py && rm get-pip.py
RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.8 10

RUN python3.8 -m pip install --upgrade pip \
 && python3.8 -m pip install scikit-build cmake==3.27.* conan

WORKDIR /build
RUN mkdir src cache out artifacts && chmod a+rwx cache out artifacts

# Set up default cmake toolchain
COPY all-linux/toolchain.cmake .
ENV CMAKE_TOOLCHAIN_FILE=/build/toolchain.cmake
ENV CONAN_HOME=/build/.conan2

# Configure ccache caching
COPY all/ccache.conf .
ENV CCACHE_CONFIGPATH=/build/ccache.conf
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
ENV CMAKE_C_COMPILER_LAUNCHER=ccache
