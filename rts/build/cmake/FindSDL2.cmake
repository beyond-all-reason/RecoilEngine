# The version of SDL we have is too old
# and doesn't provide a proper config file.
# We need to create imported targets for the config

if (APPLE AND RECOIL_MACOS_SDL3_EGL)
  find_package(SDL3 MODULE REQUIRED)

  if (SDL3_FOUND AND NOT TARGET SDL2::SDL2)
    # Recoil still has an SDL2-shaped source/CMake contract. The macOS
    # SDL3/EGL path keeps that contract at the boundary while linking SDL3.
    add_library(RecoilSDL2SourceCompat INTERFACE)
    add_library(SDL2::SDL2 ALIAS RecoilSDL2SourceCompat)
    target_link_libraries(RecoilSDL2SourceCompat INTERFACE SDL3::SDL3)
    target_include_directories(RecoilSDL2SourceCompat BEFORE INTERFACE "${CMAKE_SOURCE_DIR}/rts/System/SDLCompat")
    target_compile_definitions(RecoilSDL2SourceCompat INTERFACE RECOIL_MACOS_SDL3_EGL=1 SDL_ENABLE_OLD_NAMES=1)
  endif()

  set(SDL2_FOUND ${SDL3_FOUND})
  set(SDL2_LIBRARIES SDL2::SDL2)
  return()
endif()

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
endif()
