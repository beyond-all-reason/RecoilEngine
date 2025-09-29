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
		sTangent = RgtVector;
		tTangent = float3{};
		texCoords[0] = float2{};
		texCoords[1] = float2{};
		// boneIDs is initialized afterwards
		boneIDsLow  = DEFAULT_BONEIDS_LOW;
		boneWeights = DEFAULT_BONEWEIGHTS;
		boneIDsHigh = DEFAULT_BONEIDS_HIGH;
	}
	SVertexData(
		const float3& p,
		const float3& n,
		const float3& s,
		const float3& t,
		const float2& uv0,
		const float2& uv1)
	{
		pos = p;
		normal = n;
		sTangent = s;
		tTangent = t;
		texCoords[0] = uv0;
		texCoords[1] = uv1;
		// boneIDs is initialized afterwards
		boneIDsLow  = DEFAULT_BONEIDS_LOW;
		boneWeights = DEFAULT_BONEWEIGHTS;
		boneIDsHigh = DEFAULT_BONEIDS_HIGH;
	}

	static constexpr size_t NUM_MODEL_UVCHANNS = 2;
	static constexpr std::array<uint8_t, 4> DEFAULT_BONEIDS_HIGH = { 255, 255, 255, 255 };
	static constexpr std::array<uint8_t, 4> DEFAULT_BONEIDS_LOW = { 255, 255, 255, 255 };
	static constexpr std::array<uint8_t, 4> DEFAULT_BONEWEIGHTS = { 255, 0  ,   0,   0 };
	static constexpr uint16_t INVALID_BONEID = 0xFFFF;
	static constexpr size_t MAX_BONES_PER_VERTEX = 4;

	float3 pos;
	float3 normal;
	float3 sTangent;
	float3 tTangent;
	float2 texCoords[NUM_MODEL_UVCHANNS];

	std::array<uint8_t, MAX_BONES_PER_VERTEX> boneIDsLow;
	std::array<uint8_t, MAX_BONES_PER_VERTEX> boneWeights;
	std::array<uint8_t, MAX_BONES_PER_VERTEX> boneIDsHigh;

	template <Concepts::HasSizeAndData C>
	void SetBones(const C& bi) {
		static_assert(std::is_same_v<typename C::value_type, std::pair<uint16_t, float>>);
		assert(bi.size() >= MAX_BONES_PER_VERTEX);

		boneIDsLow = {
			static_cast<uint8_t>((bi[0].first     ) & 0xFF),
			static_cast<uint8_t>((bi[1].first     ) & 0xFF),
			static_cast<uint8_t>((bi[2].first     ) & 0xFF),
			static_cast<uint8_t>((bi[3].first     ) & 0xFF)
		};

		boneIDsHigh = {
			static_cast<uint8_t>((bi[0].first >> 8) & 0xFF),
			static_cast<uint8_t>((bi[1].first >> 8) & 0xFF),
			static_cast<uint8_t>((bi[2].first >> 8) & 0xFF),
			static_cast<uint8_t>((bi[3].first >> 8) & 0xFF)
		};

		// calc sumWeight to normalize the bone weights to cumulative 1.0f
		float sumWeight = bi[0].second + bi[1].second + bi[2].second + bi[3].second;
		sumWeight = (sumWeight <= 0.0f) ? 1.0f : sumWeight;

		boneWeights = {
			(spring::SafeCast<uint8_t>(math::round(bi[0].second / sumWeight * 255.0f))),
			(spring::SafeCast<uint8_t>(math::round(bi[1].second / sumWeight * 255.0f))),
			(spring::SafeCast<uint8_t>(math::round(bi[2].second / sumWeight * 255.0f))),
			(spring::SafeCast<uint8_t>(math::round(bi[3].second / sumWeight * 255.0f)))
		};


	}

	void TransformBy(const Transform& transform);
};