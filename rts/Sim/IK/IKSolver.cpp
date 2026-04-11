/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IKSolver.hpp"
#include "IKSolverMath.hpp"

#include <vector>
#include <algorithm>
#include <cassert>
#include <cmath>

#include "Rendering/Models/3DModelPiece.hpp"
#include "Rendering/Models/LocalModelPiece.hpp"
#include "Sim/Objects/SolidObject.h"
#include "System/ContainerUtil.h"
#include "System/float3.h"
#include "System/Log/ILog.h"

using namespace IK;

const char* IK::ResultToString(Result result) {
	switch (result) {
		case Result::FOUND: return "found";
		case Result::STRETCHING: return "stretching";
		case Result::FAILED: return "failed";
		default: return "unknown";
	}
}

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
	, solver(&IK::GetFABRIKSolver())
	, jointIdcs(std::move(indices))
{
	const auto& skelJoints = skel->GetJoints();
	const size_t n = jointIdcs.size();
	boneLengths.resize(n > 0 ? n - 1 : 0);

	for (size_t i = 0; i + 1 < n; ++i) {
		boneLengths[i] = skelJoints[jointIdcs[i]].piece->original->bposeTransform.t.distance(
			skelJoints[jointIdcs[i + 1]].piece->original->bposeTransform.t);
	}
}

void Chain::SetSolver(const IIKSolver* ikSolver)
{
	solver = (ikSolver != nullptr) ? ikSolver : &IK::GetFABRIKSolver();
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

bool Skeleton::SetJointProperties(uint32_t jointID, bool canRotate, bool canMove)
{
	if (jointID >= joints.size())
		return false;

	joints[jointID].canRotate = canRotate;
	joints[jointID].canMove = canMove;
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

	// By default, the root of a chain is fixed in position but can rotate.
	if (!indices.empty()) {
		joints[indices.front()].canMove = false;
		joints[indices.front()].canRotate = true;
	}

	return chains.emplace_back(std::make_shared<Chain>(
		*this,
		std::move(indices),
		chainWeight
	)).lock();
}

std::vector<ChainSolution> Skeleton::SolveAllChains(uint32_t maxIterations, float precision)
{
	UpdateAllJoints();

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

void Skeleton::ApplySolution(const Chain& chain, const ChainSolution& sol)
{
	if (sol.solution.empty())
		return;

	const auto& ji = chain.GetJoints();

	LOG_L(L_DEBUG, "IK-DIAG: === ApplySolution root=%u eff=%u solSize=%u ===",
		chain.rID, chain.eID, unsigned(sol.solution.size()));

	for (size_t i = 0; i < sol.solution.size(); ++i) {
		const auto pieceIdx = sol.solution[i].first;
		const auto& ypr = sol.solution[i].second;
		const auto* piece = joints[ji[i]].piece;

		if (!joints[ji[i]].canRotate) {
			LOG_L(L_DEBUG, "IK-DIAG: apply[%u] piece=%d LOCKED (rotating=false)", unsigned(i), pieceIdx);
			continue;
		}

		const_cast<LocalModelPiece*>(piece)->SetRotation(ypr);
	}
}

ChainSolution Skeleton::SolveChain(const std::shared_ptr<Chain>& ch, uint32_t maxIterations, float precision)
{
	if (!ch) {
		LOG_L(L_WARNING, "IK::SolveChain: null chain");
		return { Result::ERR_INPUTS, {}, {} };
	}

	UpdateAllJoints();

	const auto& ji = ch->GetJoints();
	const size_t n = ji.size();

	if (n < 2) {
		LOG_L(L_WARNING, "IK::SolveChain: invalid chain size=%u root=%u effector=%u", unsigned(n), ch->rID, ch->eID);
		return { Result::ERR_INPUTS, {}, {} };
	}

	const size_t numBones = n - 1;

	const float3 rootWorldPos = joints[ji[0]].worldPos;
	const float3 goalModel = WorldDirToModelDir(ch->GetGoal() - rootWorldPos);

	LOG_L(L_DEBUG, "IK-DIAG: === SolveChain root=%u eff=%u joints=%u ===", ch->rID, ch->eID, unsigned(n));

	std::vector<Bone> chain(numBones);
	for (size_t i = 0; i < numBones; i++) {
		const uint32_t jIdx = ji[i];
		const uint32_t nextJIdx = ji[i + 1];

		chain[i].length = ch->GetBoneLengths()[i];
		chain[i].constraint = joints[jIdx].constraint;
		chain[i].canRotate = joints[jIdx].canRotate;
		chain[i].canMove = joints[jIdx].canMove;

		const float3 worldDir = (joints[nextJIdx].worldPos - joints[jIdx].worldPos);
		const float3 currDirModel = WorldDirToModelDir(worldDir).SafeNormalize();
		chain[i].orientation = MakeOrientationFromBoneDir(currDirModel);

		LOG_L(L_DEBUG, "IK-DIAG: bone[%u] ji=%u->%u len=%.3f canRot=%d canMove=%d",
			unsigned(i), jIdx, nextJIdx, chain[i].length, chain[i].canRotate, chain[i].canMove);
	}

	assert(ch->GetSolver() != nullptr);
	uint32_t iters = 0;
	auto resultCode = ch->GetSolver()->Solve(chain, goalModel, maxIterations, precision, &iters);

	ChainSolution cs;
	cs.solutionKind = resultCode;
	cs.iterations = iters;

	const auto calcEffectorPos = [&](const std::vector<Bone>& bones) {
		float3 p = ZeroVector;
		for (const auto& b : bones)
			p += BoneDirFromOrientation(b.orientation) * b.length;
		return p;
	};
	const float3 solvedEffPos = calcEffectorPos(chain);
	float postSolveDistance = solvedEffPos.distance(goalModel);

	LOG_L(L_DEBUG, "IK-DIAG: solver status=%s postDist=%.3f solvedEffModel=(%.2f,%.2f,%.2f) goalModel=(%.2f,%.2f,%.2f)",
		ResultToString(resultCode), postSolveDistance,
		solvedEffPos.x, solvedEffPos.y, solvedEffPos.z,
		goalModel.x, goalModel.y, goalModel.z);

	cs.solution.reserve(numBones);
	cs.rotations.reserve(numBones);
	std::vector<CQuaternion> desiredModelRots(numBones, CQuaternion{});

	for (size_t i = 0; i < numBones; i++) {
		const auto& bposeTra     = joints[ji[i    ]].piece->original->bposeTransform;
		const auto& bposeTraNext = joints[ji[i + 1]].piece->original->bposeTransform;

		const float3 bposeBoneDir = (bposeTraNext.t - bposeTra.t).SafeNormalize();
		const float3 solvedDirModel = BoneDirFromOrientation(chain[i].orientation);

		const CQuaternion delta = CQuaternion::MakeFrom(bposeBoneDir, solvedDirModel);
		desiredModelRots[i] = (delta * bposeTra.r).Normalize();

		const CQuaternion parentModelRot = [&]() -> CQuaternion {
			if (i > 0)
				return desiredModelRots[i - 1];

			const auto* piece = joints[ji[i]].piece;
			const auto* parent = piece->parent;
			if (parent != nullptr)
				return parent->GetModelSpaceTransform().r;
			return CQuaternion{};
		}();

		const CQuaternion bakedLocalRot = joints[ji[i]].piece->original->ComposeTransform(ZeroVector, ZeroVector, 1.0f).r;

		// For locked (skipped) bones, emit identity rotation so they stay unchanged
		CQuaternion scriptRot = (
			bakedLocalRot.InverseNormalized() *
			(parentModelRot.InverseNormalized() * desiredModelRots[i]).Normalize()
		).Normalize();

		if (ch->weight < 1.0f)
			scriptRot = CQuaternion::SLerp(CQuaternion{}, scriptRot, ch->weight).Normalize();

		// Swing-twist decomposition: remove twist (roll) around the bone's
		// own longitudinal axis.  The bone rest direction in the local frame
		// (where scriptRot operates) is the bpose bone direction transformed
		// out of model space into the piece's pre-scriptRot frame.
		const CQuaternion preScriptFrame = (parentModelRot * bakedLocalRot).Normalize();
		const float3 boneRestDirLocal = preScriptFrame.InverseNormalized().Rotate(bposeBoneDir);
		const float3 desiredDirLocal  = scriptRot.Rotate(boneRestDirLocal);
		if (boneRestDirLocal.SqDistance(desiredDirLocal) > float3::apx_eps())
			scriptRot = CQuaternion::MakeFrom(boneRestDirLocal, desiredDirLocal).Normalize();
		else
			scriptRot = CQuaternion{};

		const float3 ypr = scriptRot.ToEulerYPR();

		LOG_L(L_DEBUG, "IK-DIAG: convert bone[%u] piece=%u", unsigned(i), ji[i]);
		LOG_L(L_DEBUG, "IK-DIAG:   bposeBoneDir=(%.3f,%.3f,%.3f) solvedDir=(%.3f,%.3f,%.3f)",
			bposeBoneDir.x, bposeBoneDir.y, bposeBoneDir.z,
			solvedDirModel.x, solvedDirModel.y, solvedDirModel.z);
		LOG_L(L_DEBUG, "IK-DIAG:   bposeRot=(%.3f,%.3f,%.3f,%.3f) delta=(%.3f,%.3f,%.3f,%.3f) desiredModel=(%.3f,%.3f,%.3f,%.3f)",
			bposeTra.r.x, bposeTra.r.y, bposeTra.r.z, bposeTra.r.r,
			delta.x, delta.y, delta.z, delta.r,
			desiredModelRots[i].x, desiredModelRots[i].y, desiredModelRots[i].z, desiredModelRots[i].r);
		LOG_L(L_DEBUG, "IK-DIAG:   parentModelRot=(%.3f,%.3f,%.3f,%.3f) bakedLocal=(%.3f,%.3f,%.3f,%.3f) scriptRot=(%.3f,%.3f,%.3f,%.3f)",
			parentModelRot.x, parentModelRot.y, parentModelRot.z, parentModelRot.r,
			bakedLocalRot.x, bakedLocalRot.y, bakedLocalRot.z, bakedLocalRot.r,
			scriptRot.x, scriptRot.y, scriptRot.z, scriptRot.r);
		LOG_L(L_DEBUG, "IK-DIAG:   ypr=(%.4f,%.4f,%.4f)",
			ypr.x, ypr.y, ypr.z);

		cs.solution.emplace_back(static_cast<int>(ji[i]), ypr);
		cs.rotations.push_back(scriptRot);
	}

	if (resultCode == Result::FAILED) {
		LOG_L(L_WARNING, "IK::SolveChain: root=%u effector=%u joints=%u status=%s postDist=%.3f goal=(%.1f, %.1f, %.1f) iters=%u eps=%.3f",
			ch->rID, ch->eID, unsigned(n), ResultToString(resultCode),
			postSolveDistance,
			ch->GetGoal().x, ch->GetGoal().y, ch->GetGoal().z,
			maxIterations, precision);
	} else {
		LOG_L(L_DEBUG, "IK::SolveChain: root=%u effector=%u joints=%u status=%s postDist=%.3f goal=(%.1f, %.1f, %.1f) iters=%u eps=%.3f",
			ch->rID, ch->eID, unsigned(n), ResultToString(resultCode),
			postSolveDistance,
			ch->GetGoal().x, ch->GetGoal().y, ch->GetGoal().z,
			maxIterations, precision);
	}

	return cs;
}
