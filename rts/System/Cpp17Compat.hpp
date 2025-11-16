#pragma once

#include <filesystem>
#include <string>

namespace Recoil {
	template<class T>
	concept StringLike = std::is_same_v<T, std::string_view> || std::is_same_v<T, std::string>;

	template<class T>
	concept CStringLike = std::is_same_v<std::decay_t<T>, const char*>;

	namespace filesystem {
		template<StringLike Source>
		std::filesystem::path u8path(const Source& pathStr) {
			return std::filesystem::path(reinterpret_cast<const char8_t*>(pathStr.c_str()));
		}
		template<CStringLike Source>
		std::filesystem::path u8path(const Source& pathStr) {
			return std::filesystem::path(reinterpret_cast<const char8_t*>(pathStr));
		}
	}
}