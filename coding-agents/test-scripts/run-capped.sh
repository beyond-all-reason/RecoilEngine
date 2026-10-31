#!/bin/bash
# Run the engine with a hard memory ceiling and a hard time limit.
#
# A leak in the engine can take this machine to 54 GB and freeze it so completely
# that the out-of-memory dialog itself stops responding, which costs a power
# cycle. macOS has no cgroup and ulimit -v is useless against Metal allocations,
# so poll RSS and SIGKILL.
#
#   run-capped.sh <seconds> <logfile> [extra spring args...]
#
# Environment is passed through to the engine.
set -u

LIMIT_SECONDS=${1:?seconds}
LOGFILE=${2:?logfile}
shift 2

# paths derived from where this script lives, so any checkout works
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)

BUILD=${BUILD:-$REPO/build-macos}
# SCRIPT= points it at a different start script, which is how a run against
# another game is held to the same map and resolution.
SCRIPT=${SCRIPT:-$HERE/splinterfaction-fixed-start.tdf}
# The binary loads Mesa itself, from <build>/mesa (a symlink to a Mesa install
# prefix) before falling back to the prefix compiled in, so no launcher is
# needed. Set LAUNCHER= to run something else, such as the spring launcher
# inside a packaged engine.
LAUNCHER=${LAUNCHER:-./spring}

# kill above this many kilobytes of phys_footprint
# The machine has 16 GiB of RAM and GPU memory is the same RAM, so a ceiling
# near the total freezes it before the kill lands. 10.5 GiB leaves room for the
# system and for the growth between two polls, and clears the BAR benchmark's
# footprint of about 10 GB.
MAX_RSS_KB=${MAX_RSS_KB:-11010048}   # 10.5 GiB of phys_footprint
# The kill check reads only phys_footprint, so a pass is cheap. The slower
# sampling (ps and top) runs only every LOG_SECONDS. A pass that also ran ps,
# footprint twice and top once let a 1 GB/s leak overshoot a 10.5 GiB ceiling
# to 15 GB.
POLL_SECONDS=0.5
LOG_SECONDS=4

# FULLSCREEN=1 runs in fullscreen instead of a window, which some rendering
# faults need: the view then sits below the menu bar strip of the screen
WINDOW_FLAG=--window
[ "${FULLSCREEN:-0}" = 1 ] && WINDOW_FLAG=--fullscreen

cd "$BUILD" || exit 1

"$LAUNCHER" --isolation-dir ~/dev/spring-testdata "$WINDOW_FLAG" "$SCRIPT" "$@" \
	> "$LOGFILE" 2>&1 &
PID=$!

echo "engine pid $PID, ceiling $((MAX_RSS_KB / 1024)) MiB, limit ${LIMIT_SECONDS}s"

KILLED_FOR_MEMORY=0
PEAK_KB=0

# wall clock, not iterations: a pass costs time of its own on top of the sleep,
# so counting iterations ran a 180s limit for 300s
SECONDS=0
ELAPSED=0
LAST_LOG=$((-LOG_SECONDS))

while kill -0 "$PID" 2>/dev/null; do
	# Cap on phys_footprint, not RSS. GPU and system memory are the same silicon
	# on Apple Silicon, so IOAccelerator allocations are real pressure, and ps
	# cannot see them: measured 50 GB of footprint while RSS read 1349 MB.
	FOOT_KB=$(footprint -p "$PID" 2>/dev/null | awk '/phys_footprint:/ {
		v = $2; u = $3
		if (u ~ /^GB/) print v * 1048576
		else if (u ~ /^MB/) print v * 1024
		else if (u ~ /^KB/) print v
		else print v / 1024
		exit
	}')
	FOOT_KB=${FOOT_KB%.*}

	if [ -n "$FOOT_KB" ] && [ "$FOOT_KB" -gt 0 ] 2>/dev/null; then
		[ "$FOOT_KB" -gt "$PEAK_KB" ] && PEAK_KB=$FOOT_KB

		if [ "$FOOT_KB" -gt "$MAX_RSS_KB" ]; then
			echo "MEMORY CEILING HIT at $((FOOT_KB / 1024)) MiB footprint after ${ELAPSED}s, killing"
			KILLED_FOR_MEMORY=1
			kill -9 "$PID" 2>/dev/null
			break
		fi

		# RSS ignores IOAccelerator entirely, and top's MEM counts reserved GPU
		# address space, so log all three beside the footprint the cap uses.
		# in the background, because top -l 1 blocks for about a second, long
		# enough for a leak to overshoot the ceiling by a gigabyte
		if [ $((ELAPSED - LAST_LOG)) -ge "$LOG_SECONDS" ]; then
			LAST_LOG=$ELAPSED
			(
				RSS_KB=$(ps -o rss= -p "$PID" 2>/dev/null | tr -d ' ')
				TOPMEM=$(top -l 1 -pid "$PID" -stats mem 2>/dev/null | tail -1 | tr -d ' ')
				echo "$ELAPSED rss=$((${RSS_KB:-0} / 1024))M top=${TOPMEM:-?} foot=$((FOOT_KB / 1024))M" >> "$LOGFILE.mem"
			) &
		fi
	fi

	if [ "$ELAPSED" -ge "$LIMIT_SECONDS" ]; then
		kill -TERM "$PID" 2>/dev/null
		break
	fi

	sleep "$POLL_SECONDS"
	ELAPSED=$SECONDS
done

sleep 3
if pgrep -f '\./spring' > /dev/null; then
	pkill -9 -f '\./spring'
	sleep 1
fi

echo "peak footprint $((PEAK_KB / 1024)) MiB over ${ELAPSED}s"
if [ "$KILLED_FOR_MEMORY" = 1 ]; then
	echo "RESULT: killed for memory, do not trust anything this run produced"
	exit 2
fi
if pgrep -f '\./spring' > /dev/null; then
	echo "RESULT: engine still running after cleanup"
	exit 3
fi
echo "RESULT: clean exit"
