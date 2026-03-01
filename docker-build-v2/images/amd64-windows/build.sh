#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
docker build \
  -f amd64-windows/Dockerfile \
  "$@" .
