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

if    (APPLE)
	# Apple's OpenGL.framework has no compatibility profile past 2.1, so the
	# context comes from Mesa through EGL instead, see MacEGLContext.h
	find_package(MesaEGL MODULE REQUIRED)
	list(APPEND engineLibraries MesaEGL::EGL)
else  (APPLE)
	set(OpenGL_GL_PREFERENCE LEGACY)
	find_package_static(OpenGL 3.0 REQUIRED)
	list(APPEND engineLibraries OpenGL::GL)
endif (APPLE)

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
endif (UNIX)

if (APPLE)
	find_library(COREFOUNDATION_LIBRARY Foundation)
	list(APPEND engineLibraries ${COREFOUNDATION_LIBRARY})

	# for the present path that takes the EGL pbuffer to the screen
	find_library(APPKIT_LIBRARY AppKit)
	find_library(METAL_LIBRARY Metal)
	find_library(QUARTZCORE_LIBRARY QuartzCore)
	find_library(IOSURFACE_LIBRARY IOSurface)
	find_library(OBJC_LIBRARY objc)
	list(APPEND engineLibraries
		${APPKIT_LIBRARY}
		${METAL_LIBRARY}
		${QUARTZCORE_LIBRARY}
		${IOSURFACE_LIBRARY}
		${OBJC_LIBRARY}
	)

	list(APPEND engineSources
		"${ENGINE_SRC_ROOT_DIR}/System/Platform/Mac/MacEGLContext.cpp"
		"${ENGINE_SRC_ROOT_DIR}/System/Platform/Mac/MacGLPresent.cpp"
		"${ENGINE_SRC_ROOT_DIR}/System/Platform/Mac/MetalPresent.mm"
	)
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

if (APPLE)
	# Mesa's Zink driver loads the Vulkan loader as @rpath/libvulkan.1.dylib,
	# which dyld resolves against the rpaths of the running executable. This
	# covers the build tree only, a bundled build has to relocate the dylibs.
	find_library(VULKAN_LOADER_LIBRARY
	             NAMES
	              vulkan
	             PATHS
	              /opt/homebrew
	              /usr/local
	             PATH_SUFFIXES
	              lib
	)

	if (VULKAN_LOADER_LIBRARY)
		get_filename_component(vulkanLoaderDir "${VULKAN_LOADER_LIBRARY}" DIRECTORY)
		set_property(TARGET engine-legacy APPEND PROPERTY BUILD_RPATH "${vulkanLoaderDir}")
	else()
		message(WARNING "no Vulkan loader found, engine-legacy will not get a GL context unless Mesa was built without Zink")
	endif()
endif (APPLE)

if(MSVC)
    target_link_libraries(engine-legacy engine-natvis)
endif()


### Install the executable
install(TARGETS engine-legacy DESTINATION ${BINDIR})

create_engine_build_and_install_target(legacy)
