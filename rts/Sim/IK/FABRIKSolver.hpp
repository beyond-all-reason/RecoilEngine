/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>
#include <variant>
#include <cstdint>
#include <memory>

#include "System/creg/creg_cond.h"
#include "System/float3.h"
#include "System/Quaternion.h"

class CSolidObject;
struct LocalModelPiece;

namespace IK {
	struct BallJointConstraint {
		float3 coneAxis = float3(0.0f, 1.0f, 0.0f);
		float coneAngle = 0.0f;
	};

	struct HingeJointConstraint {
		float3 axis = float3(0.0f, 1.0f, 0.0f);
		float minAngle = 0.0f;
		float maxAngle = 0.0f;
	};

	using Constraint = std::variant<std::monostate, BallJointConstraint, HingeJointConstraint>;

	struct Joint {
		const LocalModelPiece* piece;	// Direct reference to LocalModelPiece
		Constraint constraint;			// Joint constraint (ball, hinge, or none)
		float3 worldPos;				// Cached current world position
	};

	class Skeleton;

	class Chain {
	public:
		Chain(const Skeleton& skeleton, uint32_t rootID, uint32_t effectorID, float chainWeight);
		void SetGoal(const float3& effectorGoal) { eGoal = effectorGoal; }
		const auto& GetGoal() const { return eGoal; }
		const auto& GetJoints() const { return jointIdcs; }
	public:
		const uint32_t rID;
		const uint32_t eID;
		float weight = 0.0f;
	private:
		const Skeleton* skel = nullptr;
		std::vector<uint32_t> jointIdcs;
		float3 eGoal; // in world space
	};

	struct ChainSolution {
		enum class SolutionKind {
			FOUND = 0,
			STRETCHING = 1,
			FAILED = 2
		};
		SolutionKind solutionKind;
		std::vector<std::pair<int, float3>> solution; // joint/piece id and YPR angles
	};

	class Skeleton {
	public:
		Skeleton(const CSolidObject& solidObject);
		bool SetJointConstraint(uint32_t jointID, const Constraint& constraint);
		void UpdateJointHierarchy(uint32_t jointID);
		void UpdateJoint(uint32_t jointID);
		void UpdateAllJoints();
		std::shared_ptr<Chain> CreateChain(uint32_t effectorID, uint32_t rootID = 0, float chainWeight = 1.0f);
		std::vector<ChainSolution> SolveAllChains(uint32_t maxIterations = 10, float precision = 1.0f);
		const auto* GetSolidObject() const { return so; }
		const auto& GetJoints() const { return joints; }
	private:
		const CSolidObject* so = nullptr;
		std::vector<Joint> joints;
		std::vector<std::weak_ptr<Chain>> chains;
	};
};
