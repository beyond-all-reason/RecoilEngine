/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DemoFileExtension.h"

#include <ranges>

#include "System/Config/ConfigHandler.h"
#include "System/StringUtil.h"

CONFIG(std::string, DemoFileExtension).defaultValue("sdfz").description("Comma-separated list of replay file extensions. The first entry is used when recording; all entries are accepted when loading. Set by the lobby (e.g. 'barreplay,sdfz' for BAR).");

std::vector<std::string> GetDemoFileExtensions()
{
	const auto configValue = configHandler->GetString("DemoFileExtension");

	std::vector<std::string> extensions;

	for (const auto& part : configValue | std::views::split(',')) {
		auto extension = StringTrim(std::string(part.begin(), part.end()));

		if (!extension.empty() && extension.find_first_of("/\\.") == std::string::npos)
			extensions.emplace_back(std::move(extension));
	}

	if (extensions.empty())
		extensions.emplace_back("sdfz");
	return extensions;
}
