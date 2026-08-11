# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

if (NOT DEFINED SOURCE_DIR)
	get_filename_component(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

file(READ "${SOURCE_DIR}/rts/CMakeLists.txt" rts_cmake)

foreach (required_text IN ITEMS
	"option(ASAN_PREFER_SHARED_LIBS"
	"NOT USE_ASAN OR NOT ASAN_PREFER_SHARED_LIBS"
)
	string(FIND "${rts_cmake}" "${required_text}" offset)
	if (offset EQUAL -1)
		message(FATAL_ERROR "ASAN library preference contract is missing: ${required_text}")
	endif()
endforeach()

message(STATUS "ASAN library preference is configurable")
