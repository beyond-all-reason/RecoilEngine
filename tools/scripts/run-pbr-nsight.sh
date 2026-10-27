#!/usr/bin/env bash
#
# Runs the isolated PBR IBL runtime (BAR on this Recoil build) under NVIDIA
# Nsight Graphics from the command line: a C++ capture, an OpenGL Frame
# Debugger capture or a GPU Trace, with no keypress and with the game window
# kept in the background. Nsight Graphics must be installed; the default path
# is the project's install in C:/Repo/Profilling/nsight-graphics.
#
# WHY
#   Nsight starts spring.exe itself (it injects into the process it launches),
#   so run-pbr-ibl-smoke.sh cannot launch the game. Doing the launch by hand
#   repeats several traps: Recoil's OpenGL calls that Nsight does not support
#   stop an unattended capture unless the project ignores incompatibilities;
#   "--platform" makes ngfx (a Qt program) look for a Qt plugin and die; the
#   window takes focus unless SDL's no-activation hint is passed through
#   ngfx's --env; the z-order helper needs the game's process id, which only
#   ngfx's output knows; and a C++ capture stores the whole shell environment,
#   tokens included, in its metadata.json. This script does all of that the
#   same way every time.
#
# Steps
#   1. run-pbr-ibl-smoke.sh --prepare-only --window-mode background (junctions,
#      isolated settings, archive cache; --self-test arms the self-test), then
#      gldebugannotations = 1 in the isolated springsettings.cfg, so BAR's
#      LuaShader and the PBR provider name their GL groups, textures and
#      programs (PBR::CaptureFace +X, "PBR environment set 1 256", ...), and
#      LogFlush = 1, so infolog.txt keeps its last lines when Nsight ends the
#      game and shows which self-test step the capture landed in;
#   2. for the Frame Debugger and C++ capture: the project file
#      <write>/recoil-nocounters.ngfx-proj (pbr-nsight-files.py write-project);
#   3. ngfx.exe with --activity, the engine command line of the smoke launcher
#      (with --background-window when the engine has that switch; otherwise
#      --env SDL_WINDOW_NO_ACTIVATION_WHEN_SHOWN=1), --output-dir <write>/nsight
#      and every option this script does not know (the trigger options);
#      output in <write>/nsight/ngfx-stdout.log;
#   4. the game's process id from the log line "Launched process: spring.exe
#      (pid: N)" (never by process name: another session may run its own
#      spring.exe), then, only for an engine without --background-window,
#      pbr-window-to-back.ps1 -ProcessId N;
#   5. after ngfx ends: stop the helper and strip process_environment from every
#      new <write>/nsight/CppCaptures/*/metadata.json (pbr-nsight-files.py).
#   --elevated runs steps 3-5 in an elevated Git Bash (one UAC prompt), which
#   GPU Trace needs when GPU performance counters are administrator-only.
#
# USAGES (every way this script is called)
#   Options:
#     --activity NAME          required: "Generate C++ Capture", "OpenGL Frame
#                              Debugger" or "GPU Trace Profiler"
#     --write-directory PATH   isolated write directory (default
#                              test-results/pbr-nsight); also holds the project,
#                              nsight/ (captures, reports, ngfx-stdout.log) and
#                              window-to-back.log
#     --self-test              arm BAR's IBL self-test (it quits the game when
#                              done; every Nsight activity also ends the game)
#     --elevated               run ngfx elevated (UAC prompt; for GPU Trace)
#     --project PATH           use this .ngfx-proj instead of writing one
#     --no-annotations         leave gldebugannotations as it is
#     --allow-running-game     start even if a spring.exe is already running
#                              (default: refuse; GPU work of two games mixes)
#     --ngfx PATH / PBR_NGFX   ngfx.exe (default: the 2026.3.1 install below)
#     --runtime-directory, --game-directory, --maps-directory, --config-template,
#     --window-size, --window-position PATH/VALUE
#                              passed to run-pbr-ibl-smoke.sh unchanged
#     -h, --help               print the usage text below
#     anything else            passed to ngfx.exe unchanged and in order, e.g.
#                              --wait-seconds 74, --wait-frames N,
#                              --start-after-ms 75000, --limit-to-frames 3,
#                              --max-duration-ms N, --multi-pass-metrics,
#                              --auto-export, --architecture Ada,
#                              --metric-set-name "Throughput Metrics",
#                              --set-gpu-clocks boost, --launch-detached
#   Refused: --platform (kills ngfx, see WHY), and --exe, --dir, --args, --env,
#   --output-dir (this script sets them).
#   Callers: the Recoil worktree's .vscode/tasks.json "Nsight: ..." tasks; by
#   hand from Git Bash.
#   Exit status: ngfx's exit status; 1 with a message on stderr for a bad
#   option, a missing input or a game that is already running.
#
# HOW TO USE (expected output)
#   From the Recoil worktree in Git Bash, a C++ capture during the self-test's
#   IBL epochs. --wait-seconds counts from ngfx's attach; on 2026-10-20 (Debug
#   build) the captured frame was at game time = wait + about 6 s, so 60 landed
#   in the anchor tests (a capture-face frame). The self-test's step times
#   ("[PBR IBL SelfTest] ==" lines in infolog.txt) move as it changes: check the
#   last infolog line after a run and adjust. Not every epoch frame has provider
#   work (the timing matrix waits frames for query results), so retry if
#   grep 'PBR::' finds nothing:
#     tools/scripts/run-pbr-nsight.sh --activity "Generate C++ Capture" --self-test \
#       --wait-seconds 60 --write-directory test-results/pbr-nsight-cpp
#   prints the smoke launcher's block ending "Prepared; not launching
#   (--prepare-only).", then
#     Project: .../recoil-nocounters.ngfx-proj (Ignore Incompatibilities on, ...)
#     Nsight:  Generate C++ Capture, log .../nsight/ngfx-stdout.log
#     Game:    spring.exe pid 12345 (window kept in the background)
#     ... (about 75 s)
#     Stripped process_environment (87 entries): .../CppCaptures/spring__.../metadata.json
#     Result:  Running result: 0
#     Output:  .../nsight/CppCaptures/spring__2026_10_20__13_06_00
#   Then grep 'PBR::' <capture>/Frame0Part00.cpp lists the provider's groups.
#   GPU Trace, elevated, three frames from 75 s after the first present:
#     tools/scripts/run-pbr-nsight.sh --activity "GPU Trace Profiler" --self-test --elevated \
#       --start-after-ms 75000 --limit-to-frames 3 --auto-export --write-directory test-results/pbr-nsight-trace
#
# HOW TO EXTEND (worked example: a --hud option for the Frame Debugger overlay)
#   Runner options are parsed in the loop below; ngfx options need no code (they
#   pass through). To add a runner option that changes the project, e.g. turn
#   off the in-game Nsight overlay with --no-hud:
#     1. default it with the other inputs:      hud=1
#     2. parse it in the option loop:            --no-hud) hud=0; shift ;;
#     3. give pbr-nsight-files.py write-project a --no-hud flag that sets
#        launcher["activity/fd/targethud"] = False (its HOW TO EXTEND shows how),
#        and pass it in step 2 below when hud is 0;
#     4. document it in usage(), in USAGES above and in the Nsight guide.
#   Keep machine paths as defaults with an option and a PBR_* variable, never
#   as the only way.

set -euo pipefail

# Locations derived from this file, as in run-pbr-ibl-smoke.sh, so the script
# works from any folder and from VS Code tasks.
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
recoil_root="$(cd -- "${script_dir}/../.." && pwd -P)"

# Defaults; options below override them. ngfx_default is the project's install
# (C:/Repo/Profilling/nsight-graphics/README.md beside it).
ngfx_default="C:/Repo/Profilling/nsight-graphics/2026.3.1/host/windows-desktop-nomad-x64/ngfx.exe"
ngfx="${PBR_NGFX:-$ngfx_default}"
activity=""
write_directory="${recoil_root}/test-results/pbr-nsight"
self_test=0
elevated=0
# set only inside the elevated copy of this script (see run_elevated): the
# preparation already ran in the caller, so the copy starts at step 3
elevated_child=0
project=""
annotations=1
allow_running_game=0
runtime_directory="${recoil_root}/build/window"
# options handed to run-pbr-ibl-smoke.sh, and options handed to ngfx
prepare_options=()
ngfx_options=()
# every argument as given, for the elevated copy
original_arguments=("$@")

# Printed by --help (stdout) and before an "Unknown activity" error (stderr).
# Keep it in step with USAGES in the header.
usage() {
	cat <<'EOF'
Usage: run-pbr-nsight.sh --activity NAME [options] [ngfx options]
  --activity NAME          "Generate C++ Capture", "OpenGL Frame Debugger" or
                           "GPU Trace Profiler"
  --write-directory PATH   isolated write directory (default test-results/pbr-nsight)
  --self-test              arm BAR's IBL self-test
  --elevated               run ngfx elevated (one UAC prompt; GPU Trace counters)
  --project PATH           use this .ngfx-proj instead of writing one
  --no-annotations         do not set gldebugannotations = 1
  --allow-running-game     start even if a spring.exe is already running
  --ngfx PATH              ngfx.exe (default: PBR_NGFX, else the 2026.3.1 install
                           in C:/Repo/Profilling/nsight-graphics)
  --runtime-directory, --game-directory, --maps-directory, --config-template,
  --window-size, --window-position
                           passed to run-pbr-ibl-smoke.sh
Every other argument goes to ngfx.exe unchanged, e.g. --wait-seconds 74,
--start-after-ms 75000 --limit-to-frames 3 --auto-export, --launch-detached.
Never pass --platform: ngfx (Qt) then fails to start.
EOF
}

# Prints an error to stderr and stops with status 1.
die() { printf '%s\n' "$*" >&2; exit 1; }

# Option parsing. Runner options are consumed; every other token is kept for
# ngfx in its original order, so "--wait-seconds 74" arrives as two tokens.
while (($# > 0)); do
	case "$1" in
		--activity) (($# >= 2)) || die "Missing value for $1"; activity="$2"; shift 2 ;;
		--write-directory) (($# >= 2)) || die "Missing value for $1"; write_directory="$2"; shift 2 ;;
		--self-test) self_test=1; shift ;;
		--elevated) elevated=1; shift ;;
		--elevated-child) elevated_child=1; shift ;;
		--project) (($# >= 2)) || die "Missing value for $1"; project="$2"; shift 2 ;;
		--no-annotations) annotations=0; shift ;;
		--allow-running-game) allow_running_game=1; shift ;;
		--ngfx) (($# >= 2)) || die "Missing value for $1"; ngfx="$2"; shift 2 ;;
		--runtime-directory) (($# >= 2)) || die "Missing value for $1"; runtime_directory="$2"; prepare_options+=("$1" "$2"); shift 2 ;;
		--game-directory|--maps-directory|--config-template|--window-size|--window-position)
			(($# >= 2)) || die "Missing value for $1"; prepare_options+=("$1" "$2"); shift 2 ;;
		--platform) die "--platform makes ngfx (a Qt program) look for a Qt plugin and fail; leave it out (the local machine is the default)." ;;
		--exe|--dir|--args|--env|--output-dir) die "$1 is set by this script; see --help." ;;
		-h|--help) usage; exit 0 ;;
		*) ngfx_options+=("$1"); shift ;;
	esac
done

case "$activity" in
	"Generate C++ Capture"|"OpenGL Frame Debugger"|"GPU Trace Profiler") ;;
	"") usage >&2; die "--activity is required." ;;
	*) usage >&2; die "Unknown activity: $activity" ;;
esac
ngfx="$(cygpath -u "$ngfx")"
[[ -f "$ngfx" ]] || die "ngfx.exe not found: $ngfx. Pass --ngfx or PBR_NGFX."
# a relative write directory is relative to where the script was started,
# as in run-pbr-ibl-smoke.sh
write_directory="$(cygpath -u "$write_directory")"
[[ "$write_directory" == /* ]] || write_directory="$(pwd -P)/${write_directory}"
runtime_directory="$(cygpath -u "$runtime_directory")"
[[ "$runtime_directory" == /* ]] || runtime_directory="$(pwd -P)/${runtime_directory}"
nsight_directory="${write_directory}/nsight"
log="${nsight_directory}/ngfx-stdout.log"
python_command="$(command -v python || command -v python3 || true)"
[[ -n "$python_command" ]] || die "Python 3 is needed for pbr-nsight-files.py (python or python3 on PATH)."

# One game at a time (GPU timings of two games mix, and Nsight captures the
# wrong one if a stray instance answers). tasklist /FO CSV /NH prints one
# quoted line per process; MSYS_NO_PATHCONV keeps /FI and /FO as switches.
if ((!allow_running_game && !elevated_child)) &&
	MSYS_NO_PATHCONV=1 tasklist.exe /FI "IMAGENAME eq spring.exe" /FO CSV /NH 2>/dev/null | grep -qi '"spring.exe"'; then
	die "A spring.exe is already running (another test?). Wait for it, or pass --allow-running-game."
fi

# Steps 1 and 2 run once, in the caller (never in the elevated copy).
if ((!elevated_child)); then
	smoke_options=(--prepare-only --window-mode background --write-directory "$write_directory" "${prepare_options[@]}")
	((self_test)) && smoke_options+=(--self-test)
	"${script_dir}/run-pbr-ibl-smoke.sh" "${smoke_options[@]}"
	# Same replace-or-append as set_setting in run-pbr-ibl-smoke.sh.
	# LogFlush 1: every Nsight activity terminates the game, and a buffered
	# infolog.txt then loses its last seconds; flushed, its last line shows
	# which self-test step the capture landed in (to tune --wait-seconds).
	config="${write_directory}/springsettings.cfg"
	settings=(LogFlush)
	((annotations)) && settings+=(gldebugannotations)
	for setting in "${settings[@]}"; do
		sed -i "/^[[:space:]]*${setting}[[:space:]]*=/Id" "$config"
		printf '%s = 1\n' "$setting" >> "$config"
	done
	mkdir -p -- "$nsight_directory"
	if [[ -z "$project" && "$activity" != "GPU Trace Profiler" ]]; then
		project="${write_directory}/recoil-nocounters.ngfx-proj"
		"$python_command" -I "$(cygpath -w "${script_dir}/pbr-nsight-files.py")" write-project "$(cygpath -w "$project")"
	fi
fi

# --elevated: run this script again elevated, from step 3, and wait for it.
# UAC elevation (Start-Process -Verb RunAs) starts a new process with a fresh
# environment in System32, so everything it needs is passed as absolute
# arguments; Git's bin/bash.exe sets up the Git Bash environment by itself.
# The argument string is handed to PowerShell through an environment variable,
# because Windows PowerShell 5.1 mangles quotes inside native arguments.
# The elevated copy cannot write to this terminal; its ngfx log is printed here
# afterwards.
if ((elevated && !elevated_child)); then
	git_bash="$(cygpath -w /)bin\\bash.exe"
	child_arguments=("$(cygpath -m "${script_dir}/run-pbr-nsight.sh")" "${original_arguments[@]}" --elevated-child
		--write-directory "$(cygpath -m "$write_directory")" --runtime-directory "$(cygpath -m "$runtime_directory")"
		--ngfx "$(cygpath -m "$ngfx")")
	[[ -n "$project" ]] && child_arguments+=(--project "$(cygpath -m "$project")")
	argument_string=""
	for argument in "${child_arguments[@]}"; do
		argument_string+="\"${argument}\" "
	done
	printf 'Nsight:  %s, elevated (answer the UAC prompt), log %s\n' "$activity" "$log"
	child_status=0
	PBR_NSIGHT_BASH="$git_bash" PBR_NSIGHT_ARGUMENTS="$argument_string" powershell.exe -NoProfile -Command \
		'$p = Start-Process -FilePath $env:PBR_NSIGHT_BASH -ArgumentList $env:PBR_NSIGHT_ARGUMENTS -Verb RunAs -WindowStyle Hidden -Wait -PassThru; exit $p.ExitCode' ||
		child_status=$?
	[[ -f "$log" ]] && grep -E 'Launched process|Running result|saved to|Export|ERROR|Failed' "$log" || true
	printf 'Result:  elevated run exited with %s; full log %s\n' "$child_status" "$log"
	exit "$child_status"
fi

# Step 3. ngfx arguments:
#   --args      the engine command line of run-pbr-ibl-smoke.sh as ONE string,
#               with Windows paths in quotes (ngfx splits it for spring.exe)
#   --env       (older engines only) SDL shows the window without activating it
#   --verbose   the "Launched process: ... (pid: N)" line step 4 reads
# MSYS_NO_PATHCONV keeps Git Bash from rewriting anything that looks like a path.
# Background method, as in run-pbr-ibl-smoke.sh: an engine with the switch
# --background-window (Recoil 1767-feat-background-window) keeps its own window
# unfocused, behind other windows and never minimized ("engine"); an engine that
# lacks it would stop on the unknown switch, so it gets the environment hint and
# the z-order helper instead ("legacy"). The switch name is found in spring.exe.
if LC_ALL=C grep -qaF -- 'background-window' "${runtime_directory}/spring.exe"; then
	background_method=engine
	background_switch=" --background-window"
	ngfx_environment=()
else
	background_method=legacy
	background_switch=""
	ngfx_environment=(--env "SDL_WINDOW_NO_ACTIVATION_WHEN_SHOWN=1;")
fi
printf 'Background: %s
' "$background_method"
engine_arguments="--isolation --isolation-dir \"$(cygpath -w "$runtime_directory")\" --write-dir \"$(cygpath -w "$write_directory")\" -window${background_switch} \"$(cygpath -w "${script_dir}/pbr-ibl-startscript.txt")\""
ngfx_arguments=(--activity "$activity")
[[ -n "$project" ]] && ngfx_arguments+=(--project "$(cygpath -w "$project")")
ngfx_arguments+=(--exe "$(cygpath -w "${runtime_directory}/spring.exe")" --dir "$(cygpath -w "$runtime_directory")"
	--args "$engine_arguments" "${ngfx_environment[@]}"
	--output-dir "$(cygpath -w "$nsight_directory")" --verbose "${ngfx_options[@]}")

# captures that exist before this run are not touched by step 5
mkdir -p -- "$nsight_directory"
declare -A old_captures=()
shopt -s nullglob
for capture in "${nsight_directory}"/CppCaptures/*/; do old_captures["$capture"]=1; done
shopt -u nullglob

printf 'Nsight:  %s, log %s\n' "$activity" "$log"
MSYS_NO_PATHCONV=1 "$ngfx" "${ngfx_arguments[@]}" > "$log" 2>&1 &
ngfx_pid=$!

# Step 4. The pid line appears about a second after launch; wait for it while
# ngfx runs (it can fail before launching, for example without counter access).
# windows_pid stays empty if ngfx exits first.
pid_wait_seconds=120
windows_pid=""
for ((second = 0; second < pid_wait_seconds; second++)); do
	if [[ -f "$log" ]] && windows_pid="$(sed -n 's/.*Launched process: spring\.exe (pid: \([0-9]*\)).*/\1/p' "$log" | head -n 1)" && [[ -n "$windows_pid" ]]; then
		break
	fi
	kill -0 "$ngfx_pid" 2>/dev/null || break
	sleep 1
done
helper_pid=""
if [[ -n "$windows_pid" && "$background_method" == "engine" ]]; then
	printf 'Game:    spring.exe pid %s (the engine keeps its window in the background)
' "$windows_pid"
elif [[ -n "$windows_pid" ]]; then
	# same call as run-pbr-ibl-smoke.sh's legacy background mode; the helper
	# stops by itself when the process is gone
	powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File "$(cygpath -w "${script_dir}/pbr-window-to-back.ps1")" \
		-ProcessId "$windows_pid" -LogFile "$(cygpath -w "${write_directory}/window-to-back.log")" &
	helper_pid=$!
	printf 'Game:    spring.exe pid %s (window kept in the background)\n' "$windows_pid"
else
	printf 'Game:    no "Launched process" line in the log; see %s\n' "$log" >&2
fi

# Step 5. With --launch-detached ngfx returns right after the launch while the
# game keeps running, so the helper is left to stop by itself.
ngfx_status=0
wait "$ngfx_pid" || ngfx_status=$?
if [[ -n "$helper_pid" ]] && ! printf '%s\n' "${ngfx_options[@]}" | grep -qx -- '--launch-detached'; then
	kill "$helper_pid" 2>/dev/null || true
fi
new_captures=()
shopt -s nullglob
for capture in "${nsight_directory}"/CppCaptures/*/; do
	[[ -n "${old_captures[$capture]:-}" ]] || new_captures+=("$(cygpath -w "${capture%/}")")
done
shopt -u nullglob
((${#new_captures[@]} == 0)) || "$python_command" -I "$(cygpath -w "${script_dir}/pbr-nsight-files.py")" strip-environment "${new_captures[@]}"

printf 'Result:  %s\n' "$(grep -E 'Running result' "$log" | tail -n 1 || printf 'ngfx exited with %s' "$ngfx_status")"
for capture in "${new_captures[@]}"; do printf 'Output:  %s\n' "$capture"; done
shopt -s nullglob
for report in "${nsight_directory}"/*.ngfx-gputrace; do printf 'Output:  %s\n' "$report"; done
shopt -u nullglob
exit "$ngfx_status"
