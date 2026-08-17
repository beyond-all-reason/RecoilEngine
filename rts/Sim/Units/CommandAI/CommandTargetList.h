/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <optional>

#include "Command.h"

namespace CommandTargetList {

inline bool HaveSameTargets(const Command& lhs, const Command& rhs)
{
	if (lhs.GetNumParams() != rhs.GetNumParams())
		return false;

	for (unsigned int p = 0; p < lhs.GetNumParams(); ++p) {
		if (lhs.GetParam(p) != rhs.GetParam(p))
			return false;
	}

	return true;
}

inline bool ContainsTarget(const Command& command, int targetID)
{
	for (unsigned int p = 0; p < command.GetNumParams(); ++p) {
		if (static_cast<int>(command.GetParam(p)) == targetID)
			return true;
	}

	return false;
}

inline std::optional<int> NextTargetID(const Command& command, unsigned int& nextTargetIndex)
{
	if (nextTargetIndex >= command.GetNumParams())
		return std::nullopt;

	return static_cast<int>(command.GetParam(nextTargetIndex++));
}

} // namespace CommandTargetList
