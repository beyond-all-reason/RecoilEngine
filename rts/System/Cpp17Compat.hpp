#pragma once

#include <filesystem>
#include <string>

namespace Recoil {
	template<class T>
	concept StringLike = std::is_convertible_v<T, std::string_view>;

	namespace filesystem {
		template<StringLike Source>
		std::filesystem::path u8path(const Source& pathStr) {
			return std::filesystem::path(reinterpret_cast<const char8_t*>(pathStr.c_str()));
		}
	}
}