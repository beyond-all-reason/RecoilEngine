#!/bin/bash
cd "$(dirname "$(readlink -f "$0")")/.."
source ../_resolve_container_runtime.sh
exec ${RUNTIME} build \
    -t recoil-build-amd64-linux \
    --platform=linux/amd64 \
    --build-arg ENGINE_PLATFORM=amd64-linux \
    --build-arg STATIC_LIBS_BRANCH=18.04 \
    -f all-linux/Dockerfile "$@" .
