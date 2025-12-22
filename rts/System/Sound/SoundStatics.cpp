#include "SoundStatics.h"
#include "System/Misc/TracyDefs.h"
#include "System/StringHash.h"
#include "Lua/LuaParser.h"
#include "Sim/Misc/CommonDefHandler.h"
#include "Sim/Misc/GuiSoundSet.h"
#include "System/Log/ILog.h"
#include <cstdint>

#undef LOG_SECTION_CURRENT
#define LOG_SECTION_CURRENT "Sound"

const std::unordered_map<uint32_t, std::string> SoundStatics::soundFallbackMap = {
    {hashString("soundHitWet"), "soundHit"},
    {hashString("soundHitDry"), "soundHit"}
};

std::string SoundStatics::GetSoundFallbackKey(const std::string& soundKey) {
    uint32_t keyHash = hashString(soundKey.c_str());
    auto it = soundFallbackMap.find(keyHash);
    if (it != soundFallbackMap.end()) {
        return it->second;
    }
    return "";
}

bool SoundStatics::LoadSound(
    const LuaTable& table,
    const std::string& key,
    GuiSoundSet& soundSet,
    std::uint8_t depth
) {
    RECOIL_DETAILED_TRACY_ZONE;

    float volume = table.GetFloat(key + "Volume", 1.0f);

    std::string soundFile = table.GetString(key, "");
    if (!soundFile.empty()) {
        CommonDefHandler::AddSoundSetData(soundSet, soundFile, volume);
        LOG_L(L_DEBUG, "[SoundStatics::%s] Successfully loaded sound file: %s with volume: %f", __func__, soundFile.c_str(), volume);
        return true;
    }

    LuaTable soundTable = table.SubTable(key);

    if (soundTable.IsValid()) {
        bool success = false;

        for (int i = 1; i <= MAX_SOUND_FILES; i++) {
            soundFile = soundTable.GetString(i, "");

            if (soundFile.empty())
                break;

            CommonDefHandler::AddSoundSetData(soundSet, soundFile, volume);
            LOG_L(L_DEBUG, "[SoundStatics::%s] Successfully loaded sound file: %s with volume: %f", __func__, soundFile.c_str(), volume);
            success = true;
        }

        if (success) {
            return true;
        }
    }

    std::string fallbackKey = GetSoundFallbackKey(key);

    if (fallbackKey.empty()) {
        LOG_L(L_DEBUG, "[SoundStatics::%s] Sound file not found: %s, there is no fallback to try, not adding sound-set", __func__, key.c_str());
        return false;
    }

    if (depth == 0) {
        LOG_L(L_DEBUG, "[SoundStatics::%s] Sound file not found: %s, reached max depth while searching for fallbacks", __func__, key.c_str());
        return false;
    }

    depth--;

    LOG_L(L_DEBUG, "[SoundStatics::%s] Sound file not found: %s, trying fallback: %s", __func__, key.c_str(), fallbackKey.c_str());
    return SoundStatics::LoadSound(table, fallbackKey, soundSet, depth);
}

