#!/usr/bin/env bash
#
# Runs the BAR PBR V3 worktree on the Recoil PBR V3 engine in an isolated
# runtime (write directory build/window/pbr-user), without touching the normal
# BAR install.
#
# Prerequisites (the VS Code "build" task does all three):
#   cmake --build build/window --target spring
#   cmake -P build/window/cont/cmake_install.cmake   # Recoil's own install rule:
#                                                    # MinGW DLLs, fonts, base content
#   .vscode/split_symbols.sh build/window             # Debug builds only (>2 GiB limit)
#
# What this script adds on top of a plain spring.exe command line:
#   * two junctions so the engine sees exactly one BAR game (the V3 worktree,
#     whose archive name would clash with any other BAR checkout) and the
#     installed maps;
#   * a few isolated settings (self-test/probe flags, first-run capture mode);
#   * the committed start script tools/scripts/pbr-ibl-startscript.txt.
#
# The equivalent manual launch, once prepared, is:
#   build/window/spring.exe --isolation --isolation-dir <abs build/window> \
#     --write-dir <abs build/window/pbr-user> -window <abs pbr-ibl-startscript.txt>

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
recoil_root="$(cd -- "${script_dir}/../.." && pwd -P)"
workspace_root="$(cd -- "${recoil_root}/.." && pwd -P)"

build_directory="${recoil_root}/build/window"
game_directory="${workspace_root}/Beyond-All-Reason-Private-pbr-rendering-V3.sdd"
maps_directory="${workspace_root}/BARS-Install/data/maps"
start_script="${script_dir}/pbr-ibl-startscript.txt"
config_template="${recoil_root}/.vscode/springsettings-debug.cfg"
prepare_only=0
self_test=0
input_probe=0

usage() {
	cat <<'EOF'
Usage: run-pbr-ibl-smoke.sh [options]
  --build-directory PATH   engine build/install directory (default build/window)
  --game-directory PATH    BAR checkout to mount as the only game (default: BAR V3 worktree)
  --maps-directory PATH    folder with installed maps (default: BARS-Install/data/maps)
  --start-script PATH      start script (default: tools/scripts/pbr-ibl-startscript.txt)
  --self-test              run BAR's "PBR IBL Self-Test" widget, then quit;
                           report: <write-dir>/pbr-ibl-selftest/report.txt
  --input-probe            drive the PBR IBL viewer with real Windows mouse/keyboard
                           input (moves your cursor for about a minute);
                           report: <write-dir>/pbr-ibl-input-probe/report.txt
  --prepare-only           create links and settings, do not launch
EOF
}

die() { printf '%s\n' "$*" >&2; exit 1; }
absolute() { (cd -- "$(cygpath -u "$1")" && pwd -P) || die "Directory does not exist: $1"; }

while (($# > 0)); do
	case "$1" in
		--build-directory) build_directory="$2"; shift 2 ;;
		--game-directory) game_directory="$2"; shift 2 ;;
		--maps-directory) maps_directory="$2"; shift 2 ;;
		--start-script) start_script="$2"; shift 2 ;;
		--self-test) self_test=1; shift ;;
		--input-probe) input_probe=1; shift ;;
		--prepare-only) prepare_only=1; shift ;;
		-h|--help) usage; exit 0 ;;
		*) usage >&2; die "Unknown argument: $1" ;;
	esac
done

build_directory="$(absolute "$build_directory")"
game_directory="$(absolute "$game_directory")"
maps_directory="$(absolute "$maps_directory")"
start_script="$(cygpath -u "$start_script")"
[[ -f "$start_script" ]] || die "Start script does not exist: $start_script"

engine="${build_directory}/spring.exe"
write_directory="${build_directory}/pbr-user"
config="${write_directory}/springsettings.cfg"

[[ -f "$engine" ]] || die "Not built: $engine. Run the VS Code 'build' task."
[[ -f "${build_directory}/fonts/FreeSansBold.otf" && -f "${build_directory}/SDL2.dll" ]] ||
	die "Engine runtime files are not installed. Run: cmake -P $(cygpath -m "$build_directory")/cont/cmake_install.cmake"
# Windows refuses PE images above 2 GiB ("Exec format error"): a Debug build
# keeps its DWARF inside spring.exe until the symbol split has run.
(($(stat -c %s "$engine") <= 2147483647)) ||
	die "spring.exe still contains its debug symbols. Run .vscode/split_symbols.sh build/window."

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

link_directory "${build_directory}/games/$(basename -- "$game_directory")" "$game_directory"
link_directory "${build_directory}/maps" "$maps_directory"

mkdir -p -- "$write_directory"
if [[ ! -f "$config" ]]; then
	# first run: start from the developer's BAR settings when available
	if [[ -f "$config_template" ]]; then cp -- "$config_template" "$config"; else : > "$config"; fi
fi
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
	--isolation-dir "$(cygpath -m "$build_directory")"
	--write-dir "$(cygpath -m "$write_directory")"
	-window
	"$(cygpath -m "$start_script")"
)

if ((input_probe)); then
	"$engine" "${engine_arguments[@]}" &
	engine_pid=$!
	probe_status=0
	powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "${script_dir}/pbr-viewer-input-probe.ps1")" \
		-WriteDirectory "$(cygpath -w "$write_directory")" || probe_status=$?
	wait "$engine_pid" || true
	printf 'Input probe report: %s\n' "${write_directory}/pbr-ibl-input-probe/report.txt"
	exit "$probe_status"
fi

exec "$engine" "${engine_arguments[@]}"
