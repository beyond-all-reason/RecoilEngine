#pragma once

#include <cstdint>

#include "System/float4.h"
#include "System/creg/creg_cond.h"

/** @brief texture coordinates of an atlas subimage. */
struct AtlasedTexture
{
	CR_DECLARE_STRUCT(AtlasedTexture)

	explicit AtlasedTexture()
		: x(0.0f)
		, y(0.0f)
		, z(0.0f)
		, w(0.0f)
	{}
	explicit AtlasedTexture(float x_, float y_, float z_, float w_, uint32_t page_ = 0)
		: x(x_)
		, y(y_)
		, z(z_)
		, w(w_)
	{}
	explicit AtlasedTexture(const float4& f, uint32_t page_ = 0)
		: x(f.x)
		, y(f.y)
		, z(f.z)
		, w(f.w)
	{}
/*
	AtlasedTexture(AtlasedTexture&& f) noexcept { *this = std::move(f); }
	AtlasedTexture& operator= (AtlasedTexture&& f) = default;

	AtlasedTexture(const AtlasedTexture& f) { *this = f; }
	AtlasedTexture& operator= (const AtlasedTexture& f) = default;
*/
	bool operator==(const AtlasedTexture& rhs) const {
		if (this == &rhs)
			return true;

		float4 f0(x    , y    , z    , w    );
		float4 f1(rhs.x, rhs.y, rhs.z, rhs.w);
		return f0 == f1;
	}
	bool operator!=(const AtlasedTexture& rhs) const { return !(*this == rhs); }

	union {
		struct { float x, y, z, w; };
		struct { float x1, y1, x2, y2; };
		struct { float s, t, p, q; };
		struct { float xstart, ystart, xend, yend; };
	};

	static const AtlasedTexture& DefaultAtlasTexture;
};
