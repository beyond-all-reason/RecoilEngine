# Locate SDL2 and provide the SDL2::SDL2 imported target.
#
# Two cases are supported:
#  - A SDL2 CMake config package is available (e.g. provided by Conan on Windows).
#    It is used directly and exposes SDL2::SDL2.
#  - No config package (e.g. Linux, where we link against the system libsdl2-dev,
#    which is too old to ship a usable CMake config). We locate it via pkg-config.
#    Only glibc and SDL2 are linked; SDL dlopen()s X11/Wayland/EGL at runtime, which
#    is SDL's recommended portable Linux behaviour.

# Prefer a config package if one is available.
find_package(SDL2 CONFIG QUIET)
if (SDL2_FOUND AND TARGET SDL2::SDL2)
	return()
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_SDL2 QUIET sdl2)

find_path(SDL2_INCLUDE_DIR
	NAMES SDL.h
	HINTS ${PC_SDL2_INCLUDE_DIRS}
	PATH_SUFFIXES SDL2
)
find_library(SDL2_LIBRARY
	NAMES SDL2
	HINTS ${PC_SDL2_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2 DEFAULT_MSG SDL2_LIBRARY SDL2_INCLUDE_DIR)
mark_as_advanced(SDL2_INCLUDE_DIR SDL2_LIBRARY)

if (SDL2_FOUND AND NOT TARGET SDL2::SDL2)
	add_library(SDL2::SDL2 UNKNOWN IMPORTED)
	set_target_properties(SDL2::SDL2 PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
		IMPORTED_LOCATION "${SDL2_LIBRARY}"
	)
endif()
