#ifndef SOUNDUTILS_H
#define SOUNDUTILS_H

#include <cstdint>
#include <string>
#include "Lua/LuaParser.h"
#include "Sim/Misc/GuiSoundSet.h"

class SoundUtils {
    private:
        static constexpr int MAX_SOUND_FILES = 8;
    public:
        static std::string GetSoundFallbackKey(const std::string& soundKey);
        static bool LoadSound(
            const LuaTable& table,
            const std::string& key,
            GuiSoundSet& soundSet,
            std::uint8_t depth = 3
        );
};

#endif
