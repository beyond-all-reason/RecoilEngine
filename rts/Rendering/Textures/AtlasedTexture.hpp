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

struct AtlasedTextureLayered : public AtlasedTexture {
	CR_DECLARE_STRUCT(AtlasedTextureLayered)
	explicit AtlasedTextureLayered()
		: AtlasedTexture()
		, pageNum(0)
	{}
	AtlasedTextureLayered(const AtlasedTexture& other)
		: AtlasedTexture(other.x1, other.y1, other.x2, other.y2)
		, pageNum(0)
	{}

	bool operator==(const AtlasedTextureLayered& rhs) const {
		if (this == &rhs)
			return true;

		if (!AtlasedTexture::operator==(rhs))
			return false;

		return pageNum == rhs.pageNum;
	}
	bool operator!=(const AtlasedTextureLayered& rhs) const { return !(*this == rhs); }

	uint32_t pageNum;
	static const AtlasedTextureLayered& DefaultAtlasTextureLayered;
};

struct AtlasedTextureHash {
	uint32_t operator()(const AtlasedTexture& at) const {
		return spring::LiteHash(&at, sizeof(at));
	}
};

struct AtlasedTextureLayeredHash {
	uint32_t operator()(const AtlasedTextureLayered& at) const {
		return spring::LiteHash(&at, sizeof(at));
	}
};