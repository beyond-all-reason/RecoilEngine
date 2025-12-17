#ifndef AUDIOSTATICS_H
#define AUDIOSTATICS_H

#include <cstdint>

class AudioStatics {
    public:
        static bool ShouldUseHitFallback(uint32_t hash);
};

#endif

