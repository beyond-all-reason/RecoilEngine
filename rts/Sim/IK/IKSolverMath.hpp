/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>
#include <cstdint>
#include <cmath>

#include "System/float3.h"
#include "System/SpringMath.h"
#include "IKTypes.hpp"

namespace IK {
	inline constexpr float3 kBoneRestAxis = FwdVector;

	/// Construct a bone orientation that maps kBoneRestAxis (Z+) to boneDir,
	/// while keeping the local Y axis as close to model up (Y+) as possible.
	/// This "look rotation" ensures the local X axis is consistent across all
	/// bone directions, which is critical for hinge joint constraints.
	inline CQuaternion MakeOrientationFromBoneDir(const float3& boneDir)
	{
		float3 fwd = boneDir;
		fwd.SafeNormalize();

		// Step 1: Minimal rotation from (0,0,1) to fwd (has arbitrary roll).
		CQuaternion Q = CQuaternion::MakeFrom(kBoneRestAxis, fwd);

		// Step 2: Compute desired Y — model up projected perpendicular to fwd.
		float3 desiredY = UpVector - fwd * fwd.dot(UpVector);
		if (desiredY.SqLength() < float3::apx_eps())
			return Q; // fwd is parallel to up, roll is undefined
		desiredY.SafeNormalize();

		// Step 3: Compute current Y in the same perpendicular plane.
		float3 curY = Q.Rotate(UpVector);
		curY = curY - fwd * curY.dot(fwd);
		if (curY.SqLength() < float3::apx_eps())
			return Q;
		curY.SafeNormalize();

		// Step 4: Roll angle to align curY with desiredY (rotation around fwd).
		const float cosA = curY.dot(desiredY);
		const float sinA = curY.cross(desiredY).dot(fwd);
		const float roll = math::atan2(sinA, cosA);

		if (math::fabs(roll) < float3::cmp_eps())
			return Q;

		// Step 5: Apply roll correction.
		const CQuaternion rollFix = CQuaternion::MakeFrom(roll, fwd);
		return (rollFix * Q).Normalize();
	}

	inline float3 BoneDirFromOrientation(const CQuaternion& orientation)
	{
		return orientation.Rotate(kBoneRestAxis);
	}

	enum class Result : uint8_t {
		FOUND      = 0,
		STRETCHING = 1,
		FAILED     = 2,
		ERR_INPUTS = 3
	};

	class IIKSolver
	{
	public:
		virtual ~IIKSolver() = default;

		virtual Result Solve(
			std::vector<Bone>& chain,
			const float3& goal,
			uint32_t maxIterations = 10,
			float precision = 1.0f,
			uint32_t* iterCount = nullptr
		) const = 0;
	};

	// Pure FABRIK solver operating on a bone hierarchy.
	//
	// chain:            in/out bone list; orientations are updated in place.
	// goal:              desired position for the effector (end of last bone) relative to root joint.
	// maxIterations:    maximum FABRIK iteration count.
	// precision:        convergence threshold (effector-to-goal distance).
	//
	// Results are naturally expressed in the frame where the root joint is at (0,0,0).
	Result SolveFABRIK(
		std::vector<Bone>& chain,
		const float3& goal,
		uint32_t maxIterations = 10,
		float precision = 1.0f,
		uint32_t* iterCount = nullptr
	);

	// Pure CCD solver operating on the same bone hierarchy representation as FABRIK.
	Result SolveCCD(
		std::vector<Bone>& chain,
		const float3& goal,
		uint32_t maxIterations = 10,
		float precision = 1.0f,
		uint32_t* iterCount = nullptr
	);

	float3 ApplyConstraint(
		const Constraint& c,
		const float3& boneDir,
		const CQuaternion& parentOri);

	// Shared polymorphic accessors for runtime-selected IK solve algorithms.
	const IIKSolver& GetFABRIKSolver();
	const IIKSolver& GetCCDSolver();

} // namespace IK
