#!/bin/bash
#
# Runs the BackgroundWindow test cases on Linux (X11) and prints one result per case.
# It starts the engine once per case, without a game or map (it opens its built-in menu), and
# samples its window with window-focus-probe.sh. The cases and what they check are described
# in doc/site/content/docs/guides/getting-started/background-window.md.
#
# Usage:
#   run-test.sh <path to spring> [seconds=25] [out dir]
#   CASES="switch config" run-test.sh <path to spring>     a subset of: off switch fullscreen config
#
# Start it from the window that has the focus and keep that window focused.
# Exit code 0 when every case with the feature passes, 1 when one fails, 2 on a usage error.

set -u

usage() {
	echo "usage: run-test.sh <path to spring> [seconds] [out dir]"
	exit 2
}

[[ -f "${1:-}" ]] || usage

engine=$(readlink -f "$1")
seconds=${2:-25}
out=${3:-${TMPDIR:-/tmp}/recoil-background-window-test}
cases=${CASES:-off switch fullscreen config}
here=$(dirname "$(readlink -f "$0")")
log_line="background window: shown without activation, lowered, never minimized"

base_config=(
	"XResolutionWindowed = 1280"
	"YResolutionWindowed = 720"
	"WindowPosX = 40"
	"WindowPosY = 40"
	"WindowBorderless = 0"
	"LogFlush = 1"
)

# Sets case_config (extra springsettings.cfg lines) and case_args (command-line arguments).
case_setup() {
	case $1 in
		off)
			case_config=("Fullscreen = 0")
			case_args=()
			;;
		switch)
			case_config=("Fullscreen = 0")
			case_args=(--background-window)
			;;
		fullscreen)
			case_config=("Fullscreen = 1")
			case_args=(--background-window)
			;;
		config)
			case_config=("Fullscreen = 0" "BackgroundWindow = 1")
			case_args=()
			;;
		*)
			echo "unknown case $1"
			exit 2
			;;
	esac
}

# The fullscreen case also needs only windowed display modes in the infolog, and the engine
# must not have saved Fullscreen = 0 (the switch must not change the user's setting).
stayed_windowed() {
	local infolog=$1 config=$2
	grep -q 'display-mode set to .*(windowed::' "$infolog" 2>/dev/null \
		&& ! grep -q 'display-mode set to .*(fullscreen::' "$infolog" \
		&& ! grep -qE '^\s*Fullscreen\s*=\s*0' "$config"
}

failed=0
table=""
epid=""
trap '[[ -n "$epid" ]] && kill -9 "$epid" 2>/dev/null' EXIT

for c in $cases; do
	case_setup "$c"
	wd=$out/$c
	rm -rf "$wd"
	mkdir -p "$wd"
	printf '%s\n' "${base_config[@]}" "${case_config[@]}" > "$wd/springsettings.cfg"

	echo "== $c : spring ${case_args[*]+"${case_args[*]}"}"
	"$engine" \
		--isolation \
		--isolation-dir "$(dirname "$engine")" \
		--write-dir "$wd" \
		${case_args[@]+"${case_args[@]}"} \
		> "$wd/stdout.txt" 2>&1 &
	epid=$!
	probe=$("$here/window-focus-probe.sh" "$epid" "$seconds" 0.2 "$wd/probe.csv")
	kill "$epid" 2>/dev/null
	sleep 1
	kill -9 "$epid" 2>/dev/null
	wait "$epid" 2>/dev/null
	epid=""
	sed 's/^/   /' <<< "$probe"

	verdict=$(grep -o 'VERDICT background: [A-Z-]*' <<< "$probe" | awk '{print $3}')

	has_line=no
	if grep -qF "$log_line" "$wd/infolog.txt" 2>/dev/null; then
		has_line=yes
	fi

	windowed=-
	if [[ $c == fullscreen ]]; then
		windowed=no
		if stayed_windowed "$wd/infolog.txt" "$wd/springsettings.cfg"; then
			windowed=yes
		fi
	fi

	# "off" is the control: it shows what the probe sees without the feature and never fails
	result=-
	if [[ $c != off ]]; then
		result=FAIL
		if [[ $verdict == PASS && $has_line == yes && $windowed != no ]]; then
			result=PASS
		else
			failed=1
		fi
	fi

	table+=$(printf '%-11s %-8s %-12s %-9s %s' "$c" "${verdict:-?}" "$has_line" "$windowed" "$result")$'\n'
done

echo
printf '%-11s %-8s %-12s %-9s %s\n' Case Probe InfologLine Windowed Result
printf '%s' "$table"
echo "write directories, infologs and probe CSVs: $out"
exit $failed
