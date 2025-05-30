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
		valid = bitmap->Load(filename, 1.0f, reqChannels, dataType);
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
	REGISTER_LUA_CFUNC(LoadImage);
	REGISTER_LUA_CFUNC(DeleteImage);
	REGISTER_LUA_CFUNC(ReadPixel);
	REGISTER_LUA_CFUNC(ReadMapPixel);
	REGISTER_LUA_CFUNC(GetFormat);

	return true;
}


/******************************************************************************/
/******************************************************************************/

const LuaImageData* LuaImage::GetImage(unsigned int id)
{
	RECOIL_DETAILED_TRACY_ZONE;
	for(auto& image: luaImage.images) {
		if (image.id == id)
			return &image;
	}
	return nullptr;
}


/******************************************************************************
 *  Helpers
 */

const LuaImageData* GetLuaImageData(lua_State* L)
{
	const unsigned int id = luaL_checkinteger(L, 1);
	const LuaImageData* image = luaImage.GetImage(id);
	if (image == nullptr) {
		luaL_error(L, "bad image!");
	}
	return image;
		
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


/******************************************************************************
 *  Api
 */

int LuaImage::LoadImage(lua_State* L)
{
	std::string filename = luaL_checkstring(L, 1);
	bool grayscale = luaL_optboolean(L, 2, false);
	int channels = luaL_optinteger(L, 3, 0);
	int dataType = luaL_optinteger(L, 4, 0);
	LuaImageData image = LuaImageData(filename, grayscale, channels, dataType);
	if (image.valid) {
		image.id = ++luaImage.lastIndex;
		luaImage.images.emplace_back(std::move(image));
		lua_pushinteger(L, image.id);
		return 1;
	}
	return 0;
}


int LuaImage::DeleteImage(lua_State* L)
{
	const unsigned int id = luaL_checkinteger(L, 1);

	const auto it = std::find_if(luaImage.images.begin(), luaImage.images.end(),
			[&id](const LuaImageData& x) { return x.id == id;});

	if (it != luaImage.images.end())
		luaImage.images.erase(it);

	return 0;
}


int LuaImage::GetFormat(lua_State* L)
{
	const LuaImageData* image = GetLuaImageData(L);

	lua_pushinteger(L, image->width);
	lua_pushinteger(L, image->height);
	lua_pushinteger(L, image->channels);
	lua_pushinteger(L, image->bitmap->dataType);
	return 4;
}


int LuaImage::ReadPixel(lua_State* L)
{
	const LuaImageData* image = GetLuaImageData(L);

	const unsigned int x = luaL_checkinteger(L, 2) - 1;
	const unsigned int y = luaL_checkinteger(L, 3) - 1;

	return PushImagePixel(L, image, x, y);
}


int LuaImage::ReadMapPixel(lua_State* L)
{
	const LuaImageData* image = GetLuaImageData(L);

	const float mapX = luaL_checknumber(L, 2);
	const float mapY = luaL_checknumber(L, 3);

	const float mapSizeX = mapDims.mapx * SQUARE_SIZE;
	const float mapSizeY = mapDims.mapy * SQUARE_SIZE;

	const float x = (mapX * image->width) / mapSizeX;
	const float y = (mapY * image->height) / mapSizeY;

	return PushImagePixel(L, image, x, y);
}

