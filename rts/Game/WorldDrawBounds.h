/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include "Game/ViewRectangle.h"
#include "System/float3.h"

// Local render state, independent of camera bounds and the minimap view.
// Changed by unsynced Lua setters between rendering updates, never by render workers.
namespace worldDrawBounds {
	inline ViewRectangle rectangle;

	inline bool Contains(const float3& position) {
		return !rectangle.IsSet() || rectangle.Contains(position.x, position.z);
	}

	inline void Clear() { rectangle = {}; }
}
