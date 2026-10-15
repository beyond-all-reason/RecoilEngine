#!/usr/bin/env bash
#
# Runs a BAR checkout containing the PBR IBL widgets on a selected Recoil
# engine in an isolated write directory, without touching the normal BAR
# install. Paths can come from command-line options or PBR_* environment
# variables, so the two repositories do not need fixed names or locations.
#
# Prerequisites (the VS Code "build" task does all three):
#   cmake --build build/window --target spring
#   cmake -P build/window/cont/cmake_install.cmake   # Recoil's own install rule:
#                                                    # MinGW DLLs, fonts, base content
#   tools/scripts/split-debug-symbols.sh --runtime-directory build/window
#                                                     # Debug builds only (>2 GiB limit)
#
# What this script adds on top of a plain spring.exe command line:
#   * two junctions so the engine sees the selected BAR game and installed
#     maps through the selected runtime directory;
#   * a few isolated settings (self-test/probe flags, first-run capture mode);
#   * the committed start script tools/scripts/pbr-ibl-startscript.txt.
#
# The equivalent manual launch, once prepared, is:
#   <engine> --isolation --isolation-dir <runtime-dir> \
#     --write-dir <write-dir> -window <start-script>

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
recoil_root="$(cd -- "${script_dir}/../.." && pwd -P)"
workspace_root="$(cd -- "${recoil_root}/.." && pwd -P)"

runtime_directory="${PBR_RUNTIME_DIRECTORY:-${PBR_BUILD_DIRECTORY:-${recoil_root}/build/window}}"
runtime_directory_explicit=0
[[ -n "${PBR_RUNTIME_DIRECTORY:-}${PBR_BUILD_DIRECTORY:-}" ]] && runtime_directory_explicit=1
engine_input="${PBR_ENGINE:-}"
game_directory="${PBR_GAME_DIRECTORY:-}"
maps_directory="${PBR_MAPS_DIRECTORY:-}"
start_script="${PBR_START_SCRIPT:-${script_dir}/pbr-ibl-startscript.txt}"
write_directory="${PBR_WRITE_DIRECTORY:-}"
config_template="${PBR_CONFIG_TEMPLATE:-${recoil_root}/.vscode/springsettings-debug.cfg}"
game_link_name="${PBR_GAME_LINK_NAME:-Beyond-All-Reason-Private-pbr-test.sdd}"
prepare_only=0
self_test=0
input_probe=0

usage() {
	cat <<'EOF'
Usage: run-pbr-ibl-smoke.sh [options]
  --engine PATH            spring executable (default: <runtime-directory>/spring.exe)
  --runtime-directory PATH engine runtime/isolation root (default: build/window;
                           --build-directory remains a compatible alias)
  --game-directory PATH    BAR checkout containing the PBR IBL widgets
                           (default: unambiguous sibling checkout auto-discovery)
  --maps-directory PATH    folder with installed maps (default: sibling
                           BARS-Install/data/maps, then an existing runtime/maps)
  --write-directory PATH   isolated writable data directory
                           (default: <repository>/test-results/pbr-ibl)
  --start-script PATH      start script (default: tools/scripts/pbr-ibl-startscript.txt)
  --config-template PATH   optional initial springsettings.cfg template
  --game-link-name NAME    stable .sdd mount name under runtime/games
                           (default: Beyond-All-Reason-Private-pbr-test.sdd)
  --self-test              run BAR's "PBR IBL Self-Test" widget, then quit;
                           report: <write-dir>/pbr-ibl-selftest/report.txt
  --input-probe            drive the PBR IBL viewer with real Windows mouse/keyboard
                           input (moves your cursor for about a minute);
                           report: <write-dir>/pbr-ibl-input-probe/report.txt
  --prepare-only           create links and settings, do not launch

Environment equivalents:
  PBR_ENGINE, PBR_RUNTIME_DIRECTORY (or legacy PBR_BUILD_DIRECTORY),
  PBR_GAME_DIRECTORY, PBR_MAPS_DIRECTORY, PBR_WRITE_DIRECTORY,
  PBR_START_SCRIPT, PBR_CONFIG_TEMPLATE, PBR_GAME_LINK_NAME.
EOF
}

die() { printf '%s\n' "$*" >&2; exit 1; }
absolute_directory() { (cd -- "$(cygpath -u "$1")" && pwd -P) || die "Directory does not exist: $1"; }
absolute_file() {
	local path directory name
	path="$(cygpath -u "$1")"
	[[ -f "$path" ]] || die "File does not exist: $1"
	directory="$(cd -- "$(dirname -- "$path")" && pwd -P)"
	name="$(basename -- "$path")"
	printf '%s/%s\n' "$directory" "$name"
}

is_pbr_game_checkout() {
	[[ -f "$1/luaui/Widgets/dbg_pbr_ibl_viewer.lua" && -f "$1/luaui/Widgets/dbg_pbr_ibl_selftest.lua" ]]
}

discover_game_directory() {
	local preferred candidate
	local -a matches=()
	preferred="${workspace_root}/Beyond-All-Reason-Private-pbr-rendering-experimental-V1.sdd"
	if is_pbr_game_checkout "$preferred"; then
		printf '%s\n' "$preferred"
		return
	fi
	shopt -s nullglob
	for candidate in "${workspace_root}"/Beyond-All-Reason-Private*.sdd; do
		is_pbr_game_checkout "$candidate" && matches+=("$candidate")
	done
	shopt -u nullglob
	((${#matches[@]} == 1)) && { printf '%s\n' "${matches[0]}"; return; }
	((${#matches[@]} == 0)) && die "No sibling BAR checkout contains the PBR IBL widgets. Pass --game-directory or PBR_GAME_DIRECTORY."
	die "Multiple sibling BAR PBR checkouts were found. Pass --game-directory or PBR_GAME_DIRECTORY explicitly."
}

while (($# > 0)); do
	case "$1" in
		--engine) (($# >= 2)) || die "Missing value for $1"; engine_input="$2"; shift 2 ;;
		--runtime-directory|--build-directory) (($# >= 2)) || die "Missing value for $1"; runtime_directory="$2"; runtime_directory_explicit=1; shift 2 ;;
		--game-directory) (($# >= 2)) || die "Missing value for $1"; game_directory="$2"; shift 2 ;;
		--maps-directory) (($# >= 2)) || die "Missing value for $1"; maps_directory="$2"; shift 2 ;;
		--write-directory) (($# >= 2)) || die "Missing value for $1"; write_directory="$2"; shift 2 ;;
		--start-script) (($# >= 2)) || die "Missing value for $1"; start_script="$2"; shift 2 ;;
		--config-template) (($# >= 2)) || die "Missing value for $1"; config_template="$2"; shift 2 ;;
		--game-link-name) (($# >= 2)) || die "Missing value for $1"; game_link_name="$2"; shift 2 ;;
		--self-test) self_test=1; shift ;;
		--input-probe) input_probe=1; shift ;;
		--prepare-only) prepare_only=1; shift ;;
		-h|--help) usage; exit 0 ;;
		*) usage >&2; die "Unknown argument: $1" ;;
	esac
done

if [[ -n "$engine_input" ]]; then
	engine="$(absolute_file "$engine_input")"
	((runtime_directory_explicit)) || runtime_directory="$(dirname -- "$engine")"
fi
runtime_directory="$(absolute_directory "$runtime_directory")"
engine="${engine:-${runtime_directory}/spring.exe}"
[[ -f "$engine" ]] || die "Engine does not exist: $engine. Pass --engine or build/install Recoil first."

[[ -n "$game_directory" ]] || game_directory="$(discover_game_directory)"
game_directory="$(absolute_directory "$game_directory")"
is_pbr_game_checkout "$game_directory" || die "BAR checkout lacks the PBR IBL viewer/self-test widgets: $game_directory"

if [[ -z "$maps_directory" ]]; then
	if [[ -d "${workspace_root}/BARS-Install/data/maps" ]]; then
		maps_directory="${workspace_root}/BARS-Install/data/maps"
	elif [[ -d "${runtime_directory}/maps" ]]; then
		maps_directory="${runtime_directory}/maps"
	else
		die "No maps directory was found. Pass --maps-directory or PBR_MAPS_DIRECTORY."
	fi
fi
maps_directory="$(absolute_directory "$maps_directory")"
start_script="$(absolute_file "$start_script")"

if [[ -n "$config_template" && ! -f "$(cygpath -u "$config_template")" ]]; then
	# The default points at optional ignored local VS Code settings. An explicit
	# portable run does not require that file and starts from an empty config.
	config_template=""
elif [[ -n "$config_template" ]]; then
	config_template="$(absolute_file "$config_template")"
fi

[[ "$game_link_name" != */* && "$game_link_name" != *\\* && "$game_link_name" == *.sdd ]] ||
	die "--game-link-name must be one .sdd directory name, not a path: $game_link_name"

if [[ -z "$write_directory" ]]; then
	# Keep settings, logs, screenshots and reports outside the disposable build
	# tree so a clean configure/rebuild cannot erase test evidence.
	write_directory="${recoil_root}/test-results/pbr-ibl"
else
	write_directory="$(cygpath -u "$write_directory")"
	[[ "$write_directory" == /* ]] || write_directory="$(pwd -P)/${write_directory}"
fi
mkdir -p -- "$write_directory"
write_directory="$(absolute_directory "$write_directory")"

config="${write_directory}/springsettings.cfg"

[[ -f "${runtime_directory}/fonts/FreeSansBold.otf" && -f "${runtime_directory}/SDL2.dll" ]] ||
	die "Engine runtime files are not installed in $runtime_directory. For a build tree run: cmake -P $(cygpath -m "$runtime_directory")/cont/cmake_install.cmake"
# Windows refuses PE images above 2 GiB ("Exec format error"): a Debug build
# keeps its DWARF inside spring.exe until the symbol split has run.
(($(stat -c %s "$engine") <= 2147483647)) ||
	die "The engine exceeds Windows' 2 GiB executable limit. Run tools/scripts/split-debug-symbols.sh --engine $(cygpath -m "$engine")."

# Directory junction (no admin rights needed); an existing path is only
# accepted if it already points at the same target.
link_directory() {
	local path="$1" target="$2"
	if [[ -e "$path" ]]; then
		[[ "$(cd -- "$path" && pwd -P)" == "$target" ]] || die "Refusing to replace $path (it does not point to $target)"
		return
	fi
	mkdir -p -- "$(dirname -- "$path")"
	# MSYS_NO_PATHCONV: keep /d /c /J as cmd.exe switches instead of drive paths
	MSYS_NO_PATHCONV=1 cmd.exe /d /c mklink /J "$(cygpath -w "$path")" "$(cygpath -w "$target")" >/dev/null
	[[ -d "$path" ]] || die "Failed to create junction $path -> $target"
}

# Replaces (or appends) one "Key = value" line in springsettings.cfg.
set_setting() {
	sed -i "/^[[:space:]]*$1[[:space:]]*=/Id" "$config"
	printf '%s = %s\n' "$1" "$2" >> "$config"
}

link_directory "${runtime_directory}/games/${game_link_name}" "$game_directory"
link_directory "${runtime_directory}/maps" "$maps_directory"

if [[ ! -f "$config" ]]; then
	# first run: start from the developer's BAR settings when available
	if [[ -f "$config_template" ]]; then cp -- "$config_template" "$config"; else : > "$config"; fi
fi
# A copied developer config can retain absolute search paths from another
# checkout. Isolation and the runtime junctions above provide all data roots
# needed by this harness, so discard that machine-specific override.
sed -i '/^[[:space:]]*SpringData[[:space:]]*=/Id' "$config"
# The capture mode is a persistent player choice (the viewer's Capture
# dropdown); only seed it on first run, but the self-test needs it on.
if ((self_test)) || ! grep -qi '^[[:space:]]*PBREnvironmentCapture[[:space:]]*=' "$config"; then
	set_setting PBREnvironmentCapture hdr-modern
fi
# written on every run so a self-test run never leaves the next session armed
set_setting PBRIBLSelfTest "$self_test"
set_setting PBRIBLSelfTestQuit "$self_test"
# the input probe reads the viewer's layout file and needs a flushed infolog
((input_probe)) && set_setting LogFlush 1

# BAR ignores a widget's "enabled" default once its saved widget order has an
# entry; an entry of 0 (switched off in F11) would stop the self-test loading.
widget_order="${write_directory}/LuaUI/Config/BYAR.lua"
if ((self_test)) && [[ -f "$widget_order" ]]; then
	sed -i 's/\["PBR IBL Self-Test"\] = 0,/["PBR IBL Self-Test"] = 1,/' "$widget_order"
fi

printf 'Engine:  %s\nGame:    %s\nMaps:    %s\nScript:  %s\nWrite:   %s\n' \
	"$engine" "$game_directory" "$maps_directory" "$start_script" "$write_directory"
((prepare_only)) && { printf 'Prepared; not launching (--prepare-only).\n'; exit 0; }

engine_arguments=(
	--isolation
	--isolation-dir "$(cygpath -m "$runtime_directory")"
	--write-dir "$(cygpath -m "$write_directory")"
	-window
	"$(cygpath -m "$start_script")"
)

if ((input_probe)); then
	"$engine" "${engine_arguments[@]}" &
	engine_pid=$!
	probe_status=0
	powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "${script_dir}/pbr-viewer-input-probe.ps1")" \
		-WriteDirectory "$(cygpath -w "$write_directory")" -TargetProcessId "$engine_pid" || probe_status=$?
	wait "$engine_pid" || true
	printf 'Input probe report: %s\n' "${write_directory}/pbr-ibl-input-probe/report.txt"
	exit "$probe_status"
fi

exec "$engine" "${engine_arguments[@]}"
