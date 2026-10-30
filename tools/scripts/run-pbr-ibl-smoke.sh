#!/usr/bin/env bash
#
# Runs a BAR checkout containing the PBR IBL widgets on a selected Recoil
# engine in an isolated write directory, without touching the normal BAR
# install. Paths can come from command-line options or PBR_* environment
# variables, so the two repositories do not need fixed names or locations.
# The full user guide with examples is tools/scripts/PBR_IBL_SMOKE.md.
#
# WHY
#   A plain spring.exe start would read the user's normal BAR data folders and
#   settings, see every BAR checkout at once (they all carry the archive name
#   "Beyond All Reason $VERSION", so only one may be visible), and open a window
#   that takes the keyboard. Test runs must instead be reproducible, isolated,
#   unattended and leave their evidence where a rebuild cannot delete it. This
#   script is the single entry point that sets all of that up. It is bash so it
#   runs in Git Bash on Windows; the Win32-only window and input work lives in
#   the PowerShell helpers it starts, which are never entry points themselves.
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
#   * a few isolated settings (self-test/probe flags, first-run capture mode,
#     cursor mode, plain-window placement, refreshed SpringData path);
#   * an archive cache copied from an earlier run, so start-up does not hang;
#   * the committed start script tools/scripts/pbr-ibl-startscript.txt;
#   * in background window mode, the engine's --background-window switch, or for
#     an engine without it SDL's no-activation hint and the z-order helper
#     pbr-window-to-back.ps1; with --input-probe, the real-input probe
#     pbr-viewer-input-probe.ps1.
#
# The equivalent manual launch, once prepared, is:
#   <engine> --isolation --isolation-dir <runtime-dir> \
#     --write-dir <write-dir> -window <start-script>
#
# USAGES (every way this script is called)
#   Options (each has a PBR_* environment equivalent; an option wins over it):
#     --engine PATH / PBR_ENGINE                  spring.exe to run; its folder
#                                                 becomes the runtime directory
#                                                 unless one is given explicitly
#     --runtime-directory PATH / PBR_RUNTIME_DIRECTORY
#       (aliases --build-directory / PBR_BUILD_DIRECTORY) isolation root holding
#                                                 spring.exe, DLLs, fonts, base
#                                                 content; default build/window
#     --game-directory PATH / PBR_GAME_DIRECTORY  BAR checkout; default: sibling
#                                                 auto-discovery (see below)
#     --maps-directory PATH / PBR_MAPS_DIRECTORY  installed maps; default sibling
#                                                 BARS-Install/data/maps, then
#                                                 <runtime>/maps
#     --write-directory PATH / PBR_WRITE_DIRECTORY settings, logs, reports;
#                                                 default test-results/pbr-ibl
#     --start-script PATH / PBR_START_SCRIPT      default pbr-ibl-startscript.txt
#     --config-template PATH / PBR_CONFIG_TEMPLATE first-run springsettings.cfg;
#                                                 default .vscode/springsettings-
#                                                 debug.cfg when it exists; a
#                                                 named file must exist
#     --game-link-name NAME / PBR_GAME_LINK_NAME  junction name under games/
#     --cursor-mode software|hardware|keep / PBR_CURSOR_MODE
#     --window-mode background|foreground|keep / PBR_WINDOW_MODE
#     --window-size WxH / PBR_WINDOW_SIZE         default 1280x720
#     --window-position X,Y / PBR_WINDOW_POSITION default 40,40
#     --self-test       arm BAR's self-test widget; the game quits when done
#     --input-probe     run pbr-viewer-input-probe.ps1 (needs foreground mode)
#     --prepare-only    validate, link and write settings; do not launch
#     -h, --help        print the usage text below
#   Callers:
#     Recoil worktree .vscode/tasks.json: "prepare-pbr-runtime" (last step of
#       "build", --prepare-only), "Launch PBR IBL", "Run PBR IBL self-test".
#     BAR worktree .vscode/tasks.json: "PBR: prepare isolated Recoil runtime",
#       "PBR: launch isolated IBL smoke test", "PBR: run automated IBL
#       self-test" (all pass --game-directory "${workspaceFolder}").
#     By hand from Git Bash, and from measurement runs that pass their own
#     --write-directory test-results/<run>.
#   Exit status: the engine's exit status (the probe's with --input-probe);
#   1 with a message on stderr when an input is missing or invalid.
#
# HOW TO USE (expected output)
#   From the Recoil worktree in Git Bash:
#     tools/scripts/run-pbr-ibl-smoke.sh --self-test
#   prints the resolved inputs, then runs the game (about two minutes for a
#   Debug build) and returns when it quits:
#     Engine:  /c/.../RecoilEngine-pbr-rendering-experimental-V1/build/window/spring.exe
#     Game:    /c/.../Beyond-All-Reason-Private-pbr-rendering-experimental-V1.sdd
#     Maps:    /c/.../BARS-Install/data/maps
#     Script:  /c/.../tools/scripts/pbr-ibl-startscript.txt
#     Write:   /c/.../test-results/pbr-ibl
#     Cursor:  software
#     Window:  background 1280x720 at 40,40
#   The report is then in test-results/pbr-ibl/pbr-ibl-selftest/report.txt;
#   grep -E '^(PASS|FAIL|KNOWN|GAP|SUMMARY)' on it gives the verdict lines.
#   With --prepare-only the same block ends with
#     Prepared; not launching (--prepare-only).
#
# HOW TO EXTEND (worked example: an option that writes one more setting)
#   To pin VSync for timing runs with --vsync N / PBR_VSYNC:
#     1. default with the other inputs:   vsync="${PBR_VSYNC:-}"
#     2. parse it in the option loop:
#          --vsync) (($# >= 2)) || die "Missing value for $1"; vsync="$2"; shift 2 ;;
#     3. validate it next to the other checks:
#          [[ -z "$vsync" || "$vsync" =~ ^-?[0-9]+$ ]] || die "--vsync must be an integer"
#     4. write it into the isolated config after the other set_setting calls:
#          [[ -n "$vsync" ]] && set_setting VSync "$vsync"
#     5. document it in usage(), in USAGES above and in PBR_IBL_SMOKE.md.
#   Keep every new setting in the isolated write directory's springsettings.cfg
#   only; never edit the BAR checkout, the maps or the user's normal BAR data.

set -euo pipefail

# Locations are derived from this file, not from the current directory, so the
# script works from any folder and from VS Code tasks in either worktree.
# pwd -P resolves junctions and symlinks, so later comparisons see real paths.
# workspace_root is the folder holding the sibling checkouts (Recoil, BAR,
# BARS-Install) that the auto-discovery below looks in.
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
recoil_root="$(cd -- "${script_dir}/../.." && pwd -P)"
workspace_root="$(cd -- "${recoil_root}/.." && pwd -P)"

# Defaults: environment variables first, then the in-tree layout. Options parsed
# below override these. runtime_directory_explicit remembers whether the user
# chose a runtime directory, because --engine otherwise implies its own folder.
# An empty game, maps or write directory means "discover or use the default"
# after option parsing.
runtime_directory="${PBR_RUNTIME_DIRECTORY:-${PBR_BUILD_DIRECTORY:-${recoil_root}/build/window}}"
runtime_directory_explicit=0
[[ -n "${PBR_RUNTIME_DIRECTORY:-}${PBR_BUILD_DIRECTORY:-}" ]] && runtime_directory_explicit=1
engine_input="${PBR_ENGINE:-}"
game_directory="${PBR_GAME_DIRECTORY:-}"
maps_directory="${PBR_MAPS_DIRECTORY:-}"
start_script="${PBR_START_SCRIPT:-${script_dir}/pbr-ibl-startscript.txt}"
write_directory="${PBR_WRITE_DIRECTORY:-}"
config_template="${PBR_CONFIG_TEMPLATE:-${recoil_root}/.vscode/springsettings-debug.cfg}"
# 1 when the caller named a template (PBR_CONFIG_TEMPLATE or --config-template):
# a missing one is then an error, while the default local file is optional
config_template_explicit=0
[[ -n "${PBR_CONFIG_TEMPLATE:-}" ]] && config_template_explicit=1
game_link_name="${PBR_GAME_LINK_NAME:-Beyond-All-Reason-Private-pbr-test.sdd}"
prepare_only=0
self_test=0
input_probe=0
cursor_mode="${PBR_CURSOR_MODE:-software}"
window_mode="${PBR_WINDOW_MODE:-background}"
window_size="${PBR_WINDOW_SIZE:-1280x720}"
window_position="${PBR_WINDOW_POSITION:-40,40}"

# Printed by --help (stdout) and before an "Unknown argument" error (stderr).
# Keep it in step with USAGES in the header and with PBR_IBL_SMOKE.md.
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
  --config-template PATH   initial springsettings.cfg template (must exist;
                           default: .vscode/springsettings-debug.cfg if present)
  --game-link-name NAME    stable .sdd mount name under runtime/games
                           (default: Beyond-All-Reason-Private-pbr-test.sdd)
  --self-test              run BAR's "PBR IBL Self-Test" widget, then quit;
                           report: <write-dir>/pbr-ibl-selftest/report.txt
  --input-probe            drive the PBR IBL viewer with real Windows mouse/keyboard
                           input (moves your cursor for about a minute);
                           report: <write-dir>/pbr-ibl-input-probe/report.txt
  --cursor-mode MODE       software (default, visible in OBS game capture),
                           hardware, or keep (retain isolated config setting)
  --window-mode MODE       background (default): a plain window of --window-size at
                           --window-position that does not take keyboard focus when
                           it opens and stays behind your windows (the engine's
                           --background-window; older engines: pbr-window-to-back.ps1),
                           so runs need no physical mouse
                           or keyboard and never cover your windows; foreground: the same
                           window, activated normally; keep: the config's own
                           window/fullscreen settings. Never minimize a running
                           test: Recoil then draws only twice a minute.
  --window-size WxH        windowed size (default 1280x720)
  --window-position X,Y    window position in desktop pixels (default 40,40)
  --prepare-only           create links and settings, do not launch

Environment equivalents:
  PBR_ENGINE, PBR_RUNTIME_DIRECTORY (or legacy PBR_BUILD_DIRECTORY),
  PBR_GAME_DIRECTORY, PBR_MAPS_DIRECTORY, PBR_WRITE_DIRECTORY,
  PBR_START_SCRIPT, PBR_CONFIG_TEMPLATE, PBR_GAME_LINK_NAME,
  PBR_CURSOR_MODE, PBR_WINDOW_MODE, PBR_WINDOW_SIZE, PBR_WINDOW_POSITION.
EOF
}

# Prints an error to stderr and stops with status 1.
die() { printf '%s\n' "$*" >&2; exit 1; }
# Path helpers. Users may pass Windows paths (C:\x, C:/x) or Git Bash paths
# (/c/x); cygpath -u turns every form into the Git Bash form that cd and [[ -f ]]
# understand. Inside the script paths stay in that form; they are converted
# back only where a Windows program reads them (cygpath -m gives C:/x for the
# engine, cygpath -w gives C:\x for cmd.exe and PowerShell). Both helpers return
# absolute paths with junctions resolved, and die when the path is missing.
absolute_directory() { (cd -- "$(cygpath -u "$1")" && pwd -P) || die "Directory does not exist: $1"; }
absolute_file() {
	local path directory name
	path="$(cygpath -u "$1")"
	[[ -f "$path" ]] || die "File does not exist: $1"
	directory="$(cd -- "$(dirname -- "$path")" && pwd -P)"
	name="$(basename -- "$path")"
	printf '%s/%s\n' "$directory" "$name"
}

# A BAR checkout is usable only if it carries both PBR IBL widgets: the viewer
# (opened by hand and by the input probe) and the self-test (armed by
# --self-test). Checking files avoids launching a checkout without them, which
# would load fine and then silently test nothing.
is_pbr_game_checkout() {
	[[ -f "$1/luaui/Widgets/dbg_pbr_ibl_viewer.lua" && -f "$1/luaui/Widgets/dbg_pbr_ibl_selftest.lua" ]]
}

# Finds the BAR checkout when none was given: first the expected experimental
# sibling name, then exactly one sibling Beyond-All-Reason-Private*.sdd with
# both widgets. Zero or several matches stop with a message instead of picking
# one, because testing the wrong checkout gives believable but wrong results.
# nullglob makes an unmatched pattern expand to nothing rather than to itself.
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

# Option parsing. Each option taking a value checks that one follows, so a
# trailing "--engine" fails clearly instead of reading an empty string.
# --build-directory stays as an alias because older VS Code tasks pass it.
while (($# > 0)); do
	case "$1" in
		--engine) (($# >= 2)) || die "Missing value for $1"; engine_input="$2"; shift 2 ;;
		--runtime-directory|--build-directory) (($# >= 2)) || die "Missing value for $1"; runtime_directory="$2"; runtime_directory_explicit=1; shift 2 ;;
		--game-directory) (($# >= 2)) || die "Missing value for $1"; game_directory="$2"; shift 2 ;;
		--maps-directory) (($# >= 2)) || die "Missing value for $1"; maps_directory="$2"; shift 2 ;;
		--write-directory) (($# >= 2)) || die "Missing value for $1"; write_directory="$2"; shift 2 ;;
		--start-script) (($# >= 2)) || die "Missing value for $1"; start_script="$2"; shift 2 ;;
		--config-template) (($# >= 2)) || die "Missing value for $1"; config_template="$2"; config_template_explicit=1; shift 2 ;;
		--game-link-name) (($# >= 2)) || die "Missing value for $1"; game_link_name="$2"; shift 2 ;;
		--self-test) self_test=1; shift ;;
		--input-probe) input_probe=1; shift ;;
		--cursor-mode) (($# >= 2)) || die "Missing value for $1"; cursor_mode="$2"; shift 2 ;;
		--window-mode) (($# >= 2)) || die "Missing value for $1"; window_mode="$2"; shift 2 ;;
		--window-size) (($# >= 2)) || die "Missing value for $1"; window_size="$2"; shift 2 ;;
		--window-position) (($# >= 2)) || die "Missing value for $1"; window_position="$2"; shift 2 ;;
		--prepare-only) prepare_only=1; shift ;;
		-h|--help) usage; exit 0 ;;
		*) usage >&2; die "Unknown argument: $1" ;;
	esac
done

# Validate the enumerated and numeric options before anything is created on
# disk, so a typo never leaves half-prepared links or settings behind.
# BASH_REMATCH holds the regex groups of the last successful =~ match.
case "$cursor_mode" in
	software|hardware|keep) ;;
	*) die "--cursor-mode must be software, hardware, or keep (got: $cursor_mode)" ;;
esac
case "$window_mode" in
	background|foreground|keep) ;;
	*) die "--window-mode must be background, foreground, or keep (got: $window_mode)" ;;
esac
[[ "$window_size" =~ ^([0-9]+)x([0-9]+)$ ]] || die "--window-size must be WxH (got: $window_size)"
window_width="${BASH_REMATCH[1]}"; window_height="${BASH_REMATCH[2]}"
[[ "$window_position" =~ ^(-?[0-9]+),(-?[0-9]+)$ ]] || die "--window-position must be X,Y (got: $window_position)"
window_x="${BASH_REMATCH[1]}"; window_y="${BASH_REMATCH[2]}"
# The real-input probe moves the user's cursor and types into the focused game
# window, so it cannot run in the background; it must be requested explicitly.
((input_probe)) && [[ "$window_mode" == "background" ]] &&
	die "--input-probe drives real mouse and keyboard input and needs --window-mode foreground."

# Engine and runtime directory. A given engine implies its own folder as the
# runtime unless --runtime-directory / PBR_RUNTIME_DIRECTORY was also given (a
# prebuilt runtime may keep spring.exe elsewhere). Without --engine the engine
# is <runtime>/spring.exe.
if [[ -n "$engine_input" ]]; then
	engine="$(absolute_file "$engine_input")"
	((runtime_directory_explicit)) || runtime_directory="$(dirname -- "$engine")"
fi
runtime_directory="$(absolute_directory "$runtime_directory")"
engine="${engine:-${runtime_directory}/spring.exe}"
[[ -f "$engine" ]] || die "Engine does not exist: $engine. Pass --engine or build/install Recoil first."

# The BAR checkout: given, or discovered; checked for the widgets either way.
[[ -n "$game_directory" ]] || game_directory="$(discover_game_directory)"
game_directory="$(absolute_directory "$game_directory")"
is_pbr_game_checkout "$game_directory" || die "BAR checkout lacks the PBR IBL viewer/self-test widgets: $game_directory"

# Maps: the start script names Onyx Cauldron, which must be installed. The
# sibling BARS-Install (a normal BAR install) is only read through a junction,
# never written. An existing <runtime>/maps (for example the junction made by
# an earlier run) is the fallback.
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
	# The default points at optional ignored local VS Code settings; a portable
	# run without that file starts from an empty config. A template the caller
	# named must exist: a typo would otherwise silently drop their settings.
	((config_template_explicit)) && die "Config template does not exist: $config_template"
	config_template=""
elif [[ -n "$config_template" ]]; then
	config_template="$(absolute_file "$config_template")"
fi

# The game junction name is used as one directory under <runtime>/games, so a
# path here could create or replace folders elsewhere. The .sdd suffix is what
# makes the engine treat the folder as an unpacked game archive.
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

# In isolation mode the engine reads and writes its settings here, in the write
# directory; the user's normal springsettings.cfg is never opened.
config="${write_directory}/springsettings.cfg"

# Runtime files: without the DLLs beside spring.exe Windows stops before main
# (0xc0000135); without fonts/ BAR's FontFile = FreeSansBold.otf fails. One font
# and SDL2.dll stand for the whole install step (cont/cmake_install.cmake).
[[ -f "${runtime_directory}/fonts/FreeSansBold.otf" && -f "${runtime_directory}/SDL2.dll" ]] ||
	die "Engine runtime files are not installed in $runtime_directory. For a build tree run: cmake -P $(cygpath -m "$runtime_directory")/cont/cmake_install.cmake"
# Windows refuses PE images above 2 GiB ("Exec format error"): a Debug build
# keeps its DWARF inside spring.exe until the symbol split has run.
# 2^31 - 1 bytes is the largest image the Windows loader accepts; checking
# here turns a cryptic exec failure into an actionable message.
windows_max_image_bytes=2147483647
(($(stat -c %s "$engine") <= windows_max_image_bytes)) ||
	die "The engine exceeds Windows' 2 GiB executable limit. Run tools/scripts/split-debug-symbols.sh --engine $(cygpath -m "$engine")."

# Directory junction (no admin rights needed); an existing path is only
# accepted if it already points at the same target.
# A junction is an NTFS folder alias: programs opening <path> see the files of
# <target>. Unlike a symbolic link it needs neither administrator rights nor
# Developer Mode, and Recoil follows it like a normal folder. Removing a
# junction (rmdir in cmd) leaves the target untouched. pwd -P inside the
# junction prints the target, which is how an existing link is recognised.
# Refusing to replace a different path protects a real folder (for example a
# maps directory with downloaded maps) from being swapped for a link.
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
# The sed address /^ *Key *=/I deletes every existing line for the key,
# ignoring case and leading spaces (the engine matches keys without case), and
# printf appends the new value, so the key is present exactly once afterwards.
# Keys are fixed names from this script, never user text, so they need no
# regular-expression escaping.
set_setting() {
	sed -i "/^[[:space:]]*$1[[:space:]]*=/Id" "$config"
	printf '%s = %s\n' "$1" "$2" >> "$config"
}

# With --isolation the engine looks for games and maps only below the runtime
# directory, so the selected BAR checkout and the maps are linked in there. The
# game link has a stable name, so renaming the checkout never changes the
# archive path the engine sees, and only one BAR checkout is visible at a time.
link_directory "${runtime_directory}/games/${game_link_name}" "$game_directory"
link_directory "${runtime_directory}/maps" "$maps_directory"

if [[ ! -f "$config" ]]; then
	# first run: start from the developer's BAR settings when available
	if [[ -f "$config_template" ]]; then cp -- "$config_template" "$config"; else : > "$config"; fi
fi
# A copied developer or runtime config can retain absolute search paths from
# another checkout. Override them with the selected runtime on every launch;
# this also follows an explicitly supplied external runtime after a rebuild.
set_setting SpringData "$(cygpath -m "$runtime_directory")"
# OBS Game Capture sees the software cursor drawn into the game framebuffer.
# The hardware cursor is an OS overlay and can disappear from that capture.
# Only this isolated config is changed; a user can opt back in explicitly.
case "$cursor_mode" in
	software) set_setting HardwareCursor 0 ;;
	hardware) set_setting HardwareCursor 1 ;;
esac
# A new write directory has no archive cache, so the engine hashes the whole
# BAR checkout first (about 9 s here) without answering window messages;
# Windows then ghosts the "hung" window and gives it the keyboard focus. Seed
# the cache from the newest earlier run next to this write directory. After BAR
# files change the engine still re-hashes once, and that start-up can take focus.
# Background: Windows marks a window "not responding" after about 5 s without
# message processing and puts a ghost window (same title, "Not Responding") in
# its place; that ghost is activated like a normal window, so it takes the
# keyboard even though SDL showed the real window without activation.
# The cache file is cache/ArchiveCache<version>.lua (ArchiveCache22.lua on this
# branch); the glob accepts any version. ls -t lists the newest first, and
# "|| true" keeps set -e/pipefail from stopping when no earlier run exists.
# The copy is only a hint: the engine re-checks each archive's modification
# time and re-hashes what changed.
if ! compgen -G "${write_directory}/cache/ArchiveCache*.lua" >/dev/null; then
	newest_cache="$(ls -t "$(dirname -- "$write_directory")"/*/cache/ArchiveCache*.lua 2>/dev/null | head -n 1 || true)"
	if [[ -n "$newest_cache" ]]; then
		mkdir -p "${write_directory}/cache"
		cp -- "$newest_cache" "${write_directory}/cache/"
	fi
fi
# Window placement for unattended runs (see --window-mode). Fullscreen and
# borderless windows cover the desktop, so tests always use a plain window.
# The size is fixed so image checks and timings compare across runs; the
# position keeps the title bar on screen. MinimizeOnFocusLoss 0 stops the
# engine minimizing itself when the user clicks elsewhere: a minimized Recoil
# draws only about twice a minute, which stalls the self-test.
if [[ "$window_mode" != "keep" ]]; then
	set_setting Fullscreen 0
	set_setting WindowBorderless 0
	set_setting XResolutionWindowed "$window_width"
	set_setting YResolutionWindowed "$window_height"
	set_setting WindowPosX "$window_x"
	set_setting WindowPosY "$window_y"
	set_setting MinimizeOnFocusLoss 0
fi
# The capture mode is a persistent player choice (the viewer's Capture
# dropdown); only seed it on first run, but the self-test needs it on.
if ((self_test)) || ! grep -qi '^[[:space:]]*PBREnvironmentCapture[[:space:]]*=' "$config"; then
	set_setting PBREnvironmentCapture hdr-modern
fi
# written on every run so a self-test run never leaves the next session armed
# PBRIBLSelfTest arms BAR's dbg_pbr_ibl_selftest.lua widget; PBRIBLSelfTestQuit
# makes it quit the game after writing pbr-ibl-selftest/report.txt, so this
# script returns when the test is done.
set_setting PBRIBLSelfTest "$self_test"
set_setting PBRIBLSelfTestQuit "$self_test"
# the input probe reads the viewer's layout file and needs a flushed infolog
# (it waits for the "finished loading and is now ingame" line, which a buffered
# infolog.txt would only show much later). The key is not reset by later runs.
((input_probe)) && set_setting LogFlush 1

# BAR ignores a widget's "enabled" default once its saved widget order has an
# entry; an entry of 0 (switched off in F11) would stop the self-test loading.
# LuaUI/Config/BYAR.lua is the isolated write directory's own copy, so the
# user's normal widget choices are not touched. The sed pattern matches the
# exact form BAR writes; if that format changes, the substitution silently does
# nothing (see Troubleshooting in PBR_IBL_SMOKE.md).
widget_order="${write_directory}/LuaUI/Config/BYAR.lua"
if ((self_test)) && [[ -f "$widget_order" ]]; then
	sed -i 's/\["PBR IBL Self-Test"\] = 0,/["PBR IBL Self-Test"] = 1,/' "$widget_order"
fi

# Print every resolved input, so a log of the terminal shows exactly what ran.
printf 'Engine:  %s\nGame:    %s\nMaps:    %s\nScript:  %s\nWrite:   %s\nCursor:  %s\nWindow:  %s %s at %s\n' \
	"$engine" "$game_directory" "$maps_directory" "$start_script" "$write_directory" "$cursor_mode" \
	"$window_mode" "$window_size" "$window_position"
((prepare_only)) && { printf 'Prepared; not launching (--prepare-only).\n'; exit 0; }

# Background mode, two ways:
#   engine  the engine's own --background-window switch (Recoil branch
#           1767-feat-background-window): the window is created hidden, lowered behind
#           every other window, shown without activation, never minimized, and
#           Windows' "Not Responding" ghost window (which takes the focus) is off.
#           One switch, no helper process; the window stays behind until clicked.
#   legacy  for an engine without that switch (for example a released engine via
#           --engine): SDL's no-activation hint from the environment plus the
#           z-order helper pbr-window-to-back.ps1 (see the background branch below).
# An engine that does not know a switch stops with "unknown command line flag",
# so the switch is passed only when its name is in the executable (grep -a reads
# the binary as text; a Debug spring.exe of ~1 GB takes a second or two).
background_method=""
if [[ "$window_mode" == "background" ]]; then
	if LC_ALL=C grep -qaF -- 'background-window' "$engine"; then
		background_method=engine
	else
		background_method=legacy
	fi
	printf 'Background: %s\n' "$background_method"
fi

# Legacy background mode: SDL reads hints from the environment; show the window
# without activating it, so it never takes keyboard focus (it still opens on
# top; see below). SDL_WINDOW_NO_ACTIVATION_WHEN_SHOWN makes SDL show the window
# with SW_SHOWNA instead of activating it. Exported only for the engine started
# below; the user's shell is not affected.
[[ "$background_method" == "legacy" ]] && export SDL_WINDOW_NO_ACTIVATION_WHEN_SHOWN=1

# Recoil command line:
#   --isolation             use only --isolation-dir for data; ignore the user's
#                           normal data folders and their springsettings.cfg
#   --isolation-dir PATH    the runtime directory (base content, games/, maps/)
#   --write-dir PATH        where settings, infolog.txt, caches, screenshots and
#                           the widgets' reports are written
#   -window                 start windowed (the settings above place the window)
#   --background-window     engine background mode (see "Background mode" above);
#                           for this run only, nothing is written to the config
#   <start script>          last argument: the game to start (copied by the
#                           engine to <write-dir>/_script.txt)
# Paths go to the engine in C:/x form (cygpath -m): spring.exe is a Windows
# program and does not understand /c/x.
engine_arguments=(
	--isolation
	--isolation-dir "$(cygpath -m "$runtime_directory")"
	--write-dir "$(cygpath -m "$write_directory")"
	-window
)
[[ "$background_method" == "engine" ]] && engine_arguments+=(--background-window)
engine_arguments+=("$(cygpath -m "$start_script")")

# Prints the Windows process id of a process started from this shell.
# $! is the MSYS (Git Bash) process id: Git Bash gives every process, even a
# native Windows one, its own id, and the Windows id that Get-Process and the
# Win32 window calls need is published in /proc/<pid>/winpid. If that file is
# missing (another shell), the id is printed unchanged.
# Example: windows_pid="$(windows_process_id "$!")"
windows_process_id() {
	local id="$1"
	[[ -r "/proc/${id}/winpid" ]] && id="$(cat "/proc/${id}/winpid")"
	printf '%s\n' "$id"
}

# Real-input probe: the engine runs in the background while the probe drives it
# through Windows input, then this script waits for the engine (the probe ends
# the game with /quitforce) and returns the probe's status, not the engine's.
# -ExecutionPolicy Bypass lets the unsigned repository script run without
# changing the machine's policy; -NoProfile keeps the user's profile out.
# The probe gets the Windows process id (windows_process_id): it waits for a
# window of that id and stops with "no target engine window appeared" if none
# shows up within its -LoadTimeoutSeconds. `wait` still takes the MSYS id.
if ((input_probe)); then
	"$engine" "${engine_arguments[@]}" &
	engine_pid=$!
	probe_status=0
	powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "${script_dir}/pbr-viewer-input-probe.ps1")" \
		-WriteDirectory "$(cygpath -w "$write_directory")" -TargetProcessId "$(windows_process_id "$engine_pid")" || probe_status=$?
	wait "$engine_pid" || true
	printf 'Input probe report: %s\n' "${write_directory}/pbr-ibl-input-probe/report.txt"
	exit "$probe_status"
fi

if [[ "$background_method" == "legacy" ]]; then
	# A window shown without activation still lands on top of the z-order, and
	# clicking the already-active window behind it does not raise that window.
	# pbr-window-to-back.ps1 keeps the game window at the bottom, unactivated;
	# it needs the Windows process id (windows_process_id above).
	# -WindowStyle Hidden keeps the helper's console window from appearing.
	# When the engine exits, the helper is stopped; it would also stop by
	# itself within a second because the process is gone.
	"$engine" "${engine_arguments[@]}" &
	engine_pid=$!
	windows_pid="$(windows_process_id "$engine_pid")"
	powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File "$(cygpath -w "${script_dir}/pbr-window-to-back.ps1")" \
		-ProcessId "$windows_pid" -LogFile "$(cygpath -w "${write_directory}/window-to-back.log")" &
	helper_pid=$!
	engine_status=0
	wait "$engine_pid" || engine_status=$?
	kill "$helper_pid" 2>/dev/null || true
	exit "$engine_status"
fi

# Engine background, foreground and keep modes need no helper: exec replaces this shell with the
# engine, so the engine's exit status and Ctrl+C reach the caller directly.
exec "$engine" "${engine_arguments[@]}"
