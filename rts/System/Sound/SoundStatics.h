#ifndef SOUNDSTATICS_H
#define SOUNDSTATICS_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include "Lua/LuaParser.h"
#include "Sim/Misc/GuiSoundSet.h"

class SoundStatics {
    private:
        static const std::unordered_map<uint32_t, std::string> soundFallbackMap;
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

