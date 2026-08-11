# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

if (NOT DEFINED SOURCE_DIR)
	get_filename_component(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

function(require_text content needle description)
	string(FIND "${content}" "${needle}" offset)
	if (offset EQUAL -1)
		message(FATAL_ERROR "${description} is missing: ${needle}")
	endif()
endfunction()

file(READ "${SOURCE_DIR}/.github/workflows/release-build.yml" release_workflow)

set(default_targets "[\"amd64-linux\",\"arm64-linux\",\"amd64-windows\"]")
require_text(
	"${release_workflow}"
	"default: '${default_targets}'"
	"release workflow default target list"
)
require_text(
	"${release_workflow}"
	"inputs.targets"
	"release workflow target input"
)
require_text(
	"${release_workflow}"
	"build-engine-tracy"
	"Tracy release job"
)
require_text(
	"${release_workflow}"
	"inputs.build_tracy"
	"Tracy release switch"
)
require_text(
	"${release_workflow}"
	"default: true"
	"Tracy release default"
)

set(removed_target_list "[\"amd64-linux\",\"amd64-windows\"]")
string(FIND "${release_workflow}" "${removed_target_list}" removed_target_offset)
if (NOT removed_target_offset EQUAL -1)
	message(FATAL_ERROR "release workflow still hardcodes the reduced target list")
endif()

message(STATUS "release workflow preserves upstream defaults and supports dispatch overrides")
