#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
recoil_root="$(cd -- "${script_dir}/../.." && pwd -P)"
workspace_root="$(cd -- "${recoil_root}/.." && pwd -P)"

build_directory="${recoil_root}/build/window"
game_directory="${workspace_root}/Beyond-All-Reason-Private-pbr-rendering-V3.sdd"
bar_data_directory="${workspace_root}/BARS-Install/data"
start_script=''
config_template="${recoil_root}/.vscode/springsettings-debug.cfg"
game_archive_name='Beyond All Reason $VERSION'
environment_capture='hdr-modern'
prepare_only=0
self_test=0
input_probe=0

usage() {
	cat <<'EOF'
Usage: run-pbr-ibl-smoke.sh [options]
  --build-directory PATH
  --game-directory PATH
  --bar-data-directory PATH
  --start-script PATH
  --config-template PATH
  --game-archive-name NAME
  --environment-capture legacy-clone|hdr-modern|compare
  --self-test      run BAR's opt-in "PBR IBL Self-Test" widget, then quit;
                   the report is written to <write-dir>/pbr-ibl-selftest/report.txt
  --input-probe    launch, then drive the PBR IBL viewer with real Windows mouse and
                   keyboard input (moves your cursor for about a minute) and
                   check that the focused panel keeps input away from the game;
                   report: <write-dir>/pbr-ibl-input-probe/report.txt
  --prepare-only
EOF
}

to_posix_path() {
	cygpath -u "$1" 2>/dev/null || printf '%s\n' "$1"
}

canonical_directory() {
	local path
	path="$(to_posix_path "$1")"
	[[ -d "$path" ]] || { printf 'Required directory does not exist: %s\n' "$path" >&2; exit 1; }
	(cd -- "$path" && pwd -P)
}

canonical_file() {
	local path directory
	path="$(to_posix_path "$1")"
	[[ -f "$path" ]] || { printf 'Required file does not exist: %s\n' "$path" >&2; exit 1; }
	directory="$(cd -- "$(dirname -- "$path")" && pwd -P)"
	printf '%s/%s\n' "$directory" "$(basename -- "$path")"
}

while (($# > 0)); do
	case "$1" in
		--build-directory) build_directory="$2"; shift 2 ;;
		--game-directory) game_directory="$2"; shift 2 ;;
		--bar-data-directory) bar_data_directory="$2"; shift 2 ;;
		--start-script) start_script="$2"; shift 2 ;;
		--config-template) config_template="$2"; shift 2 ;;
		--game-archive-name) game_archive_name="$2"; shift 2 ;;
		--environment-capture) environment_capture="$2"; shift 2 ;;
		--self-test) self_test=1; shift ;;
		--input-probe) input_probe=1; shift ;;
		--prepare-only) prepare_only=1; shift ;;
		-h|--help) usage; exit 0 ;;
		*) printf 'Unknown argument: %s\n' "$1" >&2; usage >&2; exit 2 ;;
	esac
done

case "$environment_capture" in
	legacy-clone|hdr-modern|compare) ;;
	*) printf 'Invalid environment capture mode: %s\n' "$environment_capture" >&2; exit 2 ;;
esac

build_directory="$(canonical_directory "$build_directory")"
game_directory="$(canonical_directory "$game_directory")"
bar_data_directory="$(canonical_directory "$bar_data_directory")"
if [[ -z "$start_script" ]]; then
	start_script="${bar_data_directory}/_script.txt"
fi
start_script="$(canonical_file "$start_script")"
config_template="$(to_posix_path "$config_template")"

engine="${build_directory}/spring.exe"
base_directory="${build_directory}/base"
installed_maps="${bar_data_directory}/maps"
games_directory="${build_directory}/games"
game_mount="${games_directory}/Beyond-All-Reason-Private-pbr-rendering-V3.sdd"
map_mount="${build_directory}/maps"
write_directory="${build_directory}/pbr-user"
runtime_start_script="${write_directory}/_script-pbr-v3.txt"
runtime_config="${write_directory}/springsettings.cfg"

required_paths=(
	"$engine"
	"$base_directory"
	"$installed_maps"
	"${build_directory}/libcurl.dll"
	"${build_directory}/OpenAL32.dll"
	"${build_directory}/zlib1.dll"
	"${build_directory}/SDL2.dll"
	"${build_directory}/libfontconfig-1.dll"
	"${build_directory}/libfreetype-6.dll"
	"${build_directory}/libIL.dll"
	"${build_directory}/libvorbisfile-3.dll"
)

for required_path in "${required_paths[@]}"; do
	[[ -e "$required_path" ]] || {
		printf "Required PBR smoke-test path does not exist: %s. Run the VS Code task 'build' first.\n" "$required_path" >&2
		exit 1
	}
done

# Windows refuses to load a PE image above 2 GiB ("Exec format error"). A Debug
# build carries its DWARF inside spring.exe until the symbol split has run.
engine_bytes="$(stat -c %s "$engine")"
if ((engine_bytes > 2147483647)); then
	printf 'The engine still contains its debug symbols (%s bytes). Run .vscode/split_symbols.sh %s (the VS Code "build" task does this) before launching.\n' \
		"$engine_bytes" "$build_directory" >&2
	exit 1
fi

ensure_junction() {
	local path="$1"
	local target="$2"
	local actual win_path win_target

	if [[ -e "$path" ]]; then
		[[ -d "$path" ]] || { printf 'Refusing to replace existing non-directory path: %s\n' "$path" >&2; exit 1; }
		actual="$(cd -- "$path" && pwd -P)"
		[[ "$actual" == "$target" ]] || {
			printf 'Refusing to replace existing non-matching path: %s (resolves to %s)\n' "$path" "$actual" >&2
			exit 1
		}
		return
	fi

	mkdir -p -- "$(dirname -- "$path")"
	win_path="$(cygpath -w "$path")"
	win_target="$(cygpath -w "$target")"
	# MSYS must not rewrite the cmd.exe switches (/d, /c, /J) into drive paths;
	# otherwise cmd.exe starts interactively, exits 0, and creates nothing.
	# Pass the paths as separate arguments: embedded \" quotes reach cmd.exe
	# literally and make mklink reject the path syntax.
	MSYS_NO_PATHCONV=1 cmd.exe /d /c mklink /J "$win_path" "$win_target" >/dev/null

	[[ -d "$path" && "$(cd -- "$path" && pwd -P)" == "$target" ]] || {
		printf 'Failed to create junction %s -> %s\n' "$path" "$target" >&2
		exit 1
	}
}

set_spring_setting() {
	local file="$1"
	local name="$2"
	local value="$3"
	local temporary="${file}.tmp"

	if ! awk -v key="$name" -v value="$value" '
		BEGIN { pattern = "^[[:space:]]*" tolower(key) "[[:space:]]*="; matches = 0 }
		{
			line = $0
			sub(/\r$/, "", line)
			if (tolower(line) ~ pattern) {
				matches++
				if (matches == 1) print key " = " value
				next
			}
			print line
		}
		END {
			if (matches == 0) print key " = " value
			if (matches > 1) exit 42
		}
	' "$file" > "$temporary"; then
		rm -f -- "$temporary"
		printf 'Expected at most one %s entry in %s\n' "$name" "$file" >&2
		exit 1
	fi
	mv -f -- "$temporary" "$file"
}

mkdir -p -- "$games_directory" "$write_directory"
ensure_junction "$game_mount" "$game_directory"
ensure_junction "$map_mount" "$installed_maps"

temporary_start_script="${runtime_start_script}.tmp"
if ! awk -v archive="$game_archive_name" '
	BEGIN { pattern = "^[[:space:]]*gametype[[:space:]]*="; matches = 0 }
	{
		line = $0
		sub(/\r$/, "", line)
		if (tolower(line) ~ pattern) {
			matches++
			prefix = line
			sub(/=.*/, "=", prefix)
			line = prefix archive ";"
		}
		print line
	}
	END { if (matches != 1) exit 42 }
' "$start_script" > "$temporary_start_script"; then
	rm -f -- "$temporary_start_script"
	printf 'Expected exactly one gametype entry in %s\n' "$start_script" >&2
	exit 1
fi
mv -f -- "$temporary_start_script" "$runtime_start_script"

if [[ ! -f "$runtime_config" ]]; then
	if [[ -f "$config_template" ]]; then
		cp -- "$config_template" "$runtime_config"
	else
		: > "$runtime_config"
	fi
fi
set_spring_setting "$runtime_config" 'Fullscreen' '0'
# A copied BAR settings file (and BAR itself at runtime) stores the bare name
# FreeSansBold.otf, which only resolves inside an installed engine. The build
# tree has no installed fonts/, so pin the engine's own source font on every
# launch; otherwise startup aborts with "Failed to load FontFile".
engine_font="$(cygpath -m "${recoil_root}/cont/fonts/FreeSansBold.otf")"
[[ -f "${recoil_root}/cont/fonts/FreeSansBold.otf" ]] || { printf 'Engine font is missing: %s\n' "$engine_font" >&2; exit 1; }
set_spring_setting "$runtime_config" 'FontFile' "$engine_font"
set_spring_setting "$runtime_config" 'SmallFontFile' "$engine_font"
set_spring_setting "$runtime_config" 'PBREnvironmentCapture' "$environment_capture"
# Written explicitly on every run so that a previous --self-test run never
# leaves the self-test armed for a later manual session.
set_spring_setting "$runtime_config" 'PBRIBLSelfTest' "$self_test"
set_spring_setting "$runtime_config" 'PBRIBLSelfTestQuit' "$self_test"
if ((input_probe)); then
	# the probe reads the viewer's /pbribllayout output while the game runs
	set_spring_setting "$runtime_config" 'LogFlush' '1'
fi

# BAR only uses a widget's "enabled" default when its saved widget order has
# no entry; a saved order of 0 (widget switched off in F11 at some point)
# would silently prevent the self-test from loading. Force it on in the
# isolated runtime's widget order when the self-test is requested.
widget_order_file="${write_directory}/LuaUI/Config/BYAR.lua"
if ((self_test)) && [[ -f "$widget_order_file" ]]; then
	sed -i 's/\["PBR IBL Self-Test"\] = 0,/["PBR IBL Self-Test"] = 1,/' "$widget_order_file"
fi

printf 'PBR IBL smoke test\n'
printf '  Engine:         %s\n' "$engine"
printf '  Game:           %s\n' "$game_directory"
printf '  Maps:           %s\n' "$installed_maps"
printf '  Writable:       %s\n' "$write_directory"
printf '  Runtime script: %s\n' "$runtime_start_script"
printf '  Game archive:   %s\n' "$game_archive_name"
printf '  PBR capture:    %s\n' "$environment_capture"
printf '  Self-test:      %s\n' "$self_test"
printf '  Input probe:    %s\n' "$input_probe"

if ((prepare_only)); then
	printf 'Runtime prepared; launch skipped because --prepare-only was supplied.\n'
	exit 0
fi

engine_arguments=(
	--isolation
	--isolation-dir "$(cygpath -m "$build_directory")"
	--write-dir "$(cygpath -m "$write_directory")"
	-window
	"$(cygpath -m "$runtime_start_script")"
)

if ((input_probe)); then
	# The game runs in the background while the PowerShell probe drives it with
	# real OS input; the probe quits the game when it is done.
	"$engine" "${engine_arguments[@]}" &
	engine_pid=$!
	probe_status=0
	powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "${script_dir}/pbr-viewer-input-probe.ps1")" \
		-WriteDirectory "$(cygpath -w "$write_directory")" || probe_status=$?
	wait "$engine_pid" || true
	printf 'Input probe report: %s\n' "${write_directory}/pbr-ibl-input-probe/report.txt"
	exit "$probe_status"
fi

"$engine" "${engine_arguments[@]}"
