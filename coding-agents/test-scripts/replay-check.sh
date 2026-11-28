#!/bin/bash
# Replay sync check. Re-simulate a recorded game on this engine and fail on any
# sync divergence from the machine that recorded it.
#
# A demo (.sdfz) carries the per-frame sync checksums of the client that
# recorded it, usually an x86 Windows or Linux client on BAR's servers.
# spring-headless replays the command stream through the full simulation and
# compares every frame. One differing bit logs "[DESYNC WARNING] checksum ...".
#
#   replay-check.sh <demo.sdfz> [--engine <spring-headless>] [--speed <x>]
#                   [--timeout <secs>] [--log <file>]
#
# --engine defaults to build-macos/spring-headless. The run goes through
# run-capped.sh, so it gets the same memory ceiling and reads its game and map
# from ~/dev/spring-testdata. --speed (default 20) needs the SPRING_DEMO_SPEED
# commit, without it playback runs at 1x and the speed is ignored.
#
# Prints REPLAY_SYNC_OK and exits 0 on a clean run past the demo's last recorded
# checksum. A demo with no checksums is refused. Prints REPLAY_SYNC_FAIL <reason>
# and exits 1 otherwise, including VERSION_MISMATCH when the demo was recorded on
# another engine version.
#
# find-fleet-replays.py <engine version> lists public BAR replays recorded on
# that version. Its game and map have to be in ~/dev/spring-testdata, which
# pr-downloader can fetch when pointed at BAR's content network:
#   PRD_RAPID_REPO_MASTER=https://repos-cdn.beyondallreason.dev/repos.gz \
#   PRD_HTTP_SEARCH_URL=https://files-cdn.beyondallreason.dev/find \
#   build-macos/tools/pr-downloader/src/pr-downloader \
#     --filesystem-writepath ~/dev/spring-testdata --download-map "<map name>"
#
# From Adam Burgess's scripts/replay-check.sh in qant-au/tellurion, changed to
# run through run-capped.sh with this tree's paths.
set -uo pipefail

HERE=$(cd "$(dirname "$0")" && pwd -P)
REPO=$(cd "$HERE/../.." && pwd -P)
DEMO=""
ENGINE="$REPO/build-macos/spring-headless"
TIMEOUT=7200; LOG=""; SPEED=20
while [ $# -gt 0 ]; do
  case "$1" in
    --engine) ENGINE=$2; shift 2;;
    --timeout) TIMEOUT=$2; shift 2;;
    --speed) SPEED=$2; shift 2;;
    --log) LOG=$2; shift 2;;
    -*) echo "unknown option: $1"; exit 2;;
    *) DEMO=$1; shift;;
  esac
done
[ -f "$DEMO" ] || { echo "usage: $0 <demo.sdfz> [--engine ..] [--timeout ..] [--log ..]"; exit 2; }
[ -x "$ENGINE" ] || { echo "REPLAY_SYNC_FAIL engine not found: $ENGINE"; exit 1; }
DEMO=$(cd "$(dirname "$DEMO")" && pwd)/$(basename "$DEMO")
ENGINE_DIR=$(cd "$(dirname "$ENGINE")" && pwd)

# Version check: the demo header names the engine that recorded it.
DEMO_VER=$(python3 -c "import gzip,re,sys; h=gzip.open(sys.argv[1]).read(512).decode('latin-1'); m=re.search(r'(20\d\d\.\d\d\.\d\d)', h); print(m.group(1) if m else '')" "$DEMO" 2>/dev/null)
BIN_VER=$("$ENGINE" --version 2>/dev/null | grep -oE '20[0-9]{2}\.[0-9]{2}\.[0-9]{2}' | head -1)
if [ -z "$DEMO_VER" ] || [ "$DEMO_VER" != "$BIN_VER" ]; then
  echo "REPLAY_SYNC_FAIL VERSION_MISMATCH: demo=${DEMO_VER:-unknown} binary=${BIN_VER:-unknown}"; exit 1
fi

# Positive evidence: the engine is silent when checksums match, so a demo that
# carries none would pass whatever the simulation did.
read -r SYNC_PACKETS SYNC_LAST_FRAME < <(python3 "$(dirname "$0")/demo-sync-packets.py" "$DEMO" 2>/dev/null)
if [ "${SYNC_PACKETS:-0}" -lt 1 ]; then
  echo "REPLAY_SYNC_FAIL the demo carries no sync checksums, so a replay of it proves nothing"; exit 1
fi

# Its own write dir, so a replay never touches the shared settings.
WD=$(mktemp -d "${TMPDIR:-/tmp}/replay-check.XXXXXX")
[ -n "$LOG" ] || LOG="$WD/replay.log"
trap 'rm -rf "$WD/LuaUI" "$WD/cache" 2>/dev/null' EXIT

echo "replay-check: $(basename "$DEMO") on $BIN_VER ($ENGINE)"
START=$(date +%s)
# run-capped.sh adds --isolation-dir ~/dev/spring-testdata and kills the engine
# above its memory ceiling or after the time limit
SPRING_DEMO_SPEED="$SPEED" BUILD="$ENGINE_DIR" LAUNCHER="./$(basename "$ENGINE")" SCRIPT="$DEMO" \
  "$HERE/run-capped.sh" "$TIMEOUT" "$LOG" --write-dir "$WD" > "$WD/run-capped.out" 2>&1
RC=$?
SECS=$(( $(date +%s) - START ))
LAST_FRAME=$(grep -oE '\[f=[0-9]+\]' "$LOG" | tail -1 | tr -dc '0-9' | sed 's/^0*//')
DESYNCS=$(grep -c 'DESYNC WARNING' "$LOG")
# the game's own end (BAR logs <autoquit> there); later frames are post-game
# filler while autoquit waits, with no recorded checksums to compare
END_FRAME=$(grep -m1 '<autoquit>' "$LOG" | grep -oE '\[f=[0-9]+\]' | tr -dc '0-9' | sed 's/^0*//')

if ! grep -q 'Beginning demo playback' "$LOG"; then
  echo "REPLAY_SYNC_FAIL playback never started (missing game/map content?) - see $LOG"
  grep -E 'Fatal|content_error' "$LOG" | head -3
  exit 1
fi
if [ "$DESYNCS" -gt 0 ]; then
  echo "REPLAY_SYNC_FAIL $DESYNCS desync warning(s); first:"
  grep -m1 'DESYNC WARNING' "$LOG"
  exit 1
fi
if [ "$RC" -ne 0 ]; then
  echo "REPLAY_SYNC_FAIL engine exit $RC after ${SECS}s at frame ${LAST_FRAME:-?} (2 = memory ceiling, see $WD/run-capped.out) - see $LOG"
  exit 1
fi
if [ "${LAST_FRAME:-0}" -lt "$SYNC_LAST_FRAME" ]; then
  echo "REPLAY_SYNC_FAIL playback stopped at frame ${LAST_FRAME:-?}, before the demo's last checksum at frame $SYNC_LAST_FRAME - see $LOG"
  exit 1
fi
echo "REPLAY_SYNC_OK game_end_frame=${END_FRAME:-?} last_frame=${LAST_FRAME:-?} desyncs=0 checksums=$SYNC_PACKETS speed=${SPEED}x wall=${SECS}s log=$LOG"
