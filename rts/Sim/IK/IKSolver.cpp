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

void Skeleton::ApplySolution(const Chain& chain, const ChainSolution& sol, int skipCount)
{
	if (sol.solution.empty())
		return;

	const auto& ji = chain.GetJoints();

	LOG_L(L_DEBUG, "IK-DIAG: === ApplySolution root=%u eff=%u skip=%d solSize=%u ===",
		chain.rID, chain.eID, skipCount, unsigned(sol.solution.size()));

	for (size_t i = 0; i < sol.solution.size(); ++i) {
		const auto pieceIdx = sol.solution[i].first;
		const auto& ypr = sol.solution[i].second;
		const auto* piece = joints[ji[i]].piece;
		const float3 oldRot = piece->GetRotation();

		if (static_cast<int>(i) < skipCount) {
			LOG_L(L_DEBUG, "IK-DIAG: apply[%u] piece=%d SKIPPED oldRot=(%.4f,%.4f,%.4f) ypr=(%.4f,%.4f,%.4f)",
				unsigned(i), pieceIdx, oldRot.x, oldRot.y, oldRot.z, ypr.x, ypr.y, ypr.z);
			continue;
		}

		LOG_L(L_DEBUG, "IK-DIAG: apply[%u] piece=%d oldRot=(%.4f,%.4f,%.4f) newYpr=(%.4f,%.4f,%.4f)",
			unsigned(i), pieceIdx, oldRot.x, oldRot.y, oldRot.z, ypr.x, ypr.y, ypr.z);

		const_cast<LocalModelPiece*>(piece)->SetRotation(ypr);
	}

	// Force transform update and log resulting world positions
	for (size_t i = 0; i < ji.size(); ++i) {
		const auto* piece = joints[ji[i]].piece;
		const auto modelTra = piece->GetModelSpaceTransform();
		const float3 absPos = piece->GetAbsolutePos();
		const float3 worldPos = so->GetObjectSpacePos(absPos);

		LOG_L(L_DEBUG, "IK-DIAG: postApply piece=%u modelT=(%.2f,%.2f,%.2f) modelR=(%.3f,%.3f,%.3f,%.3f) absPos=(%.2f,%.2f,%.2f) worldPos=(%.2f,%.2f,%.2f)",
			ji[i],
			modelTra.t.x, modelTra.t.y, modelTra.t.z,
			modelTra.r.x, modelTra.r.y, modelTra.r.z, modelTra.r.r,
			absPos.x, absPos.y, absPos.z,
			worldPos.x, worldPos.y, worldPos.z);
	}
}

ChainSolution Skeleton::SolveChain(const std::shared_ptr<Chain>& ch, uint32_t maxIterations, float precision, int skipCount)
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
	const size_t skip = std::min(static_cast<size_t>(std::max(skipCount, 0)), numBones);

	const float3 effectorWorldPos = joints[ji.back()].worldPos;
	const float preSolveDistance = effectorWorldPos.distance(ch->GetGoal());

	const float3 rootWorldPos = joints[ji[0]].worldPos;
	const float3 goalModel = WorldDirToModelDir(ch->GetGoal() - rootWorldPos);

	LOG_L(L_DEBUG, "IK-DIAG: === SolveChain root=%u eff=%u joints=%u skip=%u ===", ch->rID, ch->eID, unsigned(n), unsigned(skip));
	LOG_L(L_DEBUG, "IK-DIAG: unitPos=(%.2f,%.2f,%.2f) front=(%.3f,%.3f,%.3f) right=(%.3f,%.3f,%.3f) up=(%.3f,%.3f,%.3f)",
		static_cast<float>(so->pos.x), static_cast<float>(so->pos.y), static_cast<float>(so->pos.z),
		static_cast<float>(so->frontdir.x), static_cast<float>(so->frontdir.y), static_cast<float>(so->frontdir.z),
		static_cast<float>(so->rightdir.x), static_cast<float>(so->rightdir.y), static_cast<float>(so->rightdir.z),
		static_cast<float>(so->updir.x), static_cast<float>(so->updir.y), static_cast<float>(so->updir.z));
	LOG_L(L_DEBUG, "IK-DIAG: goalWorld=(%.2f,%.2f,%.2f) rootWorld=(%.2f,%.2f,%.2f) goalModel=(%.2f,%.2f,%.2f)",
		ch->GetGoal().x, ch->GetGoal().y, ch->GetGoal().z,
		rootWorldPos.x, rootWorldPos.y, rootWorldPos.z,
		goalModel.x, goalModel.y, goalModel.z);

	std::vector<Bone> chain(numBones);
	for (size_t i = 0; i < numBones; i++) {
		const uint32_t jIdx = ji[i];
		const uint32_t nextJIdx = ji[i + 1];

		chain[i].length = ch->GetBoneLengths()[i];
		chain[i].constraint = joints[jIdx].constraint;

		const float3 worldDir = (joints[nextJIdx].worldPos - joints[jIdx].worldPos);
		const float3 currDirModel = WorldDirToModelDir(worldDir).SafeNormalize();
		chain[i].orientation = MakeOrientationFromBoneDir(currDirModel);

		const auto& bpT    = joints[jIdx].piece->original->bposeTransform;
		const auto& bpTN   = joints[ji[i+1]].piece->original->bposeTransform;
		const float3 bpDir = (bpTN.t - bpT.t).SafeNormalize();

		LOG_L(L_DEBUG, "IK-DIAG: bone[%u] ji=%u->%u len=%.3f worldDir=(%.2f,%.2f,%.2f) modelDir=(%.3f,%.3f,%.3f) bposeDir=(%.3f,%.3f,%.3f) bposeT=(%.2f,%.2f,%.2f) bposeTnext=(%.2f,%.2f,%.2f)%s",
			unsigned(i), jIdx, nextJIdx, chain[i].length,
			worldDir.x, worldDir.y, worldDir.z,
			currDirModel.x, currDirModel.y, currDirModel.z,
			bpDir.x, bpDir.y, bpDir.z,
			bpT.t.x, bpT.t.y, bpT.t.z,
			bpTN.t.x, bpTN.t.y, bpTN.t.z,
			(i < skip) ? " [LOCKED]" : "");
	}

	assert(ch->GetSolver() != nullptr);
	const auto calcEffectorPos = [&](const std::vector<Bone>& bones) {
		float3 p = ZeroVector;
		for (const auto& b : bones)
			p += BoneDirFromOrientation(b.orientation) * b.length;
		return p;
	};

	// Solve with ALL bones for full reach, regardless of skipCount.
	Result resultCode = ch->GetSolver()->Solve(chain, goalModel, maxIterations, precision);

	// After solving, analytically lock the first `skip` bones to their
	// bind-pose directions while recomputing the first free bone so the
	// end-effector stays at (or near) the solved position.
	if (skip > 0 && skip < numBones) {
		const float3 solvedFootPos = calcEffectorPos(chain);

		float3 lockedOffset = ZeroVector;
		for (size_t i = 0; i < skip; ++i) {
			const float3 bpDir = (joints[ji[i + 1]].piece->original->bposeTransform.t
				- joints[ji[i]].piece->original->bposeTransform.t).SafeNormalize();
			chain[i].orientation = MakeOrientationFromBoneDir(bpDir);
			lockedOffset += bpDir * chain[i].length;
		}

		// Accumulate the tail offset from bones after the first free bone
		float3 tailOffset = ZeroVector;
		for (size_t i = skip + 1; i < numBones; ++i)
			tailOffset += BoneDirFromOrientation(chain[i].orientation) * chain[i].length;

		// Recompute the first free bone to bridge the gap
		float3 remaining = solvedFootPos - lockedOffset - tailOffset;
		if (remaining.SqLength() > float3::apx_eps())
			chain[skip].orientation = MakeOrientationFromBoneDir(remaining.SafeNormalize());

		LOG_L(L_DEBUG, "IK-DIAG: post-lock lockedOff=(%.2f,%.2f,%.2f) tailOff=(%.2f,%.2f,%.2f) remaining=(%.2f,%.2f,%.2f)",
			lockedOffset.x, lockedOffset.y, lockedOffset.z,
			tailOffset.x, tailOffset.y, tailOffset.z,
			remaining.x, remaining.y, remaining.z);
	}

	const float3 solvedEffPos = calcEffectorPos(chain);
	float postSolveDistance = solvedEffPos.distance(goalModel);

	LOG_L(L_DEBUG, "IK-DIAG: solver status=%s postDist=%.3f solvedEffModel=(%.2f,%.2f,%.2f) goalModel=(%.2f,%.2f,%.2f)",
		ResultToString(resultCode), postSolveDistance,
		solvedEffPos.x, solvedEffPos.y, solvedEffPos.z,
		goalModel.x, goalModel.y, goalModel.z);

	for (size_t i = 0; i < numBones; i++) {
		const float3 solvedDir = BoneDirFromOrientation(chain[i].orientation);
		LOG_L(L_DEBUG, "IK-DIAG: solved bone[%u] dir=(%.3f,%.3f,%.3f)%s", unsigned(i), solvedDir.x, solvedDir.y, solvedDir.z,
			(i < skip) ? " [LOCKED]" : "");
	}

	ChainSolution cs;
	cs.solutionKind = resultCode;
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
		CQuaternion scriptRot;
		if (i < skip) {
			scriptRot = CQuaternion{};
		} else {
			scriptRot = (
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
		}

		const float3 ypr = scriptRot.ToEulerYPR();

		LOG_L(L_DEBUG, "IK-DIAG: convert bone[%u] piece=%u%s", unsigned(i), ji[i], (i < skip) ? " [LOCKED]" : "");
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
		LOG_L(L_WARNING, "IK::SolveChain: root=%u effector=%u joints=%u status=%s preDist=%.3f postDist=%.3f goal=(%.1f, %.1f, %.1f) iters=%u eps=%.3f skip=%u",
			ch->rID, ch->eID, unsigned(n), ResultToString(resultCode),
			preSolveDistance, postSolveDistance,
			ch->GetGoal().x, ch->GetGoal().y, ch->GetGoal().z,
			maxIterations, precision, unsigned(skip));
	} else {
		LOG_L(L_DEBUG, "IK::SolveChain: root=%u effector=%u joints=%u status=%s preDist=%.3f postDist=%.3f goal=(%.1f, %.1f, %.1f) iters=%u eps=%.3f skip=%u",
			ch->rID, ch->eID, unsigned(n), ResultToString(resultCode),
			preSolveDistance, postSolveDistance,
			ch->GetGoal().x, ch->GetGoal().y, ch->GetGoal().z,
			maxIterations, precision, unsigned(skip));
	}

	return cs;
}
