/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SoundUtils.h"
#include "Lua/LuaParser.h"
#include "Sim/Misc/CommonDefHandler.h"
#include "Sim/Misc/GuiSoundSet.h"
#include "System/Log/ILog.h"
#include "System/Misc/TracyDefs.h"

#undef LOG_SECTION_CURRENT
#define LOG_SECTION_CURRENT "Sound"

static bool LoadSoundFromTable(
	const LuaTable& table,
	const std::string& key,
	float volume,
	GuiSoundSet& soundSet
) {
	std::string soundFile = table.GetString(key, "");
	if (!soundFile.empty()) {
		CommonDefHandler::AddSoundSetData(soundSet, soundFile, volume);
		LOG_L(L_DEBUG, "[SoundUtils] Loaded sound '%s' (key=%s, vol=%.2f)", soundFile.c_str(), key.c_str(), volume);
		return true;
	}

	LuaTable soundTable = table.SubTable(key);
	if (!soundTable.IsValid())
		return false;

	bool any = false;
	for (int i = 1; ; i++) {
		soundFile = soundTable.GetString(i, "");
		if (soundFile.empty())
			break;

		CommonDefHandler::AddSoundSetData(soundSet, soundFile, volume);
		LOG_L(L_DEBUG, "[SoundUtils] Loaded sound '%s' (key=%s[%d], vol=%.2f)", soundFile.c_str(), key.c_str(), i, volume);
		any = true;
	}
	return any;
}

bool SoundUtils::LoadSound(
	const LuaTable& table,
	const std::string& key,
	GuiSoundSet& soundSet
) {
	RECOIL_DETAILED_TRACY_ZONE;

	if (LoadSoundFromTable(table, key, table.GetFloat(key + "Volume", 1.0f), soundSet))
		return true;

	// soundHitDry and soundHitWet both fall back to soundHit (using soundHitVolume).
	// Matches the WEAPONDUMMYTAG fallbackName declarations in WeaponDef.cpp.
	if (key == "soundHitDry" || key == "soundHitWet") {
		const float fallbackVolume = table.GetFloat("soundHitVolume", 1.0f);
		if (LoadSoundFromTable(table, "soundHit", fallbackVolume, soundSet))
			return true;
	}

	LOG_L(L_DEBUG, "[SoundUtils] No sound found for key '%s'", key.c_str());
	return false;
}
