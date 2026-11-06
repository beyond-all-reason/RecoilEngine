/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

// The first failed native targeting test, not a prediction of a future shot.
struct TargetCheckResult {
	enum Reason {
		Clear = 0,
		NotChecked,
		InvalidTarget,
		Range,
		Terrain,
		Friendly,
		Neutral,
		Feature,
		Blocked,
	};
	enum class ObjectType { None, Unit, Feature };

	constexpr TargetCheckResult(Reason reason, ObjectType type = ObjectType::None, int id = -1):
		reason(reason), objectType(type), objectID(id) {}
	constexpr bool operator==(Reason other) const { return reason == other; }

	Reason reason;
	// First blocker in native scan order, not necessarily the closest object.
	ObjectType objectType;
	int objectID;
};

inline const char* TargetCheckResultName(TargetCheckResult result)
{
	switch (result.reason) {
		case TargetCheckResult::Clear: return "clear";
		case TargetCheckResult::NotChecked: return "notChecked";
		case TargetCheckResult::InvalidTarget: return "invalidTarget";
		case TargetCheckResult::Range: return "range";
		case TargetCheckResult::Terrain: return "terrain";
		case TargetCheckResult::Friendly: return "friendly";
		case TargetCheckResult::Neutral: return "neutral";
		case TargetCheckResult::Feature: return "feature";
		case TargetCheckResult::Blocked: return "blocked";
	}
	return "blocked";
}
