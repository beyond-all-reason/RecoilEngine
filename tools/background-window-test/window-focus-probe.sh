#!/bin/bash
#
# Samples, without sending any input, whether the engine window takes the focus, is minimized
# or covers the focused window (X11). The test cases and how to run them are in
# doc/site/content/docs/guides/getting-started/background-window.md.
#
# Usage:
#   window-focus-probe.sh <pid> [seconds=30] [interval=0.2] [csv]
#
# Needs xdotool and xprop, and a window manager that publishes _NET_ACTIVE_WINDOW and
# _NET_CLIENT_LIST_STACKING (EWMH). The last line is one of:
#   VERDICT background: PASS     window seen; engine_foreground, minimized, above_foreground all 0
#   VERDICT background: FAIL
#   VERDICT background: PARTIAL  no stacking list, so "above" was not measured
#   VERDICT background: EXITED   the engine ended before <seconds>

set -u

pid=${1:?usage: window-focus-probe.sh <pid> [seconds] [interval] [csv]}
seconds=${2:-30}
interval=${3:-0.2}
csv=${4:-}

# xprop prints window ids in hex, xdotool in decimal: convert each hex id on stdin.
hex_ids() {
	grep -o '0x[0-9a-fA-F]*' | while read -r h; do
		printf '%d\n' "$h"
	done
}

active_window() {
	xprop -root _NET_ACTIVE_WINDOW 2>/dev/null | hex_ids | head -1
}

# The engine's top-level window: the one of its windows with a title. A minimized window is
# unmapped, so this does not use --onlyvisible.
engine_window() {
	local w
	for w in $(xdotool search --pid "$pid" 2>/dev/null); do
		if xprop -id "$w" WM_NAME 2>/dev/null | grep -q '= "'; then
			echo "$w"
			return
		fi
	done
}

is_minimized() {
	xprop -id "$1" _NET_WM_STATE 2>/dev/null | grep -q _NET_WM_STATE_HIDDEN
}

# Position of window $1 in the stacking list $2, which runs bottom to top.
stack_position() {
	grep -nx "$1" <<< "$2" | cut -d: -f1
}

samples=0
seen=0
foreground=0
minimized=0
above=0
stacking=yes
exited=no

if [[ -n "$csv" ]]; then
	echo "time,active,engine,engine_fg,minimized,above_fg" > "$csv"
fi

end=$(( $(date +%s) + seconds ))
while (( $(date +%s) < end )); do
	if ! kill -0 "$pid" 2>/dev/null; then
		exited=yes
		break
	fi

	active=$(active_window)
	win=$(engine_window)
	is_fg=0
	is_min=0
	is_above=0

	if [[ -n "$win" ]]; then
		seen=$((seen + 1))

		if [[ "$win" == "$active" ]]; then
			is_fg=1
			foreground=$((foreground + 1))
		fi

		if is_minimized "$win"; then
			is_min=1
			minimized=$((minimized + 1))
		fi

		stack=$(xprop -root _NET_CLIENT_LIST_STACKING 2>/dev/null | hex_ids)
		if [[ -z "$stack" ]]; then
			stacking=no
		elif (( ! is_fg )) && [[ -n "$active" ]]; then
			pos_engine=$(stack_position "$win" "$stack")
			pos_active=$(stack_position "$active" "$stack")
			if [[ -n "$pos_engine" && -n "$pos_active" ]] && (( pos_engine > pos_active )); then
				is_above=1
				above=$((above + 1))
			fi
		fi
	fi

	samples=$((samples + 1))
	if [[ -n "$csv" ]]; then
		time=$(date +%T.%N | cut -c1-12)
		echo "$time,${active:-0},${win:-0},$is_fg,$is_min,$is_above" >> "$csv"
	fi
	sleep "$interval"
done

echo "samples=$samples window_seen=$seen engine_foreground=$foreground minimized=$minimized above_foreground=$above stacking=$stacking"

if [[ $exited == yes ]]; then
	echo "VERDICT background: EXITED"
elif (( seen > 0 && foreground == 0 && minimized == 0 && above == 0 )); then
	if [[ $stacking == yes ]]; then
		echo "VERDICT background: PASS"
	else
		echo "VERDICT background: PARTIAL"
	fi
else
	echo "VERDICT background: FAIL"
fi
