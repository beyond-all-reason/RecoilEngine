# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

if (NOT DEFINED SOURCE_DIR)
	get_filename_component(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

string(TIMESTAMP TEST_STAMP "%Y%m%d%H%M%S")
set(FIXTURE_DIR "/tmp/spring-version-fallback-${TEST_STAMP}-${CMAKE_HOST_SYSTEM_PROCESSOR}")
set(OUTPUT_DIR "${FIXTURE_DIR}/generated")
set(GIT_WRAPPER "${FIXTURE_DIR}/git-wrapper")

file(MAKE_DIRECTORY "${FIXTURE_DIR}/rts/System")
file(COPY
	"${SOURCE_DIR}/VERSION.template"
	DESTINATION "${FIXTURE_DIR}"
)
file(COPY
	"${SOURCE_DIR}/rts/System/VersionGenerated.h.template"
	DESTINATION "${FIXTURE_DIR}/rts/System"
)
file(WRITE "${FIXTURE_DIR}/marker" "tagless version fixture\n")

execute_process(
	COMMAND git init --initial-branch=main
	WORKING_DIRECTORY "${FIXTURE_DIR}"
	RESULT_VARIABLE result
	OUTPUT_VARIABLE output
	ERROR_VARIABLE error
)
if (result)
	message(FATAL_ERROR "git init failed: ${output}${error}")
endif()

execute_process(
	COMMAND git -c user.name=version-test -c user.email=version-test@example.invalid add marker
	WORKING_DIRECTORY "${FIXTURE_DIR}"
	RESULT_VARIABLE result
	OUTPUT_VARIABLE output
	ERROR_VARIABLE error
)
if (result)
	message(FATAL_ERROR "git add failed: ${output}${error}")
endif()

execute_process(
	COMMAND "${CMAKE_COMMAND}" -E env
		GIT_AUTHOR_DATE=2024-01-02T03:04:05+0000
		GIT_COMMITTER_DATE=2024-01-02T03:04:05+0000
		git -c user.name=version-test -c user.email=version-test@example.invalid commit -m tagless-version
	WORKING_DIRECTORY "${FIXTURE_DIR}"
	RESULT_VARIABLE result
	OUTPUT_VARIABLE output
	ERROR_VARIABLE error
)
if (result)
	message(FATAL_ERROR "git commit failed: ${output}${error}")
endif()

# The portable-date candidate must work with a Git shim that rejects the
# non-portable --format=%cs spelling used by its parent.
file(WRITE "${GIT_WRAPPER}" "#!/bin/sh\ncase \"$*\" in\n  *--format=%cs*) exit 77 ;;\nesac\nexec /usr/bin/git \"$@\"\n")
file(CHMOD "${GIT_WRAPPER}"
	PERMISSIONS
	OWNER_READ OWNER_WRITE OWNER_EXECUTE
	GROUP_READ GROUP_EXECUTE
	WORLD_READ WORLD_EXECUTE
)

execute_process(
	COMMAND "${CMAKE_COMMAND}"
		"-DSOURCE_ROOT=${FIXTURE_DIR}"
		"-DCMAKE_MODULES_SPRING=${SOURCE_DIR}/rts/build/cmake"
		"-DGENERATE_DIR=${OUTPUT_DIR}"
		"-DVERSION_ADDITIONAL=version-test"
		"-DGIT_EXECUTABLE=${GIT_WRAPPER}"
		-P "${SOURCE_DIR}/rts/build/cmake/ConfigureVersion.cmake"
	RESULT_VARIABLE result
	OUTPUT_VARIABLE output
	ERROR_VARIABLE error
)
if (result)
	file(REMOVE_RECURSE "${FIXTURE_DIR}")
	message(FATAL_ERROR "tagless version generation failed: ${output}${error}")
endif()

file(READ "${OUTPUT_DIR}/VERSION" generated_version)
string(STRIP "${generated_version}" generated_version)
if (NOT generated_version MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+-0-g[0-9a-f]+ main$")
	file(REMOVE_RECURSE "${FIXTURE_DIR}")
	message(FATAL_ERROR "unexpected generated version: '${generated_version}'")
endif()

message(STATUS "tagless version: ${generated_version}")
file(REMOVE_RECURSE "${FIXTURE_DIR}")
