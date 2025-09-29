#pragma once

#include <vector>
#include <cstdint>
#include "System/float3.h"

struct S3DModelPiecePart {
public:
	struct RenderData {
		float3 dir;
		uint32_t indexStart;
		uint32_t indexCount;
	};

	static const int SHATTER_MAX_PARTS  = 10;
	static const int SHATTER_VARIATIONS = 2;

	std::vector<RenderData> renderData;
};

struct S3DModelHelpers {
	static void BindLegacyAttrVBOs();
	static void UnbindLegacyAttrVBOs();
};