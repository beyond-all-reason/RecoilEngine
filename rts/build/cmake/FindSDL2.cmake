# The version of SDL we have is too old
# and doesn't provide a proper config file.
# We need to create imported targets for the config

find_package(SDL2 QUIET CONFIG)

find_library(SDL2_LIBRARY
             NAMES
              SDL2
             PATHS
              ${SDL2_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2 DEFAULT_MSG SDL2_INCLUDE_DIRS SDL2_LIBRARIES SDL2_LIBRARY)
mark_as_advanced(SDL2_LIBRARIES SDL2_LIBRARY)

if (SDL2_FOUND AND NOT TARGET SDL2::SDL2)
  add_library(SDL2::SDL2 UNKNOWN IMPORTED)
  set_target_properties(SDL2::SDL2 PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
                        IMPORTED_LOCATION ${SDL2_LIBRARY}
  )
elseif(SDL2_FOUND AND TARGET SDL2::SDL2)
  # SDL2's CMake config may set INTERFACE_INCLUDE_DIRECTORIES to SDL2_INCLUDE_DIR (e.g. /include/SDL2)
  # but this project uses #include <SDL2/header.h>, so we need the parent directory too
  # Fix the include directories to include both the SDL2 subdirectory and its parent
  get_target_property(_sdl2_includes SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
  if(_sdl2_includes)
    set(_new_includes "")
    foreach(_inc ${_sdl2_includes})
      list(APPEND _new_includes "${_inc}")
      # If this path ends with /SDL2, also add the parent directory
      if(_inc MATCHES "/SDL2$")
        get_filename_component(_parent "${_inc}" DIRECTORY)
        list(APPEND _new_includes "${_parent}")
      endif()
    endforeach()
    list(REMOVE_DUPLICATES _new_includes)
    set_target_properties(SDL2::SDL2 PROPERTIES
                          INTERFACE_INCLUDE_DIRECTORIES "${_new_includes}"
    )
  endif()
endif()
