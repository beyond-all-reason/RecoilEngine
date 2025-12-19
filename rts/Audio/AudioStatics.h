#ifndef AUDIOSTATICS_H
#define AUDIOSTATICS_H

#include <cstdint>
#include <lualib.h>
#include "Lua/LuaParser.h"
#include "Sim/Misc/GuiSoundSet.h"

class AudioStatics {
    private:
        static const std::unordered_map<uint32_t, std::string> soundFallbackMap;
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

