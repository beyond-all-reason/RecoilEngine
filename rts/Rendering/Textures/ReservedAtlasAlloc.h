/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "TextureAtlasTypes.h"

struct ReservedAtlasRequest {
	std::string id;
	int2 contentSize = int2(0, 0);
	AtlasPadding padding;
	std::optional<uint32_t> existingPage;
	std::optional<AtlasPixelRect> existingAllocation;
};

struct ReservedAtlasPlacement {
	std::string id;
	uint32_t page = 0;
	AtlasPixelRect content;
	AtlasPixelRect allocation;
	bool reusedExisting = false;

	bool operator==(const ReservedAtlasPlacement&) const = default;
};

class ReservedAtlasAlloc
{
public:
	ReservedAtlasAlloc(int32_t width, int32_t height, std::vector<AtlasPageDefinition> pages);

	[[nodiscard]] std::optional<std::vector<ReservedAtlasPlacement>> Plan(const std::vector<ReservedAtlasRequest>& requests) const;

private:
	int32_t width;
	int32_t height;
	std::vector<AtlasPageDefinition> pages;
};
