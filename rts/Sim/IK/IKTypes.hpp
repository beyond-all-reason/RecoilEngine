/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <variant>

#include "System/float3.h"
#include "System/Quaternion.h"

namespace IK {
	struct BallJointConstraint {
		float3 coneAxis = FwdVector;
		float coneAngle = 0.0f;
	};

	struct HingeJointConstraint {
		float3 axis = FwdVector;
		float minAngle = 0.0f;
		float maxAngle = 0.0f;
	};

	using Constraint = std::variant<std::monostate, BallJointConstraint, HingeJointConstraint>;

	struct Bone {
		float length = 0.0f;
		CQuaternion orientation; // root-relative (model space) orientation
		Constraint constraint;
		bool canRotate = true;
		bool canMove = true;
	};
}
