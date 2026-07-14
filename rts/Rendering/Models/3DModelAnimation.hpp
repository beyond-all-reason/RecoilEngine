#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>
#include <algorithm>

#include "System/UnorderedMap.hpp"
#include "System/float3.h"
#include "System/Quaternion.h"

namespace ModelAnimation {
	enum class Interpolation : uint8_t {
		Linear = 0,
		Step = 1,
		CubicSpline = 2
	};

	template<typename T>
	struct TypedSequence {
		std::vector<float> timeFrames;
		std::vector<T> values;
		// only populated for CubicSpline interpolation
		std::vector<T> inTangents;
		std::vector<T> outTangents;
		Interpolation interpolation = Interpolation::Linear;
	};

	using Sequence = std::tuple<
		TypedSequence<float3>,
		TypedSequence<CQuaternion>,
		TypedSequence<float>
	>;

	using PieceInfoDataMap = spring::unordered_map<size_t, Sequence>;

	struct AnimationEntry {
		std::string      name;
		float            duration = 0.0f;
		PieceInfoDataMap pieceData;
	};

	class Map {
	public:
		size_t GetOrAddAnimation(const std::string& name) {
			auto it = std::find_if(animationMap.begin(), animationMap.end(), [&name](const AnimationEntry& entry) { return entry.name == name; });

			if (it != animationMap.end())
				return static_cast<size_t>(std::distance(animationMap.begin(), it));

			const size_t id = animationMap.size();
			animationMap.emplace_back(name, 0.0f, PieceInfoDataMap{});
			return id;
		}

		template<typename T>
		TypedSequence<T>& GetPieceAnimationVectors(size_t id, size_t pieceIdx) {
			assert(id < animationMap.size());
			return std::get<TypedSequence<T>>(animationMap[id].pieceData[pieceIdx]);
		}

		// Const access to piece sequences for sampling. Returns nullptr if not found.
		template<typename T>
		const TypedSequence<T>* GetPieceAnimationVectors(size_t id, size_t pieceIdx) const {
			if (id >= animationMap.size())
				return nullptr;

			auto pieceIt = animationMap[id].pieceData.find(pieceIdx);
			if (pieceIt == animationMap[id].pieceData.end())
				return nullptr;

			return &std::get<TypedSequence<T>>(pieceIt->second);
		}

		// Removes animations/sequences whose data is all-default, compacts the vector
		void FinalizeAnimations();

		// Returns clip ID, or size_t(-1) if not found.
		size_t GetAnimationId(const std::string& name) const {
			auto it = std::find_if(animationMap.begin(), animationMap.end(), [&name](const AnimationEntry& entry) { return entry.name == name; });
			return (it != animationMap.end()) ? static_cast<size_t>(std::distance(animationMap.begin(), it)) : static_cast<size_t>(-1);
		}

		const std::string& GetAnimationName(size_t id) const {
			return animationMap[id].name;
		}

		bool HasAnimation(const std::string& name) const {
			auto it = std::find_if(animationMap.begin(), animationMap.end(), [&name](const AnimationEntry& entry) { return entry.name == name; });
			return it != animationMap.end();
		}

		bool HasAnimation(size_t id) const {
			return id < animationMap.size();
		}

		float GetAnimationDuration(const std::string& name) const {
			const size_t id = GetAnimationId(name);
			return (id != static_cast<size_t>(-1)) ? animationMap[id].duration : 0.0f;
		}

		float GetAnimationDuration(size_t id) const {
			return HasAnimation(id) ? animationMap[id].duration : 0.0f;
		}

		AnimationEntry& operator[](size_t id) {
			return animationMap[id];
		}

		const AnimationEntry& operator[](size_t id) const {
			return animationMap[id];
		}

		auto cbegin() const { return animationMap.cbegin(); }
		auto   cend() const { return animationMap.cend(); }
		size_t size() const { return animationMap.size(); }
		bool  empty() const { return animationMap.empty(); }

	private:
		std::vector<AnimationEntry> animationMap;
	};

	template<typename T>
	T SampleSequence(const TypedSequence<T>& seq, float time);
}