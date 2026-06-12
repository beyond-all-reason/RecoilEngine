#!/bin/bash
cd "$(dirname "$(readlink -f "$0")")/.."
source ../_resolve_container_runtime.sh
exec ${RUNTIME} build \
    -t recoil-build-arm64-linux \
    --platform=linux/arm64 \
    --build-arg ENGINE_PLATFORM=arm64-linux \
    --build-arg STATIC_LIBS_BRANCH=18.04-arm \
    -f all-linux/Dockerfile "$@" .
