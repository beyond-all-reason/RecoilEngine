#ifndef SOUNDUTILS_H
#define SOUNDUTILS_H

#include <string>
#include "Lua/LuaParser.h"
#include "Sim/Misc/GuiSoundSet.h"

class SoundUtils {
public:
	static bool LoadSound(
		const LuaTable& table,
		const std::string& key,
		GuiSoundSet& soundSet
	);
};

#endif
