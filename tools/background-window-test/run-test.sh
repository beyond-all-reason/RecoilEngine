#!/bin/bash
# Runs the BackgroundWindow test cases on Linux (X11) with window-focus-probe.sh and
# prints one result per case. The cases and what they check are described in
# doc/site/content/docs/guides/getting-started/background-window.md.
#
#   run-test.sh <path to spring> [seconds=25] [out dir]      CASES="switch config" to choose
#
# Start it from the window that has the focus and keep that window focused. Exit code 0
# when every case with the feature passes.
set -u
[[ -f "${1:-}" ]] || { echo "usage: run-test.sh <path to spring> [seconds] [out dir]"; exit 2; }
engine=$(readlink -f "$1")
seconds=${2:-25}
out=${3:-${TMPDIR:-/tmp}/recoil-background-window-test}
cases=${CASES:-off switch fullscreen config}
here=$(dirname "$(readlink -f "$0")")
log_line="background window: shown without activation, lowered, never minimized"
failed=0
table=""
epid=""
trap '[[ -n "$epid" ]] && kill -9 "$epid" 2>/dev/null' EXIT

for c in $cases; do
	args=()
	cfg=("XResolutionWindowed = 1280" "YResolutionWindowed = 720" "WindowPosX = 40" "WindowPosY = 40" "WindowBorderless = 0" "LogFlush = 1")
	case $c in
		off) cfg+=("Fullscreen = 0") ;;
		switch) cfg+=("Fullscreen = 0"); args=(--background-window) ;;
		fullscreen) cfg+=("Fullscreen = 1"); args=(--background-window) ;;
		config) cfg+=("Fullscreen = 0" "BackgroundWindow = 1") ;;
		*) echo "unknown case $c"; exit 2 ;;
	esac
	wd=$out/$c
	rm -rf "$wd"; mkdir -p "$wd"
	printf '%s\n' "${cfg[@]}" > "$wd/springsettings.cfg"

	echo "== $c : spring ${args[*]+"${args[*]}"}"
	"$engine" --isolation --isolation-dir "$(dirname "$engine")" --write-dir "$wd" ${args[@]+"${args[@]}"} > "$wd/stdout.txt" 2>&1 &
	epid=$!
	probe=$("$here/window-focus-probe.sh" "$epid" "$seconds" 0.2 "$wd/probe.csv")
	kill "$epid" 2>/dev/null; sleep 1; kill -9 "$epid" 2>/dev/null; wait "$epid" 2>/dev/null
	epid=""
	sed 's/^/   /' <<< "$probe"

	verdict=$(grep -o 'VERDICT background: [A-Z-]*' <<< "$probe" | awk '{print $3}')
	has_line=no; grep -qF "$log_line" "$wd/infolog.txt" 2>/dev/null && has_line=yes
	windowed=-
	if [[ $c == fullscreen ]]; then
		# the engine logs the mode it set; it must not have saved Fullscreen = 0 either
		windowed=no
		grep -q 'display-mode set to .*(windowed::' "$wd/infolog.txt" 2>/dev/null \
			&& ! grep -q 'display-mode set to .*(fullscreen::' "$wd/infolog.txt" \
			&& ! grep -qE '^\s*Fullscreen\s*=\s*0' "$wd/springsettings.cfg" && windowed=yes
	fi
	result=-
	if [[ $c != off ]]; then
		result=FAIL
		[[ $verdict == PASS && $has_line == yes && $windowed != no ]] && result=PASS
		[[ $result == FAIL ]] && failed=1
	fi
	table+=$(printf '%-11s %-8s %-12s %-9s %s' "$c" "${verdict:-?}" "$has_line" "$windowed" "$result")$'\n'
done

printf '\n%-11s %-8s %-12s %-9s %s\n' Case Probe InfologLine Windowed Result
printf '%s' "$table"
echo "write directories, infologs and probe CSVs: $out"
exit $failed
