# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# Downloads and extracts the RecoilEngineLibs package for the current build target.
#
# Usage:
#   include(DownloadRecoilLibs)
#   download_recoil_libs("<version>" "<destination-dir>")
#
# The tarball is extracted flat into <destination-dir> (i.e. include/, lib/, bin/
# appear directly inside it), matching the layout of a manually extracted package.
#
# The RECOILLIBS_VERSION cache variable can override the version passed by the caller.

function(_recoillibs_triplet out_var)
    if(MSVC)
        set(_t "x64-windows-msvc")
    elseif(CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(_t "x64-mingw")
    elseif(MINGW)
        set(_t "x64-mingw")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|ARM64|arm64)$")
        set(_t "arm64-linux")
    else()
        set(_t "x64-linux")
    endif()
    set(${out_var} "${_t}" PARENT_SCOPE)
endfunction()

function(download_recoil_libs version dest_dir)
    _recoillibs_triplet(_triplet)
    set(_tarball "recoil-libs-${_triplet}.tar.gz")
    set(_url "https://github.com/RecoilEngine/RecoilEngineLibs/releases/download/${version}/${_tarball}")
    set(_dl_tmp "${CMAKE_BINARY_DIR}/_recoil_libs_dl")
    set(_tar_path "${_dl_tmp}/${_tarball}")

    file(MAKE_DIRECTORY "${_dl_tmp}")
    file(MAKE_DIRECTORY "${dest_dir}")

    message(STATUS "Downloading RecoilEngineLibs ${version} (${_triplet})...")
    message(STATUS "  URL: ${_url}")
    file(DOWNLOAD "${_url}" "${_tar_path}"
        STATUS _status
        SHOW_PROGRESS
        TLS_VERIFY ON
    )
    list(GET _status 0 _err_code)
    if(_err_code)
        list(GET _status 1 _err_msg)
        message(FATAL_ERROR
            "Failed to download RecoilEngineLibs ${version}: ${_err_msg}\n"
            "URL: ${_url}\n"
            "Set -DRECOILLIBS=<path> to point to a local package instead.")
    endif()

    message(STATUS "Extracting RecoilEngineLibs to ${dest_dir}...")
    file(ARCHIVE_EXTRACT INPUT "${_tar_path}" DESTINATION "${dest_dir}")
    file(REMOVE_RECURSE "${_dl_tmp}")
    message(STATUS "RecoilEngineLibs ${version} installed to ${dest_dir}")
endfunction()
