#!/bin/bash
# Samples, without sending any input, whether the engine window takes the focus, is
# minimized or covers the focused window (X11). The test cases and how to run them are in
# doc/site/content/docs/guides/getting-started/background-window.md.
#
#   window-focus-probe.sh <pid> [seconds=30] [interval=0.2] [csv]
#
# Needs xdotool and xprop, and a window manager that publishes _NET_ACTIVE_WINDOW and
# _NET_CLIENT_LIST_STACKING (EWMH). Last line: VERDICT background: PASS | FAIL |
# PARTIAL (no stacking list, so "above" was not measured) | EXITED (the engine ended early).
set -u
pid=${1:?usage: window-focus-probe.sh <pid> [seconds] [interval] [csv]}
seconds=${2:-30}
interval=${3:-0.2}
csv=${4:-}

# xprop prints window ids in hex, xdotool in decimal
hex_ids() { grep -o '0x[0-9a-fA-F]*' | while read -r h; do printf '%d\n' "$h"; done; }

end=$(( $(date +%s) + seconds ))
n=0; seen=0; fg=0; minimized=0; above=0; stacking=yes; exited=no
[[ -n "$csv" ]] && echo "time,active,engine,engine_fg,minimized,above_fg" > "$csv"
while (( $(date +%s) < end )); do
	kill -0 "$pid" 2>/dev/null || { exited=yes; break; }
	active=$(xprop -root _NET_ACTIVE_WINDOW 2>/dev/null | hex_ids | head -1)
	# the top-level window with a title; a minimized window is unmapped, so not --onlyvisible
	win=""
	for w in $(xdotool search --pid "$pid" 2>/dev/null); do
		xprop -id "$w" WM_NAME 2>/dev/null | grep -q '= "' && { win=$w; break; }
	done
	isfg=0; ismin=0; isabove=0
	if [[ -n "$win" ]]; then
		seen=$((seen + 1))
		[[ "$win" == "${active:-}" ]] && { isfg=1; fg=$((fg + 1)); }
		xprop -id "$win" _NET_WM_STATE 2>/dev/null | grep -q _NET_WM_STATE_HIDDEN && { ismin=1; minimized=$((minimized + 1)); }
		stack=$(xprop -root _NET_CLIENT_LIST_STACKING 2>/dev/null | hex_ids)
		if [[ -z "$stack" ]]; then
			stacking=no
		elif (( ! isfg )) && [[ -n "${active:-}" ]]; then
			# the list runs bottom to top
			pe=$(grep -nx "$win" <<< "$stack" | cut -d: -f1)
			pa=$(grep -nx "$active" <<< "$stack" | cut -d: -f1)
			[[ -n "$pe" && -n "$pa" ]] && (( pe > pa )) && { isabove=1; above=$((above + 1)); }
		fi
	fi
	n=$((n + 1))
	[[ -n "$csv" ]] && echo "$(date +%T.%N | cut -c1-12),${active:-0},${win:-0},$isfg,$ismin,$isabove" >> "$csv"
	sleep "$interval"
done
echo "samples=$n window_seen=$seen engine_foreground=$fg minimized=$minimized above_foreground=$above stacking=$stacking"
if [[ $exited == yes ]]; then
	echo "VERDICT background: EXITED"
elif (( seen > 0 && fg == 0 && minimized == 0 && above == 0 )); then
	[[ $stacking == yes ]] && echo "VERDICT background: PASS" || echo "VERDICT background: PARTIAL"
else
	echo "VERDICT background: FAIL"
fi
