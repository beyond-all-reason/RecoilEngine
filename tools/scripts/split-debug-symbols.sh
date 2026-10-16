#!/usr/bin/env bash
# Splits MinGW DWARF data out of a Windows Recoil executable so the PE image
# remains below Windows' 2 GiB executable limit. Tool and path overrides make
# the helper usable outside this repository's original development layout.

set -euo pipefail

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

runtime_directory="$(absolute_directory "$runtime_directory")"
if [[ -n "$engine_input" ]]; then
	binary="$(absolute_file "$engine_input")"
	runtime_directory="$(dirname -- "$binary")"
	binary_name="$(basename -- "$binary")"
else
	binary="${runtime_directory}/${binary_name}"
	[[ -f "$binary" ]] || die "Engine does not exist: $binary"
fi

debug_file="${runtime_directory}/${binary_name%.exe}.dbg"
temporary_debug_file="${debug_file}.tmp"
objcopy="$(find_tool "${PBR_OBJCOPY:-}" objcopy 'C:/mingw64/bin/objcopy.exe')"
objdump="$(find_tool "${PBR_OBJDUMP:-}" objdump 'C:/mingw64/bin/objdump.exe')"
strip="$(find_tool "${PBR_STRIP:-}" strip 'C:/mingw64/bin/strip.exe')"

binary_sections="$($objdump -h "$binary")"
has_debug_link=0
has_embedded_debug=0
grep -q '\.gnu_debuglink' <<<"$binary_sections" && has_debug_link=1
grep -Eq '\.debug_(info|line|abbrev)' <<<"$binary_sections" && has_embedded_debug=1

verify_symbols() {
	((has_debug_link)) || die "The runtime has no .gnu_debuglink: $binary"
	[[ -f "$debug_file" ]] || die "The external debug symbol file is missing: $debug_file"
	"$objdump" -h "$debug_file" | grep -Eq '\.debug_(info|line|abbrev)' ||
		die "The external symbol file has no DWARF debug sections: $debug_file"
	printf 'Verified stripped runtime and external symbols:\n  Runtime: %s\n  Symbols: %s\n' "$binary" "$debug_file"
}

if ((verify_only)); then
	verify_symbols
	exit 0
fi

if ((has_debug_link)) && [[ -f "$debug_file" ]]; then
	printf 'Symbols are already split and linked; verifying existing files.\n'
	verify_symbols
	exit 0
fi

((has_embedded_debug)) || die "The executable has neither embedded DWARF sections nor a usable external debug link: $binary"

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

binary_sections="$($objdump -h "$binary")"
has_debug_link=0
grep -q '\.gnu_debuglink' <<<"$binary_sections" && has_debug_link=1
verify_symbols
