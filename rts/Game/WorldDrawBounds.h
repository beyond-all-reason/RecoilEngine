/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include "Game/ViewRectangle.h"
#include "System/float3.h"

// Local render state, independent of camera bounds and the minimap view.
// Changed by unsynced Lua setters between rendering updates, never by render workers.
namespace worldDrawBounds {
	inline ViewRectangle terrainRectangle;
	inline ViewRectangle objectRectangle;

	inline bool Contains(const float3& position) {
		return !objectRectangle.IsSet() || objectRectangle.Contains(position.x, position.z);
	}

	// Keep the original combined setter's behavior; the object setter can then
	// extend drawing into cubicle barriers or beyond the physical map edge.
	inline void Set(const ViewRectangle& rectangle) {
		terrainRectangle = rectangle;
		objectRectangle = rectangle;
	}

	inline void Clear() { Set({}); }
}
