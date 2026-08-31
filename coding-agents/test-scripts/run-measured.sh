#!/bin/bash
# Run the engine for a measurement, with the window brought to the front and
# focus monitored for the whole run.
#
#   run-measured.sh <seconds> <logfile> [extra spring args...]
#
# Why this exists. Every frame rate figure produced before it was measured on a
# backgrounded and probably occluded window. macOS drops the scheduling priority
# of a background app and marks fully covered windows as occluded, and focus
# alone moved one result by 20%, which is larger than most differences worth
# measuring. So a run where the engine was not frontmost throughout is void, and
# this says so rather than leaving it to be noticed later.
#
# What it does not check: occlusion. There is no way to read NSWindow
# occlusionState from a shell, so frontmost is the proxy. Keep the window
# uncovered as well as focused.
#
# Wraps run-capped.sh, so the memory ceiling and the time limit still apply.
#
# To prove what was on screen rather than watching it, use
# install-probe.sh --shots. Driving the engine's screenshot action from here
# through synthetic keystrokes was tried and produced no files at all.
set -u

LIMIT_SECONDS=${1:?seconds}
LOGFILE=${2:?logfile}
shift 2

HERE=$(dirname "$0")
POLL_SECONDS=2

if pgrep -f 'spring-premtl4|\./spring' > /dev/null; then
	echo "REFUSING: an engine is already running. A second one aborts in"
	echo "UDPListener::TryBindSocket and the failure looks like a code fault."
	pgrep -fl 'spring-premtl4|\./spring'
	exit 1
fi

frontmost_pid() {
	osascript -e 'tell application "System Events" to get unix id of first process whose frontmost is true' 2>&1
}

CAPPED_OUT=$(mktemp -t run-measured)
"$HERE/run-capped.sh" "$LIMIT_SECONDS" "$LOGFILE" "$@" > "$CAPPED_OUT" 2>&1 &
CAPPED_JOB=$!

# run-capped.sh prints "engine pid N" as its first line
PID=""
for _ in $(seq 1 15); do
	# strip the trailing comma of "engine pid N, ceiling ..."
	PID=$(awk '/^engine pid /{gsub(/[^0-9]/, "", $3); print $3; exit}' "$CAPPED_OUT" 2>/dev/null)
	[ -n "$PID" ] && break
	sleep 1
done

if [ -z "$PID" ]; then
	echo "RESULT: void, never saw an engine pid from run-capped.sh"
	cat "$CAPPED_OUT"  # bash-guard: allow
	wait "$CAPPED_JOB"
	exit 4
fi

echo "engine pid $PID, bringing it to the front"

focus_engine() {
	osascript -e "tell application \"System Events\" to set frontmost of (first process whose unix id is $PID) to true" 2>&1
}

# Ask, then check. "set frontmost" returns success against a process that has no
# window yet, which it does not have for the first several seconds of loading, so
# trusting the return code declares victory about six seconds early and the run
# spends its whole length in the background. That is what voided the first
# attempt at this.
FOCUS_SET=0
for _ in $(seq 1 90); do
	LAST_ERR=$(focus_engine)
	if [ "$(frontmost_pid)" = "$PID" ]; then
		FOCUS_SET=1
		break
	fi
	kill -0 "$PID" 2>/dev/null || break
	sleep 1
done

if [ "$FOCUS_SET" = 0 ]; then
	echo "WARNING: never confirmed the engine as frontmost: ${LAST_ERR:-no error reported}"
	echo "If that is an accessibility permission error, grant the terminal control of"
	echo "System Events. Otherwise click the window by hand now."
else
	echo "confirmed frontmost, measuring"
fi

POLLS=0
FRONT=0
while kill -0 "$PID" 2>/dev/null; do
	NOW=$(frontmost_pid)
	POLLS=$((POLLS + 1))
	if [ "$NOW" = "$PID" ]; then
		FRONT=$((FRONT + 1))
	else
		echo "poll $POLLS: frontmost is pid ${NOW}, re-asserting" >> "$LOGFILE.focus"
		focus_engine > /dev/null
	fi
	sleep "$POLL_SECONDS"
done

wait "$CAPPED_JOB"
CAPPED_STATUS=$?
cat "$CAPPED_OUT"  # bash-guard: allow
rm "$CAPPED_OUT"

echo "focus: engine frontmost on $FRONT of $POLLS polls"

if [ "$CAPPED_STATUS" != 0 ]; then
	echo "RESULT: void, run-capped.sh exited $CAPPED_STATUS"
	exit "$CAPPED_STATUS"
fi

if [ "$POLLS" = 0 ]; then
	echo "RESULT: void, never polled focus at all"
	exit 5
fi

LOST=$((POLLS - FRONT))

# A single blip is re-asserted and does not invalidate 15 cells, but anything more
# than a twentieth of the run means the engine spent real time descheduled.
if [ "$LOST" -gt $((POLLS / 20)) ]; then
	echo "RESULT: void, lost focus on $LOST of $POLLS polls. See $LOGFILE.focus"
	exit 5
fi

if [ "$LOST" != 0 ]; then
	echo "RESULT: measured, but focus blipped on $LOST of $POLLS polls. Check $LOGFILE.focus"
	echo "against the cell timestamps before trusting a small difference."
	exit 0
fi

echo "RESULT: measured, frontmost throughout"
