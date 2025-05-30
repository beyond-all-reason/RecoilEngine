/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaImage.h"

#include "LuaInclude.h"
#include "LuaUtils.h"

#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"
#include "Map/ReadMap.h"

#include "System/Misc/TracyDefs.h"


LuaImage luaImage;


/******************************************************************************
 * LuaImageData
 ******************************************************************************/

LuaImageData::LuaImageData(std::string filename, bool grayscale, int reqChannels, int reqDataType) : filename(filename)
{
	id     = 0;
	bitmap = std::make_shared<CBitmap>();
	if (grayscale)
		valid = bitmap->LoadGrayscale(filename);
	else
		valid = bitmap->Load(filename, 1.0f, reqChannels, reqDataType);
	if (bitmap->dataType != GL_UNSIGNED_BYTE && bitmap->dataType != GL_UNSIGNED_SHORT && bitmap->dataType != GL_FLOAT)
		valid = false;
	if (valid) {
		width = bitmap->xsize;
		height = bitmap->ysize;
		channels = bitmap->channels;
	}
}


LuaImageData::~LuaImageData()
{
	// delete bitmap from array
	id = 0;
}


/******************************************************************************
 * LuaImage
 ******************************************************************************/

LuaImage::~LuaImage()
{
	// TODO clean up
}


bool LuaImage::Init(lua_State* L)
{
	lastIndex = 0;
	images.clear();
	return true;
}


void LuaImage::Shutdown(lua_State* L)
{
	lastIndex = 0;
	images.clear();
}


bool LuaImage::PushEntries(lua_State* L)
{
	CreateMetatable(L);

	REGISTER_LUA_CFUNC(LoadImage);
	REGISTER_LUA_CFUNC(DeleteImage);

	return true;
}

bool LuaImage::CreateMetatable(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	luaL_newmetatable(L, "Image");

	HSTR_PUSH_CFUNC(L, "__gc",        meta_gc);
	HSTR_PUSH_CFUNC(L, "__index",     meta_index);
	HSTR_PUSH_CFUNC(L, "__tostring",  meta_tostring);
	LuaPushNamedString(L, "__metatable", "protected metatable");

		// push userdata callouts
		REGISTER_LUA_CFUNC(ReadPixel);
		REGISTER_LUA_CFUNC(ReadMapPixel);

		REGISTER_LUA_CFUNC(GetFormat);

	lua_pop(L, 1);
	return true;
}


/******************************************************************************
 *  Helpers
 */

inline LuaImageData* toimage(lua_State* L, int idx)
{
	auto image = static_cast<std::shared_ptr<LuaImageData>*>(luaL_checkudata(L, idx, "Image"));

	if (*image == nullptr)
		luaL_error(L, "attempt to use a deleted image");

	return image->get();
}


int PushImagePixel(lua_State* L, const LuaImageData* image, int x, int y)
{
	if (x > image->width || y > image->height || x < 0 || y < 0)
		return 0;

	int pixelCoords = (x + y * image->width) * image->channels;

	switch (image->bitmap->dataType) {
		case GL_UNSIGNED_BYTE: {
			const uint8_t* data = image->bitmap->GetRawMem() + pixelCoords;
			for (int i=0; i < image->channels; ++i) {
				lua_pushinteger(L, data[i]);
			}
		} break;
		case GL_UNSIGNED_SHORT: {
			const uint16_t* data = reinterpret_cast<uint16_t*>(image->bitmap->GetRawMem()) + pixelCoords;
			for (int i=0; i < image->channels; ++i) {
				lua_pushinteger(L, data[i]);
			}
		} break;
		case GL_FLOAT: {
			const float* data = reinterpret_cast<float*>(image->bitmap->GetRawMem()) + pixelCoords;
			for (int i=0; i < image->channels; ++i) {
				lua_pushnumber(L, data[i]);
			}
		} break;
	}
	return image->channels;
}


std::shared_ptr<LuaImageData> LoadImageObject(lua_State* L)
{
	std::string filename = luaL_checkstring(L, 1);
	bool grayscale = luaL_optboolean(L, 2, false);
	int channels = luaL_optinteger(L, 3, 0);
	int dataType = luaL_optinteger(L, 4, 0);
	auto image = make_shared<LuaImageData>(filename, grayscale, channels, dataType);
	if (image->valid) {
		return image;
	}
	return nullptr;
}


/******************************************************************************/
/******************************************************************************/

int LuaImage::meta_gc(lua_State* L)
{
	if (lua_isnil(L, 1))
		return 0;

	auto image = std::move(*static_cast<std::shared_ptr<LuaImageData>*>(luaL_checkudata(L, 1, "Image")));
	image = {};

	return 0;
}


int LuaImage::meta_tostring(lua_State* L)
{
	if (lua_isnil(L, 1))
		return 0;

	auto image = toimage(L, 1);
	lua_pushstring(L, std::format("Image(\"{}\", 0x{:x})", image->filename, reinterpret_cast<intptr_t>(image)).c_str());

	return 1;
}


int LuaImage::meta_index(lua_State* L)
{
	// first check if there is a function
	luaL_getmetatable(L, "Image");
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (!lua_isnil(L, -1))
		return 1;

	lua_pop(L, 1);

	// couldn't find a function, so check properties
	auto image = toimage(L, 1);

	if (lua_israwstring(L, 2)) {
		switch (hashString(lua_tostring(L, 2))) {
			case hashString("width"): {
				lua_pushinteger(L, image->width);
				return 1;
			} break;
			case hashString("height"): {
				lua_pushinteger(L, image->height);
				return 1;
			} break;

			case hashString("channels"): {
				lua_pushinteger(L, image->channels);
				return 1;
			} break;

			case hashString("format"): {
				lua_pushinteger(L, image->bitmap->dataType);
				return 1;
			} break;

			default: {
			} break;
		}
	}

	return 0;
}


/******************************************************************************
 *  Api
 */

int LuaImage::LoadImage(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto image = LoadImageObject(L);
	if (image == nullptr)
		return 0;

	auto shPtrImagePtr = static_cast<decltype(image)*>(lua_newuserdata(L, sizeof(decltype(image))));
	memset(shPtrImagePtr, 0, sizeof(decltype(image)));

	*shPtrImagePtr = std::move(image);

	luaL_getmetatable(L, "Image");
	lua_setmetatable(L, -2);
	return 1;
}


int LuaImage::DeleteImage(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return meta_gc(L);
}


int LuaImage::GetFormat(lua_State* L)
{
	const LuaImageData* image = toimage(L, 1);

	lua_pushinteger(L, image->width);
	lua_pushinteger(L, image->height);
	lua_pushinteger(L, image->channels);
	lua_pushinteger(L, image->bitmap->dataType);
	return 4;
}


int LuaImage::ReadPixel(lua_State* L)
{
	const LuaImageData* image = toimage(L, 1);

	const unsigned int x = luaL_checkinteger(L, 2) - 1;
	const unsigned int y = luaL_checkinteger(L, 3) - 1;

	return PushImagePixel(L, image, x, y);
}


int LuaImage::ReadMapPixel(lua_State* L)
{
	const LuaImageData* image = toimage(L, 1);

	const float mapX = luaL_checknumber(L, 2);
	const float mapY = luaL_checknumber(L, 3);

	const float mapSizeX = mapDims.mapx * SQUARE_SIZE;
	const float mapSizeY = mapDims.mapy * SQUARE_SIZE;

	const float x = (mapX * image->width) / mapSizeX;
	const float y = (mapY * image->height) / mapSizeY;

	return PushImagePixel(L, image, x, y);
}

