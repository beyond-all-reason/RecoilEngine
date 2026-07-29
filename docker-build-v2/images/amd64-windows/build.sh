#!/bin/bash
cd "$(dirname "$(readlink -f "$0")")/.."
source ../_resolve_container_runtime.sh
exec ${RUNTIME} build \
    -t recoil-build-amd64-windows \
    --platform=linux/amd64 \
    -f amd64-windows/Dockerfile "$@" .
