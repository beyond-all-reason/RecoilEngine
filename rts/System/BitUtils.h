/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <climits>

namespace spring {

// Returns a mask with the low `n` bits set. Saturates at the width of T to
// avoid the undefined behavior of shifting by >= the type's bit width.
template<class T = uint32_t>
constexpr T LowBitsMask(unsigned int n) {
	if (n == 0)
		return T(0);
	if (n >= sizeof(T) * CHAR_BIT)
		return ~T(0);
	return (T(1) << n) - T(1);
}

} // namespace spring
