/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <array>
#include <cstdint>

#include "Sim/Misc/GlobalConstants.h"
#include "System/float4.h"

class CCustomColorPalette {
public:
	void Init();
	void Kill();

	void SetColor(uint16_t index, float r, float g, float b);
	float4 GetColor(uint16_t index) const;
	static bool IsValidIndex(uint16_t index);

	static uint16_t EncodePaletteIndex(uint16_t customIndex) {
		return CUSTOM_COLOR_PALETTE_BASE + customIndex;
	}

	static uint16_t DecodePaletteIndex(uint16_t paletteIndex) {
		return paletteIndex - CUSTOM_COLOR_PALETTE_BASE;
	}

	static bool IsCustomPaletteIndex(uint16_t paletteIndex) {
		return paletteIndex >= CUSTOM_COLOR_PALETTE_BASE;
	}

	static CCustomColorPalette& GetInstance() {
		static CCustomColorPalette instance;
		return instance;
	}

private:
	std::array<float4, MAX_CUSTOM_COLORS> colors{};
	bool initialized = false;
};

extern CCustomColorPalette customColorPalette;
