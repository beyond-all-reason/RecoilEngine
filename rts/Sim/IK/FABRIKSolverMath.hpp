/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>
#include <cstdint>

#include "System/float3.h"
#include "Sim/IK/FABRIKSolver.hpp" // for Constraint types

namespace IK {

	enum class FABRIKResult : uint8_t {
		FOUND = 0,
		STRETCHING = 1,
		FAILED = 2
	};

	// Pure FABRIK solver operating entirely on world-space positions.
	// No engine types (CSolidObject, LocalModelPiece) are required.
	// Constraint axes are interpreted directly as world-space vectors.
	//
	// positions:        in/out joint positions; positions[0] is the root (pinned)
	// segLengths:       bone lengths between consecutive joints (size = positions.size() - 1)
	// constraints:      per-joint constraints (size = positions.size(), or empty for unconstrained)
	// bindPoseRootDir:  rest direction for constraint reference at the root joint (i=0)
	// goal:             desired world-space position for the effector (last joint)
	// maxIterations:    maximum FABRIK iteration count
	// precision:        convergence threshold (effector-to-goal distance)
	FABRIKResult SolveFABRIK(
		std::vector<float3>& positions,
		const std::vector<float>& segLengths,
		const std::vector<Constraint>& constraints,
		const float3& bindPoseRootDir,
		const float3& goal,
		uint32_t maxIterations = 10,
		float precision = 1.0f
	);

} // namespace IK
