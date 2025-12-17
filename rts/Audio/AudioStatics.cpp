#include "AudioStatics.h"
#include "System/StringHash.h"
#include <cstdint>

static const std::unordered_set<uint32_t> hitFallbackSoundHashes = {
    hashString("soundHitWet"),
    hashString("soundHitDry"),
};

bool AudioStatics::ShouldUseHitFallback(uint32_t hash) {
    return (hitFallbackSoundHashes.find(hash) != hitFallbackSoundHashes.end());
}

