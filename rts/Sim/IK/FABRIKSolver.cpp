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

		// Build a parent-frame quaternion for bone i by swing-adjusting the
		// bind-pose orientation of the parent bone (i-1).  This preserves the
		// model's intended roll for the constraint axis, which is critical for
		// hinge joints on legs that point sideways — reconstructing roll from
		// model-up or chain geometry produces the wrong constraint plane.
		//
		// The swing rotation is the minimal rotation from the parent's initial
		// bone direction to its current direction (from the position array).
		// Applied to the initial orientation, it updates the forward direction
		// while preserving the roll.
		inline CQuaternion ParentFrameFromPositions(
			const std::vector<float3>& pos,
			const std::vector<float3>& initDirs,
			const std::vector<Bone>& chain,
			size_t i)
		{
			if (i == 0)
				return CQuaternion();

			// Current forward direction of the parent bone (bone i-1).
			const float3 curDir = (pos[i] - pos[i - 1]).SafeNormalize();
			// Initial forward direction of the parent bone.
			const float3 initDir = initDirs[i - 1];

			// Swing rotation: minimal rotation from initial to current direction.
			// This changes the forward axis without adding roll.
			const CQuaternion swing = CQuaternion::MakeFrom(initDir, curDir);

			// Apply swing to the bind-pose parent orientation.
			// When curDir ≈ initDir, swing ≈ identity → returns initial orientation.
			// When they differ, the frame rotates to match while preserving roll.
			return (swing * chain[i - 1].orientation).Normalize();
		}

		// FABRIK-specific constraint evaluation that uses the parent frame's
		// roll directly (from MakeFromChain) instead of reconstructing with
		// model-up.  This is the same as ApplyConstraint for ball joints, but
		// for hinge joints it preserves the chain-based roll which is critical
		// for sideways-pointing legs.
		float3 ApplyConstraintFABRIK(
			const Constraint& c,
			const float3& boneDir,
			const CQuaternion& parentOri)
		{
			if (const auto* bc = std::get_if<BallJointConstraint>(&c)) {
				if (bc->coneAngle < 0.0f || bc->coneAngle >= math::PI)
					return boneDir;

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
				// Use parentOri directly — it has consistent roll from
				// MakeFromChain, which gives the correct hinge plane for
				// sideways-pointing legs.
				const CQuaternion& lookOri = parentOri;

				float3 dirL = lookOri.InverseNormalized().Rotate(boneDir);
				float3 axisL = hc->axis;
				axisL.SafeNormalize();

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
			// direction.  The solver's parentOri may have arbitrary roll from
			// accumulated CCD rotations, which makes the constraint axis meaningless.
			// Look-rotation fixes this by aligning local Y ≈ model up.
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
			// Goal is unreachable — stretch each bone toward the goal as far as
			// constraints allow, then extract orientations from the stretched positions.
			float3 goalDir = goal;
			goalDir.SafeNormalize();

			pos[0] = ZeroVector;
			for (size_t i = 0; i < n; i++) {
				const CQuaternion parentOri = ParentFrameFromPositions(pos, initDirs, chain, i);
				float3 constrainedDir = ApplyConstraintFABRIK(chain[i].constraint, goalDir, parentOri);
				pos[i + 1] = pos[i] + constrainedDir * chain[i].length;
			}

			// Orientations: each bone's orientation encodes its own world-space direction.
			for (size_t i = 0; i < n; ++i) {
				chain[i].orientation = MakeOrientationFromBoneDir((pos[i + 1] - pos[i]).SafeNormalize());
			}

			return Result::STRETCHING;
		}

		const float precisionSq = precision * precision;

		for (uint32_t iter = 0; iter < maxIterations; iter++) {
			if (iterCount != nullptr)
				*iterCount = iter + 1;

			// Forward pass: effector → root.  Standard FABRIK — maintain bone lengths.
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

			// Backward pass: root → effector.  Re-anchor at origin and apply
			// constraints.  On the first iteration, skip hinge constraints to
			// let bone[0] develop a Z component via the unconstrained forward
			// pass.  This mirrors how CCD implicitly rotates bone[0] on its
			// first pass, establishing a parent frame whose hinge plane includes
			// the forward/backward axis for sideways-pointing legs.
			pos[0] = ZeroVector;
			for (size_t i = 0; i < n; i++) {
				float3 boneDir;
				const bool isRigid = !chain[i].canRotate || (i + 1 < n && !chain[i + 1].canMove);

				if (isRigid) {
					boneDir = initDirs[i];
				} else {
					boneDir = (pos[i + 1] - pos[i]);
					boneDir.SafeNormalize();

					const CQuaternion parentOri = ParentFrameFromPositions(pos, initDirs, chain, i);

					if (iter == 0 && std::get_if<HingeJointConstraint>(&chain[i].constraint)) {
						// First iteration: skip hinge to let bone positions
						// establish a good parent frame.  Ball joints and
						// unconstrained bones are still constrained normally.
					} else {
						boneDir = ApplyConstraintFABRIK(chain[i].constraint, boneDir, parentOri);
					}
				}

				pos[i + 1] = pos[i] + boneDir * chain[i].length;
			}

			if (pos[n].SqDistance(goal) < precisionSq)
				break;
		}

		// Extract final orientations from converged positions.
		// Each bone's orientation must encode its own world-space direction
		// so that BoneDirFromOrientation() returns the correct vector.
		for (size_t i = 0; i < n; ++i) {
			chain[i].orientation = MakeOrientationFromBoneDir((pos[i + 1] - pos[i]).SafeNormalize());
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
