/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaEncoding.h"

#include "base64.h"
#include "Lua/LuaUtils.h"


// For validation
static const std::string base64Chars =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/=";

static const std::string base64UrlChars =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789-_";


/***
 * Lua Encoding API
 * @table Encoding
 */

bool LuaEncoding::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(EncodeBase64);
	REGISTER_LUA_CFUNC(DecodeBase64);
	REGISTER_LUA_CFUNC(IsValidBase64);

	REGISTER_LUA_CFUNC(EncodeBase64Url);
	REGISTER_LUA_CFUNC(DecodeBase64Url);
	REGISTER_LUA_CFUNC(IsValidBase64Url);

	return true;
}


/*** Decodes a base64 string
 *
 * @function Encoding.DecodeBase64
 *
 * Decodes a base64 encoded string. If it encounters
 * invalid data, it stops and returns whatever it parsed so far.
 *
 * @param text string Text to decode
 * @return string decoded Decoded text
 */
int LuaEncoding::DecodeBase64(lua_State* L)
{
	const char* text = luaL_checkstring(L, 1);
	const std::string decoded = base64_decode(text);

	lua_pushsstring(L, decoded);
	return 1;
}


/*** Encodes a base64 string
 *
 * @function Encoding.EncodeBase64
 *
 * @param text string Text to encode
 * @param stripPadding? boolean Remove padding (`=` characters) at the end when 'true'. Defaults to `true`.
 * @return string encoded Encoded text
 */
int LuaEncoding::EncodeBase64(lua_State* L)
{
	size_t textLen;
	// querying with checklstring since there could be embedded '\0' characters.
	const char* text = luaL_checklstring(L, 1, &textLen);
	std::string encoded = base64_encode(reinterpret_cast<const uint8_t*>(text), textLen);

	if (luaL_optboolean(L, 2, true)) {
		encoded.erase(encoded.find_last_not_of("=") + 1);
	}

	lua_pushsstring(L, encoded);
	return 1;
}


/*** Validates a base64 string
 *
 * @function Encoding.IsValidBase64
 *
 * @param text string Text to validate
 * @return boolean valid Whether the text is valid base64
 */
int LuaEncoding::IsValidBase64(lua_State* L)
{
	const std::string text = luaL_checkstring(L, 1);

	bool valid = text.find_first_not_of(base64Chars) == std::string::npos;

	if (valid) {
		size_t firstPadding = text.find_first_of("=");
		if (firstPadding != std::string::npos)
			valid = firstPadding == (text.find_last_not_of("=") + 1);
	}

	lua_pushboolean(L, valid);
	return 1;
}


/*** Decodes a base64url string
 *
 * @function Encoding.DecodeBase64Url
 *
 * Decodes a base64url encoded string. If it encounters
 * invalid data, it stops and returns whatever it parsed so far.
 *
 * @param text string Text to decode
 * @return string decoded Decoded text
 */
int LuaEncoding::DecodeBase64Url(lua_State* L)
{
	std::string text = luaL_checkstring(L, 1);
	for (char& c: text) {
		switch (c)
		{
			case '-': c = '+'; break;
			case '_': c = '/'; break;
		}
	}
	const std::string decoded = base64_decode(text);

	lua_pushsstring(L, decoded);
	return 1;
}


/*** Encodes a base64url string
 *
 * @function Encoding.EncodeBase64Url
 *
 * @param text string Text to encode
 * @return string encoded Encoded text
 */
int LuaEncoding::EncodeBase64Url(lua_State* L)
{
	size_t textLen;
	const char* text = luaL_checklstring(L, 1, &textLen);
	std::string encoded = base64_encode(reinterpret_cast<const uint8_t*>(text), textLen);

	encoded.erase(encoded.find_last_not_of("=") + 1);

	for (char& c: encoded) {
		switch (c)
		{
			case '+': c = '-'; break;
			case '/': c = '_'; break;
		}
	}

	lua_pushsstring(L, encoded);
	return 1;
}


/*** Validates a base64url string
 *
 * @function Encoding.IsValidBase64Url
 *
 * @param text string Text to validate
 * @return boolean valid Whether the text is valid base64url
 */
int LuaEncoding::IsValidBase64Url(lua_State* L)
{
	const std::string text = luaL_checkstring(L, 1);

	bool valid = text.find_first_not_of(base64UrlChars) == std::string::npos;

	lua_pushboolean(L, valid);
	return 1;
}

