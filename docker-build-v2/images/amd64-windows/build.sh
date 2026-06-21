#!/bin/bash
cd "$(dirname "$(readlink -f "$0")")/.."
source ../_resolve_container_runtime.sh
source ../images_versions.sh
exec ${RUNTIME} build \
    -t recoil-build-amd64-windows \
    --platform=linux/amd64 \
    --build-arg RECOIL_LIBS_VERSION=${RECOIL_LIBS_VERSION} \
    -f amd64-windows/Dockerfile "$@" .
