#!/bin/bash

# Pass MINGWLIBS when libs are mounted via --local-libs (at /build/recoil-libs).
MINGWLIBS_FLAG=()
if [ -d /build/recoil-libs ]; then
    MINGWLIBS_FLAG=(-DMINGWLIBS=/build/recoil-libs)
fi

cmake --fresh -S /build/src -B /build/out \
    -DCMAKE_INSTALL_PREFIX:PATH=/build/out/install \
    -DAI_EXCLUDE_REGEX="^CppTestAI$" \
    -DUSERDOCS_PLAIN=ON \
    -DCMAKE_BUILD_TYPE=RELWITHDEBINFO \
    -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG" \
    -DCMAKE_C_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG" \
    -G Ninja \
    "${MINGWLIBS_FLAG[@]}" \
    "$@"
