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
set(FIXTURE_DIR "/tmp/spring-cmake-testing-option-${TEST_STAMP}-${CMAKE_HOST_SYSTEM_PROCESSOR}")
set(OFF_BUILD "${FIXTURE_DIR}/testing-off")
set(DEFAULT_BUILD "${FIXTURE_DIR}/testing-default")

execute_process(
	COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${OFF_BUILD}"
		${COMMON_ARGS} -DBUILD_TESTING=OFF
	RESULT_VARIABLE result
	OUTPUT_VARIABLE output
	ERROR_VARIABLE error
)
if (result)
	file(REMOVE_RECURSE "${FIXTURE_DIR}")
	message(FATAL_ERROR "BUILD_TESTING=OFF configure failed: ${output}${error}")
endif()

# The optional configuration must not expose the aggregate test target.
execute_process(
	COMMAND "${CMAKE_COMMAND}" --build "${OFF_BUILD}" --target help
	RESULT_VARIABLE result
	OUTPUT_VARIABLE output
	ERROR_VARIABLE error
)
if (result)
	file(REMOVE_RECURSE "${FIXTURE_DIR}")
	message(FATAL_ERROR "BUILD_TESTING=OFF target listing failed: ${output}${error}")
endif()
string(FIND "${output}" "tests: phony" off_target)
if (off_target EQUAL -1)
	string(FIND "${output}" "... tests" off_target)
endif()
if (NOT off_target EQUAL -1)
	file(REMOVE_RECURSE "${FIXTURE_DIR}")
	message(FATAL_ERROR "BUILD_TESTING=OFF unexpectedly exposed the tests target")
endif()

# The normal configuration must still expose the aggregate target.
execute_process(
	COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${DEFAULT_BUILD}"
		${COMMON_ARGS}
	RESULT_VARIABLE result
	OUTPUT_VARIABLE output
	ERROR_VARIABLE error
)
if (result)
	file(REMOVE_RECURSE "${FIXTURE_DIR}")
	message(FATAL_ERROR "default test configure failed: ${output}${error}")
endif()

execute_process(
	COMMAND "${CMAKE_COMMAND}" --build "${DEFAULT_BUILD}" --target help
	RESULT_VARIABLE result
	OUTPUT_VARIABLE output
	ERROR_VARIABLE error
)
if (result)
	file(REMOVE_RECURSE "${FIXTURE_DIR}")
	message(FATAL_ERROR "default test target listing failed: ${output}${error}")
endif()
string(FIND "${output}" "tests: phony" default_target)
if (default_target EQUAL -1)
	string(FIND "${output}" "... tests" default_target)
endif()
if (default_target EQUAL -1)
	file(REMOVE_RECURSE "${FIXTURE_DIR}")
	message(FATAL_ERROR "default configuration does not expose the tests target")
endif()

message(STATUS "BUILD_TESTING=OFF omits tests while the default configuration retains it")
file(REMOVE_RECURSE "${FIXTURE_DIR}")
