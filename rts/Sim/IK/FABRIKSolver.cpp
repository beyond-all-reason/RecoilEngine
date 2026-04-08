/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FABRIKSolver.hpp"
#include "FABRIKSolverMath.hpp"

#include <vector>
#include <deque>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>

#include "Rendering/Models/3DModelPiece.hpp"
#include "Sim/Objects/SolidObject.h"
#include "System/ContainerUtil.h"
#include "System/float3.h"

using namespace IK;

// Convert a world-space direction vector to model-space, accounting for the
// WORLD_TO_OBJECT_SPACE x-axis flip: world = frontdir*mz - rightdir*mx + updir*my
static float3 WorldDirToModelDir(const float3& wd, const CSolidObject& so)
{
	return float3(
		-wd.dot(so.rightdir),
		 wd.dot(so.updir),
		 wd.dot(so.frontdir)
	);
}

// Inverse of WorldDirToModelDir
static float3 ModelDirToWorldDir(const float3& md, const CSolidObject& so)
{
	return so.frontdir * md.z - so.rightdir * md.x + so.updir * md.y;
}




Chain::Chain(const Skeleton& skeleton, uint32_t rootID, uint32_t effectorID, float chainWeight)
	: skel(&skeleton)
	, rID(rootID)
	, eID(effectorID)
	, eGoal()
	, weight(chainWeight)
{
	const auto& lm = skel->GetSolidObject()->localModel;
	const auto& skelJoints = skel->GetJoints();

	auto currID = eID;
	jointIdcs.emplace_back(currID);

	while (currID < skelJoints.size() && currID != rID) {
		currID = lm.pieces[currID].parent ? lm.pieces[currID].parent->GetLModelPieceIndex() : uint32_t(-1);
		jointIdcs.emplace_back(currID);
	}

	// order rID --> eID
	std::reverse(jointIdcs.begin(), jointIdcs.end());

	// path to the rID from the eID doesn't exist
	if (currID != rID) {
		jointIdcs = {};
		throw std::logic_error("EffectorID and RootID don't belong to the same hierarchy");
	}
}

Skeleton::Skeleton(const CSolidObject& solidObject)
	: so{ &solidObject }
{
	const auto& lm = so->localModel;

	if (lm.pieces.size() <= 2)
		throw std::logic_error("Invalid skeleton size");

	joints.reserve(lm.pieces.size());

	for (const auto& piece : lm.pieces) {
		auto& joint = joints.emplace_back();
		joint.piece = &piece;
	}
	UpdateAllJoints();
}

bool Skeleton::SetJointConstraint(uint32_t jointID, const Constraint& constraint)
{
	if (jointID >= joints.size())
		return false;

	joints[jointID].constraint = constraint;
	return true;
}

void Skeleton::UpdateJointHierarchy(uint32_t jointID)
{
	const auto& lm = so->localModel;

	std::deque<uint32_t> queue;
	queue.push_back(jointID);

	while (!queue.empty()) {
		jointID = queue.front();
		queue.pop_front();

		UpdateJoint(jointID);

		for (const auto* lmp : lm.pieces[jointID].children) {
			queue.push_back(lmp->GetLModelPieceIndex());
		}
	}
}

void Skeleton::UpdateJoint(uint32_t jointID)
{
	assert(jointID < joints.size());

	auto& joint = joints[jointID];
	const auto modelPos = joint.piece->GetAbsolutePos();
	joint.worldPos = so->GetObjectSpacePos(modelPos);
}

void Skeleton::UpdateAllJoints()
{
	for (uint32_t jointID = 0; jointID < joints.size(); ++jointID) {
		UpdateJoint(jointID);
	}
}

std::shared_ptr<Chain> Skeleton::CreateChain(uint32_t effectorID, uint32_t rootID, float chainWeight)
{
	const auto& lm = so->localModel;
	auto currID = effectorID;

	while (currID < joints.size() && currID != rootID) {
		currID = lm.pieces[currID].parent ? lm.pieces[currID].parent->GetLModelPieceIndex() : uint32_t(-1);
	}

	// path to the rootID from the effectorID doesn't exist
	if (currID != rootID)
		return std::shared_ptr<Chain>(nullptr);

	try {
		return chains.emplace_back(std::make_shared<Chain>(
			*this,
			rootID,
			effectorID,
			chainWeight
		)).lock();
	}
	catch (std::logic_error&) {
		return std::shared_ptr<Chain>(nullptr);
	}
}

std::vector<ChainSolution> Skeleton::SolveAllChains(uint32_t maxIterations, float precision)
{
	// remove chains expired somewhere else
	spring::VectorEraseIfAll(chains, [](const auto& chWPtr) { return chWPtr.expired(); });

	std::vector<ChainSolution> result;
	result.reserve(chains.size());

	for (const auto& chWPtr : chains) {
		auto ch = chWPtr.lock();
		const auto& ji = ch->GetJoints(); // root-to-effector joint indices
		const size_t n = ji.size();

		if (n < 2) {
			result.emplace_back(ChainSolution::SolutionKind::FAILED, std::vector<std::pair<int, float3>>{});
			continue;
		}

		// Collect current world positions
		std::vector<float3> pos(n);
		for (size_t i = 0; i < n; i++)
			pos[i] = joints[ji[i]].worldPos;

		// Segment lengths are taken from the current (possibly animated) pose and
		// held constant throughout solving. This preserves whatever bone lengths the
		// animation system has already set rather than reverting to bind-pose lengths.
		std::vector<float> segLen(n - 1);
		for (size_t i = 0; i < n - 1; i++)
			segLen[i] = pos[i + 1].distance(pos[i]);

		const float3 goal    = ch->GetGoal();
		const float3 rootPos = pos[0];

		// Bind-pose direction of the root bone in world space — used as the hinge reference
		// for the root joint (i=0) only, since it has no parent bone in the chain.
		float3 bposeRootDir = (joints[ji[1]].piece->original->bposeTransform.t
		                     - joints[ji[0]].piece->original->bposeTransform.t);
		bposeRootDir.SafeNormalize();
		bposeRootDir = ModelDirToWorldDir(bposeRootDir, *so);
		bposeRootDir.SafeNormalize();

		// Convert constraints into world space for the math solver
		std::vector<Constraint> worldConstraints(n);
		for (size_t i = 0; i < n; i++) {
			worldConstraints[i] = joints[ji[i]].constraint;
			if (auto* bc = std::get_if<BallJointConstraint>(&worldConstraints[i])) {
				bc->coneAxis = ModelDirToWorldDir(bc->coneAxis, *so);
				bc->coneAxis.SafeNormalize();
			} else if (auto* hc = std::get_if<HingeJointConstraint>(&worldConstraints[i])) {
				hc->axis = ModelDirToWorldDir(hc->axis, *so);
				hc->axis.SafeNormalize();
			}
		}

		IK::FABRIKResult resultCode = IK::SolveFABRIK(pos, segLen, worldConstraints, bposeRootDir, goal, maxIterations, precision);
		ChainSolution::SolutionKind kind;

		switch (resultCode) {
			case IK::FABRIKResult::STRETCHING: kind = ChainSolution::SolutionKind::STRETCHING; break;
			case IK::FABRIKResult::FOUND:      kind = ChainSolution::SolutionKind::FOUND;      break;
			case IK::FABRIKResult::FAILED:     kind = ChainSolution::SolutionKind::FAILED;     break;
			default:                           kind = ChainSolution::SolutionKind::FAILED;     break;
		}

		// Convert solved world positions to piece-local YPR rotations.
		//
		// For each bone (joint[i] -> joint[i+1]), we find the rotation at joint[i]
		// that orients the bone toward the solved direction.
		//
		// The bpose (bind-pose) model-space transform of each piece gives:
		//   bposeRot = accumulated model-space rotation with no script rotation
		// The script rotation (what SetRotation sets) is applied on top of bposeRot:
		//   modelRot = bposeRot * scriptRot
		//
		// After FABRIK we want modelRot_new = delta * bposeRot, where delta rotates
		// the rest bone direction to the solved bone direction in model space.
		// Therefore: scriptRot_new = bposeRot^-1 * delta * bposeRot
		std::vector<std::pair<int, float3>> solution;
		solution.reserve(n - 1);

		for (size_t i = 0; i < n - 1; i++) {
			const auto& bposeTra     = joints[ji[i    ]].piece->original->bposeTransform;
			const auto& bposeTraNext = joints[ji[i + 1]].piece->original->bposeTransform;

			// Bind-pose bone direction in model space
			float3 bposeBoneDir = (bposeTraNext.t - bposeTra.t);
			bposeBoneDir.SafeNormalize();

			// Solved bone direction: world space -> model space
			float3 solvedWorldDir = (pos[i + 1] - pos[i]);
			solvedWorldDir.SafeNormalize();
			float3 solvedModelDir = WorldDirToModelDir(solvedWorldDir, *so);
			solvedModelDir.SafeNormalize();

			// Delta rotation in model space: bposeBoneDir -> solvedModelDir
			const CQuaternion delta       = CQuaternion::MakeFrom(bposeBoneDir, solvedModelDir);
			const CQuaternion newModelRot = (delta * bposeTra.r).Normalize();
			CQuaternion scriptRot         = (bposeTra.r.InverseNormalized() * newModelRot).Normalize();

			// Blend toward identity by chain weight (weight=0 → no IK, weight=1 → full IK)
			if (ch->weight < 1.0f)
				scriptRot = CQuaternion::SLerp(CQuaternion{}, scriptRot, ch->weight).Normalize();

			solution.emplace_back(static_cast<int>(ji[i]), scriptRot.ToEulerYPR());
		}

		result.emplace_back(kind, std::move(solution));
	}

	return result;
}
