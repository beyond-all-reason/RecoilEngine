/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IKSolverMath.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "System/float3.h"
#include "System/SpringMath.h"
#include "System/Quaternion.h"

namespace IK {
	namespace {
		class FABRIKSolver final: public IIKSolver
		{
		public:
			Result Solve(
				std::vector<Bone>& chain,
				const float3& goal,
				uint32_t maxIterations,
				float precision,
				uint32_t* iterCount
			) const override {
				return SolveFABRIK(chain, goal, maxIterations, precision, iterCount);
			}
		};
	}

	float3 ApplyConstraint(
		const Constraint& c,
		const float3& boneDir,
		const CQuaternion& parentOri)
	{
		if (const auto* bc = std::get_if<BallJointConstraint>(&c)) {
			if (bc->coneAngle < 0.0f || bc->coneAngle >= math::PI)
				return boneDir;

			// coneAxis is in parent's local space
			float3 coneAxisW = parentOri.Rotate(bc->coneAxis);
			coneAxisW.SafeNormalize();

			const float cosLimit = math::cos(bc->coneAngle);
			const float cosActual = std::clamp(boneDir.dot(coneAxisW), -1.0f, 1.0f);
			if (cosActual < cosLimit) {
				float3 perp = boneDir - coneAxisW * cosActual;
				if (perp.SqLength() < float3::apx_eps())
					return coneAxisW;

				perp.SafeNormalize();
				float3 result = coneAxisW * cosLimit + perp * math::sin(bc->coneAngle);
				result.SafeNormalize();
				return result;
			}
			return boneDir;
		}

		if (const auto* hc = std::get_if<HingeJointConstraint>(&c)) {
			// Reconstruct a clean look-rotation frame from the parent's forward
			// direction.  The solver's parentOri has arbitrary roll from
			// MakeFrom(), which makes the constraint axis meaningless — different
			// bone directions map the same local (1,0,0) to different model-space
			// axes.  Look-rotation fixes this by aligning local Y ≈ model up.
			const float3 parentFwd = parentOri.Rotate(kBoneRestAxis);
			const CQuaternion lookOri = MakeOrientationFromBoneDir(parentFwd);

			float3 dirL = lookOri.InverseNormalized().Rotate(boneDir);
			float3 axisL = hc->axis;
			axisL.SafeNormalize();

			// restProjL is the parent bone direction in local space
			float3 restProjL = kBoneRestAxis - axisL * axisL.z;
			if (restProjL.SqLength() < float3::apx_eps()) {
				float3 alt = (std::abs(axisL.x) < 0.9f) ? RgtVector : UpVector;
				restProjL = axisL.cross(alt);
			}
			restProjL.SafeNormalize();

			const float axialComponent = dirL.dot(axisL);
			float3 dirProjL = dirL - axisL * axialComponent;
			if (dirProjL.SqLength() < float3::apx_eps()) {
				dirProjL = restProjL;
			} else {
				dirProjL.Normalize();
			}

			const float dot = dirProjL.dot(restProjL);
			const float det = restProjL.cross(dirProjL).dot(axisL);
			const float unclampedAngle = math::atan2(det, dot);
			const float minAngle = std::min(hc->minAngle, hc->maxAngle);
			const float maxAngle = std::max(hc->minAngle, hc->maxAngle);
			const float angle = std::clamp(unclampedAngle, minAngle, maxAngle);

			float3 resultL = restProjL * math::cos(angle) + axisL.cross(restProjL) * math::sin(angle);
			return lookOri.Rotate(resultL);
		}
		return boneDir;
	}

	Result SolveFABRIK(
		std::vector<Bone>& chain,
		const float3& goal,
		uint32_t maxIterations,
		float precision,
		uint32_t* iterCount)
	{
		const size_t n = chain.size();
		if (n == 0)
			return Result::ERR_INPUTS;

		if (iterCount != nullptr)
			*iterCount = 0;

		std::vector<float3> pos(n + 1);
		float totalLen = 0.0f;

		std::vector<float3> initDirs(n);
		for (size_t i = 0; i < n; i++) {
			totalLen += chain[i].length;
			pos[i + 1] = pos[i] + BoneDirFromOrientation(chain[i].orientation) * chain[i].length;
			initDirs[i] = BoneDirFromOrientation(chain[i].orientation);
		}

		const float distSq = goal.SqLength();
		if (distSq > (totalLen + precision) * (totalLen + precision)) {
			float3 goalDir = goal;
			goalDir.SafeNormalize();
			pos[0] = ZeroVector;
			for (size_t i = 0; i < n; i++) {
				CQuaternion parentOri = (i > 0) ? chain[i - 1].orientation : CQuaternion();
				float3 constrainedDir = ApplyConstraint(chain[i].constraint, goalDir, parentOri);

				pos[i + 1] = pos[i] + constrainedDir * chain[i].length;

				const bool isRigid = !chain[i].canRotate || (i + 1 < n && !chain[i + 1].canMove);
				if (!isRigid) {
					float3 restDirW = parentOri.Rotate(kBoneRestAxis);
					CQuaternion delta = CQuaternion::MakeFrom(restDirW, constrainedDir).Normalize();
					chain[i].orientation = (delta * parentOri).Normalize();
				}
			}
			return Result::STRETCHING;
		}

		const float precisionSq = precision * precision;

		for (uint32_t iter = 0; iter < maxIterations; iter++) {
			if (iterCount != nullptr)
				*iterCount = iter + 1;

			// Forward pass: effector to root, unconstrained.
			// Standard FABRIK position adjustment — just maintain bone lengths
			// and move joints toward the goal. No constraint evaluation here;
			// constraints are only applied in the backward pass to avoid
			// fighting between the two passes.
			pos[n] = goal;
			for (size_t i = n; i-- > 0; ) {
				float3 boneDir;
				const bool isRigid = !chain[i].canRotate || (i + 1 < n && !chain[i + 1].canMove);

				if (isRigid) {
					boneDir = initDirs[i];
				} else {
					boneDir = (pos[i + 1] - pos[i]);
					boneDir.SafeNormalize();
				}

				pos[i] = pos[i + 1] - boneDir * chain[i].length;
			}

			// Backward pass: root to effector, constrained.
			// This is the sole source of constraint evaluation — cascades
			// orientations root→effector so every bone sees its parent's
			// just-updated orientation. No inconsistency with a forward pass.
			pos[0] = ZeroVector;
			for (size_t i = 0; i < n; i++) {
				float3 boneDir;
				const bool isRigid = !chain[i].canRotate || (i + 1 < n && !chain[i + 1].canMove);

				if (isRigid) {
					boneDir = BoneDirFromOrientation(chain[i].orientation);
				} else {
					boneDir = (pos[i + 1] - pos[i]);
					boneDir.SafeNormalize();

					CQuaternion parentOri = (i > 0) ? chain[i - 1].orientation : CQuaternion();
					boneDir = ApplyConstraint(chain[i].constraint, boneDir, parentOri);

					// Update orientation immediately for the next bone's constraint reference.
					float3 restDirW = parentOri.Rotate(kBoneRestAxis);
					CQuaternion delta = CQuaternion::MakeFrom(restDirW, boneDir).Normalize();
					chain[i].orientation = (delta * parentOri).Normalize();
				}

				pos[i + 1] = pos[i] + boneDir * chain[i].length;
			}

			if (pos[n].SqDistance(goal) < precisionSq)
				break;
		}

		if (pos[n].SqDistance(goal) > precisionSq)
			return Result::FAILED;

		return Result::FOUND;
	}

	const IIKSolver& GetFABRIKSolver()
	{
		static const FABRIKSolver solver;
		return solver;
	}

} // namespace IK
