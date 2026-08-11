/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
std::string readFile(const char* path)
{
	std::ifstream file(path);
	REQUIRE(file.good());

	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str();
}

std::string extractFunction(const std::string& source, const char* functionStart)
{
	const auto start = source.find(functionStart);
	REQUIRE(start != std::string::npos);

	const auto end = source.find("\nend", start);
	REQUIRE(end != std::string::npos);
	return source.substr(start, end + 4 - start);
}

std::string makeTestChunk(
	const std::string& function,
	const char* callIn,
	const char* arguments,
	const char* expectedFirst,
	const char* expectedSecond,
	const char* expectedThird
)
{
	std::string chunk = R"lua(
local function r_ipairs(tbl)
  local function r_iter(tbl, key)
    if (key <= 1) then
      return nil
    end
    return (key - 1), tbl[key - 1]
  end
  return r_iter, tbl, (1 + #tbl)
end

local gadgetHandler = {
  )lua";
	chunk += callIn;
	chunk += R"lua(List = {},
}

)lua";
	chunk += function;
	chunk += "\n\nlocal received\ntable.insert(gadgetHandler.";
	chunk += callIn;
	chunk += R"lua(List, {
  )lua";
	chunk += callIn;
	chunk += R"lua( = function(_, ...)
    received = {...}
  end,
})

)lua";
	chunk += "gadgetHandler:";
	chunk += callIn;
	chunk += "(";
	chunk += arguments;
	chunk += ")\nassert(received ~= nil)\nassert(#received == 3)\nassert(received[1] == ";
	chunk += expectedFirst;
	chunk += ")\nassert(received[2] == ";
	chunk += expectedSecond;
	chunk += ")\nassert(received[3] == ";
	chunk += expectedThird;
	chunk += ")\n";
	return chunk;
}

void runChunk(const std::string& chunk, const char* name)
{
	lua_State* lua = luaL_newstate();
	REQUIRE(lua != nullptr);
	luaL_openlibs(lua);

	const auto loadResult = luaL_loadbuffer(lua, chunk.data(), chunk.size(), name);
	REQUIRE(loadResult == 0);

	const auto callResult = lua_pcall(lua, 0, 0, 0);
	if (callResult != 0)
		INFO(lua_tostring(lua, -1));
	CHECK(callResult == 0);

	lua_close(lua);
}

const std::string& gadgetSource()
{
	static const std::string source = [] {
		const char* sourcePath = std::getenv("SPRING_TEST_LUA_GADGETS_FILE");
		return readFile(sourcePath != nullptr ? sourcePath : LUA_GADGETS_SOURCE_FILE);
	}();
	return source;
}
} // namespace

TEST_CASE("LuaGadgets forwards ProjectileDestroyed arguments")
{
	const auto chunk = makeTestChunk(
		extractFunction(gadgetSource(), "function gadgetHandler:ProjectileDestroyed("),
		"ProjectileDestroyed",
		"101, 202, 303",
		"101",
		"202",
		"303"
	);
	runChunk(chunk, "ProjectileDestroyed test");
}

TEST_CASE("LuaGadgets dispatches UnitMoveFailed arguments")
{
	const auto chunk = makeTestChunk(
		extractFunction(gadgetSource(), "function gadgetHandler:UnitMoveFailed("),
		"UnitMoveFailed",
		"401, 402, 403",
		"401",
		"402",
		"403"
	);
	runChunk(chunk, "UnitMoveFailed test");
}
