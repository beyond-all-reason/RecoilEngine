/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaImage.h"

#include "LuaInclude.h"
#include "LuaUtils.h"

#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"
#include "Map/ReadMap.h"

#include "System/Misc/TracyDefs.h"


/******************************************************************************
 * LuaImageData
 ******************************************************************************/

/***
 * @class Image
 * @field width
 * @field height
 * @field size
 * @field format
 * @field channels
 * @field dataType
 */

LuaImageData::LuaImageData(std::string filename, int reqChannels, int reqDataType, bool luminance) : filename(filename)
{
	bitmap = std::make_shared<CBitmap>();
	if (luminance)
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


/******************************************************************************
 * LuaImage
 ******************************************************************************/


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


std::shared_ptr<LuaImageData> LuaImage::LoadImageObject(lua_State* L)
{
	std::string filename = luaL_checkstring(L, 1);
	int channels = LuaImage::ParseFormat(L, 2);
	int dataType = LuaImage::ParseDataType(L, 3);
	bool luminance = luaL_optboolean(L, 4, false);
	auto image = make_shared<LuaImageData>(filename, channels, dataType, luminance);
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

			case hashString("size"): {
				lua_pushinteger(L, image->width);
				lua_pushinteger(L, image->height);
				return 2;
			} break;

			case hashString("channels"): {
				lua_pushinteger(L, image->channels);
				return 1;
			} break;

			case hashString("format"): {
				lua_pushstring(L, ChannelsToFormat(image->channels));
				return 1;
			} break;

			case hashString("dataType"): {
				lua_pushstring(L, DataTypeToString(image->bitmap->dataType));
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

/*** Loads an image
 *
 * @function Image.LoadImage
 *
 * @param filePath string VFS path to the file, for example "map/windfield.png".
 * @param format string Force resulting Image format. Leave as "default" or nil for default.
 * @param dataType string Force resulting Image dataType. Leave as "default" or nil for default.
 *
 * @return Image userdata object to operate on the image
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


/*** Deletes an image
 *
 * @function Image.DeleteImage
 * @param image Image Image object to delete.
 */

int LuaImage::DeleteImage(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return meta_gc(L);
}


/*** Reads a pixel from the image
 *
 * @function Image:ReadPixel
 *
 * Return values depend on the image channels and type.
 *
 * @param x integer x coordinate in pixels.
 * @param y integer y coordinate in pixels.
 * @return r number
 * @return g number?
 * @return b number?
 * @return a number?
 */

int LuaImage::ReadPixel(lua_State* L)
{
	const LuaImageData* image = toimage(L, 1);

	const unsigned int x = luaL_checkinteger(L, 2) - 1;
	const unsigned int y = luaL_checkinteger(L, 3) - 1;

	return PushImagePixel(L, image, x, y);
}


/*** Reads a pixel from the image in map coordinates
 *
 * @function Image:ReadMapPixel
 *
 * Return values depend on the image channels and type.
 *
 * @param x number x coordinate in elmos.
 * @param y number y coordinate in elmos.
 * @return r number
 * @return g number?
 * @return b number?
 * @return a number?
 */


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


/******************************************************************************
 *  Formatting
 */

const char* LuaImage::DataTypeToString(int dataType)
{
	switch(dataType) {
		case GL_UNSIGNED_BYTE:
			return "byte";
		case GL_UNSIGNED_SHORT:
			return "short";
		case GL_FLOAT:
			return "float";
		default:
			return "unknown";
	}
}


int LuaImage::StringToDataType(const char* str)
{
	switch (hashString(str)) {
		case hashString("byte"):
			return GL_UNSIGNED_BYTE;
		case hashString("short"):
			return GL_UNSIGNED_SHORT;
		case hashString("float"):
			return GL_FLOAT;
		case hashString("default"):
			return 0;
		default:
			return -1;
	}
}



const char* LuaImage::ChannelsToFormat(int channels)
{
	switch(channels) {
		case 1:
			return "grayscale";
		case 3:
			return "rgb";
		case 4:
			return "rgba";
		default:
			return "unknown";
	}
}


int LuaImage::FormatToChannels(const char* format)
{
	switch (hashString(format)) {
		case hashString("grayscale"):
			return 1;
		case hashString("rgb"):
			return 3;
		case hashString("rgba"):
			return 4;
		case hashString("default"):
			return 0;
		default:
			return -1;
	}
}


int LuaImage::ParseFormat(lua_State* L, int index)
{
	int channels = FormatToChannels(luaL_optstring(L, index, "default"));
	if (channels == -1)
		luaL_error(L, "Invalid format for image");
	return channels;
}


int LuaImage::ParseDataType(lua_State* L, int index)
{
	int dataType = StringToDataType(luaL_optstring(L, index, "default"));
	if (dataType == -1)
		luaL_error(L, "Invalid data type for image");
	return dataType;
}

