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
#include "Map/Ground.h"

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

	// Capture bind-pose positions before any script-driven transforms are applied.
	// GetAbsolutePos() returns engine model-space coordinates (X negated by WORLD_TO_OBJECT_SPACE).
	// These are the actual positions the engine uses for rendering.
	for (auto& joint : joints) {
		joint.bindPosePos = joint.piece->GetAbsolutePos();
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

bool Skeleton::SetJointAlignment(uint32_t jointID, AlignMode mode, const float3& localAxis, const float3& customDir)
{
	if (jointID >= joints.size())
		return false;

	joints[jointID].alignMode = mode;
	joints[jointID].alignAxis = localAxis;
	joints[jointID].alignCustomDir = customDir;
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
	// Use bind-pose model-space positions to avoid feedback loop.
	// Live positions change each frame from previous IK rotations, causing oscillation.
	joint.worldPos = joint.bindPosePos;
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

	LOG_L(L_NOTICE, "IK-DIAG: === ApplySolution root=%u eff=%u solSize=%u ===",
		chain.rID, chain.eID, unsigned(sol.solution.size()));

	for (size_t i = 0; i < sol.solution.size(); ++i) {
		const auto pieceIdx = sol.solution[i].first;
		const auto& ypr = sol.solution[i].second;
		const auto* piece = joints[ji[i]].piece;

		if (!joints[ji[i]].canRotate) {
			LOG_L(L_NOTICE, "IK-DIAG: apply[%u] piece=%d LOCKED (rotating=false)", unsigned(i), pieceIdx);
			continue;
		}

		LOG_L(L_NOTICE, "IK-DIAG: apply[%u] piece=%d ypr=(%.4f,%.4f,%.4f)", unsigned(i), pieceIdx, ypr.x, ypr.y, ypr.z);

		const_cast<LocalModelPiece*>(piece)->SetRotation(ypr);
	}

	const auto& effectorJoint = joints[chain.eID];
	if (effectorJoint.alignMode != AlignMode::NONE) {
		const auto* effPiece = effectorJoint.piece;

		const auto& effTransform = effPiece->GetModelSpaceTransform();
		const float3 effWorldPos = so->pos + ModelDirToWorldDir(effTransform.t);

		float3 targetDirWorld;
		if (effectorJoint.alignMode == AlignMode::TERRAIN) {
			targetDirWorld = CGround::GetNormal(effWorldPos.x, effWorldPos.z);
		} else {
			targetDirWorld = effectorJoint.alignCustomDir;
		}

		const float3 tnIntModel = WorldDirToModelDir(targetDirWorld);

		const CQuaternion pieceModelRot = effTransform.r;
		const float3 currentAxis = pieceModelRot.Rotate(effectorJoint.alignAxis);
		const CQuaternion deltaRot = CQuaternion::MakeFrom(currentAxis, tnIntModel);
		const CQuaternion desiredModelRot = (deltaRot * pieceModelRot).Normalize();

		const CQuaternion parentModelRot = effPiece->parent
			? effPiece->parent->GetModelSpaceTransform().r
			: CQuaternion{};
		const CQuaternion bakedLocalRot = effPiece->original->ComposeTransform(ZeroVector, ZeroVector, 1.0f).r;
		const CQuaternion scriptRot = (bakedLocalRot.InverseNormalized() * (parentModelRot.InverseNormalized() * desiredModelRot)).Normalize();

		const float3 ypr = scriptRot.ToEulerYPR();
		const_cast<LocalModelPiece*>(effPiece)->SetRotation(ypr);

		LOG_L(L_NOTICE, "IK-DIAG: joint-align eff=%u mode=%s target=(%.3f,%.3f,%.3f) tnModel=(%.3f,%.3f,%.3f) ypr=(%.4f,%.4f,%.4f)",
			chain.eID,
			effectorJoint.alignMode == AlignMode::TERRAIN ? "terrain" : "custom",
			targetDirWorld.x, targetDirWorld.y, targetDirWorld.z,
			tnIntModel.x, tnIntModel.y, tnIntModel.z,
			ypr.x, ypr.y, ypr.z);
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

	// Joint positions are in engine model space (GetAbsolutePos, X negated by WORLD_TO_OBJECT_SPACE).
	// WorldDirToModelDir gives negated X, so negate X again to match engine model space.
	const float3 rootModelPos = joints[ji[0]].worldPos;
	const float3 goalWorldDelta = ch->GetGoal() - so->pos;
	const float3 wdm = WorldDirToModelDir(goalWorldDelta);
	const float3 goalModelFull = float3(-wdm.x, wdm.y, wdm.z);
	const float3 goalModel = goalModelFull - rootModelPos;

	// Diagnostic: compare goal in model space with effector's bind-pose position
	const float3 effBindPos = joints[ji[numBones]].bindPosePos;

	LOG_L(L_NOTICE, "IK-DIAG: === SolveChain root=%u eff=%u goalWorld=(%.1f,%.1f,%.1f) goalModelFull=(%.1f,%.1f,%.1f) effBindPose=(%.1f,%.1f,%.1f) rootModel=(%.1f,%.1f,%.1f) goalModel=(%.1f,%.1f,%.1f) ===",
		ch->rID, ch->eID,
		ch->GetGoal().x, ch->GetGoal().y, ch->GetGoal().z,
		goalModelFull.x, goalModelFull.y, goalModelFull.z,
		effBindPos.x, effBindPos.y, effBindPos.z,
		rootModelPos.x, rootModelPos.y, rootModelPos.z,
		goalModel.x, goalModel.y, goalModel.z);

	std::vector<Bone> chain(numBones);
	for (size_t i = 0; i < numBones; i++) {
		const uint32_t jIdx = ji[i];
		const uint32_t nextJIdx = ji[i + 1];

		chain[i].length = ch->GetBoneLengths()[i];
		chain[i].constraint = joints[jIdx].constraint;
		chain[i].canRotate = joints[jIdx].canRotate;
		chain[i].canMove = joints[jIdx].canMove;

		// Both joint positions are in model space (bind-pose).
		// The direction between them is already in model space — no conversion needed.
		const float3 boneDirModel = (joints[nextJIdx].worldPos - joints[jIdx].worldPos).SafeNormalize();
		chain[i].orientation = MakeOrientationFromBoneDir(boneDirModel);

		LOG_L(L_NOTICE, "IK-DIAG: bone[%u] ji=%u->%u len=%.3f canRot=%d canMove=%d boneDir=(%.3f,%.3f,%.3f)",
			unsigned(i), jIdx, nextJIdx, chain[i].length, chain[i].canRotate, chain[i].canMove,
			boneDirModel.x, boneDirModel.y, boneDirModel.z);

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

	LOG_L(L_NOTICE, "IK-DIAG: solver status=%s postDist=%.3f iters=%u solvedEffModel=(%.2f,%.2f,%.2f) goalModel=(%.2f,%.2f,%.2f)",
		ResultToString(resultCode), postSolveDistance, iters,
		solvedEffPos.x, solvedEffPos.y, solvedEffPos.z,
		goalModel.x, goalModel.y, goalModel.z);

	cs.solution.reserve(numBones);
	cs.rotations.reserve(numBones);
	std::vector<CQuaternion> desiredModelRots(numBones, CQuaternion{});

	for (size_t i = 0; i < numBones; i++) {
		const auto& bposeTra     = joints[ji[i    ]].piece->original->bposeTransform;
		const auto& bposeTraNext = joints[ji[i + 1]].piece->original->bposeTransform;

		const float3 bposeBoneDir = (bposeTraNext.t - bposeTra.t).SafeNormalize();
		// Solver bone direction is in engine model space (from GetAbsolutePos-based positions).
		// bposeBoneDir is in internal model space. Negate X to convert to internal space.
		const float3 solvedDirModel = BoneDirFromOrientation(chain[i].orientation);
		const float3 solvedDirInternal = float3(-solvedDirModel.x, solvedDirModel.y, solvedDirModel.z);

		const CQuaternion delta = CQuaternion::MakeFrom(bposeBoneDir, solvedDirInternal);
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
		// own longitudinal axis.  The axis is the bone rest direction in the
		// local frame where scriptRot operates.
		const CQuaternion preScriptFrame = (parentModelRot * bakedLocalRot).Normalize();
		const float3 boneRestDirLocal = preScriptFrame.InverseNormalized().Rotate(bposeBoneDir);
		const auto [swing, twist] = CQuaternion::SwingTwist(scriptRot, boneRestDirLocal);
		(void)twist;
		scriptRot = swing;

		const float3 ypr = scriptRot.ToEulerYPR();

		LOG_L(L_NOTICE, "IK-DIAG: bone[%u] piece=%u canRot=%d swing=(%.3f,%.3f,%.3f,%.3f) twist=(%.3f,%.3f,%.3f,%.3f) ypr=(%.4f,%.4f,%.4f)",
			unsigned(i), ji[i], joints[ji[i]].canRotate,
			swing.x, swing.y, swing.z, swing.r,
			twist.x, twist.y, twist.z, twist.r,
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
