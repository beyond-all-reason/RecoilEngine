#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <tuple>
#include <type_traits>

#include "System/TemplateUtils.hpp"
#include "System/Transform.hpp"
#include "System/type2.h"
#include "System/SafeUtil.h"

struct SVertexData {
	SVertexData() {
		pos = float3{};
		normal = UpVector;
		tangent = RgtVector;
		texCoords[0] = float2{};
		texCoords[1] = float2{};
		// boneIDs is initialized afterwards
		boneIDs = DEFAULT_BONEIDS;
		boneWeights = DEFAULT_BONEWEIGHTS;
	}
	SVertexData(
		const float3& p,
		const float3& n,
		const float3& s,
		const float4& t,
		const float2& uv0,
		const float2& uv1)
	{
		pos = p;
		normal = n;
		tangent = s;
		texCoords[0] = uv0;
		texCoords[1] = uv1;
		// boneIDs is initialized afterwards
		boneIDs = DEFAULT_BONEIDS;
		boneWeights = DEFAULT_BONEWEIGHTS;
	}

	static constexpr size_t NUM_MODEL_UVCHANNS = 2;
	static constexpr std::array<uint16_t, 4> DEFAULT_BONEIDS = { 0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu };
	static constexpr std::array<uint8_t, 4> DEFAULT_BONEWEIGHTS = { 255, 0, 0, 0 };
	static constexpr uint16_t INVALID_BONEID = 0xFFFF;
	static constexpr size_t MAX_BONES_PER_VERTEX = 4;

	float3 pos;
	float3 normal;
	float4 tangent;
	float2 texCoords[NUM_MODEL_UVCHANNS];

	// 4 bone IDs (matches shader uvec3 layout: uvec3.x = bone0|bone1, uvec3.y = bone2|bone3)
	std::array<uint16_t, 4> boneIDs;
	std::array<uint8_t, 4> boneWeights;

	template <Concepts::HasSizeAndData C>
	void SetBones(const C& bi) {
		static_assert(std::is_same_v<typename C::value_type, std::pair<uint16_t, float>>);
		assert(bi.size() >= MAX_BONES_PER_VERTEX);

		// Store bone IDs as individual uint16_t values
		boneIDs = {
			bi[0].first,
			bi[1].first,
			bi[2].first,
			bi[3].first
		};

#if 0
		// calc sumWeight to normalize the bone weights to cumulative 1.0f
		float sumWeight = bi[0].second + bi[1].second + bi[2].second + bi[3].second;
		sumWeight = (sumWeight <= 0.0f) ? 1.0f : sumWeight;
#else
		static constexpr float sumWeight = 1.0f;
#endif
		boneWeights = {
			(spring::SafeCast<uint8_t>(math::round(bi[0].second / sumWeight * 255.0f))),
			(spring::SafeCast<uint8_t>(math::round(bi[1].second / sumWeight * 255.0f))),
			(spring::SafeCast<uint8_t>(math::round(bi[2].second / sumWeight * 255.0f))),
			(spring::SafeCast<uint8_t>(math::round(bi[3].second / sumWeight * 255.0f)))
		};
	}

	void TransformBy(const Transform& transform);
	SVertexData TransformBy(const Transform& transform) const;
};