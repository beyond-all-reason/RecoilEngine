/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_ENCODING_H
#define LUA_ENCODING_H

struct lua_State;

class LuaEncoding {
	public:
		static bool PushEntries(lua_State* L);
	private:
		static int EncodeBase64(lua_State* L);
		static int DecodeBase64(lua_State* L);
		static int IsValidBase64(lua_State* L);

		static int EncodeBase64Url(lua_State* L);
		static int DecodeBase64Url(lua_State* L);
		static int IsValidBase64Url(lua_State* L);
};


#endif /* LUA_ENCODING_H */
