# This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html

# - Find Mesa's EGL
#
# macOS has no system EGL and Apple's OpenGL.framework tops out at a 2.1
# compatibility profile, below what the engine needs, so on that platform the
# GL context comes from a Mesa build instead. The Homebrew keg is the default;
# set MESAEGL_ROOT to use a Mesa installed elsewhere.
#
#  MESAEGL_INCLUDE_DIR - where to find EGL/egl.h
#  MESAEGL_LIBRARY     - the EGL library to link against
#  MESAEGL_FOUND       - true if Mesa's EGL was found
#
# Also defines the imported target MesaEGL::EGL.

set(MESAEGL_ROOT "" CACHE PATH "Root of the Mesa installation providing EGL")

set(MESAEGL_SEARCH_PATHS
	/opt/homebrew/opt/mesa
	/usr/local/opt/mesa
)

find_path(MESAEGL_INCLUDE_DIR
          NAMES
           EGL/egl.h
          HINTS
           ${MESAEGL_ROOT}
           ENV MESAEGL_ROOT
          PATHS
           ${MESAEGL_SEARCH_PATHS}
          PATH_SUFFIXES
           include
)

find_library(MESAEGL_LIBRARY
             NAMES
              EGL
             HINTS
              ${MESAEGL_ROOT}
              ENV MESAEGL_ROOT
             PATHS
              ${MESAEGL_SEARCH_PATHS}
             PATH_SUFFIXES
              lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MesaEGL DEFAULT_MSG MESAEGL_LIBRARY MESAEGL_INCLUDE_DIR)
mark_as_advanced(MESAEGL_LIBRARY MESAEGL_INCLUDE_DIR)

if (MESAEGL_FOUND AND NOT TARGET MesaEGL::EGL)
	add_library(MesaEGL::EGL UNKNOWN IMPORTED)
	set_target_properties(MesaEGL::EGL PROPERTIES
	                      INTERFACE_INCLUDE_DIRECTORIES "${MESAEGL_INCLUDE_DIR}"
	                      IMPORTED_LOCATION ${MESAEGL_LIBRARY}
	)
endif()
