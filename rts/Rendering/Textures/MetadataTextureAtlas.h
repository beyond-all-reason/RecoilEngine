/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ITextureAtlas.h"
#include "ReservedAtlasAlloc.h"
#include "TextureAtlasManifest.h"
#include "System/UnorderedMap.hpp"

class CBitmap;

class CMetadataTextureAtlas : public ITextureAtlas
{
public:
	enum class State { DEFINED, PLANNED, FINALIZED };

	CMetadataTextureAtlas(TextureAtlasManifest manifest, const AtlasVariant& variant);
	~CMetadataTextureAtlas() override;

	CMetadataTextureAtlas(const CMetadataTextureAtlas&) = delete;
	CMetadataTextureAtlas& operator=(const CMetadataTextureAtlas&) = delete;

	void QueuePatch(AtlasPatchDefinition patch);
	void Plan();
	void Finalize();

	const std::string& GetName() const override { return manifest.name; }
	uint32_t GetTexID() const override { return textureID; }
	uint32_t GetTexTarget() const override;
	int2 GetSize() const override { return int2(manifest.width, manifest.height); }
	uint32_t GetNumPages() const override { return manifest.pages.size(); }
	int GetNumTexLevels() const override { return manifest.mipLevels; }
	const AtlasedTexture* FindTexture(const std::string& name) const override;
	AtlasedTexture* GetTexturePtr(const std::string& name) override;
	std::string GetTextureName(const AtlasedTexture* texture) const override;
	void ReloadTexture() override;

	State GetState() const { return state; }
	const AtlasVariant& GetVariant() const { return variant; }
	bool IsModified() const { return !patches.empty(); }
	bool IsCompressed() const { return compressed; }
	const TextureAtlasManifest& GetManifest() const { return manifest; }
	const auto& GetTextures() const { return textures; }

private:
	struct CommittedPatch {
		AtlasPatchDefinition definition;
		ReservedAtlasPlacement placement;
		AtlasPadding padding;
		int2 size;
	};

	void ValidateDDSFiles() const;
	void UploadDirect();
	void BuildModifiedPixels();
	void UploadModified();
	void DeleteTexture();

	TextureAtlasManifest manifest;
	AtlasVariant variant;
	State state = State::DEFINED;
	uint32_t textureID = 0;
	bool compressed = false;
	std::vector<AtlasPatchDefinition> patches;
	std::vector<CommittedPatch> committedPatches;
	std::vector<std::vector<uint8_t>> committedPixels;
	spring::unordered_map<std::string, AtlasedTexture> textures;
	mutable spring::unordered_map<const AtlasedTexture*, std::string> textureNames;
};
