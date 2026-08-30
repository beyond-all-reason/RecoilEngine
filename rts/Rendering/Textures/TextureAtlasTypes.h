/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "AtlasedTexture.hpp"
#include "System/type2.h"

enum class AtlasPaddingMode : uint8_t {
	CLAMP,
	TILE,
};

struct AtlasPixelRect {
	int32_t x = 0;
	int32_t y = 0;
	int32_t width = 0;
	int32_t height = 0;

	constexpr bool operator==(const AtlasPixelRect&) const = default;

	[[nodiscard]] constexpr bool HasPositiveArea() const { return width > 0 && height > 0; }
	[[nodiscard]] constexpr int64_t Right64() const { return int64_t{x} + width; }
	[[nodiscard]] constexpr int64_t Bottom64() const { return int64_t{y} + height; }
	[[nodiscard]] constexpr bool IsNonNegative() const { return x >= 0 && y >= 0 && width >= 0 && height >= 0; }
	[[nodiscard]] constexpr bool HasRepresentableEdges() const {
		return Right64() <= std::numeric_limits<int32_t>::max() && Bottom64() <= std::numeric_limits<int32_t>::max();
	}
	[[nodiscard]] constexpr bool IsValid() const { return HasPositiveArea() && IsNonNegative() && HasRepresentableEdges(); }
	[[nodiscard]] constexpr bool FitsInside(int32_t boundsWidth, int32_t boundsHeight) const {
		return IsValid() && boundsWidth > 0 && boundsHeight > 0 && Right64() <= boundsWidth && Bottom64() <= boundsHeight;
	}
	[[nodiscard]] constexpr bool Contains(const AtlasPixelRect& other) const {
		return IsValid() && other.IsValid() && other.x >= x && other.y >= y && other.Right64() <= Right64() && other.Bottom64() <= Bottom64();
	}
	[[nodiscard]] constexpr bool Overlaps(const AtlasPixelRect& other) const {
		return IsValid() && other.IsValid() && x < other.Right64() && other.x < Right64() && y < other.Bottom64() && other.y < Bottom64();
	}
};

struct AtlasPadding {
	AtlasPaddingMode mode = AtlasPaddingMode::CLAMP;
	uint32_t pixels = 0;
	uint32_t tilesX = 0;
	uint32_t tilesY = 0;

	constexpr bool operator==(const AtlasPadding&) const = default;
};

struct AtlasVariant {
	std::string id;
	std::string format;
	std::vector<std::string> files;
	std::vector<std::string> sha256;
};

struct AtlasPageDefinition {
	std::vector<AtlasPixelRect> reserves;
};

struct AtlasEntryDefinition {
	std::string id;
	std::string source;
	uint32_t page = 0;
	int2 sourceSize = int2(0, 0);
	AtlasPixelRect content;
	AtlasPixelRect allocation;
	AtlasPadding padding;
};

struct AtlasPatchDefinition {
	std::string id;
	std::string source;
	std::optional<AtlasPadding> padding;
};

struct TextureAtlasManifest {
	std::string name;
	std::string target = "2d";
	int32_t width = 0;
	int32_t height = 0;
	uint32_t mipLevels = 0;
	AtlasPadding defaultPadding;
	std::vector<AtlasVariant> variants;
	std::vector<AtlasPageDefinition> pages;
	std::vector<AtlasEntryDefinition> entries;
};

[[nodiscard]] inline std::optional<int2> GetPaddedAtlasSize(const int2 contentSize, const AtlasPadding& padding)
{
	if (contentSize.x <= 0 || contentSize.y <= 0)
		return std::nullopt;

	int64_t width = contentSize.x;
	int64_t height = contentSize.y;
	if (padding.mode == AtlasPaddingMode::CLAMP) {
		width += int64_t{padding.pixels} * 2;
		height += int64_t{padding.pixels} * 2;
	} else {
		width *= int64_t{padding.tilesX} * 2 + 1;
		height *= int64_t{padding.tilesY} * 2 + 1;
	}

	if (width <= 0 || height <= 0 || width > std::numeric_limits<int32_t>::max() || height > std::numeric_limits<int32_t>::max())
		return std::nullopt;

	return int2(static_cast<int32_t>(width), static_cast<int32_t>(height));
}

[[nodiscard]] inline std::optional<AtlasPixelRect> GetContentRect(const AtlasPixelRect& allocation, const int2 contentSize, const AtlasPadding& padding)
{
	const auto paddedSize = GetPaddedAtlasSize(contentSize, padding);
	if (!allocation.IsValid() || !paddedSize || paddedSize->x > allocation.width || paddedSize->y > allocation.height)
		return std::nullopt;

	const int64_t offsetX = (padding.mode == AtlasPaddingMode::CLAMP) ? padding.pixels : int64_t{padding.tilesX} * contentSize.x;
	const int64_t offsetY = (padding.mode == AtlasPaddingMode::CLAMP) ? padding.pixels : int64_t{padding.tilesY} * contentSize.y;
	const AtlasPixelRect content {
		static_cast<int32_t>(int64_t{allocation.x} + offsetX),
		static_cast<int32_t>(int64_t{allocation.y} + offsetY),
		contentSize.x,
		contentSize.y,
	};
	return allocation.Contains(content) ? std::optional<AtlasPixelRect>{content} : std::nullopt;
}

[[nodiscard]] inline AtlasedTexture AtlasPixelRectToUV(const AtlasPixelRect& rect, int32_t atlasWidth, int32_t atlasHeight, uint32_t page)
{
	if (!rect.FitsInside(atlasWidth, atlasHeight))
		return AtlasedTexture::DefaultAtlasTexture;

	return AtlasedTexture(
		static_cast<float>(rect.x) / atlasWidth,
		static_cast<float>(rect.y) / atlasHeight,
		static_cast<float>(rect.Right64()) / atlasWidth,
		static_cast<float>(rect.Bottom64()) / atlasHeight,
		page
	);
}
