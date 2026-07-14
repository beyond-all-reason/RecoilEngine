#include "3DModelAnimation.hpp"

#include <algorithm>
#include <cassert>

void ModelAnimation::Map::FinalizeAnimations()
{
	const auto IsEmptySequence = [](ModelAnimation::Sequence& seq) {
		static const auto IsEmpty = []<typename T>(const ModelAnimation::TypedSequence<T>& ts) {
			return ts.timeFrames.empty();
		};

		static const auto RemoveDefault = []<typename T>(ModelAnimation::TypedSequence<T>& ts) -> void {
			if constexpr (std::is_same_v<T, float3>) {
				static constexpr auto def = float3();
				if (std::all_of(ts.values.begin(), ts.values.end(), [](const auto& val) { return val == def; })) {
					ts.timeFrames.clear();
					ts.values.clear();
					ts.inTangents.clear();
					ts.outTangents.clear();
				}
			}
			else if constexpr (std::is_same_v<T, CQuaternion>) {
				static constexpr auto def = CQuaternion();
				if (std::all_of(ts.values.begin(), ts.values.end(), [](const auto& val) { return val == def; })) {
					ts.timeFrames.clear();
					ts.values.clear();
					ts.inTangents.clear();
					ts.outTangents.clear();
				}
			}
			else if constexpr (std::is_same_v<T, float>) {
				static constexpr auto def = 1.0f;
				if (std::all_of(ts.values.begin(), ts.values.end(), [](const auto& val) { return val == def; })) {
					ts.timeFrames.clear();
					ts.values.clear();
					ts.inTangents.clear();
					ts.outTangents.clear();
				}
			}
		};

		std::apply([&](auto&... elems) {
			(RemoveDefault(elems), ...);
		}, seq);

		return std::apply([&](const auto&... elems) {
			return (IsEmpty(elems) && ...);
		}, seq);
	};

	for (auto& entry : animationMap) {
		for (auto it = entry.pieceData.begin(); it != entry.pieceData.end(); ) {
			if (IsEmptySequence(it->second))
				it = entry.pieceData.erase(it);
			else
				++it;
		}
	}

	animationMap.erase(
		std::remove_if(animationMap.begin(), animationMap.end(), [](const AnimationEntry& e) { return e.pieceData.empty(); }), animationMap.end()
	);

	for (auto& entry : animationMap) {
		entry.duration = 0.0f;
		for (const auto& [pieceIdx, seq] : entry.pieceData) {
			const auto& [tSeq, rSeq, sSeq] = seq;
			if (!tSeq.timeFrames.empty()) entry.duration = std::max(entry.duration, tSeq.timeFrames.back());
			if (!rSeq.timeFrames.empty()) entry.duration = std::max(entry.duration, rSeq.timeFrames.back());
			if (!sSeq.timeFrames.empty()) entry.duration = std::max(entry.duration, sSeq.timeFrames.back());
		}
	}
}

static size_t FindKeyframeBracket(const std::vector<float>& timeFrames, float time)
{
	assert(!timeFrames.empty());
	if (time <= timeFrames.front())
		return 0;
	if (time >= timeFrames.back())
		return timeFrames.size() - 1;

	// upper_bound gives first element > time; step back one to get lower frame
	auto it = std::upper_bound(timeFrames.begin(), timeFrames.end(), time);
	return static_cast<size_t>(std::distance(timeFrames.begin(), it)) - 1;
}

namespace Impl {
	auto GetCubicSplineCoefficients(float alpha) {
		const float a2 = alpha * alpha, a3 = a2 * alpha;
		const float h00 =  2*a3 - 3*a2 + 1, h10 = a3 - 2*a2 + alpha;
		const float h01 = -2*a3 + 3*a2    , h11 = a3 - a2;
		return std::make_tuple(h00, h10, h01, h11);
	}
}

template<>
float3 ModelAnimation::SampleSequence(const TypedSequence<float3>& seq, float time)
{
	if (seq.timeFrames.empty())
		return float3{};

	const size_t i = FindKeyframeBracket(seq.timeFrames, time);
	if (seq.interpolation == Interpolation::Step || i + 1 >= seq.timeFrames.size())
		return seq.values[i];

	const float t0 = seq.timeFrames[i];
	const float t1 = seq.timeFrames[i + 1];
	const float alpha = (t1 > t0) ? (time - t0) / (t1 - t0) : 0.0f;

	if (seq.interpolation == Interpolation::CubicSpline) {
		const float dt = t1 - t0;
		const auto [h00, h10, h01, h11] = Impl::GetCubicSplineCoefficients(alpha);
		return h00 * seq.values[i + 0] + h10 * dt * seq.outTangents[i]
		     + h01 * seq.values[i + 1] + h11 * dt * seq.inTangents[i+1];
	}

	const float3& v0 = seq.values[i];
	const float3& v1 = seq.values[i + 1];
	return v0 + (v1 - v0) * alpha;
}


template<>
CQuaternion ModelAnimation::SampleSequence(const TypedSequence<CQuaternion>& seq, float time)
{
	if (seq.timeFrames.empty())
		return CQuaternion{};

	const size_t i = FindKeyframeBracket(seq.timeFrames, time);
	if (seq.interpolation == Interpolation::Step || i + 1 >= seq.timeFrames.size())
		return seq.values[i];

	const float t0 = seq.timeFrames[i];
	const float t1 = seq.timeFrames[i + 1];
	const float alpha = (t1 > t0) ? (time - t0) / (t1 - t0) : 0.0f;

	if (seq.interpolation == Interpolation::CubicSpline) {
		// Hermite cubic on quaternions treated as 4-vectors, then normalize (GLTF spec)
		const float dt = t1 - t0;
		const auto [h00, h10, h01, h11] = Impl::GetCubicSplineCoefficients(alpha);
		const auto& p0  = seq.values[i];       const auto& p1  = seq.values[i+1];
		const auto& ot0 = seq.outTangents[i];  const auto& it1 = seq.inTangents[i+1];
		CQuaternion result(
			h00*p0.x + h10*dt*ot0.x + h01*p1.x + h11*dt*it1.x,
			h00*p0.y + h10*dt*ot0.y + h01*p1.y + h11*dt*it1.y,
			h00*p0.z + h10*dt*ot0.z + h01*p1.z + h11*dt*it1.z,
			h00*p0.r + h10*dt*ot0.r + h01*p1.r + h11*dt*it1.r
		);
		result.Normalize();
		return result;
	}

	return CQuaternion::SLerp(seq.values[i], seq.values[i + 1], alpha);
}


template<>
float ModelAnimation::SampleSequence(const TypedSequence<float>& seq, float time)
{
	if (seq.timeFrames.empty())
		return 1.0f; // default scale is 1, not 0

	const size_t i = FindKeyframeBracket(seq.timeFrames, time);
	if (seq.interpolation == Interpolation::Step || i + 1 >= seq.timeFrames.size())
		return seq.values[i];

	const float t0 = seq.timeFrames[i];
	const float t1 = seq.timeFrames[i + 1];
	const float alpha = (t1 > t0) ? (time - t0) / (t1 - t0) : 0.0f;

	if (seq.interpolation == Interpolation::CubicSpline) {
		const float dt = t1 - t0;
		const auto [h00, h10, h01, h11] = Impl::GetCubicSplineCoefficients(alpha);
		return h00 * seq.values[i + 0] + h10 * dt * seq.outTangents[i]
		     + h01 * seq.values[i + 1] + h11 * dt * seq.inTangents[i+1];
	}

	return seq.values[i] + (seq.values[i + 1] - seq.values[i]) * alpha;
}
