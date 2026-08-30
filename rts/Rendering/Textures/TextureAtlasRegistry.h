/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <memory>
#include <string>

#include "ITextureAtlas.h"
#include "MetadataTextureAtlas.h"
#include "System/UnorderedMap.hpp"

class CTextureAtlasRegistry
{
public:
	void LoadDefinitions();
	ITextureAtlas* GetAtlas(const std::string& name) const;
	ITextureAtlas* GetAtlasByRole(const std::string& role) const;
	void QueueStartupPatch(const std::string& name, AtlasPatchDefinition patch);
	void PlanAtlas(const std::string& name);
	void FinalizeAtlas(const std::string& name);
	void FinalizeRemainingAtlases();
	void RegisterProceduralAtlas(const std::string& name, const std::string& role, ITextureAtlas* atlas);
	void UnregisterProceduralAtlas(ITextureAtlas* atlas);
	void ReloadAtlas(const std::string& name);
	void Kill();
	bool Empty() const { return entries.empty(); }

private:
	struct Entry {
		std::unique_ptr<CMetadataTextureAtlas> owned;
		ITextureAtlas* atlas = nullptr;
		std::string role;
		bool procedural = false;
	};

	Entry& RequireEntry(const std::string& name);
	const Entry& RequireEntry(const std::string& name) const;
	static std::string NormalizeAtlasName(const std::string& name);

	spring::unordered_map<std::string, Entry> entries;
	spring::unordered_map<std::string, ITextureAtlas*> roles;
};

extern CTextureAtlasRegistry* textureAtlasRegistry;
