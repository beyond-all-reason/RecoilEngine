# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

if (NOT DEFINED SOURCE_DIR)
	get_filename_component(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

set(COMMON_ARGS
	-DBUILD_spring-legacy=OFF
	-DBUILD_spring-dedicated=OFF
	-DBUILD_spring-headless=OFF
	-DNO_SOUND=ON
	-DINSTALL_PORTABLE=OFF
	-DUSE_MIMALLOC=OFF
	-DBUILD_SHARED_LIBS=ON
)

string(TIMESTAMP TEST_STAMP "%Y%m%d%H%M%S")
set(FIXTURE_DIR "/tmp/spring-cmake-duplicate-tests-${TEST_STAMP}-${CMAKE_HOST_SYSTEM_PROCESSOR}")

execute_process(
	COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${FIXTURE_DIR}"
		${COMMON_ARGS} -DBUILD_TESTING=ON
	RESULT_VARIABLE result
	OUTPUT_VARIABLE output
	ERROR_VARIABLE error
)
if (result)
	file(REMOVE_RECURSE "${FIXTURE_DIR}")
	message(FATAL_ERROR "explicit BUILD_TESTING=ON configure failed: ${output}${error}")
endif()

# The dependency's aggregate target and the engine's test target must remain
# visible after the duplicate-target guard resolves their shared name.
execute_process(
	COMMAND "${CMAKE_COMMAND}" --build "${FIXTURE_DIR}" --target help
	RESULT_VARIABLE result
	OUTPUT_VARIABLE output
	ERROR_VARIABLE error
)
if (result)
	file(REMOVE_RECURSE "${FIXTURE_DIR}")
	message(FATAL_ERROR "the tests target listing failed: ${output}${error}")
endif()
string(FIND "${output}" "tests: phony" tests_target)
if (tests_target EQUAL -1)
	string(FIND "${output}" "... tests" tests_target)
endif()
if (tests_target EQUAL -1)
	file(REMOVE_RECURSE "${FIXTURE_DIR}")
	message(FATAL_ERROR "the tests aggregate target is not visible")
endif()

message(STATUS "explicit BUILD_TESTING=ON configures with the shared tests target")
file(REMOVE_RECURSE "${FIXTURE_DIR}")
