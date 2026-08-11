#!/bin/bash

set -euo pipefail

: "${ENGINE_PLATFORM:=linux}"

rm -rf /build/out/install /build/artifacts/*
mkdir -p /build/out/install/bin /build/out/install/symbols
printf 'engine\n' > /build/out/install/bin/spring
printf 'debug symbols\n' > "/build/out/install/symbols/with space.dbg"
printf '2024.01.02-0-gabcdef1 feature/name\n' > /build/out/VERSION

# The old script recomputes a Git description. Make that value deliberately
# different from VERSION so the archive-name contract is observable without
# relying on the checkout's Git metadata inside the container.
stub_dir=$(mktemp -d)
trap 'rm -rf "$stub_dir"' EXIT
printf '%s\n' \
    '#!/bin/sh' \
    'if [ "${1-}" = describe ]; then' \
    '    printf "%s\\n" "2026.07.01-99-gdeadbee"' \
    '    exit 0' \
    'fi' \
    'exec /usr/bin/git "$@"' > "$stub_dir/git"
chmod +x "$stub_dir/git"

PATH="$stub_dir:$PATH" \
    ENGINE_PLATFORM="$ENGINE_PLATFORM" \
    /build/src/docker-build-v2/scripts/package.sh

archive_name="recoil_2024.01.02-0-gabcdef1-feature-name_${ENGINE_PLATFORM}.7z"
debug_name="recoil_2024.01.02-0-gabcdef1-feature-name_${ENGINE_PLATFORM}-dbgsym.tar.zst"

test -s "/build/artifacts/$archive_name"
test -s "/build/artifacts/$debug_name"

gzip --decompress --stdout /build/out/install/files.md5.gz \
    | (cd /build/out/install && md5sum --check -)

zstd --decompress --stdout "/build/artifacts/$debug_name" \
    | tar --list --file=- \
    | grep --fixed-strings --line-regexp './symbols/with space.dbg'

printf 'package test passed: %s and %s\n' "$archive_name" "$debug_name"
