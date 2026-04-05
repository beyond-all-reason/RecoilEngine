FROM docker.io/ubuntu:24.04
ENV ENGINE_PLATFORM=amd64-windows

RUN apt-get update \
 && apt-get upgrade --yes \
 && apt-get install --no-install-recommends --yes \
    g++-mingw-w64-x86-64-posix git ninja-build ccache pipx p7zip-full binutils zstd \
    pkg-config gcc-13 g++-13 make file \
 && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 10 \
 && update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-13 10 \
 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 10 \
 && update-alternatives --install /usr/bin/cpp cpp /usr/bin/g++-13 10 \
 && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-13 10

# Using `pipx --global` would be better but it's available only since pipx 1.5
RUN PIPX_HOME=/opt/pipx PIPX_BIN_DIR=/usr/local/bin pipx install cmake==3.27.* conan==2.*

WORKDIR /build
RUN mkdir conan-out src cache out artifacts && chmod a+rwx cache out artifacts

# Set up default cmake toolchain
COPY docker-build-v2/images/amd64-windows/toolchain.cmake .

ENV CONAN_HOME=/build/conan-home
RUN mkdir -p $CONAN_HOME/profiles
COPY docker-build-v2/images/amd64-all/conan_build_profile $CONAN_HOME/profiles
COPY docker-build-v2/images/amd64-windows/conan_profile $CONAN_HOME/profiles
COPY docker-build-v2/images/all/conan-deps.sh .
COPY conanfile.py .
RUN ./conan-deps.sh
ENV CMAKE_TOOLCHAIN_FILE=/build/conan-out/conan/conan_toolchain.cmake


# Configure ccache caching
COPY docker-build-v2/images/all/ccache.conf .
ENV CCACHE_CONFIGPATH=/build/ccache.conf
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
ENV CMAKE_C_COMPILER_LAUNCHER=ccache
