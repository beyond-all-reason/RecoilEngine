/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <stdexcept>

#include "LuaImage.h"

#include "LuaInclude.h"
#include "LuaUtils.h"

#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"
#include "Map/ReadMap.h"

#include "System/Misc/TracyDefs.h"

#ifdef LoadImage // windows defines this and messes up our LoadImage.
#undef LoadImage
#endif

class LuaImageException : public std::runtime_error
{
	using std::runtime_error::runtime_error;
};


/******************************************************************************
 * LuaImageData
 ******************************************************************************/

/***
 * @class Image
 * @field width integer
 * @field height integer
 * @field size integer[]
 * @field format string
 * @field channels integer
 * @field dataType string
 */

LuaImageData::LuaImageData(std::string filename, int reqChannels, int reqDataType) : filename(filename)
{
	bitmap = std::make_shared<CBitmap>();
	if (reqChannels == -1) {
		if (!bitmap->LoadGrayscale(filename))
			throw LuaImageException{"failure loading image"};
	}
	else {
		if (!bitmap->Load(filename, 1.0f, reqChannels, reqDataType))
			throw LuaImageException{"failure loading image"};
	}
	if (bitmap->dataType != GL_UNSIGNED_BYTE && bitmap->dataType != GL_UNSIGNED_SHORT && bitmap->dataType != GL_UNSIGNED_INT && bitmap->dataType != GL_FLOAT)
		throw LuaImageException{"bad resulting dataType"};
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
		REGISTER_LUA_CFUNC(ReadPixels);
		REGISTER_LUA_CFUNC(ReadMapPixels);

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
	if (x > image->width || y > image->height || x < 0 || y < 0) {
		luaL_error(L, "x or y coordinates out of bounds");
		return 0;
	}

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
		case GL_UNSIGNED_INT: {
			const uint32_t* data = image->bitmap->GetRawMem() + pixelCoords;
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


int ReadPixelsRaw(lua_State* L, const LuaImageData* image, int startX, int startY, int endX, int endY, int index, bool mapCoord)
{
	if (startX > image->width || startY > image->height || startX < 0 || startY < 0) {
		luaL_error(L, "start coordinates out of bounds");
		return 0;
	}

	if (endX > image->width || endY > image->height || endX < 0 || endY < 0 || endX <= startX || endY <= startY) {
		luaL_error(L, "end coordinates out of bounds");
		return 0;
	}

	if (!lua_isfunction(L, index)) {
		luaL_error(L, std::format("{}th parameter has to be a function", index).c_str());
		return 0;
	}

	const float mapFactorX = (mapDims.mapx * SQUARE_SIZE) / image->width;
	const float mapFactorY = (mapDims.mapy * SQUARE_SIZE) / image->height;

	const int nArgs = image->channels + 2;
	for (int y=startY; y < endY; ++y) {
		int linepos = y * image->width;
		for (int x=startX; x < endX; ++x) {
			int pixelCoords = (x + linepos) * image->channels;
			lua_pushvalue(L, index);
			if (mapCoord) {
				// TODO probably send the pixel center instead
				lua_pushnumber(L, x*mapFactorX);
				lua_pushnumber(L, y*mapFactorY);
			} else {
				lua_pushinteger(L, x);
				lua_pushinteger(L, y);
			}
			switch (image->bitmap->dataType) {
				case GL_UNSIGNED_BYTE: {
					const uint8_t* data = image->bitmap->GetRawMem() + pixelCoords;
					for (int i=0; i < image->channels; ++i) {
						lua_pushinteger(L, data[i]);
					}
					if (lua_pcall(L, nArgs, 0, 0)) {
						luaL_error(L, "ReadPixelsRaw failed");
					}
				} break;
				case GL_UNSIGNED_SHORT: {
					const uint16_t* data = reinterpret_cast<uint16_t*>(image->bitmap->GetRawMem()) + pixelCoords;
					for (int i=0; i < image->channels; ++i) {
						lua_pushinteger(L, data[i]);
					}
					if (lua_pcall(L, nArgs, 0, 0)) {
						luaL_error(L, "ReadPixelsRaw failed");
					}
				} break;
				case GL_UNSIGNED_INT: {
					const uint32_t* data = reinterpret_cast<uint32_t*>(image->bitmap->GetRawMem()) + pixelCoords;
					for (int i=0; i < image->channels; ++i) {
						lua_pushinteger(L, data[i]);
					}
					if (lua_pcall(L, nArgs, 0, 0)) {
						luaL_error(L, "ReadPixelsRaw failed");
					}
				} break;
				case GL_FLOAT: {
					const float* data = reinterpret_cast<float*>(image->bitmap->GetRawMem()) + pixelCoords;
					for (int i=0; i < image->channels; ++i) {
						lua_pushnumber(L, data[i]);
					}
					if (lua_pcall(L, nArgs, 0, 0)) {
						luaL_error(L, "ReadPixelsRaw failed");
					}
				} break;
			}
		}
	}
	return image->channels;
}


std::shared_ptr<LuaImageData> LuaImage::LoadImageObject(lua_State* L)
{
	std::string filename = luaL_checkstring(L, 1);
	int channels = LuaImage::ParseFormat(L, 2);
	int dataType = LuaImage::ParseDataType(L, 3);
	try {
		auto image = make_shared<LuaImageData>(filename, channels, dataType);
		return image;
	}
	catch (LuaImageException &e) {
		luaL_error(L, std::format("error loading image: {}", e.what()).c_str());
		return nullptr;
	}
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
 * Will load any image from the provided VFS `filePath`. The image will be converted into
 * the provided `format` and `dataType`.
 *
 * @param filePath string VFS path to the file, for example "map/windfield.png".
 * @param format string Force resulting Image format. Leave as "default" or nil for default.
 * Accepts the following options:
 * - grayscale: 1 channel.
 * - rgb: 3 channels.
 * - rgba: 4 channels.
 * - luminance: 1 channel with special luminance conversion if origin image is rgb or rgba. Also forces 'byte' `dataType`.
 * - default: same amount of channels as the source image.
 *
 * @param dataType string Force resulting Image dataType. Leave as "default" or nil for default.
 * Accepts the following options:
 * - byte: unsigned 8 bit integer.
 * - short: unsigned 16 bit integer.
 * - integer: unsigned 32 bit integer.
 * - float: float values.
 * - default: same dataType as the source image.
 *
 * @return Image userdata object to operate on the image
 */

int LuaImage::LoadImage(lua_State* L)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto image = LoadImageObject(L);
	if (image == nullptr)
		return 0;

	auto shPtrImagePtr = static_cast<decltype(image)*>(lua_newuserdata(L, sizeof(image)));
	memset(shPtrImagePtr, 0, sizeof(image));

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
 * @return number r
 * @return number? g
 * @return number? b
 * @return number? a
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
 * @return number r
 * @return number? g
 * @return number? b
 * @return number? a
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


/*** Reads pixels from an image rect
 *
 * @function Image:ReadPixels
 *
 * This method will iterate over the area pixels and call the
 * provided callback function, with each pixel channels as arguments.
 *
 * example:
 *
 *  image:ReadPixels(20, 20, 100, 100, function(r, g, b, a)
 *    Spring.Echo("got pixel", r, g, b, a)
 *  end)

 * Callback function arguments will depend on the image channels.
 *
 * @param x integer x coordinate in pixels.
 * @param y integer y coordinate in pixels.
 * @param width integer width in pixels.
 * @param height integer height in pixels.
 * @param callbackFn fun(...) Function to call for each pixel
 */

int LuaImage::ReadPixels(lua_State* L)
{
	const LuaImageData* image = toimage(L, 1);

	const unsigned int x = luaL_checkinteger(L, 2) - 1;
	const unsigned int y = luaL_checkinteger(L, 3) - 1;
	const unsigned int width = luaL_checkinteger(L, 4);
	const unsigned int height = luaL_checkinteger(L, 5);

	return ReadPixelsRaw(L, image, x, y, x + width, y + height, 6, false);
}

/*** Reads pixels from a map rect
 *
 * @function Image:ReadMapPixels
 *
 * This method will map the provided map coordinates to pixels,
 * and then iterate over them calling the provided function for
 * each pixel, like Image:ReadPixel.
 *
 * @param x integer x coordinate in elmos.
 * @param y integer y coordinate in elmos.
 * @param width integer width in elmos.
 * @param height integer height in elmos.
 * @param callbackFn fun(...) Function to call for each pixel
 *
 * @see Image:ReadPixels
 */


int LuaImage::ReadMapPixels(lua_State* L)
{
	const LuaImageData* image = toimage(L, 1);

	const float mapX = luaL_checkinteger(L, 2) - 1;
	const float mapY = luaL_checkinteger(L, 3) - 1;
	const float mapWidth = luaL_checkinteger(L, 4);
	const float mapHeight = luaL_checkinteger(L, 5);

	const float mapSizeX = mapDims.mapx * SQUARE_SIZE;
	const float mapSizeY = mapDims.mapy * SQUARE_SIZE;

	const float x = (mapX * image->width) / mapSizeX;
	const float y = (mapY * image->height) / mapSizeY;
	const float width = (mapX * image->width) / mapSizeX;
	const float height = (mapY * image->height) / mapSizeY;

	return ReadPixelsRaw(L, image, x, y, x + width, y + height, 6, true);
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
		case GL_UNSIGNED_INT:
			return "integer";
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
		case hashString("integer"):
			return GL_UNSIGNED_INT;
		case hashString("float"):
			return GL_FLOAT;
		case hashString("default"):
			return 0;
		default:
			throw LuaImageException(std::format("invalid image dataType '{}'", str));
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
		case hashString("luminance"):
			return -1; // special value
		case hashString("grayscale"):
			return 1;
		case hashString("rgb"):
			return 3;
		case hashString("rgba"):
			return 4;
		case hashString("default"):
			return 0;
		default:
			throw LuaImageException(std::format("invalid image format '{}'", format));
	}
}


int LuaImage::ParseFormat(lua_State* L, int index)
{
	try {
		int channels = FormatToChannels(luaL_optstring(L, index, "default"));
		return channels;
	}
	catch (LuaImageException& e) {
		return luaL_error(L, e.what());
	}
}


int LuaImage::ParseDataType(lua_State* L, int index)
{
	try {
		int dataType = StringToDataType(luaL_optstring(L, index, "default"));
		return dataType;
	}
	catch (LuaImageException& e) {
		return luaL_error(L, e.what());
	}
}

