#!/usr/bin/env bash
# Splits MinGW DWARF data out of a Windows Recoil executable so the PE image
# remains below Windows' 2 GiB executable limit. Tool and path overrides make
# the helper usable outside this repository's original development layout.
#
# WHY
#   A Debug MinGW build (-O0 -g3) embeds its DWARF debug information in
#   spring.exe, which makes it about 2.7 GB. Windows refuses to load a PE image
#   of 2 GiB or more: Git Bash reports "Exec format error" and nothing starts.
#   The usual GNU fix is to move the debug sections into a separate file and
#   leave a .gnu_debuglink section in the executable that names it; GDB follows
#   that link, so debugging keeps full symbols while the runtime image shrinks
#   (about 50 MB here). run-pbr-ibl-smoke.sh refuses an unsplit image and points
#   here.
#
# USAGES
#   split-debug-symbols.sh [--runtime-directory DIR | --engine PATH]
#                          [--binary-name NAME] [--verify-only]
#     --runtime-directory DIR  folder with the executable (default build/window,
#                              relative to the current directory; alias
#                              --build-directory; env PBR_RUNTIME_DIRECTORY or
#                              PBR_BUILD_DIRECTORY)
#     --engine PATH            the executable itself (env PBR_ENGINE); its folder
#                              and name replace the two options above
#     --binary-name NAME       executable name in the folder (default spring.exe)
#     --verify-only            only check an existing split; change nothing
#     PBR_OBJCOPY, PBR_OBJDUMP, PBR_STRIP  binutils to use; otherwise PATH, then
#                              C:/mingw64/bin (the toolchain this project builds with)
#   Callers: by hand after a Debug build (tools/scripts/PBR_IBL_SMOKE.md), and
#   the launcher's error message. The local VS Code tasks "strip-binary-symbols"
#   and "verify-binary-symbols" run an older untracked copy with the same steps.
#   Exit status 0 = split (or already split) and verified; 1 = a message on stderr.
#
# HOW TO USE (expected output)
#   From the Recoil worktree after a Debug build:
#     tools/scripts/split-debug-symbols.sh --runtime-directory build/window
#   First run:
#     Extracting DWARF symbols from /c/.../build/window/spring.exe
#     Stripping debug and unneeded symbols from the runtime executable
#     Verified stripped runtime and external symbols:
#       Runtime: /c/.../build/window/spring.exe
#       Symbols: /c/.../build/window/spring.dbg
#   Second run (nothing to do):
#     Symbols are already split and linked; verifying existing files.
#     Verified stripped runtime and external symbols: ...
#   A rebuild relinks spring.exe with embedded DWARF again, so run it after
#   every Debug build (the VS Code "build" task does).
#
# HOW TO EXTEND (worked example)
#   Split a second executable from the same build, e.g. the headless engine
#   (when that target was built into the same folder):
#     tools/scripts/split-debug-symbols.sh --runtime-directory build/window \
#       --binary-name spring-headless.exe
#   writes spring-headless.dbg beside it. To add another verification (for
#   example that the .dbg is newer than the .exe), add a test to
#   verify_symbols(); it runs in all three paths (fresh split, already split,
#   --verify-only). Use a different binutils build with PBR_OBJCOPY=... etc.

set -euo pipefail

# Defaults; options below override them. runtime_directory is relative to the
# caller's current directory, unlike run-pbr-ibl-smoke.sh which resolves its
# default from the repository root.
runtime_directory="${PBR_RUNTIME_DIRECTORY:-${PBR_BUILD_DIRECTORY:-build/window}}"
engine_input="${PBR_ENGINE:-}"
binary_name="spring.exe"
verify_only=0

usage() {
	cat <<'EOF'
Usage: split-debug-symbols.sh [options]
  --engine PATH            executable to split
  --runtime-directory PATH runtime/build directory (default: build/window)
  --build-directory PATH   compatibility alias for --runtime-directory
  --binary-name NAME       executable name under the runtime directory
  --verify-only            verify an existing .gnu_debuglink and .dbg file

Tool overrides: PBR_OBJCOPY, PBR_OBJDUMP, PBR_STRIP.
EOF
}

# Prints an error to stderr and stops with status 1.
die() { printf '%s\n' "$*" >&2; exit 1; }
# Path helpers (same as run-pbr-ibl-smoke.sh): cygpath -u accepts C:\x, C:/x
# and /c/x alike and returns the Git Bash form; the result is absolute.
absolute_directory() { (cd -- "$(cygpath -u "$1")" && pwd -P) || die "Directory does not exist: $1"; }
absolute_file() {
	local path directory name
	path="$(cygpath -u "$1")"
	[[ -f "$path" ]] || die "File does not exist: $1"
	directory="$(cd -- "$(dirname -- "$path")" && pwd -P)"
	name="$(basename -- "$path")"
	printf '%s/%s\n' "$directory" "$name"
}
# Resolves one binutils program: the PBR_* override if set (must exist), else
# the first on PATH, else the given MinGW fallback path. objcopy, objdump and
# strip must come from a toolchain that understands PE/COFF with DWARF, which
# the MinGW binutils do.
find_tool() {
	local override="$1" command_name="$2" fallback="$3" candidate
	if [[ -n "$override" ]]; then
		absolute_file "$override"
		return
	fi
	candidate="$(command -v "$command_name" 2>/dev/null || true)"
	if [[ -n "$candidate" ]]; then
		absolute_file "$candidate"
		return
	fi
	[[ -f "$fallback" ]] && { absolute_file "$fallback"; return; }
	die "Cannot find $command_name. Put it on PATH or set the matching PBR_* tool variable."
}

# Option parsing; options taking a value check that one follows.
while (($# > 0)); do
	case "$1" in
		--verify-only) verify_only=1; shift ;;
		--engine) (($# >= 2)) || die "Missing value for $1"; engine_input="$2"; shift 2 ;;
		--binary-name) (($# >= 2)) || die "Missing value for $1"; binary_name="$2"; shift 2 ;;
		--runtime-directory|--build-directory) (($# >= 2)) || die "Missing value for $1"; runtime_directory="$2"; shift 2 ;;
		-h|--help) usage; exit 0 ;;
		*) usage >&2; die "Unknown argument: $1" ;;
	esac
done

# Resolve the executable. --engine wins and defines the folder and name, so the
# default runtime directory (build/window) is resolved only without it and need
# not exist when --engine is given.
if [[ -n "$engine_input" ]]; then
	binary="$(absolute_file "$engine_input")"
	runtime_directory="$(dirname -- "$binary")"
	binary_name="$(basename -- "$binary")"
else
	runtime_directory="$(absolute_directory "$runtime_directory")"
	binary="${runtime_directory}/${binary_name}"
	[[ -f "$binary" ]] || die "Engine does not exist: $binary"
fi

# spring.exe -> spring.dbg beside it; GDB looks for the .gnu_debuglink name in
# the executable's own folder first. The symbols are written to a .tmp name and
# renamed only after strip succeeded, so a failed run never leaves a .dbg that
# looks valid.
debug_file="${runtime_directory}/${binary_name%.exe}.dbg"
temporary_debug_file="${debug_file}.tmp"
# the MinGW toolchain this project builds with, used when a tool is not on PATH
mingw_bin='C:/mingw64/bin'
objcopy="$(find_tool "${PBR_OBJCOPY:-}" objcopy "${mingw_bin}/objcopy.exe")"
objdump="$(find_tool "${PBR_OBJDUMP:-}" objdump "${mingw_bin}/objdump.exe")"
strip="$(find_tool "${PBR_STRIP:-}" strip "${mingw_bin}/strip.exe")"

# Inspect the executable's section table (objdump -h): .gnu_debuglink means it
# was already split; .debug_info/.debug_line/.debug_abbrev mean DWARF is still
# embedded. These two flags choose between verify, split or error below.
binary_sections="$("$objdump" -h "$binary")"
has_debug_link=0
has_embedded_debug=0
grep -q '\.gnu_debuglink' <<<"$binary_sections" && has_debug_link=1
grep -Eq '\.debug_(info|line|abbrev)' <<<"$binary_sections" && has_embedded_debug=1

# A split is valid when the executable carries a debug link, the .dbg file
# exists, and that file really holds DWARF sections (an empty or wrong file
# would leave GDB without symbols while everything else looks fine).
verify_symbols() {
	((has_debug_link)) || die "The runtime has no .gnu_debuglink: $binary"
	[[ -f "$debug_file" ]] || die "The external debug symbol file is missing: $debug_file"
	"$objdump" -h "$debug_file" | grep -Eq '\.debug_(info|line|abbrev)' ||
		die "The external symbol file has no DWARF debug sections: $debug_file"
	printf 'Verified stripped runtime and external symbols:\n  Runtime: %s\n  Symbols: %s\n' "$binary" "$debug_file"
}

# Three paths: --verify-only checks and stops; an already split executable is
# verified (running the script twice is harmless); otherwise DWARF must be
# embedded, or there is nothing to split and the build is not a debug build.
if ((verify_only)); then
	verify_symbols
	exit 0
fi

if ((has_debug_link)) && [[ -f "$debug_file" ]]; then
	printf 'Symbols are already split and linked; verifying existing files.\n'
	verify_symbols
	exit 0
fi

# Stripped but not linked: an earlier run stopped between the rename of the
# .dbg file and objcopy --add-gnu-debuglink. The symbols are complete in the
# .dbg file, so only the link step is repeated (in the runtime directory, so
# the link holds the bare file name), then the result is verified.
if ! ((has_embedded_debug)) && ! ((has_debug_link)) && [[ -f "$debug_file" ]] &&
	"$objdump" -h "$debug_file" | grep -Eq '\.debug_(info|line|abbrev)'; then
	printf 'The runtime is stripped but not linked to %s; adding the debug link.\n' "$debug_file"
	(
		cd -- "$runtime_directory"
		"$objcopy" "--add-gnu-debuglink=$(basename -- "$debug_file")" "$binary_name"
	)
	binary_sections="$("$objdump" -h "$binary")"
	grep -q '\.gnu_debuglink' <<<"$binary_sections" && has_debug_link=1
	verify_symbols
	exit 0
fi

((has_embedded_debug)) || die "The executable has neither embedded DWARF sections nor a usable external debug link: $binary"

# The split, in the order the GNU binutils documentation gives:
#   1. objcopy --only-keep-debug copies the debug sections into the .dbg file;
#   2. strip --strip-debug --strip-unneeded removes them (and symbols not
#      needed for relocation) from the executable, which shrinks it below 2 GiB;
#   3. objcopy --add-gnu-debuglink adds the link section naming the .dbg file
#      and its CRC, so GDB loads the right symbols. It runs inside the runtime
#      directory so the link stores only the file name, not an absolute path.
# The trap deletes the temporary file if any step fails; it is cleared once the
# .dbg file is in place.
rm -f -- "$temporary_debug_file"
trap 'rm -f -- "$temporary_debug_file"' EXIT
printf 'Extracting DWARF symbols from %s\n' "$binary"
"$objcopy" --only-keep-debug "$binary" "$temporary_debug_file"
printf 'Stripping debug and unneeded symbols from the runtime executable\n'
"$strip" --strip-debug --strip-unneeded "$binary"
mv -f -- "$temporary_debug_file" "$debug_file"
(
	cd -- "$runtime_directory"
	"$objcopy" "--add-gnu-debuglink=$(basename -- "$debug_file")" "$binary_name"
)
trap - EXIT

# Re-read the section table so verify_symbols sees the new debug link.
binary_sections="$("$objdump" -h "$binary")"
has_debug_link=0
grep -q '\.gnu_debuglink' <<<"$binary_sections" && has_debug_link=1
verify_symbols
