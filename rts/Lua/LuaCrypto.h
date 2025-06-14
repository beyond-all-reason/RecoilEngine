/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CRYPTO_H
#define LUA_CRYPTO_H

#include <memory>
#include <string>

struct lua_State;


class LuaCrypto {
	public:
		LuaCrypto() { }

		static bool PushEntries(lua_State* L);
	public:
		static int TestEcdsa(lua_State* L);
		static int TestEcdh(lua_State* L);
		static int TestAes(lua_State* L);
		static int TestTinyAes(lua_State* L);
};

#endif /* LUA_CRYPTO_H */
