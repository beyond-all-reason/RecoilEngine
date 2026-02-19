/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <algorithm>
#include <functional>
#include "System/float3.h"
#include "System/type2.h"
#include "System/Matrix44f.h"
#include "System/TemplateUtils.hpp"

struct AABB {
public:
	static bool RangeOverlap(const float2& a, const float2& b) {
		uint8_t n = 0;

		n += (b.x >= a.x && b.x <= a.y);
		n += (b.y >= a.x && b.y <= a.y);
		n += (a.x >= b.x && a.x <= b.y);
		n += (a.y >= b.x && a.y <= b.y);

		return (n > 0);
	}

	void Combine(const AABB& other) {
		mins = float3::min(mins, other.mins);
		maxs = float3::max(maxs, other.maxs);
	}

	bool Intersects(const AABB& b) const {
		uint8_t n = 0;

		n += RangeOverlap({mins.x, maxs.x}, {b.mins.x, b.maxs.x});
		n += RangeOverlap({mins.y, maxs.y}, {b.mins.y, b.maxs.y});
		n += RangeOverlap({mins.z, maxs.z}, {b.mins.z, b.maxs.z});

		return (n == 3);
	}

	bool Contains(const float3& p) const {
		uint8_t n = 0;

		n += (p.x >= mins.x && p.x <= maxs.x);
		n += (p.y >= mins.y && p.y <= maxs.y);
		n += (p.z >= mins.z && p.z <= maxs.z);

		return (n == 3);
	};

	float3 ClampInto(const float3& pnt) const;

	void CalcCorners(std::array<float3, 8>& verts) const;

	template<Concepts::CanTransform T>
	void CalcCorners(const T& t, std::array<float3, 8>& verts) const {
		CalcCorners(verts);
		std::for_each(verts.begin(), verts.end(), [&t](auto& vert) { vert = t * vert; });
	}

	template<Concepts::CanTransform T>
	AABB CalcTransformed(const T& t) const {
		AABB ret;
		ret.AddPoints(GetCorners(t));
		return ret;
	}

	template<Concepts::CanTransform T>
	std::array<float3, 8> GetCorners(const T& t) const {
		std::array<float3, 8> corners;
		CalcCorners(t, corners);
		return corners;
	}

	std::array<float3, 8> GetCorners() const {
		std::array<float3, 8> corners;
		CalcCorners(corners);
		return corners;
	}

	float3 CalcCenter(const CMatrix44f& mat) const { return (mat * CalcCenter()); }
	float3 CalcCenter() const { return ((maxs + mins) * 0.5f); }
	float3 CalcScales() const { return ((maxs - mins) * 0.5f); }

	float CalcRadiusSq() const { return (CalcScales().SqLength()); }
	float CalcRadius() const { return (CalcScales().Length()); }

	float3 GetVertexP(const float3& normal) const;
	float3 GetVertexN(const float3& normal) const;

	void AddPoint(const float3& p) {
		mins = float3::min(mins, p);
		maxs = float3::max(maxs, p);
	}

	template<Concepts::HasMemberBeginEnd T>
	void AddPoints(const T& t) {
		std::for_each(t.begin(), t.end(), std::bind_front(&AABB::AddPoint, this));
	}

	void Reset() {
		mins = DEF_MINS;
		maxs = DEF_MAXS;
	}

	bool IsReset() const {
		return (mins == DEF_MINS) && (maxs == DEF_MAXS);
	}
public:
	float3 mins = DEF_MINS;
	float3 maxs = DEF_MAXS;
	static constexpr auto DEF_MINS = float3{ std::numeric_limits<float>::max()    };
	static constexpr auto DEF_MAXS = float3{ std::numeric_limits<float>::lowest() };
};