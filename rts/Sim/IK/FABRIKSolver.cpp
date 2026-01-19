/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FABRIKSolver.hpp"

#include <vector>
#include <deque>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>

#include "Sim/Objects/SolidObject.h"
#include "System/SpringMath.h"
#include "System/ContainerUtil.h"
#include "System/UnorderedMap.hpp"

using namespace IK;


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

	std::vector<ChainSolution> result(chains.size(), ChainSolution(ChainSolution::SolutionKind::FAILED, {}));

	if (result.empty())
		return result;

	const auto& lm = so->localModel;

	// collect all segment lengths
	spring::unordered_map<std::pair<uint32_t, uint32_t>, float> lengths;
	{
		std::deque<uint32_t> queue;
		for (const auto* lmp : lm.GetRoot()->children) {
			queue.emplace_back(lmp->GetLModelPieceIndex());
		}

		while (!queue.empty()) {
			const auto jointID = queue.front();
			queue.pop_front();

			const auto jointIDPar = lm.GetPiece(jointID)->parent->GetLModelPieceIndex();

			const float dist = joints[jointID].worldPos.distance(joints[jointIDPar].worldPos);

			lengths[std::make_pair(jointID, jointIDPar)] = dist;
			lengths[std::make_pair(jointIDPar, jointID)] = dist;

			for (const auto* lmp : lm.GetPiece(jointID)->children) {
				queue.emplace_back(lmp->GetLModelPieceIndex());
			}
		}
	}

	// to figure out shared joints
	std::vector<std::vector<uint32_t>> jointToChains(joints.size());


	for (uint32_t ci = 0; ci < chains.size(); ++ci) {
		auto chSPtr = chains[ci].lock();

		const auto& chainJointIdcs = chSPtr->GetJoints();
		for (size_t i = 0; i < chainJointIdcs.size(); ++i) {
			const auto jointID = chainJointIdcs[i];
			jointToChains[jointID].emplace_back(ci);
		}

		float chainLength = 0.0f;

		for (size_t i = 0; i < chainJointIdcs.size() - 1; ++i) {
			const auto jointID0 = chainJointIdcs[i + 0];
			const auto jointID1 = chainJointIdcs[i + 1];
			chainLength += lengths[std::make_pair(jointID0, jointID1)];
		}

		if (joints[chSPtr->rID].worldPos.distance(joints[chSPtr->eID].worldPos) > chainLength) {
			result[ci].solutionKind = ChainSolution::SolutionKind::STRETCHING;
			// TODO
		}
	}

	return result;
}
