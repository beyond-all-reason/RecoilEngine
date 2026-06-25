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
	 nowide::nowide
	)

### Assemble libraries
find_package(SDL2 MODULE REQUIRED)

set(engineLibraries SDL2::SDL2)

if (APPLE AND RECOIL_MACOS_SDL3_EGL)
	find_package(MesaEGL MODULE REQUIRED)
	list(APPEND engineLibraries MesaEGL::EGL)
else()
	set(OpenGL_GL_PREFERENCE LEGACY)
	find_package_static(OpenGL 3.0 REQUIRED)
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

if (APPLE AND NOT RECOIL_MACOS_SDL3_EGL)
	find_library(COREFOUNDATION_LIBRARY Foundation)
	list(APPEND engineLibraries ${COREFOUNDATION_LIBRARY})
endif()

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

if (APPLE AND RECOIL_MACOS_SDL3_EGL)
	target_link_options(engine-legacy PRIVATE
		"SHELL:-framework Cocoa"
		"SHELL:-framework Foundation"
		"SHELL:-framework QuartzCore"
		"SHELL:-framework Metal"
	)
	set_target_properties(engine-legacy PROPERTIES
		BUILD_RPATH "@loader_path/../../artifacts/runtime/thread-03-sdl3/lib;@loader_path/../../artifacts/runtime/thread-06-mesa-zink/lib;@loader_path/../../artifacts/runtime/thread-05-continuation-vulkan-loader/lib;@loader_path/../../artifacts/runtime/thread-05-moltenvk-production-pr-2746-spirv-cross/lib"
		INSTALL_RPATH "@loader_path/../lib"
	)
endif()

# Export symbols for plugin access (replaces CMP0065 OLD behavior)
set_target_properties(engine-legacy PROPERTIES ENABLE_EXPORTS TRUE)

if(MSVC)
    target_link_libraries(engine-legacy engine-natvis)
endif()


### Install the executable
install(TARGETS engine-legacy DESTINATION ${BINDIR})

create_engine_build_and_install_target(legacy)
