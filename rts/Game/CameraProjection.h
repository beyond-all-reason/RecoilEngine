/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CAMERA_PROJECTION_H
#define CAMERA_PROJECTION_H

#include <cmath>

#include "System/float3.h"
#include "System/float4.h"

namespace CameraProjection {

	// Clip-space w of a perspective projection is the depth in front of the camera, in elmos.
	// A magnitude below this means the point lies in the camera plane.
	static constexpr float MIN_CLIP_W = 1e-6f;

	// Maps a clip-space position to viewport coordinates (x, y in pixels, z in [0, 1] when between the
	// near and far planes), the same as gluProject().
	//
	// A point in the camera plane has w == 0. Dividing by it raises a divide-by-zero in builds that
	// enable float exceptions (Debug); other builds got an infinity, which callers skip because z is
	// outside [0, 1]. Dividing by a tiny w of the same sign gives a huge finite value outside [0, 1]
	// instead, so callers see the same "off-screen" result without the exception.
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

#endif
