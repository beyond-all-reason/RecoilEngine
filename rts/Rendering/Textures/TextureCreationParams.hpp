#pragma once

#include <optional>
#include <cstdint>
#include <array>
#include "System/float4.h"

namespace GL {
	struct TextureCreationParams {
		float aniso = 0.0f;
		float lodBias = 0.0f;
		uint32_t texID = 0;
		int32_t reqNumLevels = 0;
		bool linearMipMapFilter = true;
		bool linearTextureFilter = true;
		bool wrapMirror = true;
		bool repeatMirror = false;
		std::optional<float4> clampBorder = std::nullopt;
		std::optional<uint32_t> minFilter = std::nullopt;
		std::optional<uint32_t> magFilter = std::nullopt;
		std::optional<std::array<int32_t, 3>> wrapModes = std::nullopt;
		uint32_t GetMinFilter(int32_t numLevels) const;
		uint32_t GetMagFilter() const;
		uint32_t GetWrapMode() const;
	};
}