# We pick ubuntu 18 instead of newer one because we are dynamically linking
# to glibc and a few other libraries and we want to keep compatibility for as
# many users as possible.
FROM docker.io/ubuntu:18.04
ARG ARCH
ENV ENGINE_PLATFORM=$ARCH-linux

RUN apt-get update \
 && apt-get upgrade --yes \
 && apt-get install --no-install-recommends --yes software-properties-common \
 && add-apt-repository ppa:ubuntu-toolchain-r/test --yes \
 && apt-get remove --purge --yes software-properties-common \
 && apt-get autoremove --purge --yes \
 && rm -rf /var/lib/apt/lists/*

RUN apt-get update \
 && apt-get install --no-install-recommends --yes \
    file curl gcc-13 g++-13 git ninja-build curl p7zip-full \
    pkg-config python3.8 python3.8-distutils jq \
 && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.8 10 \
 && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 10 \
 && update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-13 10 \
 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 10 \
 && update-alternatives --install /usr/bin/cpp cpp /usr/bin/g++-13 10 \
 && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-13 10

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
 && make -j$(nproc) \
 && mv programs/zstd /usr/local/bin \
 && cd .. \
 && rm -rf zstd

RUN curl -L -O https://bootstrap.pypa.io/pip/3.8/get-pip.py && python3.8 get-pip.py && rm get-pip.py
ENV CMAKE_VERSION=3.31.10
RUN python3 -m pip install scikit-build cmake==$CMAKE_VERSION conan==2.*

WORKDIR /build
RUN mkdir conan-out src cache out artifacts && chmod a+rwx cache out artifacts

# Set up default cmake toolchain
COPY docker-build-v2/images/all-linux/toolchain.cmake .

ENV CONAN_HOME=/build/conan-home
RUN mkdir -p $CONAN_HOME/profiles
COPY docker-build-v2/images/$ARCH-all/conan_build_profile $CONAN_HOME/profiles
COPY docker-build-v2/images/$ENGINE_PLATFORM/conan_profile $CONAN_HOME/profiles
COPY docker-build-v2/images/all/conan-deps.sh .
COPY conanfile.py .
RUN ./conan-deps.sh
ENV CMAKE_TOOLCHAIN_FILE=/build/conan-out/conan/conan_toolchain.cmake

# Configure ccache caching
COPY docker-build-v2/images/all/ccache.conf .
ENV CCACHE_CONFIGPATH=/build/ccache.conf
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
ENV CMAKE_C_COMPILER_LAUNCHER=ccache
