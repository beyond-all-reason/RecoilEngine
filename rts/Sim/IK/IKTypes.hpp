/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <variant>

#include "System/float3.h"

namespace IK {
	struct BallJointConstraint {
		float3 coneAxis = float3(0.0f, 1.0f, 0.0f);
		float coneAngle = 0.0f;
	};

	struct HingeJointConstraint {
		float3 axis = float3(0.0f, 1.0f, 0.0f);
		float minAngle = 0.0f;
		float maxAngle = 0.0f;
	};

	using Constraint = std::variant<std::monostate, BallJointConstraint, HingeJointConstraint>;
}
