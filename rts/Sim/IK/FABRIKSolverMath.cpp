/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FABRIKSolverMath.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "System/float3.h"
#include "System/SpringMath.h"
#include "System/Quaternion.h"

namespace IK {
	struct ConstraintDebugCounters {
		int ballClamped = 0;
		int hingeDegenerateProjection = 0;
		int hingeClamped = 0;
		int hingeAtMin = 0;
		int hingeAtMax = 0;
		uint64_t hingeClampMask = 0;
		uint64_t hingeMinMask = 0;
		uint64_t hingeMaxMask = 0;
	};

	static bool IsDebugTraceEnabled()
	{
		const char* env = std::getenv("FABRIK_DEBUG_TRACE");
		return (env != nullptr) && (env[0] != '\0') && (env[0] != '0');
	}

	static float3 ApplyConstraint(
		const Constraint& c,
		const float3& boneDir,
		const CQuaternion& parentOri,
		ConstraintDebugCounters* debugCounters = nullptr,
		size_t jointIdx = 0)
	{
		if (const auto* bc = std::get_if<BallJointConstraint>(&c)) {
			if (bc->coneAngle < 0.0f)
				return boneDir;

			// coneAxis is in parent's local space
			float3 coneAxisW = parentOri.Rotate(bc->coneAxis);
			coneAxisW.SafeNormalize();

			const float cosLimit = math::cos(bc->coneAngle);
			const float cosActual = std::clamp(boneDir.dot(coneAxisW), -1.0f, 1.0f);
			if (cosActual < cosLimit) {
				if (debugCounters != nullptr)
					debugCounters->ballClamped += 1;

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
			// Transform boneDir to parent space
			float3 dirL = parentOri.Inverse().Rotate(boneDir);
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
				if (debugCounters != nullptr)
					debugCounters->hingeDegenerateProjection += 1;

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

			if (debugCounters != nullptr) {
				constexpr float eps = 1e-5f;
				if (std::abs(unclampedAngle - angle) > eps) {
					debugCounters->hingeClamped += 1;
					if (jointIdx < 64)
						debugCounters->hingeClampMask |= (1ull << jointIdx);
				}
				if (std::abs(angle - minAngle) <= eps) {
					debugCounters->hingeAtMin += 1;
					if (jointIdx < 64)
						debugCounters->hingeMinMask |= (1ull << jointIdx);
				}
				if (std::abs(angle - maxAngle) <= eps) {
					debugCounters->hingeAtMax += 1;
					if (jointIdx < 64)
						debugCounters->hingeMaxMask |= (1ull << jointIdx);
				}
			}

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

		// Initial positions in root-relative frame
		std::vector<float3> pos(n + 1);

		//pos[0] = ZeroVector;

		for (size_t i = 0; i < n; i++) {
			pos[i + 1] = pos[i] + BoneDirFromOrientation(chain[i].orientation) * chain[i].length;
		}

		// Reachability check
		float totalLen = 0.0f;
		for (const auto& b : chain)
			totalLen += b.length;

		const float distSq = goal.SqLength();
		if (distSq > (totalLen + precision) * (totalLen + precision)) {
			float3 goalDir = goal;
			goalDir.SafeNormalize();
			pos[0] = ZeroVector;
			for (size_t i = 0; i < n; i++) {
				pos[i + 1] = pos[i] + goalDir * chain[i].length;
			}
			// Update orientations
			for (size_t i = 0; i < n; i++) {
				float3 newDir = (pos[i+1] - pos[i]).SafeNormalize();
				CQuaternion parentOri = (i > 0) ? chain[i-1].orientation : CQuaternion();
				float3 restDirW = parentOri.Rotate(kBoneRestAxis);
				CQuaternion delta = CQuaternion::MakeFrom(restDirW, newDir).Normalize();
				chain[i].orientation = (delta * parentOri).Normalize();
			}
			return FABRIKResult::STRETCHING;
		}

		for (uint32_t iter = 0; iter < maxIterations; iter++) {
			if (iterCount != nullptr)
				*iterCount = iter + 1;

			const bool debugTrace = IsDebugTraceEnabled();
			const float beforeError = pos[n].distance(goal);
			ConstraintDebugCounters forwardDebug;
			ConstraintDebugCounters backwardDebug;

			// Forward pass: effector to root
			pos[n] = goal;
			for (size_t i = n; i-- > 0; ) {
				float3 boneDir = (pos[i + 1] - pos[i]);
				boneDir.SafeNormalize();

				// For forward pass constraints, the "parent" reference is less intuitive,
				// but applying them relative to current orientation helps stability.
				CQuaternion parentOri = (i > 0) ? chain[i - 1].orientation : CQuaternion();
				boneDir = ApplyConstraint(chain[i].constraint, boneDir, parentOri, debugTrace ? &forwardDebug : nullptr, i);

				pos[i] = pos[i + 1] - boneDir * chain[i].length;
			}

			// Backward pass: root to effector
			pos[0] = ZeroVector;
			for (size_t i = 0; i < n; i++) {
				float3 boneDir = (pos[i + 1] - pos[i]);
				boneDir.SafeNormalize();

				CQuaternion parentOri = (i > 0) ? chain[i - 1].orientation : CQuaternion();
				boneDir = ApplyConstraint(chain[i].constraint, boneDir, parentOri, debugTrace ? &backwardDebug : nullptr, i);

				pos[i + 1] = pos[i] + boneDir * chain[i].length;

				// Update orientation immediately for the next bone's constraint reference.
				// We derive the new orientation from parentOri to maintain consistent roll
				// and avoid drift across iterations.
				float3 restDirW = parentOri.Rotate(kBoneRestAxis);
				CQuaternion delta = CQuaternion::MakeFrom(restDirW, boneDir).Normalize();
				chain[i].orientation = (delta * parentOri).Normalize();
			}

			if (debugTrace) {
				const float afterError = pos[n].distance(goal);
				std::printf(
					"[FABRIK_TRACE] iter=%u err_before=%.6f err_after=%.6f delta=%.6f | "
					"fw(ball=%d hingeClamp=%d deg=%d min=%d max=%d mask=0x%llx minMask=0x%llx maxMask=0x%llx) "
					"bw(ball=%d hingeClamp=%d deg=%d min=%d max=%d mask=0x%llx minMask=0x%llx maxMask=0x%llx)\n",
					iter,
					beforeError,
					afterError,
					beforeError - afterError,
					forwardDebug.ballClamped,
					forwardDebug.hingeClamped,
					forwardDebug.hingeDegenerateProjection,
					forwardDebug.hingeAtMin,
					forwardDebug.hingeAtMax,
					static_cast<unsigned long long>(forwardDebug.hingeClampMask),
					static_cast<unsigned long long>(forwardDebug.hingeMinMask),
					static_cast<unsigned long long>(forwardDebug.hingeMaxMask),
					backwardDebug.ballClamped,
					backwardDebug.hingeClamped,
					backwardDebug.hingeDegenerateProjection,
					backwardDebug.hingeAtMin,
					backwardDebug.hingeAtMax,
					static_cast<unsigned long long>(backwardDebug.hingeClampMask),
					static_cast<unsigned long long>(backwardDebug.hingeMinMask),
					static_cast<unsigned long long>(backwardDebug.hingeMaxMask)
				);
			}

			if (pos[n].distance(goal) < precision)
				break;
		}

		if (pos[n].distance(goal) > precision)
			return FABRIKResult::FAILED;

		return FABRIKResult::FOUND;
	}

} // namespace IK