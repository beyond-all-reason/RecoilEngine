#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
recoil_root="$(cd -- "${script_dir}/../.." && pwd -P)"
build_directory="${recoil_root}/build/window"
mingwlibs_directory="${recoil_root}/mingwlibs64"

usage() {
	printf 'Usage: %s [--build-directory PATH] [--mingwlibs-directory PATH]\n' "$0"
}

to_posix_path() {
	cygpath -u "$1" 2>/dev/null || printf '%s\n' "$1"
}

while (($# > 0)); do
	case "$1" in
		--build-directory)
			build_directory="$2"
			shift 2
			;;
		--mingwlibs-directory)
			mingwlibs_directory="$2"
			shift 2
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			printf 'Unknown argument: %s\n' "$1" >&2
			usage >&2
			exit 2
			;;
	esac
done

build_directory="$(to_posix_path "$build_directory")"
mingwlibs_directory="$(to_posix_path "$mingwlibs_directory")"
source_directory="${mingwlibs_directory}/dll"
engine="${build_directory}/spring.exe"

[[ -f "$engine" ]] || { printf 'The engine has not been built: %s\n' "$engine" >&2; exit 1; }
[[ -d "$source_directory" ]] || { printf 'The MinGW runtime DLL directory does not exist: %s\n' "$source_directory" >&2; exit 1; }

mapfile -d '' runtime_libraries < <(find "$source_directory" -maxdepth 1 -type f -iname '*.dll' -print0)
((${#runtime_libraries[@]} > 0)) || { printf 'No runtime DLLs were found in %s\n' "$source_directory" >&2; exit 1; }

cp -f -- "${runtime_libraries[@]}" "$build_directory/"

required_libraries=(
	libcurl.dll
	OpenAL32.dll
	zlib1.dll
	SDL2.dll
	libfontconfig-1.dll
	libfreetype-6.dll
	libIL.dll
	libvorbisfile-3.dll
)

for library in "${required_libraries[@]}"; do
	[[ -f "${build_directory}/${library}" ]] || {
		printf 'Required runtime DLL was not installed: %s\n' "${build_directory}/${library}" >&2
		exit 1
	}
done

printf 'Installed and verified %d runtime DLLs in %s\n' "${#runtime_libraries[@]}" "$build_directory"
