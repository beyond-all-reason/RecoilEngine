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
		, pageNum(0)
	{}
	AtlasedTexture(float x_, float y_, float z_, float w_, uint32_t pageNum_)
		: x(x_)
		, y(y_)
		, z(z_)
		, w(w_)
		, pageNum(pageNum_)
	{}
	AtlasedTexture(const float4& f, uint32_t pageNum_)
		: x(f.x)
		, y(f.y)
		, z(f.z)
		, w(f.w)
		, pageNum(pageNum_)
	{}

	explicit operator float4() const {
		return float4(x + pageNum, y, z, w);
	}

	bool operator==(const AtlasedTexture& rhs) const {
		if (this == &rhs)
			return true;

		if (pageNum != rhs.pageNum)
			return false;

		return static_cast<float4>(*this) == static_cast<float4>(rhs);
	}
	bool operator!=(const AtlasedTexture& rhs) const { return !(*this == rhs); }

	union {
		struct { float x, y, z, w; };
		struct { float x1, y1, x2, y2; };
		struct { float s, t, p, q; };
		struct { float xstart, ystart, xend, yend; };
	};
	uint32_t pageNum;

	static const AtlasedTexture& DefaultAtlasTexture;
};

struct AtlasedTextureHash {
	uint32_t operator()(const AtlasedTexture& at) const {
		return spring::LiteHash(&at, sizeof(at));
	}
};