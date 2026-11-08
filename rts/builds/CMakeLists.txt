### builds
#
# Macros and functions defined in this file:
# * create_engine_build_and_install_target
# * recoil_add_variant_library
#

include_directories(${CMAKE_BINARY_DIR}/src-generated/engine)
include_directories(${gflags_BINARY_DIR}/include)

# Only install the given engine target executable & dependencies
# example:
# 	create_engine_build_and_install_target(headless)
# ->
# 	make spring-headless
# 	make install-spring-headless
macro    (create_engine_build_and_install_target targetName)
	# relative source path, eg: "rts/builds/default"
	file(RELATIVE_PATH relSrcDirPath ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

	set(myBuildTarget spring-${targetName})
	set(${targetName}-Deps
			engine-${targetName}
			basecontent
			unitsync
			${DEPS_AI_ALL}
		)
	if (CREATE_MAN_PAGES)
		list(APPEND ${targetName}-Deps manpages)
	endif()
	if (userdoc_INSTALL_HTML)
		list(APPEND ${targetName}-Deps userdocs)
	endif()

	# Create a custom meta build target
	add_custom_target(${myBuildTarget}
		WORKING_DIRECTORY
			"${CMAKE_BINARY_DIR}"
		COMMENT
			"  ${myBuildTarget}: Building all dependencies ..." VERBATIM
		)
	# This also works for custom targets
	add_dependencies(${myBuildTarget} ${${targetName}-Deps})
	add_dependencies(engine-${targetName} generateVersionFiles basecontent)

	# Only install spring-<targetName> executable & dependencies
	set(${targetName}-InstallDirs
			"${relSrcDirPath}"
			"doc"
			"tools/unitsync"
			"cont"
			"rts/lib/luasocket"
		)
	if    (NOT "${AI_TYPES}" STREQUAL "NONE")
		set(${targetName}-InstallDirs ${${targetName}-InstallDirs} "AI")
	endif (NOT "${AI_TYPES}" STREQUAL "NONE")

	create_install_target(spring-${targetName} myBuildTarget ${targetName}-InstallDirs)

	# allow easy switching of the default engine
	if    (${targetName} STREQUAL "legacy")
		# Create shortcuts
		# * make spring
		# * make install-spring
		message(STATUS "Using ${targetName} as default engine")
		add_dependencies(spring spring-${targetName}) # This also works for custom targets
		add_dependencies(install-spring install-spring-${targetName}) # This also works for custom targets
		set_target_properties(engine-${targetName} PROPERTIES OUTPUT_NAME "spring")
	else()
		set_target_properties(engine-${targetName} PROPERTIES OUTPUT_NAME "spring-${targetName}")
	endif ()
endmacro (create_engine_build_and_install_target targetName)


# This allows us to disable each build type individually at configure stage.
macro    (add_engine_build build)
	option(BUILD_spring-${build} "Configure the spring-${build} target." TRUE)
	if     (BUILD_spring-${build})
		add_subdirectory(${build})
	endif  ()
endmacro (add_engine_build)

# Engine build variants
#
# Builds compile the same sources with different directory-level defines (HEADLESS, ...),
# so a library is described once as an INTERFACE library (sources, includes, build-agnostic
# deps) and each build creates its variant with this function. Interface sources are compiled
# by the linking target with its defines. Don't link the INTERFACE library directly:
# every target that links it compiles all its sources.
# example:
# 	recoil_add_variant_library(GameHeadless GameCommon)
function(recoil_add_variant_library targetName commonTarget)
	if (NOT TARGET ${commonTarget})
		message(FATAL_ERROR "recoil_add_variant_library: ${commonTarget} target does not exist")
	endif()
	get_target_property(commonType ${commonTarget} TYPE)
	if (NOT commonType STREQUAL "INTERFACE_LIBRARY")
		message(FATAL_ERROR "recoil_add_variant_library: ${commonTarget} must be an INTERFACE library, got ${commonType}")
	endif()

	add_library(${targetName} STATIC)
	target_link_libraries(${targetName} PRIVATE ${commonTarget})
endfunction()

# add default dummy target which is used later on
add_custom_target(spring WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
add_custom_target(install-spring WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

### Add icon and manifest to exe using windres
if    (WIN32)
	set(ENGINE_ICON "${ENGINE_SRC_ROOT_DIR}/icon.rc")

	if (MSVC)
		# Make sure we disable the default manifest generation to prevent conflicts
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO /NODEFAULTLIB:LIBCMT")
	endif()
endif (WIN32)

if (MSVC)
	add_library(engine-natvis INTERFACE)
	target_sources(engine-natvis INTERFACE ${RECOIL_NATVIS_FILES})
endif()

# the build targets
add_engine_build(legacy)
add_engine_build(dedicated)
add_engine_build(headless)
