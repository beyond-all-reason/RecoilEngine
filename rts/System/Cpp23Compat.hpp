#pragma once

#include <cstddef>
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>

// Polyfill for C++23 features not yet available in all toolchains (e.g. Apple libc++)

namespace Recoil {

#if defined(__cpp_lib_ranges_enumerate) && __cpp_lib_ranges_enumerate >= 202302L
	using std::views::enumerate;
#else
	// Minimal std::views::enumerate polyfill
	template<typename Range>
	class enumerate_view {
		Range range_;
	public:
		explicit enumerate_view(Range&& r) : range_(std::forward<Range>(r)) {}
		explicit enumerate_view(const Range& r) : range_(r) {}

		struct iterator {
			using inner_iter = decltype(std::begin(std::declval<Range&>()));
			using value_type = std::tuple<std::ptrdiff_t, decltype(*std::declval<inner_iter>())>;

			std::ptrdiff_t index;
			inner_iter it;

			auto operator*() const { return std::tie(index, *it); }
			iterator& operator++() { ++index; ++it; return *this; }
			bool operator!=(const iterator& other) const { return it != other.it; }
			bool operator==(const iterator& other) const { return it == other.it; }
		};

		auto begin() { return iterator{0, std::begin(range_)}; }
		auto end() { return iterator{0, std::end(range_)}; }
		auto begin() const { return iterator{0, std::begin(range_)}; }
		auto end() const { return iterator{0, std::end(range_)}; }
	};

	struct enumerate_fn {
		template<typename Range>
		auto operator()(Range&& r) const {
			return enumerate_view<Range>(std::forward<Range>(r));
		}
	};

	inline constexpr enumerate_fn enumerate{};
#endif

} // namespace Recoil
