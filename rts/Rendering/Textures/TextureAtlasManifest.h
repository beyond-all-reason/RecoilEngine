/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <string>
#include <vector>

#include "TextureAtlasTypes.h"

struct TextureAtlasRegistryDefinition {
	std::string id;
	std::string manifest;
	std::vector<std::string> patches;
	std::string role;
};

class TextureAtlasManifestLoader
{
public:
	static std::vector<TextureAtlasRegistryDefinition> LoadRegistry(const std::string& fileName = "gamedata/textureatlases.lua");
	static TextureAtlasManifest LoadManifest(const std::string& fileName);
	static std::vector<AtlasPatchDefinition> LoadPatchSidecar(const std::string& fileName);

	static void ValidateManifest(const TextureAtlasManifest& manifest);
	static void ValidatePatchSet(const TextureAtlasManifest& manifest, const std::vector<AtlasPatchDefinition>& patches);
	static std::string NormalizeEntryID(const std::string& id);
	static const AtlasVariant* ResolveVariantFiles(const TextureAtlasManifest& manifest, const std::string& requestedFormat);
};
