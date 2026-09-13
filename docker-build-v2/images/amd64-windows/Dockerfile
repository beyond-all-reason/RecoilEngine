FROM docker.io/ubuntu:24.04
ENV ENGINE_PLATFORM=amd64-windows

RUN apt-get update \
 && apt-get upgrade --yes \
 && apt-get install --no-install-recommends --yes \
    curl g++-mingw-w64-x86-64-posix git ninja-build make ccache pipx p7zip-full binutils zstd \
    libasound2-dev libpulse-dev libudev-dev libdbus-1-dev libpci-dev \
    xorg-dev libgl1-mesa-dev libglu1-mesa-dev \
 && rm -rf /var/lib/apt/lists/*

# Using `pipx --global` would be better but it's available only since pipx 1.5
RUN PIPX_HOME=/opt/pipx PIPX_BIN_DIR=/usr/local/bin pipx install cmake==3.27.*

WORKDIR /build
RUN mkdir src cache out artifacts && chmod a+rwx cache out artifacts

# Fetch library dependencies and configure resolution
RUN git clone --depth=1 https://github.com/beyond-all-reason/mingwlibs64.git
ENV MINGWLIBS=/build/mingwlibs64

# Copy toolchain early so SDL3 build can use it
COPY amd64-windows/toolchain.cmake .

# Build SDL3 for MinGW cross-compile (mingwlibs64 only ships SDL2)
ARG SDL3_VERSION="3.2.10"
RUN curl -fsSL "https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VERSION}/SDL3-${SDL3_VERSION}.tar.gz" -o SDL3.tar.gz \
 && tar xzf SDL3.tar.gz \
 && mkdir -p SDL3-${SDL3_VERSION}/build \
 && cd SDL3-${SDL3_VERSION}/build \
 && cmake .. -DCMAKE_TOOLCHAIN_FILE=/build/toolchain.cmake \
    -DCMAKE_INSTALL_PREFIX=/build/mingwlibs64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DSDL_TEST=OFF -DSDL_TESTS=OFF -DSDL_INSTALL_CMAKE_CONFIG=ON \
    -DSDL_X11=OFF -DSDL_WAYLAND=OFF -DSDL_PULSEAUDIO=OFF -DSDL_ALSA=OFF \
    -DSDL_OPENGL=ON -DSDL_OPENGL_ES=ON -DSDL_DIRECTX=ON \
    -G "Unix Makefiles" \
 && cmake --build . -j$(nproc) \
 && cmake --install . \
 && cd / && rm -rf SDL3.tar.gz SDL3-*

# Set up default cmake toolchain
ENV CMAKE_TOOLCHAIN_FILE=/build/toolchain.cmake

# Configure ccache caching
COPY all/ccache.conf .
ENV CCACHE_CONFIGPATH=/build/ccache.conf
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
ENV CMAKE_C_COMPILER_LAUNCHER=ccache
