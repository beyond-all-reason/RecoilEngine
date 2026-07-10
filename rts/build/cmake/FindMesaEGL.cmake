# Project-local Mesa EGL discovery for the macOS SDL3/EGL source-port route.

set(_RECOIL_MESA_EGL_HINTS
	"${MESA_EGL_ROOT}"
	"${MesaEGL_ROOT}"
	"$ENV{MESA_EGL_ROOT}"
	"/Users/yeojun/Desktop/BAR/artifacts/runtime/thread-06-mesa-zink"
)

find_path(MesaEGL_INCLUDE_DIR
	NAMES EGL/egl.h
	PATHS ${_RECOIL_MESA_EGL_HINTS}
	PATH_SUFFIXES include
	NO_DEFAULT_PATH
)

find_library(MesaEGL_LIBRARY
	NAMES EGL libEGL.1.dylib
	PATHS ${_RECOIL_MESA_EGL_HINTS}
	PATH_SUFFIXES lib
	NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MesaEGL DEFAULT_MSG MesaEGL_INCLUDE_DIR MesaEGL_LIBRARY)

if (MesaEGL_FOUND AND NOT TARGET MesaEGL::EGL)
	add_library(MesaEGL::EGL UNKNOWN IMPORTED)
	set_target_properties(MesaEGL::EGL PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${MesaEGL_INCLUDE_DIR}"
		IMPORTED_LOCATION "${MesaEGL_LIBRARY}"
	)
endif()

unset(_RECOIL_MESA_EGL_HINTS)
