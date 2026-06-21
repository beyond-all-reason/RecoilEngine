#!/bin/bash
cd "$(dirname "$(readlink -f "$0")")/.."
source ../_resolve_container_runtime.sh
source ../images_versions.sh
exec ${RUNTIME} build \
    -t recoil-build-arm64-linux \
    --platform=linux/arm64 \
    --build-arg ENGINE_PLATFORM=arm64-linux \
    --build-arg RECOIL_LIBS_VERSION=${RECOIL_LIBS_VERSION} \
    -f all-linux/Dockerfile "$@" .
