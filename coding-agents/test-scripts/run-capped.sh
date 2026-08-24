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
# Environment passed through, so SPRING_PHASE_DUMP etc still work.
set -u

LIMIT_SECONDS=${1:?seconds}
LOGFILE=${2:?logfile}
shift 2

# paths derived from where this script lives, so any checkout works
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)

BUILD=${BUILD:-$REPO/build-macos-legacy}
# SCRIPT= points it at a different start script, which is how a run against
# another game is held to the same map and resolution.
SCRIPT=${SCRIPT:-$HERE/splinterfaction-fixed-start.tdf}
# Default to the pre-Metal4 Mesa. 26.2.0-rc3 carries c08dba83025, which never
# reclaims the memory a render pass allocates, and the engine dies at about 12
# seconds. Set LAUNCHER=./run-macos.sh to go back to it for an A/B.
LAUNCHER=${LAUNCHER:-./run-macos-premtl4.sh}

# kill above this many kilobytes of resident memory
# 16 GB of physical RAM on this machine, usually with a colima VM alongside, so
# the headroom is much smaller than the total suggests
# Loading alone reaches about 10 GiB of footprint, and the engine then grows at
# roughly 5 GiB/s on this driver, so a low ceiling kills during load and a high
# one lets the machine freeze. 20 GiB clears loading and stops well short of the
# 54 GiB that took the system down.
MAX_RSS_KB=${MAX_RSS_KB:-20971520}   # 20 GiB of phys_footprint
POLL_SECONDS=2

cd "$BUILD" || exit 1

"$LAUNCHER" --isolation-dir ~/dev/spring-testdata --window "$SCRIPT" "$@" \
	> "$LOGFILE" 2>&1 &
PID=$!

echo "engine pid $PID, ceiling $((MAX_RSS_KB / 1024)) MiB, limit ${LIMIT_SECONDS}s"

KILLED_FOR_MEMORY=0
ELAPSED=0
PEAK_KB=0

while kill -0 "$PID" 2>/dev/null; do
	# Cap on ps RSS, which is known to behave. Log the others beside it: RSS
	# ignores IOAccelerator entirely, and top's MEM counts reserved GPU address
	# space, reporting 9.6 GiB six seconds into a load on a 16 GB machine. Which
	# of these tracks real pressure is an open question, so record all three.
	RSS_KB=$(ps -o rss= -p "$PID" 2>/dev/null | tr -d ' ')

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

		TOPMEM=$(top -l 1 -pid "$PID" -stats mem 2>/dev/null | tail -1 | tr -d ' ')
		FOOT=$(footprint -p "$PID" 2>/dev/null | awk '/phys_footprint:/ {print $2 $3; exit}')
		echo "$ELAPSED rss=$((RSS_KB / 1024))M top=${TOPMEM:-?} foot=${FOOT:-?}" >> "$LOGFILE.mem"
		if [ "$FOOT_KB" -gt "$MAX_RSS_KB" ]; then
			echo "MEMORY CEILING HIT at $((FOOT_KB / 1024)) MiB footprint after ${ELAPSED}s, killing"
			KILLED_FOR_MEMORY=1
			kill -9 "$PID" 2>/dev/null
			break
		fi
	fi

	if [ "$ELAPSED" -ge "$LIMIT_SECONDS" ]; then
		kill -TERM "$PID" 2>/dev/null
		break
	fi

	sleep "$POLL_SECONDS"
	ELAPSED=$((ELAPSED + POLL_SECONDS))
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
