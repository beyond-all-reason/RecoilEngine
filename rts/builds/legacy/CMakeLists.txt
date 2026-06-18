# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

add_library(Game STATIC)
target_sources(Game PRIVATE ${sources_engine_Game})
add_dependencies(Game generateVersionFiles)
target_include_directories(Game PRIVATE "${CMAKE_BINARY_DIR}/src-generated/engine")
target_link_libraries(Game PRIVATE
	 Tracy::TracyClient
	 SDL2::SDL2
	 RmlUi::RmlUi
	 ${sound-impl}
	 prd::jsoncpp
	 streflop
	 fmt::fmt
)

### Assemble libraries
find_package(SDL2 MODULE REQUIRED)





set(engineLibraries SDL2::SDL2)

set(OpenGL_GL_PREFERENCE LEGACY)
find_package_static(OpenGL 3.0 REQUIRED)
# On macOS, linking Apple's OpenGL.framework causes its NSWindow notification
# observer to register and then bus-error in +[NSOpenGLContext currentContext]
# on macOS 26 (Tahoe). When a Mesa libEGL is provided, all GL calls in this
# binary are routed through glad function pointers loaded via eglGetProcAddress,
# so we don't need libGL at link time at all -- libEGL alone is enough.
if (APPLE AND SPRING_MAC_LIBEGL)
	get_filename_component(_MESA_LIB_DIR "${SPRING_MAC_LIBEGL}" DIRECTORY)
	if (EXISTS "${_MESA_LIB_DIR}/libGL.dylib")
		list(APPEND engineLibraries "${_MESA_LIB_DIR}/libGL.dylib")
		message(STATUS "macOS: linking Mesa libGL at ${_MESA_LIB_DIR}/libGL.dylib")
	else()
		message(STATUS "macOS: no libGL.dylib next to ${SPRING_MAC_LIBEGL}; relying on libEGL + eglGetProcAddress at runtime")
	endif()
else()
	list(APPEND engineLibraries OpenGL::GL)
endif()

find_fontconfig_hack()
find_package_static(Fontconfig 2.11 REQUIRED)
list(APPEND engineLibraries Fontconfig::Fontconfig)
add_definitions(-DUSE_FONTCONFIG)

if (NOT WIN32)
	find_package_static(EXPAT 2.2.5 REQUIRED)
	list(APPEND engineLibraries EXPAT::EXPAT)
endif()

# !!! If you change freetype resolution here, change it also in rts/lib
find_freetype_hack() # hack to find different named freetype.dll
find_package_static(Freetype 2.8.1 REQUIRED)
list(APPEND engineLibraries Freetype::Freetype)

if    (UNIX AND NOT APPLE)
	find_package(X11 REQUIRED)
	target_link_libraries(Game PRIVATE X11::Xcursor)
	list(APPEND engineLibraries ${X11_Xcursor_LIB} ${X11_X11_LIB})
endif (UNIX AND NOT APPLE)

if (APPLE)
	find_library(COREFOUNDATION_LIBRARY Foundation)
	list(APPEND engineLibraries ${COREFOUNDATION_LIBRARY})
	# Metal + QuartzCore + IOSurface for the manual pbuffer->CAMetalLayer present
	# path (rts/System/Platform/Mac/MetalPresent.mm). IOSurface backs the
	# zero-copy glReadPixels destination shared with the MTLTexture.
	find_library(METAL_LIBRARY Metal)
	find_library(QUARTZCORE_LIBRARY QuartzCore)
	find_library(IOSURFACE_LIBRARY IOSurface)
	list(APPEND engineLibraries ${METAL_LIBRARY} ${QUARTZCORE_LIBRARY} ${IOSURFACE_LIBRARY})
	set(SPRING_MAC_LIBEGL "" CACHE FILEPATH
		"Optional path to a Mesa libEGL.dylib (e.g. a Zink+KosmicKrisp build). \
Leave empty to link only against Apple's OpenGL.framework.")
	if (SPRING_MAC_LIBEGL)
		list(APPEND engineLibraries "${SPRING_MAC_LIBEGL}")
	endif()
	list(APPEND engineLibraries objc)
endif (APPLE)

list(APPEND engineLibraries squish rgetc1)
list(APPEND engineLibraries ${sound-impl})
list(APPEND engineLibraries engineSystemNet)
list(APPEND engineLibraries ${engineCommonLibraries})
#list(APPEND engineLibraries engineaGui)
list(APPEND engineLibraries ${SPRING_SIM_LIBRARIES})
list(APPEND engineLibraries engineSim)
list(APPEND engineLibraries pr-downloader)
list(APPEND engineLibraries RmlUi::Core)
list(APPEND engineLibraries RmlUi::Debugger)
list(APPEND engineLibraries lunasvg)
list(APPEND engineLibraries glad)

### Assemble external include dirs
list(APPEND engineIncludes ${ENGINE_SRC_ROOT_DIR}/lib/asio/include)
list(APPEND engineIncludes ${ENGINE_SRC_ROOT_DIR}/lib/slimsig/include)
list(APPEND engineIncludes ${ENGINE_SRC_ROOT_DIR}/lib/cereal/include)

include_directories(${engineIncludes})

### Build the executable
add_executable(engine-legacy ${EXE_FLAGS} ${engineSources} ${ENGINE_ICON} ${engineHeaders})
target_link_libraries(engine-legacy ${engineLibraries} Game)

# Export symbols for plugin access (replaces CMP0065 OLD behavior)
set_target_properties(engine-legacy PROPERTIES ENABLE_EXPORTS TRUE)

# Suppress "ignoring duplicate libraries" linker warning on macOS.
# Static library transitive deps (engineSim, pr-downloader, sound, etc.) propagate
# dependencies that are already in engineLibraries, causing benign duplicates.
if(APPLE)
	target_link_options(engine-legacy PRIVATE "LINKER:-no_warn_duplicate_libraries")
endif()

if(MSVC)
    target_link_libraries(engine-legacy engine-natvis)
endif()


### Install the executable
install(TARGETS engine-legacy DESTINATION ${BINDIR})

create_engine_build_and_install_target(legacy)
