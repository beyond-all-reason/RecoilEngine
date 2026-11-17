/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cmath>

#include "System/float3.h"
#include "System/float4.h"

namespace CameraProjection {
	static constexpr float MIN_CLIP_W = 1e-6f;

	// Same as gluProject().
	// A point in the camera plane has clip w == 0; the division then raises a float exception in Debug builds.
	// A tiny w of the same sign gives a huge finite result with z outside [0, 1], which callers skip as off-screen.
	inline float3 ClipToViewport(const float4& clip, const int* viewport)
	{
		const float w = (std::fabs(clip.w) < MIN_CLIP_W) ? std::copysign(MIN_CLIP_W, clip.w) : clip.w;
		const float3 ndc = float3(clip) / w;

		float3 vpPos;
		vpPos.x = viewport[2] * (ndc.x + 1.0f) * 0.5f;
		vpPos.y = viewport[3] * (ndc.y + 1.0f) * 0.5f;
		vpPos.z =               (ndc.z + 1.0f) * 0.5f;
		return vpPos;
	}
}
