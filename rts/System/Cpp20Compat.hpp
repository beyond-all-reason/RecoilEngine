/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>

// Polyfill for C++20 features not available on all toolchains we target
// (notably older Apple libc++), following the pattern of Cpp17Compat.hpp.

namespace Recoil {
	namespace ranges {

#if defined(__cpp_lib_ranges) && __cpp_lib_ranges >= 201911L
		using std::ranges::find_if;
#else
		// Minimal range overload of find_if for toolchains lacking std::ranges.
		template<typename Range, typename Pred>
		constexpr auto find_if(Range&& r, Pred pred) {
			return std::find_if(std::begin(r), std::end(r), std::move(pred));
		}
#endif

	} // namespace ranges
} // namespace Recoil
