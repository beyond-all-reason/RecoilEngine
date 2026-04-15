/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>
#include <cstdint>

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
		const float3& z = boneDir;

		float3 y = UpVector - z * z.dot(UpVector);
		if unlikely(auto ySqLen = y.SqLength(); ySqLen < float3::apx_eps()) {
			return CQuaternion::MakeFrom(kBoneRestAxis, z); // degenerate case
		} else {
			y *= math::isqrt(ySqLen);
		}

		float3 x = y.cross(z); // x is already normalized since y and z are orthogonal

		return CQuaternion::MakeFromAxes(x, y, z);
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
