/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IKSolver.hpp"
#include "IKSolverMath.hpp"

#include <vector>
#include <algorithm>
#include <cassert>
#include <cmath>

#include "Rendering/Models/3DModelPiece.hpp"
#include "Sim/Objects/SolidObject.h"
#include "System/ContainerUtil.h"
#include "System/float3.h"

using namespace IK;

float3 Skeleton::WorldDirToModelDir(const float3& wd) const
{
	return float3(
		-wd.dot(so->rightdir),
		 wd.dot(so->updir),
		 wd.dot(so->frontdir)
	);
}

float3 Skeleton::ModelDirToWorldDir(const float3& md) const
{
	return so->frontdir * md.z - so->rightdir * md.x + so->updir * md.y;
}

Chain::Chain(const Skeleton& skeleton, std::vector<uint32_t> indices, float chainWeight)
	: rID(indices.front())
	, eID(indices.back())
	, weight(chainWeight)
	, skel(&skeleton)
	, jointIdcs(std::move(indices))
{
	const auto& skelJoints = skel->GetJoints();
	const size_t n = jointIdcs.size();
	boneLengths.resize(n > 0 ? n - 1 : 0);

	for (size_t i = 0; i + 1 < n; ++i) {
		boneLengths[i] = skelJoints[jointIdcs[i]].piece->GetAbsolutePos().distance(
			skelJoints[jointIdcs[i + 1]].piece->GetAbsolutePos());
	}
}

Skeleton::Skeleton(const CSolidObject& solidObject)
	: so{ &solidObject }
	, solver{ &IK::GetFABRIKSolver() }
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

void Skeleton::SetSolver(const IIKSolver* ikSolver)
{
	solver = (ikSolver != nullptr) ? ikSolver : &IK::GetFABRIKSolver();
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

	std::vector<uint32_t> queue;
	queue.push_back(jointID);

	for (size_t head = 0; head < queue.size(); ++head) {
		const auto id = queue[head];
		UpdateJoint(id);

		for (const auto* lmp : lm.pieces[id].children) {
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

	std::vector<uint32_t> indices;
	indices.emplace_back(effectorID);

	auto currID = effectorID;
	while (currID < joints.size() && currID != rootID) {
		currID = lm.pieces[currID].parent ? lm.pieces[currID].parent->GetLModelPieceIndex() : uint32_t(-1);
		if (currID < joints.size())
			indices.emplace_back(currID);
	}

	if (currID != rootID)
		return nullptr;

	std::reverse(indices.begin(), indices.end());

	return chains.emplace_back(std::make_shared<Chain>(
		*this,
		std::move(indices),
		chainWeight
	)).lock();
}

std::vector<ChainSolution> Skeleton::SolveAllChains(uint32_t maxIterations, float precision)
{
	// remove chains expired somewhere else
	spring::VectorEraseIfAll(chains, [](const auto& chWPtr) { return chWPtr.expired(); });

	std::vector<ChainSolution> result;
	result.reserve(chains.size());

	for (const auto& chWPtr : chains) {
		if (auto ch = chWPtr.lock()) {
			result.emplace_back(SolveChain(ch, maxIterations, precision));
		}
	}

	return result;
}

ChainSolution Skeleton::SolveChain(const std::shared_ptr<Chain>& ch, uint32_t maxIterations, float precision)
{
	if (!ch)
		return { FABRIKResult::ERR_INPUTS, {} };

	const auto& ji = ch->GetJoints();
	const size_t n = ji.size();

	if (n < 2)
		return { FABRIKResult::ERR_INPUTS, {} };

	// Preparations for the redesigned SolveFABRIK:
	// 1. Goal relative to root joint in model space
	const float3 rootWorldPos = joints[ji[0]].worldPos;
	const float3 goalModel = WorldDirToModelDir(ch->GetGoal() - rootWorldPos);

	// 2. Bone hierarchy with local-space constraints and model-space orientations.
	// The math solver and wrapper conversions share kBoneRestAxis.
	std::vector<Bone> chain(n - 1);
	for (size_t i = 0; i < n - 1; i++) {
		const uint32_t jIdx = ji[i];
		const uint32_t nextJIdx = ji[i + 1];

		chain[i].length = ch->GetBoneLengths()[i];
		chain[i].constraint = joints[jIdx].constraint;

		// Initial orientation rotates kBoneRestAxis to current bone direction in model space
		const float3 currDirModel = WorldDirToModelDir(joints[nextJIdx].worldPos - joints[jIdx].worldPos).SafeNormalize();
		chain[i].orientation = MakeOrientationFromBoneDir(currDirModel);
	}

	assert(solver != nullptr);
	const FABRIKResult resultCode = solver->Solve(chain, goalModel, maxIterations, precision);

	// Convert solved orientations back to piece-local YPR rotations.
	std::vector<std::pair<int, float3>> solution;
	solution.reserve(n - 1);

	for (size_t i = 0; i < n - 1; i++) {
		const auto& bposeTra     = joints[ji[i    ]].piece->original->bposeTransform;
		const auto& bposeTraNext = joints[ji[i + 1]].piece->original->bposeTransform;

		// Bind-pose bone direction in model space
		const float3 bposeBoneDir = (bposeTraNext.t - bposeTra.t).SafeNormalize();

		// Solved bone direction in model space
		const float3 solvedDirModel = BoneDirFromOrientation(chain[i].orientation);

		// Compute final script rotation relative to bind-pose
		const CQuaternion delta       = CQuaternion::MakeFrom(bposeBoneDir, solvedDirModel);
		const CQuaternion newModelRot = (delta * bposeTra.r).Normalize();
		CQuaternion scriptRot         = (bposeTra.r.InverseNormalized() * newModelRot).Normalize();

		if (ch->weight < 1.0f)
			scriptRot = CQuaternion::SLerp(CQuaternion{}, scriptRot, ch->weight).Normalize();

		solution.emplace_back(static_cast<int>(ji[i]), scriptRot.ToEulerYPR());
	}

	return { resultCode, std::move(solution) };
}
