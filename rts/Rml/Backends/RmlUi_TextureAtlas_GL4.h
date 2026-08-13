/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "RmlUi_Backend.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <RmlUi/Core/Types.h>

class CTextureRenderAtlas;

namespace RmlGui {

class TextureAtlasManifest
{
public:
	bool Register(const std::string& path);
	bool Register(const std::vector<std::string>& paths);
	bool Finalize();
	void Reset();

	const std::vector<std::string>& GetSources() const { return sources; }
	const TextureAtlasStats& GetStats() const { return stats; }
	bool IsFinalized() const { return stats.finalized; }

	static std::string CanonicalizePath(const std::string& path);

private:
	std::unordered_map<std::string, std::string> originals;
	std::vector<std::string> sources;
	TextureAtlasStats stats;
};

struct StaticAtlasRecordGL4 {
	Rml::Vector2i dimensions = {};
	Rml::Vector4f uv_rect = {};
	uint32_t page = 0;
};

class StaticTextureAtlasGL4
{
public:
	StaticTextureAtlasGL4();
	~StaticTextureAtlasGL4();

	bool Build(TextureAtlasManifest& manifest);
	const StaticAtlasRecordGL4* Find(const std::string& canonical_path) const;
	uint32_t GetTextureId() const;
	uint32_t GetTextureTarget() const;
	const TextureAtlasStats& GetStats() const { return stats; }

private:
	std::unique_ptr<CTextureRenderAtlas> atlas;
	std::unordered_map<std::string, StaticAtlasRecordGL4> records;
	TextureAtlasStats stats;
};

} // namespace RmlGui
