/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_IMAGE_H
#define LUA_IMAGE_H

#include <vector>
#include <string>

#include "Rendering/GL/myGL.h"


struct lua_State;
class CBitmap;

class LuaImageData {
	public:
		LuaImageData(std::string filename, int channels, int dataType, bool luminance);

		std::string filename;
		bool valid;
		std::shared_ptr<CBitmap> bitmap;
		unsigned int width;
		unsigned int height;
		unsigned int channels;
};

class LuaImage {
	public:
		LuaImage() { }

		static bool PushEntries(lua_State* L);

	private: // helpers
		static bool CreateMetatable(lua_State* L);

	private: // metatable methods
		static int meta_gc(lua_State* L);
		static int meta_index(lua_State* L);
		static int meta_tostring(lua_State* L);

	public:
		static int LoadImage(lua_State* L);

	private: // call-outs
		static int DeleteImage(lua_State* L);

		static int ReadPixel(lua_State* L);
		static int ReadMapPixel(lua_State* L);
		static int ReadPixels(lua_State* L);
		static int ReadMapPixels(lua_State* L);
	private:
		static std::shared_ptr<LuaImageData> LoadImageObject(lua_State* L);

		static const char* DataTypeToString(int dataType);
		static int StringToDataType(const char* str);
		static const char* ChannelsToFormat(int channels);
		static int FormatToChannels(const char* format);

		static int ParseFormat(lua_State* L, int index);
		static int ParseDataType(lua_State* L, int index);
};

#endif /* LUA_IMAGE_H */
