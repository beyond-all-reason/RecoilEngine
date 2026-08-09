# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

if (NOT DEFINED SOURCE_DIR)
	get_filename_component(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

file(READ "${SOURCE_DIR}/CMakePresets.json" presets)
string(FIND "${presets}" "\"DEBUG_SIGNAL_NANS\"" signal_nans_offset)
if (NOT signal_nans_offset EQUAL -1)
	message(FATAL_ERROR "debug preset overrides DEBUG_SIGNAL_NANS instead of inheriting the engine default")
endif()

execute_process(
	COMMAND "${CMAKE_COMMAND}" --list-presets=all -S "${SOURCE_DIR}"
	RESULT_VARIABLE result
	OUTPUT_VARIABLE output
	ERROR_VARIABLE error
)
if (result)
	message(FATAL_ERROR "CMake preset discovery failed: ${output}${error}")
endif()

foreach (preset IN ITEMS debug release release-debuginfo release-debuginfo-ccache)
	string(FIND "${output}" "${preset}" preset_offset)
	if (preset_offset EQUAL -1)
		message(FATAL_ERROR "Required CMake preset is missing: ${preset}")
	endif()
endforeach()

message(STATUS "all four standard CMake presets are discoverable")
