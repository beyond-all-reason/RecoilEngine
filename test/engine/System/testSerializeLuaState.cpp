/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/creg/Serializer.h"
#include "System/creg/SerializeLuaState.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include <catch_amalgamated.hpp>


static int handlepanic(lua_State* L)
{
	throw "lua paniced";
}


// from lauxlib.cpp
static void* l_alloc (void* ud, void* ptr, size_t osize, size_t nsize) {
	(void)ud;
	(void)osize;
	if (nsize == 0) {
		free(ptr);
		return NULL;
	} else {
		return realloc(ptr, nsize);
	}
}

struct FakeLuaHandle {
	lua_State* L = nullptr;
	lua_State* L_GC = nullptr;
};

FakeLuaHandle flh;


struct LuaRoot {
	CR_DECLARE_STRUCT(LuaRoot);
	void Serialize(creg::ISerializer* s);
};

CR_BIND(LuaRoot, );
CR_REG_METADATA(LuaRoot, (
	CR_SERIALIZER(Serialize)
))


void LuaRoot::Serialize(creg::ISerializer* s) {
	creg::SerializeLuaState(s, &flh.L);
	creg::SerializeLuaThread(s, &flh.L_GC);
}

TEST_CASE("SerializeLuaState")
{
	int context = 1;


	flh.L = lua_newstate(l_alloc, &context);
	lua_atpanic(flh.L, handlepanic);
	SPRING_LUA_OPEN_LIB(flh.L, luaopen_base);
	SPRING_LUA_OPEN_LIB(flh.L, luaopen_math);
	SPRING_LUA_OPEN_LIB(flh.L, luaopen_table);
	SPRING_LUA_OPEN_LIB(flh.L, luaopen_string);

	lua_settop(flh.L, 0);
	creg::AutoRegisterCFunctions("Test::", flh.L);
	flh.L_GC = lua_newthread(flh.L);
	int idx = luaL_ref(flh.L, LUA_REGISTRYINDEX);
	const char* code = "local co = coroutine.create(function ()\n local function f()\n coroutine.yield()\n end\n f()\n end);\ncoroutine.resume(co);\n";

	int err = luaL_loadbuffer(flh.L, code, strlen(code), "yield");
	if (err)
	{
		printf("%s\n", lua_tostring(flh.L, -1));
		lua_pop(flh.L, 1);
	}
	lua_pcall(flh.L, 0, 0, 0);

	std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
	{
		LuaRoot root;
		creg::COutputStreamSerializer oser;
		oser.SavePackage(&ss, &root, root.GetClass());
	}

	{
		creg::CInputStreamSerializer iser;
		void* loaded;
		creg::Class* loadedCls;
		creg::CopyLuaContext(flh.L);
		LUA_CLOSE(&flh.L);
		iser.LoadPackage(&ss, loaded, loadedCls);
		LuaRoot* loadedRoot = (LuaRoot*) loaded;
		delete loadedRoot;
	}

	lua_rawgeti(flh.L, LUA_REGISTRYINDEX, idx);
	lua_State* L_GC = lua_tothread(flh.L, -1);
	CHECK(L_GC == flh.L_GC);

	lua_close(flh.L);
}


// Like the engine's smmalloc pool: a freed block stays mapped and its first
// eight bytes become a free-list link, which is not a pointer.
static std::vector<void*> freedBlocks;

static void* l_alloc_keep_freed(void* ud, void* ptr, size_t osize, size_t nsize)
{
	(void)ud;
	void* mem = nullptr;
	if (nsize > 0) {
		mem = malloc(nsize);
		if (ptr != nullptr)
			memcpy(mem, ptr, std::min(osize, nsize));
	}
	if (ptr != nullptr) {
		const std::uint64_t link = ~std::uint64_t(0);
		memcpy(ptr, &link, std::min(osize, sizeof(link)));
		freedBlocks.push_back(ptr);
	}
	return mem;
}

TEST_CASE("SerializeLuaStateWeakTableRemovedKeys")
{
	int context = 1;

	flh.L = lua_newstate(l_alloc_keep_freed, &context);
	lua_atpanic(flh.L, handlepanic);
	SPRING_LUA_OPEN_LIB(flh.L, luaopen_base);
	SPRING_LUA_OPEN_LIB(flh.L, luaopen_table);

	lua_settop(flh.L, 0);
	creg::AutoRegisterCFunctions("Test::", flh.L);
	flh.L_GC = lua_newthread(flh.L);
	luaL_ref(flh.L, LUA_REGISTRYINDEX);

	// A coroutine registry like the one of a Lua unit script gadget: live
	// threads stay referenced, finished ones are removed by assigning nil.
	const char* code = R"(
threads = setmetatable({}, {__mode = "kv"})
sleepers = {}
local function Start(long)
	local co = coroutine.create(function() coroutine.yield() if long then coroutine.yield() end end)
	threads[co] = {}
	coroutine.resume(co)
	if long then sleepers[#sleepers + 1] = {co, threads[co]} end
	return co
end
for i = 1, 8 do Start(true) end
local finished = {}
for i = 1, 6 do finished[i] = Start(false) end
for i = 1, 6 do
	coroutine.resume(finished[i])
	threads[finished[i]] = nil
end
)";
	REQUIRE(luaL_loadbuffer(flh.L, code, strlen(code), "threads") == 0);
	REQUIRE(lua_pcall(flh.L, 0, 0, 0) == 0);

	// as CLuaStateCollector::Read; this frees the finished threads but leaves their keys
	lua_gc(flh.L_GC, LUA_GCCOLLECT, 0);

	std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
	{
		LuaRoot root;
		creg::COutputStreamSerializer oser;
		oser.SavePackage(&ss, &root, root.GetClass());
	}

	{
		creg::CInputStreamSerializer iser;
		void* loaded;
		creg::Class* loadedCls;
		creg::CopyLuaContext(flh.L);
		LUA_CLOSE(&flh.L);
		iser.LoadPackage(&ss, loaded, loadedCls);
		LuaRoot* loadedRoot = (LuaRoot*) loaded;
		delete loadedRoot;
	}

	const char* check = "local n = 0 for co in pairs(threads) do n = n + 1 end return n";
	REQUIRE(luaL_loadbuffer(flh.L, check, strlen(check), "check") == 0);
	REQUIRE(lua_pcall(flh.L, 0, 1, 0) == 0);
	CHECK(lua_tonumber(flh.L, -1) == 8);

	lua_close(flh.L);
	for (void* p: freedBlocks) {
		free(p);
	}
	freedBlocks.clear();
}