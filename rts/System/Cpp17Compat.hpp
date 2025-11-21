#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace Recoil {
	namespace filesystem {
		std::filesystem::path u8path(const std::string& pathStr) {
			return std::filesystem::path(reinterpret_cast<const char8_t*>(pathStr.c_str()));
		}
		std::filesystem::path u8path(const std::string_view& pathStr) {
			return std::filesystem::path(reinterpret_cast<const char8_t*>(pathStr.data()));
		}
		std::filesystem::path u8path(const char* pathStr) {
			return std::filesystem::path(reinterpret_cast<const char8_t*>(pathStr));
		}
	}
}