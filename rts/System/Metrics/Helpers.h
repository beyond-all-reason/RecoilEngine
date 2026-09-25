/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <algorithm>

/// Deliberately free of prometheus, so the pure-logic parts of the metrics
/// path stay reachable from unit tests without needing the prom dependency.
namespace metrics {

/// the netcode works in milliseconds, prometheus wants base units
inline constexpr double msToSecs = 0.001;

/**
 * @brief advance a delta baseline and return what accrued since the last call
 */
template<typename T> T DeltaSince(T cur, T& last)
{
	const T delta = cur - std::min(cur, last);

	last = cur;
	return delta;
}

}
