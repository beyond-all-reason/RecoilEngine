/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <algorithm>
#include <span>

#include "System/Rectangle.h"

// Region edges must align to the grid used by their consumer.
// Rectangles are half-open world coordinates, as for SRectangle::Inside.
inline bool ValidRegionPartitions(std::span<const SRectangle> regions, int width, int height, int alignment)
{
	if (alignment <= 0)
		return false;

	for (size_t i = 0; i < regions.size(); ++i) {
		const SRectangle& r = regions[i];
		if (r.x1 < 0 || r.z1 < 0 || r.x2 > width || r.z2 > height || r.GetWidth() <= 0 || r.GetHeight() <= 0)
			return false;
		if (r.x1 % alignment || r.z1 % alignment || r.x2 % alignment || r.z2 % alignment)
			return false;
		for (size_t j = 0; j < i; ++j) {
			if (r.CheckOverlap(regions[j]))
				return false;
		}
	}
	return true;
}

inline const SRectangle* FindRegionPartition(std::span<const SRectangle> regions, float x, float z)
{
	for (const SRectangle& region: regions) {
		if (x >= region.x1 && x < region.x2 && z >= region.z1 && z < region.z2)
			return &region;
	}
	return nullptr;
}

// Positions beyond the map edge, such as aircraft flying past it, belong to the
// region at the nearest map position. Region edges are integers, so clamping to
// the last whole elmo keeps the half-open far edges exclusive.
inline const SRectangle* FindNearestRegionPartition(std::span<const SRectangle> regions, float x, float z, int width, int height)
{
	return FindRegionPartition(regions, std::clamp(x, 0.0f, width - 1.0f), std::clamp(z, 0.0f, height - 1.0f));
}
