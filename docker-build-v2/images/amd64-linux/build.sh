#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
docker build \
  --platform=linux/amd64 \
  --build-arg ENGINE_PLATFORM=amd64-linux \
  --build-arg STATIC_LIBS_BRANCH=18.04 \
  -f all-linux/Dockerfile \
  "$@" .
