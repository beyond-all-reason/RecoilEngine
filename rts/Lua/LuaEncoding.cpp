/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaEncoding.h"

#include "base64.h"
#include "Lua/LuaUtils.h"


bool LuaEncoding::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(EncodeBase64);
	REGISTER_LUA_CFUNC(DecodeBase64);

	return true;
}


/*** Decodes a base64 string
 *
 * @function Spring.DecodeBase64
 *
 * @param string Text to decode
 * @return string Decoded text
 */
int LuaEncoding::DecodeBase64(lua_State* L)
{
	const char* text = luaL_checkstring(L, 1);
	const std::string res = base64_decode(text);

	lua_pushsstring(L, res);
	return 1;
}


/*** Encodes a base64 string
 *
 * @function Spring.EncodeBase64
 *
 * @param string Text to encode
 * @return string Encoded text
 */
int LuaEncoding::EncodeBase64(lua_State* L)
{
	const std::string text = luaL_checkstring(L, 1);
	const std::string res = base64_encode(reinterpret_cast<const uint8_t*>(text.c_str()), text.size());
	lua_pushsstring(L, res);
	return 1;
}

