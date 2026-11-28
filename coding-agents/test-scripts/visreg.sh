#!/bin/bash
# Visual regression. Play a replay in a real game window, take engine screenshots
# at fixed sim frames from a fixed camera, and compare them with approved
# baselines in visreg-baselines/<engine version>/<demo>/ beside this script.
#
#   visreg.sh --demo <file.sdfz> [--bless]
#
# --bless copies the pictures into visreg-baselines as the new baselines. Only do
# that after a person has looked at them and approved them.
#
# Opens a game window for about two minutes, through run-capped.sh, so the memory
# ceiling applies and the game and map come from ~/dev/spring-testdata. Ask before
# running it. It needs the SPRING_DEMO_SPEED commit, or the replay plays at 1x
# and the run hits its time limit. Prints VISREG_OK, VISREG_FAIL or
# VISREG_NO_BASELINE. The pictures are kept in ~/dev/spring-testdata/logs/.
#
# Pictures are keyed to sim frames, and the window size is fixed in the run's
# own config. Unsynced animation such as water and sky still moves a little
# between runs, so a shot fails only when its mean per-channel difference
# exceeds VISREG_MAX_MEAN (default 2.0 of 255), when more than VISREG_MAX_BAD_PCT
# percent of pixels (default 1.0) differ by over 40 in any channel, or when over
# 60 percent of any 64 px window differs by over 40. A limits.txt beside the
# baselines overrides the first two per picture.
#
# The baselines belong to one exact demo. --bless records its sha256 in
# demo.sha256 beside them, and a run with a different file stops before playing.
#
# The comparison needs python3 with Pillow (visreg/requirements.txt).
#
# From Adam Burgess's scripts/visreg.sh in qant-au/tellurion, changed to run this
# tree's build-macos engine through run-capped.sh.
set -uo pipefail
HERE=$(cd "$(dirname "$0")" && pwd -P)
REPO=$(cd "$HERE/../.." && pwd -P)
BUILD="$REPO/build-macos"
DEMO=""
BLESS=0
while [ $# -gt 0 ]; do
  case "$1" in
    --demo) DEMO=$2; shift 2;;
    --bless) BLESS=1; shift;;
    --perf) shift;;
    *) echo "unknown option: $1"; exit 2;;
  esac
done
[ -f "$DEMO" ] || { echo "VISREG_FAIL demo not found: ${DEMO:-none given, use --demo}"; exit 1; }
[ -x "$BUILD/spring" ] || { echo "VISREG_FAIL engine not found: $BUILD/spring"; exit 1; }

ENGINE_VER=$("$BUILD/spring" --version 2>/dev/null | grep -oE '20[0-9]{2}\.[0-9]{2}\.[0-9]{2}' | head -1)
NAME=$(basename "$DEMO" .sdfz)
BASE="$HERE/visreg-baselines/$ENGINE_VER/$NAME"
DEMO_SHA=$(shasum -a 256 "$DEMO" | awk '{print $1}')
[ -n "$DEMO_SHA" ] || { echo "VISREG_FAIL cannot hash the demo (shasum -a 256 $DEMO)"; exit 1; }
SHA_FILE="$BASE/demo.sha256"
# Checks for a run that will be compared, made before the two-minute replay.
if [ "$BLESS" != "1" ] && [ -d "$BASE" ]; then
  python3 -c 'import PIL' 2>/dev/null || { echo "VISREG_FAIL the comparison needs the Pillow package: python3 -m pip install -r $HERE/visreg/requirements.txt"; exit 1; }
  if [ -f "$SHA_FILE" ]; then
    # A different demo under the same name would be judged against pictures
    # of another game.
    WANT_SHA=$(awk '!/^#/ && NF {print $1; exit}' "$SHA_FILE")
    [ "$DEMO_SHA" = "$WANT_SHA" ] || { echo "VISREG_FAIL demo is not the one the baselines were made from: $DEMO has sha256 $DEMO_SHA, $SHA_FILE records $WANT_SHA"; exit 1; }
  else
    echo "note: no demo.sha256 beside the baselines in $BASE, so the demo is not checked (--bless writes it)"
  fi
fi
STAMP=$(date +%Y%m%d-%H%M%S)
OUT="$HOME/dev/spring-testdata/logs/visreg-$STAMP"; mkdir -p "$OUT"

# Isolated write dir: the widget, the shot list and fixed settings live here, so
# the player's own settings and widgets are never touched.
WD=$(mktemp -d "${TMPDIR:-/tmp}/visreg.XXXXXX")
trap 'rm -rf "$WD"' EXIT
mkdir -p "$WD/LuaUI/Widgets"
cp "$HERE/visreg/visreg_capture.lua" "$WD/LuaUI/Widgets/"
# VISREG_SHOW_INTERFACE=1 keeps the interface in the pictures (for checks of how
# it is drawn), the baselines are taken without it
if [ "${VISREG_SHOW_INTERFACE:-0}" = "1" ]; then
  sed -i '' '/hideinterface 1/d' "$WD/LuaUI/Widgets/visreg_capture.lua"
fi
# <frame> <name> <x-fraction> <z-fraction> <camera height>
cat > "$WD/LuaUI/visreg_frames.txt" <<'SHOTS'
900 01-start-overview 0.5 0.5 9000
900 02-start-west 0.25 0.5 2500
6000 03-early-east 0.75 0.5 2500
12000 04-mid-overview 0.5 0.5 9000
18000 05-late-west 0.25 0.5 2500
24000 06-end-east 0.75 0.5 2500
SHOTS
cat > "$WD/springsettings.cfg" <<'CFG'
Fullscreen = 0
XResolutionWindowed = 1280
YResolutionWindowed = 720
WindowPosX = 40
WindowPosY = 40
WindowBorderless = 0
VSync = 0
EdgeMoveWidth = 0
WindowedEdgeMove = 0
Sound = 0
CFG
# extra engine settings for one run, e.g. VISREG_EXTRA_SETTINGS='GPUTimers = 1'
# (newline-separated); the pictures are still compared against the baselines
[ -n "${VISREG_EXTRA_SETTINGS:-}" ] && printf '%s\n' "$VISREG_EXTRA_SETTINGS" >> "$WD/springsettings.cfg"

echo "visreg: $NAME on $ENGINE_VER (window opens for about two minutes)"
SPRING_DEMO_SPEED=20 BUILD="$BUILD" SCRIPT="$DEMO" "$HERE/run-capped.sh" 420 "$OUT/engine.log" \
  --write-dir "$WD" --config "$WD/springsettings.cfg" > "$OUT/run-capped.out" 2>&1
RC=$?
cp "$WD/infolog.txt" "$OUT/" 2>/dev/null || true

# map screenshots to shot names in order (the engine names them by time)
SHOTS_TAKEN=$(grep -c '\[visreg\] SHOT ' "$OUT/infolog.txt" 2>/dev/null || echo 0)
i=0
grep -oE '\[visreg\] SHOT \S+' "$OUT/infolog.txt" 2>/dev/null | awk '{print $3}' > "$OUT/names.txt"
ls "$WD"/screenshots/*.png 2>/dev/null | sort > "$OUT/files.txt"
paste -d' ' "$OUT/names.txt" "$OUT/files.txt" | while read -r n f; do
  [ -n "$n" ] && [ -f "$f" ] && cp "$f" "$OUT/$n.png"
done
EXPECTED=$(wc -l < "$WD/LuaUI/visreg_frames.txt" 2>/dev/null || echo 6)
GOT=$(ls "$OUT"/[0-9]*.png 2>/dev/null | wc -l | tr -d ' ')
if ! grep -q '\[visreg\] DONE' "$OUT/infolog.txt" 2>/dev/null || [ "$GOT" -ne "$EXPECTED" ]; then
  echo "VISREG_FAIL capture incomplete: $GOT of $EXPECTED pictures, engine exit $RC - see $OUT"
  exit 1
fi

if [ "$BLESS" = "1" ]; then
  mkdir -p "$BASE"; cp "$OUT"/[0-9]*.png "$BASE/"
  # shasum's own format, so 'shasum -a 256 -c' beside the demo checks it too
  printf '%s  %s\n' "$DEMO_SHA" "$(basename "$DEMO")" > "$SHA_FILE"
  echo "VISREG_BLESSED $GOT baselines -> $BASE"; exit 0
fi
if [ ! -d "$BASE" ]; then
  echo "VISREG_NO_BASELINE pictures for review: $OUT (approve, then rerun with --bless)"
  exit 3
fi
python3 "$HERE/visreg/compare.py" "$BASE" "$OUT" \
  --max-mean "${VISREG_MAX_MEAN:-2.0}" --max-bad-pct "${VISREG_MAX_BAD_PCT:-1.0}"
