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
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)

string(TIMESTAMP TEST_STAMP "%Y%m%d%H%M%S")
set(FIXTURE_DIR "/tmp/spring-cmake-no-sync-${TEST_STAMP}-${CMAKE_HOST_SYSTEM_PROCESSOR}")

execute_process(
	COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${FIXTURE_DIR}"
		${COMMON_ARGS}
		-DSYNCCHECK=OFF
		-DDEBUG_SIGNAL_NANS=ON
	RESULT_VARIABLE result
	OUTPUT_VARIABLE output
	ERROR_VARIABLE error
)
if (result)
	file(REMOVE_RECURSE "${FIXTURE_DIR}")
	message(FATAL_ERROR "no-sync-check configure failed: ${output}${error}")
endif()

file(STRINGS "${FIXTURE_DIR}/CMakeCache.txt" cache_entries)
string(JOIN "\n" cache_text ${cache_entries})
foreach (expected IN ITEMS
	"SYNCCHECK:BOOL=OFF"
	"SYNC_HISTORY:BOOL=OFF"
	"DEBUG_SIGNAL_NANS:BOOL=OFF"
)
	string(FIND "${cache_text}" "${expected}" cache_offset)
	if (cache_offset EQUAL -1)
		file(REMOVE_RECURSE "${FIXTURE_DIR}")
		message(FATAL_ERROR "missing no-sync-check cache contract: ${expected}")
	endif()
endforeach()

file(READ "${FIXTURE_DIR}/compile_commands.json" compile_commands)
if (compile_commands MATCHES "-fsignaling-nans")
	file(REMOVE_RECURSE "${FIXTURE_DIR}")
	message(FATAL_ERROR "no-sync-check compile commands still enable signaling NaNs")
endif()

message(STATUS "no-sync-check disables sync history and signaling-NaN compiler flags")
file(REMOVE_RECURSE "${FIXTURE_DIR}")
