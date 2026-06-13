# SDL3 compatibility shim.
#
# The engine has been migrated to SDL3, but a lot of build targets still call
# find_package(SDL2 ...) and link the SDL2::SDL2 target. Rather than touch every
# one of those sites, this module resolves SDL3 and re-exports it under the
# legacy SDL2::SDL2 name.
#
# SDL3 can come from two places:
#   - the system (an installed SDL3 CMake config), or
#   - a vendored/static build that already created the SDL3::SDL3[-static]
#     target before this module runs (see RECOIL_SDL3_STATIC in the top-level
#     CMakeLists.txt). In that case we reuse the existing target.
#
# Two compatibility concerns are handled here:
#   1. Linking: prefer the static SDL3 target (SDL3::SDL3-static) when present,
#      otherwise fall back to the regular SDL3::SDL3 target.
#   2. Includes: the engine sources use SDL2's bare include style
#      (<SDL.h>, <SDL_events.h>, ...). SDL3 ships those same headers under an
#      <SDL3/...> subdirectory, so we add that subdir to the include path and
#      the existing bare includes keep resolving unchanged.

# Only probe the system if a vendored SDL3 target isn't already present.
if (NOT TARGET SDL3::SDL3 AND NOT TARGET SDL3::SDL3-static)
  find_package(SDL3 QUIET CONFIG)
endif ()

if (TARGET SDL3::SDL3 OR TARGET SDL3::SDL3-static)
  set(SDL2_FOUND TRUE)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2
  REQUIRED_VARS SDL2_FOUND
  FAIL_MESSAGE "SDL3 not found (the SDL2 find-module is an SDL3 shim)"
)

if (SDL2_FOUND AND NOT TARGET SDL2::SDL2)
  if    (TARGET SDL3::SDL3-static)
    set(_recoil_sdl3_target SDL3::SDL3-static)
  else  ()
    set(_recoil_sdl3_target SDL3::SDL3)
  endif ()

  add_library(SDL2::SDL2 INTERFACE IMPORTED)
  set_target_properties(SDL2::SDL2 PROPERTIES
    INTERFACE_LINK_LIBRARIES "${_recoil_sdl3_target}"
  )

  # bare-include shim: add the dir holding the SDL3/ header subdir so the
  # engine's bare <SDL_xxx.h> includes resolve. SDL3's own include dirs live on a
  # sub-target, so resolve the path independently here.
  #
  # For a vendored SDL3 (FetchContent) the headers are in the fetched source
  # tree. We set the path directly because find_path() is restricted to
  # CMAKE_FIND_ROOT_PATH under cross-compilation (e.g. the mingw CI build) and
  # would not see the source tree. For a system SDL3 we fall back to find_path.
  if    (DEFINED sdl3_SOURCE_DIR AND EXISTS "${sdl3_SOURCE_DIR}/include/SDL3/SDL.h")
    set(SDL2_SHIM_INCLUDE_DIR "${sdl3_SOURCE_DIR}/include")
  else  ()
    find_path(SDL2_SHIM_INCLUDE_DIR
      NAMES SDL3/SDL.h
      HINTS
        ${SDL3_DIR}/../../../include
        ${SDL3_INCLUDE_DIRS}
    )
  endif ()
  if (SDL2_SHIM_INCLUDE_DIR)
    set_property(TARGET SDL2::SDL2 APPEND PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES "${SDL2_SHIM_INCLUDE_DIR}/SDL3")
  endif ()

  # SDL3 renamed the bulk of SDL2's enums/functions (e.g. SDL_MOUSEMOTION ->
  # SDL_EVENT_MOUSE_MOTION, SDL_FreeSurface -> SDL_DestroySurface, SDL_TRUE ->
  # true). SDL ships <SDL3/SDL_oldnames.h> which #defines every old name to its
  # new spelling when SDL_ENABLE_OLD_NAMES is set. We force-include the umbrella
  # <SDL.h> (which pulls SDL_oldnames.h at its end) into every SDL-using TU. This
  # does double duty:
  #   - the mechanical renames resolve without editing ~100 files;
  #   - all SDL headers are parsed *before* any engine header, so engine macros
  #     like Camera.h's `#define camera ...` can no longer collide with SDL3's
  #     newer headers (e.g. SDL_camera.h's `camera` parameter).
  # Structural/semantic changes (window-event split, SDL_CreateWindow signature,
  # bool-return flips, SDL_ShowCursor, SDL_syswm) are NOT covered by oldnames and
  # are fixed in the engine sources.
  set_property(TARGET SDL2::SDL2 APPEND PROPERTY
    INTERFACE_COMPILE_DEFINITIONS SDL_ENABLE_OLD_NAMES)
  if    (MSVC)
    set_property(TARGET SDL2::SDL2 APPEND PROPERTY
      INTERFACE_COMPILE_OPTIONS "/FISDL.h")
  else  ()
    set_property(TARGET SDL2::SDL2 APPEND PROPERTY
      INTERFACE_COMPILE_OPTIONS "-include" "SDL.h")
  endif ()

  unset(_recoil_sdl3_target)
endif ()
