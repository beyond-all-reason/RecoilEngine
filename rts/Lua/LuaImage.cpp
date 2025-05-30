/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaImage.h"

#include "LuaInclude.h"
#include "LuaUtils.h"

#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"

#include "System/Misc/TracyDefs.h"


LuaImage luaImage;

/******************************************************************************
 * ImageData
 ******************************************************************************/

LuaImageData::LuaImageData(std::string filename) : filename(filename)
{
	id     = 0;
	bitmap = std::make_shared<CBitmap>();
	valid = bitmap->Load(filename);
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
 * Image
 * @see rts/Lua/LuaImage.cpp
******************************************************************************/

LuaImage::~LuaImage()
{
	// TODO clean up
}


/******************************************************************************/
/******************************************************************************/

bool LuaImage::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(LoadImage);
	REGISTER_LUA_CFUNC(DeleteImage);
	REGISTER_LUA_CFUNC(ReadPixel);
	REGISTER_LUA_CFUNC(GetSize);

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


/******************************************************************************/
/******************************************************************************/

const LuaImageData* GetLuaImageData(lua_State* L)
{
	const unsigned int id = luaL_checkinteger(L, 1);
	const LuaImageData* image = luaImage.GetImage(id);
	if (image == nullptr) {
		luaL_error(L, "bad image!");
	}
	return image;
		
}

int LuaImage::LoadImage(lua_State* L)
{
	std::string filename = luaL_checkstring(L, 1);
	LuaImageData image = LuaImageData(filename);
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

int LuaImage::GetSize(lua_State* L)
{
	const LuaImageData* image = GetLuaImageData(L);

	lua_pushinteger(L, image->width);
	lua_pushinteger(L, image->height);
	return 0;
}

int LuaImage::ReadPixel(lua_State* L)
{
	const LuaImageData* image = GetLuaImageData(L);

	const unsigned int x = luaL_checkinteger(L, 2) - 1;
	const unsigned int y = luaL_checkinteger(L, 3) - 1;

	if (x > image->width || y > image->height || x < 0 || y < 0)
		return 0;

	if (image->bitmap->dataType != GL_UNSIGNED_BYTE)
		return 0;

	const uint8_t* data = image->bitmap->GetRawMem() + (x + y * image->width) * image->channels;
	for (int i=0; i < image->channels; ++i) {
		lua_pushinteger(L, data[i]);
	}
	return image->channels;
}

