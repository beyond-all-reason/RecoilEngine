# We pick ubuntu 18 instead of newer one because we are dynamically linking
# to glibc and a few other libraries and we want to keep compatibility for as
# many users as possible.
FROM docker.io/ubuntu:18.04
ARG ENGINE_PLATFORM
ARG STATIC_LIBS_BRANCH
ENV ENGINE_PLATFORM=${ENGINE_PLATFORM}

RUN apt-get update \
 && apt-get install --no-install-recommends --yes \
    curl git ninja-build p7zip-full python3-pip python3-setuptools \
    libsdl2-dev libopenal-dev libfreetype6-dev libfontconfig1-dev xz-utils make \
 && apt-get upgrade --yes \
 && rm -rf /var/lib/apt/lists/*

# Install modern GCC 15 toolchain from tttapa/toolchains (targets bionic glibc 2.27)
ARG TOOLCHAIN_GCC_VERSION="gcc15"
ARG TOOLCHAIN_TRIPLET="x86_64-bionic-linux-gnu"
RUN curl -L https://github.com/tttapa/toolchains/releases/latest/download/x-tools-${TOOLCHAIN_TRIPLET}-${TOOLCHAIN_GCC_VERSION}.tar.xz \
    | tar xJ -C /opt
ENV PATH="/opt/x-tools/${TOOLCHAIN_TRIPLET}/bin:${PATH}"

# We need a newer ccache for compression support
ARG CCACHE_VERSION="4.10.2"
RUN curl -L -O https://github.com/ccache/ccache/releases/download/v${CCACHE_VERSION}/ccache-${CCACHE_VERSION}-linux-x86_64.tar.xz \
 && tar -xf ccache-${CCACHE_VERSION}-linux-x86_64.tar.xz \
 && mv ccache-${CCACHE_VERSION}-linux-x86_64/ccache /usr/bin \
 && rm -rf ccache-${CCACHE_VERSION}-linux-x86_64*

# And need newer zstd :(, fortunately it's quick build
RUN git clone --depth 1 --branch v1.5.7 https://github.com/facebook/zstd.git \
 && cd zstd \
 && CC=${TOOLCHAIN_TRIPLET}-gcc make -j$(nproc) \
 && mv programs/zstd /usr/local/bin \
 && cd .. \
 && rm -rf zstd

RUN pip3 install --upgrade pip \
 && pip3 install scikit-build

ARG CMAKE_VERSION="4.0.2"
RUN curl -L https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz \
    | tar xz -C /usr/local --strip-components=1

WORKDIR /build
RUN mkdir src cache out artifacts && chmod a+rwx cache out artifacts

# Fetch library dependencies and configure resolution
RUN git clone --depth=1 https://github.com/beyond-all-reason/spring-static-libs.git -b ${STATIC_LIBS_BRANCH} spring-static-libs

# Populate the toolchain sysroot so GCC finds system headers and libraries.
# Headers: prefer spring-static-libs, then fill gaps with system -dev packages.
# Libraries: prefer spring-static-libs static archives, then system shared stubs.
RUN SYSROOT=/opt/x-tools/x86_64-bionic-linux-gnu/x86_64-bionic-linux-gnu/sysroot \
 && for item in /build/spring-static-libs/include/*; do \
      ln -sf "$item" "${SYSROOT}/usr/include/$(basename $item)"; \
    done \
 && for d in X11 SDL2 AL freetype2 fontconfig; do \
      ln -sf "/usr/include/$d" "${SYSROOT}/usr/include/$d"; \
    done \
 && ln -sf /usr/include/x86_64-linux-gnu "${SYSROOT}/usr/include/x86_64-linux-gnu" \
 && for f in /build/spring-static-libs/lib/lib*.a /build/spring-static-libs/lib/lib*.so; do \
      [ -e "$f" ] && ln -sf "$f" "${SYSROOT}/usr/lib/$(basename $f)"; \
    done \
 && ln -sf /usr/lib/x86_64-linux-gnu "${SYSROOT}/usr/lib/x86_64-linux-gnu" \
 && ln -sf /lib/x86_64-linux-gnu "${SYSROOT}/lib/x86_64-linux-gnu"

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
