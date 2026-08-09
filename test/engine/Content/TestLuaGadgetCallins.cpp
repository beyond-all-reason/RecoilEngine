/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>

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

std::string projectileDestroyedFunction(const std::string& source)
{
	constexpr const char* functionStart = "function gadgetHandler:ProjectileDestroyed(";
	const auto start = source.find(functionStart);
	REQUIRE(start != std::string::npos);

	const auto end = source.find("\nend", start);
	REQUIRE(end != std::string::npos);
	return source.substr(start, end + 4 - start);
}

std::string makeTestChunk(const std::string& function)
{
	return R"lua(
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
  ProjectileDestroyedList = {},
}

)lua" + function + R"lua(

local received
table.insert(gadgetHandler.ProjectileDestroyedList, {
  ProjectileDestroyed = function(_, ...)
    received = {...}
  end,
})

gadgetHandler:ProjectileDestroyed(101, 202, 303)
assert(received ~= nil)
assert(#received == 3)
assert(received[1] == 101)
assert(received[2] == 202)
assert(received[3] == 303)
)lua";
}

} // namespace

TEST_CASE("LuaGadgets forwards ProjectileDestroyed arguments")
{
	const char* sourcePath = std::getenv("SPRING_TEST_LUA_GADGETS_FILE");
	const auto source = readFile(sourcePath != nullptr ? sourcePath : LUA_GADGETS_SOURCE_FILE);
	const auto chunk = makeTestChunk(projectileDestroyedFunction(source));

	lua_State* lua = luaL_newstate();
	REQUIRE(lua != nullptr);
	luaL_openlibs(lua);

	const auto loadResult = luaL_loadbuffer(lua, chunk.data(), chunk.size(), "ProjectileDestroyed test");
	REQUIRE(loadResult == 0);

	const auto callResult = lua_pcall(lua, 0, 0, 0);
	if (callResult != 0)
		INFO(lua_tostring(lua, -1));
	CHECK(callResult == 0);

	lua_close(lua);
}
