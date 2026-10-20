/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <algorithm>
#include <cmath>

// Unsynced view state only. An unset rectangle retains the map's normal view.
struct ViewRectangle
{
	float x0 = 0.0f;
	float z0 = 0.0f;
	float x1 = 0.0f;
	float z1 = 0.0f;

	bool IsSet() const { return x1 > x0 && z1 > z0; }
	float Width() const { return x1 - x0; }
	float Height() const { return z1 - z0; }
	float CenterX() const { return x0 + Width() * 0.5f; }
	float CenterZ() const { return z0 + Height() * 0.5f; }
	bool Contains(float x, float z) const { return x >= x0 && x <= x1 && z >= z0 && z <= z1; }
	float ClampX(float x) const {
		const float margin = std::min(0.01f, Width() * 0.25f);
		return std::clamp(x, x0 + margin, x1 - margin);
	}
	float ClampZ(float z) const {
		const float margin = std::min(0.01f, Height() * 0.25f);
		return std::clamp(z, z0 + margin, z1 - margin);
	}

	bool Valid() const {
		return std::isfinite(x0) && std::isfinite(z0) && std::isfinite(x1) && std::isfinite(z1) && IsSet();
	}
	bool ValidForMap(float mapWidth, float mapHeight) const {
		return Valid() && x0 >= 0.0f && z0 >= 0.0f && x1 <= mapWidth && z1 <= mapHeight;
	}
};
