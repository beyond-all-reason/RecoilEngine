/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IKSolverMath.hpp"

#include <algorithm>
#include <cmath>

#include "System/float3.h"
#include "System/SpringMath.h"
#include "System/Quaternion.h"

namespace IK {
	namespace {
		class CCDSolver final: public IIKSolver
		{
		public:
			Result Solve(
				std::vector<Bone>& chain,
				const float3& goal,
				uint32_t maxIterations,
				float precision,
				uint32_t* iterCount
			) const override {
				return SolveCCD(chain, goal, maxIterations, precision, iterCount);
			}
		};
	}

	Result SolveCCD(
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

		for (size_t i = 0; i < n; ++i) {
			totalLen += chain[i].length;
			pos[i + 1] = pos[i] + BoneDirFromOrientation(chain[i].orientation) * chain[i].length;
		}

		const float distSq = goal.SqLength();
		if (distSq > (totalLen + precision) * (totalLen + precision)) {
			float3 goalDir = goal;
			goalDir.SafeNormalize();

			pos[0] = ZeroVector;
			for (size_t i = 0; i < n; ++i) {
				CQuaternion parentOri = (i > 0) ? chain[i - 1].orientation : CQuaternion();
				float3 constrainedDir = ApplyConstraint(chain[i].constraint, goalDir, parentOri);

				const float3 restDirW = parentOri.Rotate(kBoneRestAxis);
				const CQuaternion delta = CQuaternion::MakeFrom(restDirW, constrainedDir).Normalize();

				const bool isRigid = !chain[i].canRotate || (i + 1 < n && !chain[i + 1].canMove);
				if (!isRigid)
					chain[i].orientation = (delta * parentOri).Normalize();

				pos[i + 1] = pos[i] + BoneDirFromOrientation(chain[i].orientation) * chain[i].length;
			}

			return Result::STRETCHING;
		}

		const float precisionSq = precision * precision;

		for (uint32_t iter = 0; iter < maxIterations; ++iter) {
			if (iterCount != nullptr)
				*iterCount = iter + 1;

			if (pos[n].SqDistance(goal) <= precisionSq)
				break;

			// Standard CCD pass from effector parent toward root.
			for (size_t i = n; i-- > 0; ) {
				const bool isRigid = !chain[i].canRotate || (i + 1 < n && !chain[i + 1].canMove);
				if (isRigid)
					continue;

				float3 toEffector = (pos[n] - pos[i]);
				float3 toGoal = (goal - pos[i]);

				if (toEffector.SqLength() < float3::apx_eps() || toGoal.SqLength() < float3::apx_eps())
					continue;

				toEffector.SafeNormalize();
				toGoal.SafeNormalize();

				const CQuaternion ccdDelta = CQuaternion::MakeFrom(toEffector, toGoal).Normalize();

				const CQuaternion parentOri = (i > 0) ? chain[i - 1].orientation : CQuaternion();

				// Compute what bone i's direction would be after the CCD rotation,
				// then apply the joint constraint.
				float3 constrainedDir = ccdDelta.Rotate(BoneDirFromOrientation(chain[i].orientation));
				constrainedDir = ApplyConstraint(chain[i].constraint, constrainedDir, parentOri);

				const float3 restDirW = parentOri.Rotate(kBoneRestAxis);
				const CQuaternion constrainedOri = (CQuaternion::MakeFrom(restDirW, constrainedDir).Normalize() * parentOri).Normalize();

				// Apply the net rotation (original -> constrained) to bone i and all
				// descendants in a single pass instead of rotate-then-correct.
				const CQuaternion netDelta = (constrainedOri * chain[i].orientation.InverseNormalized()).Normalize();
				for (size_t j = i; j < n; ++j) {
					chain[j].orientation = (netDelta * chain[j].orientation).Normalize();
				}

				// Rebuild only the affected suffix positions.
				for (size_t j = i; j < n; ++j) {
					pos[j + 1] = pos[j] + BoneDirFromOrientation(chain[j].orientation) * chain[j].length;
				}

				if (pos[n].SqDistance(goal) <= precisionSq)
					break;
			}
		}

		return (pos[n].SqDistance(goal) <= precisionSq) ? Result::FOUND : Result::FAILED;
	}

	const IIKSolver& GetCCDSolver()
	{
		static const CCDSolver solver;
		return solver;
	}

} // namespace IK
