#!/bin/bash
# streflop cross-architecture sync test. Builds tools/sync-test natively (NEON on
# Apple Silicon), runs the reference suite, and requires a bit-exact match with
# the committed x86 SSE and arm64 NEON references. CPU only, it takes seconds.
#
#   sync-test.sh [--build-dir DIR] [-n N]    (default N=10000)
#
# The same check as upstream's "STREFLOP Float Sync Test" workflow, run locally.
# Prints SYNCTEST_OK or SYNCTEST_FAIL. From Adam Burgess's scripts/run-synctest.sh
# in qant-au/tellurion, with the paths changed for this tree.
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd -P)
REPO=$(cd "$HERE/../.." && pwd -P)
SRC="$REPO"
BUILD="$REPO/build-synctest"
N=10000
# fewest tests that must be compared against each reference (47,852 and 52,080
# at the default N); set SYNCTEST_MIN_COMMON when running with a smaller -n
MIN_COMMON="${SYNCTEST_MIN_COMMON:-40000}"
while [ $# -gt 0 ]; do
  case "$1" in
    --build-dir) BUILD=$2; shift 2;;
    -n) N=$2; shift 2;;
    *) echo "unknown arg: $1"; exit 2;;
  esac
done
REF="$SRC/tools/sync-test/reference"

cmake -S "$SRC/tools/sync-test" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null
ninja -C "$BUILD" streflop-float-test >/dev/null
( cd "$BUILD" && ./streflop-float-test -n "$N" synctest_local >/dev/null )

FAIL=0
for ref in streflop_results_SSE_x86_64 streflop_results_NEON_arm64; do
  [ -f "$REF/$ref.bin" ] || { echo "FATAL: missing reference $REF/$ref.bin"; exit 1; }
  out=$(python3 "$SRC/tools/sync-test/compare_results.py" "$REF/$ref.bin" "$BUILD/synctest_local.bin" 2>&1)
  verdict=$(echo "$out" | grep -E '^RESULT:' | tail -1)
  common=$(echo "$out" | grep -E 'Common tests:' | awk '{print $3}')
  echo "vs $ref: $verdict (common tests: ${common:-?})"
  echo "$verdict" | grep -q 'BIT-EXACT MATCH' || { echo "$out" | tail -20; FAIL=1; }
  # the comparison prints BIT-EXACT for zero tests in common, so require a floor
  common_n=$(printf '%s' "${common:-0}" | tr -dc '0-9')
  [ "${common_n:-0}" -ge "$MIN_COMMON" ] || { echo "only ${common:-0} tests in common with $ref (need $MIN_COMMON)"; FAIL=1; }
done
[ "$FAIL" = 0 ] && echo "SYNCTEST_OK" || { echo "SYNCTEST_FAIL"; exit 1; }
