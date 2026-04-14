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
	 ${ZLIB_LIBRARIES}
)

### Assemble libraries
find_package(SDL2 REQUIRED)

set(engineLibraries SDL2::SDL2)

set(OpenGL_GL_PREFERENCE LEGACY)
find_package_static(OpenGL REQUIRED)
list(APPEND engineLibraries OpenGL::GL)

find_package_static(Fontconfig REQUIRED)
list(APPEND engineLibraries Fontconfig::Fontconfig)
add_definitions(-DUSE_FONTCONFIG)

if (NOT WIN32)
	find_package_static(EXPAT REQUIRED)
	if (TARGET expat::expat)
		list(APPEND engineLibraries expat::expat)
	else()
		list(APPEND engineLibraries EXPAT::EXPAT)
	endif()
endif()

find_package_static(Freetype 2.8.1 REQUIRED)
list(APPEND engineLibraries Freetype::Freetype)

if    (UNIX)
	find_package(X11 REQUIRED)
	target_link_libraries(Game PRIVATE X11::Xcursor)
	list(APPEND engineLibraries ${X11_Xcursor_LIB} ${X11_X11_LIB})
endif (UNIX)

if (APPLE)
	find_library(COREFOUNDATION_LIBRARY Foundation)
	list(APPEND engineLibraries ${COREFOUNDATION_LIBRARY})
endif (APPLE)

list(APPEND engineLibraries squish rgetc1)
list(APPEND engineLibraries ${sound-impl})
list(APPEND engineLibraries engineSystemNet)
list(APPEND engineLibraries ${engineCommonLibraries})
list(APPEND engineLibraries ${SPRING_SIM_LIBRARIES})
list(APPEND engineLibraries engineSim)
list(APPEND engineLibraries pr-downloader)
list(APPEND engineLibraries RmlUi::Core)
list(APPEND engineLibraries RmlUi::Debugger)
list(APPEND engineLibraries lunasvg)
list(APPEND engineLibraries glad)
list(APPEND engineLibraries ${ZLIB_LIBRARIES})

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

if(MSVC)
    target_link_libraries(engine-legacy engine-natvis)
endif()


### Install the executable
install(TARGETS engine-legacy DESTINATION ${BINDIR})

create_engine_build_and_install_target(legacy)
