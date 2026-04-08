/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FABRIKSolverMath.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <variant>

#include "System/float3.h"

// World-space constraint application.
// Identical to ApplyConstraint in FABRIKSolver.cpp but constraint axes are
// already in world space — no model↔world conversion is performed.
static float3 ApplyConstraintWorldSpace(
	const IK::Constraint& c,
	float3 dir,
	const float3& restDir)
{
	if (std::holds_alternative<IK::BallJointConstraint>(c)) {
		const auto& bc = std::get<IK::BallJointConstraint>(c);
		if (bc.coneAngle <= 0.0f)
			return dir;

		float3 coneAxisW = bc.coneAxis;
		coneAxisW.SafeNormalize();

		const float cosLimit = std::cos(bc.coneAngle);
		const float cosActual = dir.dot(coneAxisW);
		if (cosActual < cosLimit) {
			float3 perp = dir - coneAxisW * cosActual;
			perp.SafeNormalize();
			dir = coneAxisW * cosLimit + perp * std::sin(bc.coneAngle);
			dir.SafeNormalize();
		}
	}
	else if (std::holds_alternative<IK::HingeJointConstraint>(c)) {
		const auto& hc = std::get<IK::HingeJointConstraint>(c);

		float3 axisW = hc.axis;
		axisW.SafeNormalize();

		float3 dirProj  = dir     - axisW * dir.dot(axisW);
		float3 restProj = restDir - axisW * restDir.dot(axisW);
		dirProj.SafeNormalize();
		restProj.SafeNormalize();

		const float cosA    = std::clamp(dirProj.dot(restProj), -1.0f, 1.0f);
		const float sinSign = restProj.cross(dirProj).dot(axisW) >= 0.0f ? 1.0f : -1.0f;
		float angle = sinSign * std::acos(cosA);

		angle = std::clamp(angle, hc.minAngle, hc.maxAngle);
		dir   = restProj * std::cos(angle) + axisW.cross(restProj) * std::sin(angle);
		dir.SafeNormalize();
	}
	return dir;
}


IK::FABRIKResult IK::SolveFABRIK(
	std::vector<float3>& positions,
	const std::vector<float>& segLengths,
	const std::vector<Constraint>& constraints,
	const float3& bindPoseRootDir,
	const float3& goal,
	uint32_t maxIterations,
	float precision)
{
	const size_t n = positions.size();

	if (n < 2)
		return FABRIKResult::FAILED;

	assert(segLengths.size() == n - 1);
	assert(constraints.empty() || constraints.size() == n);

	const float3 rootPos = positions[0];

	float totalLen = 0.0f;
	for (size_t i = 0; i < n - 1; i++)
		totalLen += segLengths[i];

	if (rootPos.distance(goal) >= totalLen) {
		// Goal is unreachable: stretch chain straight toward goal
		for (size_t i = 0; i < n - 1; i++) {
			float3 dir = (goal - positions[i]);
			dir.SafeNormalize();
			positions[i + 1] = positions[i] + dir * segLengths[i];
		}
		return FABRIKResult::STRETCHING;
	}

	// Rest direction for root joint constraint reference (i=0 has no parent bone)
	float3 bposeRootDir = bindPoseRootDir;
	bposeRootDir.SafeNormalize();

	for (uint32_t iter = 0; iter < maxIterations; iter++) {
		// Forward pass: pull effector to goal, propagate toward root
		positions[n - 1] = goal;
		for (size_t i = n - 1; i-- > 0; ) {
			float3 dir = (positions[i] - positions[i + 1]);
			dir.SafeNormalize();
			positions[i] = positions[i + 1] + dir * segLengths[i];
		}

		// Backward pass: fix root, propagate toward effector with constraints
		positions[0] = rootPos;
		for (size_t i = 0; i < n - 1; i++) {
			float3 dir = (positions[i + 1] - positions[i]);
			dir.SafeNormalize();

			if (!constraints.empty() && !std::holds_alternative<std::monostate>(constraints[i])) {
				float3 restDir = (i > 0) ? (positions[i] - positions[i - 1]) : bposeRootDir;
				restDir.SafeNormalize();
				dir = ApplyConstraintWorldSpace(constraints[i], dir, restDir);
			}

			positions[i + 1] = positions[i] + dir * segLengths[i];
		}

		if (positions[n - 1].distance(goal) < precision)
			break;
	}

	if (positions[n - 1].distance(goal) >= precision)
		return FABRIKResult::FAILED;

	return FABRIKResult::FOUND;
}
