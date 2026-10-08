# Spring supplied CMake build file

# deactivate signaling-NANs for this library
remove_flag("-fsignaling-nans")

INCLUDE_DIRECTORIES(BEFORE ../streflop include)

SET(luaSources
		"src/lapi.cpp"
		"src/lauxlib.cpp"
		"src/lbaselib.cpp"
		"src/lcode.cpp"
		"src/ldblib.cpp"
		"src/ldebug.cpp"
		"src/ldo.cpp"
		"src/ldump.cpp"
		"src/lfunc.cpp"
		"src/lgc.cpp"
		"src/linit.cpp"
		"src/liolib.cpp"
		"src/llex.cpp"
		"src/lmathlib.cpp"
		"src/lmem.cpp"
		"src/loadlib.cpp"
		"src/lobject.cpp"
		"src/lopcodes.cpp"
		"src/loslib.cpp"
		"src/lparser.cpp"
		"src/lstate.cpp"
		"src/lstring.cpp"
		"src/lstrlib.cpp"
		"src/ltable.cpp"
		"src/ltablib.cpp"
		"src/ltm.cpp"
		"src/lundump.cpp"
		"src/lvm.cpp"
		"src/lzio.cpp"
		"src/print.cpp"
		"src/lua_privileges.cpp"
		"include/LuaUser.cpp"
	)

ADD_LIBRARY(lua STATIC EXCLUDE_FROM_ALL ${luaSources})

if (ENABLE_STREFLOP)
TARGET_LINK_LIBRARIES(lua streflop)
else ()
TARGET_LINK_LIBRARIES(lua)
endif ()

target_include_directories(lua PUBLIC include)

if (UNIX)
	SET_TARGET_PROPERTIES(lua PROPERTIES COMPILE_FLAGS "-DLUA_USE_MKSTEMP ${PIC_FLAG}")
else (UNIX)
	SET_TARGET_PROPERTIES(lua PROPERTIES COMPILE_FLAGS "${PIC_FLAG}")
endif (UNIX)

# lvm.cpp dispatches opcodes with computed gotos; without this GCC's
# cross-jumping merges the per-opcode fetch+dispatch tails back into a single
# indirect jump. Only affects code layout (no effect on FP or semantics).
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set_source_files_properties(src/lvm.cpp PROPERTIES COMPILE_OPTIONS "-fno-crossjumping")
endif()

# Vendored lua has an intentional empty-body loop (lauxlib.cpp shebang skip).
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(lua PRIVATE
		-Wno-empty-body
	)
endif()
