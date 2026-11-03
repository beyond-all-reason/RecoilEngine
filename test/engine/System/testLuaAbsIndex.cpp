/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaInclude.h"

#include <catch_amalgamated.hpp>

#include <climits>
#include <cstdlib>


static void* Alloc(void*, void* ptr, size_t, size_t nsize)
{
	if (nsize == 0) {
		free(ptr);
		return nullptr;
	}
	return realloc(ptr, nsize);
}

struct State {
	lua_State* L = lua_newstate(Alloc, nullptr);
	~State() { lua_close(L); }
};

// Number of ways lua_absindex disagrees with the expected results for the current frame:
// a positive index is itself, "-k" is top - k + 1 and names the same slot, a pseudo-index is returned unchanged.
static int CountMismatches(lua_State* L)
{
	const int top = lua_gettop(L);
	int bad = 0;

	for (int k = 1; k <= top; k++) {
		bad += (lua_absindex(L, k) != k);
		bad += (lua_absindex(L, -k) != top - k + 1);
		bad += (lua_rawequal(L, -k, top - k + 1) != 1);
	}
	for (const int pseudo: {LUA_REGISTRYINDEX, LUA_ENVIRONINDEX, LUA_GLOBALSINDEX, lua_upvalueindex(1), lua_upvalueindex(255), INT_MIN})
		bad += (lua_absindex(L, pseudo) != pseudo);

	return bad;
}

// Catch's REQUIRE throws, and Lua would swallow that inside a C function, so the frame reports through these.
static int frameTop = -1;
static int frameMismatches = -1;

static int CheckFrame(lua_State* L)
{
	frameTop = lua_gettop(L);
	frameMismatches = CountMismatches(L);
	frameMismatches += (lua_tointeger(L, lua_absindex(L, lua_upvalueindex(2))) != 22);

	lua_checkstack(L, 4);
	for (int i = 0; i < 3; i++)
		lua_pushinteger(L, i);

	frameMismatches += CountMismatches(L);
	return 0;
}

static void PushCheckFrame(lua_State* L)
{
	lua_pushinteger(L, 11);
	lua_pushinteger(L, 22);
	lua_pushcclosure(L, CheckFrame, 2);
}

TEST_CASE("LuaAbsIndex")
{
	State s;
	lua_State* L = s.L;
	REQUIRE(L != nullptr);

	SECTION("relative, absolute and pseudo-indices") {
		for (int i = 0; i < 8; i++)
			lua_pushinteger(L, i);

		REQUIRE(CountMismatches(L) == 0);
		REQUIRE(lua_absindex(L, INT_MAX) == INT_MAX);
	}

	SECTION("an absolute index keeps its value while more is pushed") {
		lua_newtable(L);
		const int table = lua_absindex(L, -1);

		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);

		REQUIRE(lua_istable(L, table));
		REQUIRE(lua_absindex(L, -1) == table + 3);
	}

	SECTION("inside a C function with arguments and upvalues") {
		PushCheckFrame(L);
		for (int i = 0; i < 5; i++)
			lua_pushinteger(L, 100 + i);
		lua_call(L, 5, 0);

		REQUIRE(frameTop == 5);
		REQUIRE(frameMismatches == 0);
	}

	SECTION("in a coroutine") {
		lua_State* co = lua_newthread(L);
		for (int i = 0; i < 4; i++)
			lua_pushinteger(co, i);
		REQUIRE(CountMismatches(co) == 0);

		lua_settop(co, 0);
		PushCheckFrame(co);
		lua_pushinteger(co, 1);
		lua_pushinteger(co, 2);
		REQUIRE(lua_resume(co, 2) == 0);

		REQUIRE(frameTop == 2);
		REQUIRE(frameMismatches == 0);
	}

	SECTION("after the stack has been reallocated") {
		REQUIRE(lua_checkstack(L, 5000) != 0);
		for (int i = 0; i < 4000; i++)
			lua_pushinteger(L, i);

		REQUIRE(CountMismatches(L) == 0);
		REQUIRE(lua_absindex(L, -4000) == 1);
	}
}
