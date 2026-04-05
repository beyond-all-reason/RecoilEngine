#!/bin/bash
cd "$(dirname "$(readlink -f "$0")")/../../.."
source docker-build-v2/_resolve_container_runtime.sh
exec ${RUNTIME} build \
    -t recoil-build-arm64-linux \
    --platform=linux/arm64 \
    --build-arg ARCH=arm64 \
    -f docker-build-v2/images/all-linux/Dockerfile "$@" .
