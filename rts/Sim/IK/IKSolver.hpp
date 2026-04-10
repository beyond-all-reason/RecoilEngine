/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>
#include <cstdint>
#include <memory>

#include "System/creg/creg_cond.h"
#include "System/Quaternion.h"
#include "Sim/IK/IKTypes.hpp"
#include "Sim/IK/IKSolverMath.hpp"

class CSolidObject;
struct LocalModelPiece;

namespace IK {

	struct Joint {
		const LocalModelPiece* piece;	// Direct reference to LocalModelPiece
		Constraint constraint;			// Joint constraint (ball, hinge, or none)
		float3 worldPos;				// Cached current world position
	};

	class Skeleton;

	class Chain {
	public:
		Chain(const Skeleton& skeleton, std::vector<uint32_t> indices, float chainWeight);
		void SetGoal(const float3& effectorGoal) { eGoal = effectorGoal; }
		const auto& GetGoal() const { return eGoal; }
		const std::vector<uint32_t>& GetJoints() const { return jointIdcs; }
		const std::vector<float>& GetBoneLengths() const { return boneLengths; }
		void SetSolver(const IIKSolver* ikSolver);
		const IIKSolver* GetSolver() const { return solver; }
		const Skeleton* GetSkeleton() const { return skel; }
	public:
		const uint32_t rID;
		const uint32_t eID;
		float weight = 0.0f;
	private:
		const Skeleton* skel = nullptr;
		const IIKSolver* solver = nullptr;
		std::vector<uint32_t> jointIdcs;
		std::vector<float> boneLengths;
		float3 eGoal; // in world space
	};

	struct ChainSolution {
		Result solutionKind;
		std::vector<std::pair<int, float3>> solution; // joint/piece id and YPR angles
		std::vector<CQuaternion> rotations;           // script-relative rotation (relative to bind pose)
	};

	const char* ResultToString(Result result);

	class Skeleton {
	public:
		Skeleton(const CSolidObject& solidObject);

		bool SetJointConstraint(uint32_t jointID, const Constraint& constraint);
		void UpdateJointHierarchy(uint32_t jointID);
		void UpdateJoint(uint32_t jointID);
		void UpdateAllJoints();
		std::shared_ptr<Chain> CreateChain(uint32_t effectorID, uint32_t rootID = 0, float chainWeight = 1.0f);
		std::vector<ChainSolution> SolveAllChains(uint32_t maxIterations = 10, float precision = 1.0f);
		ChainSolution SolveChain(const std::shared_ptr<Chain>& chain, uint32_t maxIterations = 10, float precision = 1.0f, int skipCount = 0);
		void ApplySolution(const Chain& chain, const ChainSolution& sol, int skipCount = 0);
		const auto* GetSolidObject() const { return so; }
		const auto& GetJoints() const { return joints; }
	private:
		float3 WorldDirToModelDir(const float3& wd) const;
		float3 ModelDirToWorldDir(const float3& md) const;
	private:
		const CSolidObject* so = nullptr;
		std::vector<Joint> joints;
		std::vector<std::weak_ptr<Chain>> chains;
	};
};
