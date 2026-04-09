/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FABRIKSolverMath.hpp"

#include <algorithm>
#include <cassert>

#include "System/float3.h"
#include "System/SpringMath.h"
#include "System/Quaternion.h"

namespace IK {

	static float3 ApplyConstraint(
		const Constraint& c,
		const float3& boneDir,
		const CQuaternion& parentOri)
	{
		static constexpr float3 defaultAxis = float3(0.0f, 1.0f, 0.0f);

		if (const auto* bc = std::get_if<BallJointConstraint>(&c)) {
			if (bc->coneAngle < 0.0f)
				return boneDir;

			// coneAxis is in parent's local space
			float3 coneAxisW = parentOri.Rotate(bc->coneAxis);
			coneAxisW.SafeNormalize();

			const float cosLimit = math::cos(bc->coneAngle);
			const float cosActual = std::clamp(boneDir.dot(coneAxisW), -1.0f, 1.0f);
			if (cosActual < cosLimit) {
				float3 perp = boneDir - coneAxisW * cosActual;
				if (perp.SqLength() < 1e-6f)
					return coneAxisW;

				perp.SafeNormalize();
				float3 result = coneAxisW * cosLimit + perp * math::sin(bc->coneAngle);
				result.SafeNormalize();
				return result;
			}
			return boneDir;
		}

		if (const auto* hc = std::get_if<HingeJointConstraint>(&c)) {
			// Transform boneDir to parent space
			float3 dirL = parentOri.Inverse().Rotate(boneDir);
			float3 axisL = hc->axis;
			axisL.SafeNormalize();

			// restProjL is the parent bone direction in local space (identity by our convention)
			float3 restProjL = defaultAxis - axisL * axisL.y;
			if (restProjL.SqLength() < 1e-4f) {
				float3 alt = (std::abs(axisL.x) < 0.9f) ? float3(1,0,0) : float3(0,0,1);
				restProjL = axisL.cross(alt);
			}
			restProjL.SafeNormalize();

			const float axialComponent = dirL.dot(axisL);
			float3 dirProjL = dirL - axisL * axialComponent;
			if (dirProjL.SqLength() < 1e-4f) {
				dirProjL = restProjL;
			} else {
				dirProjL.Normalize();
			}

			const float cosA = std::clamp(dirProjL.dot(restProjL), -1.0f, 1.0f);
			const float sinSign = restProjL.cross(dirProjL).dot(axisL) >= 0.0f ? 1.0f : -1.0f;
			const float angle = std::clamp(sinSign * math::acos(cosA), std::min(hc->minAngle, hc->maxAngle), std::max(hc->minAngle, hc->maxAngle));

			float3 resultL = restProjL * math::cos(angle) + axisL.cross(restProjL) * math::sin(angle);
			return parentOri.Rotate(resultL);
		}
		return boneDir;
	}

	FABRIKResult SolveFABRIK(
		std::vector<Bone>& chain,
		const float3& goal,
		uint32_t maxIterations,
		float precision,
		uint32_t* iterCount)
	{
		const size_t n = chain.size();
		if (n == 0)
			return FABRIKResult::ERR_INPUTS;

		if (iterCount != nullptr)
			*iterCount = 0;

		const float3 defaultAxis = float3(0.0f, 1.0f, 0.0f);

		// Initial positions in root-relative frame
		std::vector<float3> pos(n + 1);
		pos[0] = float3(0.0f, 0.0f, 0.0f);
		for (size_t i = 0; i < n; i++) {
			pos[i + 1] = pos[i] + chain[i].orientation.Rotate(defaultAxis) * chain[i].length;
		}

		// Reachability check
		float totalLen = 0.0f;
		for (const auto& b : chain)
			totalLen += b.length;

		const float distSq = goal.SqLength();
		if (distSq > (totalLen + precision) * (totalLen + precision)) {
			float3 goalDir = goal;
			goalDir.SafeNormalize();
			pos[0] = float3(0, 0, 0);
			for (size_t i = 0; i < n; i++) {
				pos[i + 1] = pos[i] + goalDir * chain[i].length;
			}
			// Update orientations
			for (size_t i = 0; i < n; i++) {
				float3 newDir = (pos[i+1] - pos[i]).SafeNormalize();
				float3 oldDir = chain[i].orientation.Rotate(defaultAxis);
				CQuaternion delta = CQuaternion::MakeFrom(oldDir, newDir);
				chain[i].orientation = (delta * chain[i].orientation).Normalize();
			}
			return FABRIKResult::STRETCHING;
		}

		for (uint32_t iter = 0; iter < maxIterations; iter++) {
			if (iterCount != nullptr)
				*iterCount = iter + 1;

			// Forward pass: effector to root
			pos[n] = goal;
			for (size_t i = n; i-- > 0; ) {
				float3 boneDir = (pos[i + 1] - pos[i]);
				boneDir.SafeNormalize();

				// For forward pass constraints, the "parent" reference is less intuitive, 
				// but applying them relative to current orientation helps stability.
				// improves success rate, but slows down the solver convergence
				#if 0
				CQuaternion parentOri = (i > 0) ? chain[i - 1].orientation : CQuaternion();
				boneDir = ApplyConstraint(chain[i].constraint, boneDir, parentOri);
				#endif

				pos[i] = pos[i + 1] - boneDir * chain[i].length;
			}

			// Backward pass: root to effector
			pos[0] = float3(0, 0, 0);
			for (size_t i = 0; i < n; i++) {
				float3 boneDir = (pos[i + 1] - pos[i]);
				boneDir.SafeNormalize();

				CQuaternion parentOri = (i > 0) ? chain[i - 1].orientation : CQuaternion();
				boneDir = ApplyConstraint(chain[i].constraint, boneDir, parentOri);

				pos[i + 1] = pos[i] + boneDir * chain[i].length;

				// Update orientation immediately for the next bone's constraint reference
				float3 oldDir = chain[i].orientation.Rotate(defaultAxis);
				CQuaternion delta = CQuaternion::MakeFrom(oldDir, boneDir);
				chain[i].orientation = (delta * chain[i].orientation).Normalize();
			}

			if (pos[n].distance(goal) < precision)
				break;
		}

		if (pos[n].distance(goal) > precision)
			return FABRIKResult::FAILED;

		return FABRIKResult::FOUND;
	}

} // namespace IK