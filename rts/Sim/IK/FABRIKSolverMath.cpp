/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FABRIKSolverMath.hpp"

#include <algorithm>
#include <cassert>
#include <variant>

#include "System/float3.h"
#include "System/SpringMath.h"

static float3 ApplyHingeConstraint(const float3& boneDir, const float3& restDirNormalized, const IK::HingeJointConstraint& hc)
{
	float3 axisW = hc.axis;
	axisW.SafeNormalize();

	// Preserve the component along the hinge axis for 3D realism
	const float axialComponent = boneDir.dot(axisW);
	float3 dirProj  = boneDir - axisW * axialComponent;
	float3 restProj = restDirNormalized - axisW * restDirNormalized.dot(axisW);

	// Fallback if restProj is zero (parent is parallel to hinge axis)
	if (restProj.SqLength() < 1e-4f) {
		float3 alt = (std::abs(axisW.x) < 0.9f) ? RgtVector : UpVector;
		restProj = axisW.cross(alt);
	}
	restProj.SafeNormalize();

	// If dirProj is zero, the bone is pointing along the axis; pick any valid start in plane
	if (dirProj.SqLength() < 1e-4f) {
		dirProj = restProj;
	} else {
		dirProj.Normalize();
	}

	const float cosA    = std::clamp(dirProj.dot(restProj), -1.0f, 1.0f);
	const float sinSign = restProj.cross(dirProj).dot(axisW) >= 0.0f ? 1.0f : -1.0f;
	const float angle   = std::clamp(sinSign * math::acos(cosA), std::min(hc.minAngle, hc.maxAngle), std::max(hc.minAngle, hc.maxAngle));

	float3 result = restProj * math::cos(angle) + axisW.cross(restProj) * math::sin(angle);
	// Result is already unit-length and orthogonal to axisW
	return result;
}


// World-space constraint application.
// Constraint axes are already in world space — no model<=>world conversion is performed.
static float3 ApplyConstraintWorldSpace(
	const IK::Constraint& c,
	float3 dir,
	const float3& restDirNormalized)
{
	if (std::holds_alternative<IK::BallJointConstraint>(c)) {
		const auto& bc = std::get<IK::BallJointConstraint>(c);
		if (bc.coneAngle < 0.0f)
			return dir;

		float3 coneAxisW = bc.coneAxis;
		coneAxisW.SafeNormalize();

		const float cosLimit = math::cos(bc.coneAngle);
		const float cosActual = dir.dot(coneAxisW);
		if (cosActual < cosLimit) {
			float3 perp = dir - coneAxisW * cosActual;
			if (perp.SqLength() < 1e-6f)
				return coneAxisW;

			perp.SafeNormalize();
			dir = coneAxisW * cosLimit + perp * math::sin(bc.coneAngle);
			dir.SafeNormalize();
		}
	}
	else if (std::holds_alternative<IK::HingeJointConstraint>(c)) {
		return ApplyHingeConstraint(dir, restDirNormalized, std::get<IK::HingeJointConstraint>(c));
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
		return FABRIKResult::ERR_INPUTS;

	if (segLengths.size() != n - 1)
		return FABRIKResult::ERR_INPUTS;

	if (!constraints.empty() && constraints.size() != n - 1)
		return FABRIKResult::ERR_INPUTS;

	for (float len : segLengths) {
		if (len <= 0.0f)
			return FABRIKResult::ERR_INPUTS;
	}

	const float3 rootPos = positions[0];

	float totalLen = 0.0f;
	for (size_t i = 0; i < n - 1; i++)
		totalLen += segLengths[i];

	if (rootPos.distance(goal) > totalLen) {
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
			float3 boneDir = (positions[i + 1] - positions[i]);
			boneDir.SafeNormalize();


		// Unlike the outcomes in https://stackoverflow.com/questions/76805554/is-it-a-known-issue-that-2d-inverse-kinematics-with-fabrik-plus-angle-constraint
		// applying constraints in the forward pass seems to improve the convergence of the solver.
#if 1
			if (!constraints.empty() && !std::holds_alternative<std::monostate>(constraints[i])) {
				float3 restDir = (i > 0) ? (positions[i] - positions[i - 1]).SafeNormalize() : bposeRootDir;
				boneDir = ApplyConstraintWorldSpace(constraints[i], boneDir, restDir);
			}
#endif
			positions[i] = positions[i + 1] - boneDir * segLengths[i];
		}

		// Backward pass: fix root, propagate toward effector with constraints
		positions[0] = rootPos;
		for (size_t i = 0; i < n - 1; i++) {
			float3 dir = (positions[i + 1] - positions[i]);
			dir.SafeNormalize();

			if (!constraints.empty() && !std::holds_alternative<std::monostate>(constraints[i])) {
				float3 restDir = (i > 0) ? (positions[i] - positions[i - 1]).SafeNormalize() : bposeRootDir;
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
