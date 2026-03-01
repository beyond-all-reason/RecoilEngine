#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
docker build \
  --platform=linux/arm64 \
  --build-arg ENGINE_PLATFORM=arm64-linux \
  --build-arg STATIC_LIBS_BRANCH=18.04-arm \
  -f all-linux/Dockerfile \
  "$@" .
