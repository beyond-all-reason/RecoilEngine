# Project-local SDL3 discovery for the macOS SDL3/EGL source-port route.

set(_RECOIL_SDL3_HINTS
	"${SDL3_ROOT}"
	"${SDL3_DIR}"
	"$ENV{SDL3_ROOT}"
	"/Users/yeojun/Desktop/BAR/artifacts/runtime/thread-03-sdl3"
)

find_package(SDL3 QUIET CONFIG
	PATHS ${_RECOIL_SDL3_HINTS}
	PATH_SUFFIXES lib/cmake/SDL3
	NO_DEFAULT_PATH
)

if (NOT SDL3_FOUND)
	find_path(SDL3_INCLUDE_DIR
		NAMES SDL3/SDL.h
		PATHS ${_RECOIL_SDL3_HINTS}
		PATH_SUFFIXES include
		NO_DEFAULT_PATH
	)

	find_library(SDL3_LIBRARY
		NAMES SDL3 SDL3.0
		PATHS ${_RECOIL_SDL3_HINTS}
		PATH_SUFFIXES lib
		NO_DEFAULT_PATH
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(SDL3 DEFAULT_MSG SDL3_INCLUDE_DIR SDL3_LIBRARY)

	if (SDL3_FOUND AND NOT TARGET SDL3::SDL3)
		add_library(SDL3::SDL3 UNKNOWN IMPORTED)
		set_target_properties(SDL3::SDL3 PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIR}"
			IMPORTED_LOCATION "${SDL3_LIBRARY}"
		)
	endif()
endif()

unset(_RECOIL_SDL3_HINTS)
