#pragma once

#include <filesystem>
#include <string>

namespace Recoil {
	std::filesystem::path Utf8stringToPath(const std::string& pathStr) {
		return std::filesystem::path(reinterpret_cast<const char8_t*>(pathStr.c_str()));
	}
}