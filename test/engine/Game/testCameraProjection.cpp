/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cmath>

#if defined(__SSE__) || defined(_M_X64)
	#include <xmmintrin.h>
	#define CAMERA_PROJECTION_TEST_HAS_SSE 1
#endif

#include "Game/CameraProjection.h"
#include "System/float3.h"
#include "System/float4.h"

#include <catch_amalgamated.hpp>

namespace {
	// Debug engine builds unmask these exceptions, so a division by zero stops the process instead of giving an infinity.
	struct ScopedFloatTraps {
		ScopedFloatTraps()
		{
		#ifdef CAMERA_PROJECTION_TEST_HAS_SSE
			oldCsr = _mm_getcsr();
			_mm_setcsr((oldCsr & ~_MM_EXCEPT_MASK) & ~(_MM_MASK_DIV_ZERO | _MM_MASK_INVALID | _MM_MASK_OVERFLOW));
		#endif
		}
		~ScopedFloatTraps()
		{
		#ifdef CAMERA_PROJECTION_TEST_HAS_SSE
			_mm_setcsr(oldCsr);
		#endif
		}
		unsigned int oldCsr = 0;
	};

	constexpr int VIEWPORT[4] = {0, 0, 1920, 1080};

	bool OffScreenDepth(const float3& vp) { return vp.z < 0.0f || vp.z > 1.0f; }
}


TEST_CASE("ClipToViewport")
{
	ScopedFloatTraps traps;

	SECTION("a point in front of the camera is projected as before")
	{
		const float3 vp = CameraProjection::ClipToViewport(float4(0.0f, 0.0f, 50.0f, 100.0f), VIEWPORT);
		CHECK(vp.x == Catch::Approx(960.0f));
		CHECK(vp.y == Catch::Approx(540.0f));
		CHECK(vp.z == Catch::Approx(0.75f));
	}

	SECTION("a point in the camera plane (w == 0) gives a finite, off-screen result")
	{
		for (const float w: {0.0f, -0.0f}) {
			const float3 vp = CameraProjection::ClipToViewport(float4(300.0f, -200.0f, -2.0f, w), VIEWPORT);
			CHECK(std::isfinite(vp.x));
			CHECK(std::isfinite(vp.y));
			CHECK(std::isfinite(vp.z));
			CHECK(OffScreenDepth(vp));
		}
	}

	SECTION("all-zero clip coordinates stay finite")
	{
		const float3 vp = CameraProjection::ClipToViewport(float4(0.0f, 0.0f, 0.0f, 0.0f), VIEWPORT);
		CHECK(std::isfinite(vp.x));
		CHECK(std::isfinite(vp.y));
		CHECK(std::isfinite(vp.z));
	}

	SECTION("a point just behind the camera keeps the sign of w")
	{
		const float3 vp = CameraProjection::ClipToViewport(float4(1.0f, 1.0f, 1.0f, -1e-9f), VIEWPORT);
		CHECK(std::isfinite(vp.z));
		CHECK(OffScreenDepth(vp));
	}
}
