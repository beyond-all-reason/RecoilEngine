/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MetadataTextureAtlas.h"

#include <algorithm>
#include <cstring>
#include <unordered_map>

#include "Bitmap.h"
#include "nv_dds.h"
#include "Rendering/GL/myGL.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"

namespace {
	std::string DDSFormatName(nv_dds::DDSFormat format)
	{
		switch (format) {
			case nv_dds::DDSFormat::BC1: return "bc1";
			case nv_dds::DDSFormat::BC2: return "bc2";
			case nv_dds::DDSFormat::BC3: return "bc3";
			case nv_dds::DDSFormat::BC4: return "bc4";
			case nv_dds::DDSFormat::BC5: return "bc5";
			case nv_dds::DDSFormat::BC7: return "bc7";
			case nv_dds::DDSFormat::RGBA8: return "rgba8";
			default: return "unsupported";
		}
	}

	void CopyPatchPixels(std::vector<uint8_t>& destination, int atlasWidth, const CBitmap& source, const ReservedAtlasPlacement& placement, const AtlasPadding& padding)
	{
		const auto* pixels = source.GetRawMem();
		const auto Sample = [&](int x, int y) -> const uint8_t* {
			if (padding.mode == AtlasPaddingMode::CLAMP) {
				x = std::clamp(x, 0, source.xsize - 1);
				y = std::clamp(y, 0, source.ysize - 1);
			} else {
				x = ((x % source.xsize) + source.xsize) % source.xsize;
				y = ((y % source.ysize) + source.ysize) % source.ysize;
			}
			return pixels + (y * source.xsize + x) * 4;
		};
		const int originX = placement.content.x - placement.allocation.x;
		const int originY = placement.content.y - placement.allocation.y;
		for (int y = 0; y < placement.allocation.height; ++y) {
			for (int x = 0; x < placement.allocation.width; ++x) {
				const auto* src = Sample(x - originX, y - originY);
				auto* dst = destination.data() + ((placement.allocation.y + y) * atlasWidth + placement.allocation.x + x) * 4;
				std::memcpy(dst, src, 4);
			}
		}
	}
}

CMetadataTextureAtlas::CMetadataTextureAtlas(TextureAtlasManifest manifest_, const AtlasVariant& variant_)
	: manifest(std::move(manifest_))
	, variant(variant_)
{
	TextureAtlasManifestLoader::ValidateManifest(manifest);
	ValidateDDSFiles();
}

CMetadataTextureAtlas::~CMetadataTextureAtlas()
{
	DeleteTexture();
}

void CMetadataTextureAtlas::QueuePatch(AtlasPatchDefinition patch)
{
	if (state != State::DEFINED)
		throw content_error("Cannot add a patch after metadata atlas planning: " + manifest.name);
	patch.id = TextureAtlasManifestLoader::NormalizeEntryID(patch.id);
	patches.push_back(std::move(patch));
}

void CMetadataTextureAtlas::ValidateDDSFiles() const
{
	for (const auto& file: variant.files) {
		nv_dds::DDSHeaderInfo info;
		if (!nv_dds::InspectDDSHeader(file, info))
			throw content_error("Invalid DDS page '" + file + "' for atlas " + manifest.name);
		const std::string inspectedFormat = DDSFormatName(info.format);
		const bool formatMatches = inspectedFormat == variant.format || (inspectedFormat == "bc1" && variant.format == "bc1a");
		if (info.width != manifest.width || info.height != manifest.height || info.mipLevels != manifest.mipLevels || info.arraySize != 1 || !formatMatches)
			throw content_error("DDS page does not match manifest variant geometry or format: " + file);
	}
}

void CMetadataTextureAtlas::Plan()
{
	if (state != State::DEFINED)
		throw content_error("Metadata atlas can only be planned once: " + manifest.name);
	TextureAtlasManifestLoader::ValidatePatchSet(manifest, patches);

	std::unordered_map<std::string, const AtlasEntryDefinition*> originals;
	for (const auto& entry: manifest.entries)
		originals.emplace(entry.id, &entry);
	std::vector<ReservedAtlasRequest> requests;
	std::vector<int2> patchSizes;
	std::vector<AtlasPadding> patchPadding;
	for (const auto& patch: patches) {
		CBitmap bitmap;
		if (!bitmap.Load(patch.source) || bitmap.compressed || bitmap.channels != 4)
			throw content_error("Could not decode RGBA patch '" + patch.source + "' for atlas " + manifest.name);
		const auto original = originals.find(patch.id);
		const AtlasPadding padding = patch.padding.value_or(original != originals.end() ? original->second->padding : manifest.defaultPadding);
		ReservedAtlasRequest request {patch.id, int2(bitmap.xsize, bitmap.ysize), padding};
		if (original != originals.end()) {
			request.existingPage = original->second->page;
			request.existingAllocation = original->second->allocation;
		}
		requests.push_back(std::move(request));
		patchSizes.emplace_back(bitmap.xsize, bitmap.ysize);
		patchPadding.push_back(padding);
	}

	const auto placements = ReservedAtlasAlloc(manifest.width, manifest.height, manifest.pages).Plan(requests);
	if (!placements)
		throw content_error("Reserved capacity exhausted while planning atlas " + manifest.name);
	std::unordered_map<std::string, ReservedAtlasPlacement> placementByID;
	for (const auto& placement: *placements)
		placementByID.emplace(placement.id, placement);

	textures.clear();
	textures.reserve(manifest.entries.size() + patches.size());
	for (const auto& entry: manifest.entries)
		textures.emplace(entry.id, AtlasPixelRectToUV(entry.content, manifest.width, manifest.height, entry.page));
	committedPatches.clear();
	for (size_t i = 0; i < patches.size(); ++i) {
		const auto& placement = placementByID.at(patches[i].id);
		textures[patches[i].id] = AtlasPixelRectToUV(placement.content, manifest.width, manifest.height, placement.page);
		committedPatches.push_back({patches[i], placement, patchPadding[i], patchSizes[i]});
	}
	state = State::PLANNED;
}

void CMetadataTextureAtlas::Finalize()
{
	if (state != State::PLANNED)
		throw content_error("Metadata atlas must be planned before finalization: " + manifest.name);
	if (patches.empty())
		UploadDirect();
	else {
		BuildModifiedPixels();
		UploadModified();
		LOG_L(L_WARNING, "Atlas \"%s\" was modified during startup and is using an uncompressed runtime texture; consider rebuilding it offline.", manifest.name.c_str());
	}
	state = State::FINALIZED;
}

uint32_t CMetadataTextureAtlas::GetTexTarget() const
{
	return manifest.pages.size() > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
}

const AtlasedTexture* CMetadataTextureAtlas::FindTexture(const std::string& name) const
{
	if (state == State::DEFINED)
		return nullptr;
	const auto it = textures.find(TextureAtlasManifestLoader::NormalizeEntryID(name));
	return it != textures.end() ? &it->second : nullptr;
}

AtlasedTexture* CMetadataTextureAtlas::GetTexturePtr(const std::string& name)
{
	return const_cast<AtlasedTexture*>(FindTexture(name));
}

std::string CMetadataTextureAtlas::GetTextureName(const AtlasedTexture* texture) const
{
	if (textureNames.empty()) {
		for (const auto& [name, value]: textures)
			textureNames.emplace(&value, name);
	}
	const auto it = textureNames.find(texture);
	return it != textureNames.end() ? it->second : "";
}

void CMetadataTextureAtlas::DeleteTexture()
{
#ifndef HEADLESS
	if (textureID != 0)
		glDeleteTextures(1, &textureID);
#endif
	textureID = 0;
}

void CMetadataTextureAtlas::UploadDirect()
{
#ifdef HEADLESS
	compressed = variant.format != "rgba8";
	return;
#else
	DeleteTexture();
	std::vector<nv_dds::CDDSImage> images(variant.files.size());
	for (size_t i = 0; i < images.size(); ++i) {
		if (!images[i].load(variant.files[i], nv_dds::DDSOrientation::Preserve))
			throw content_error("Could not load DDS page " + variant.files[i]);
	}
	compressed = images.front().is_compressed();
	glGenTextures(1, &textureID);
	glBindTexture(GetTexTarget(), textureID);
	if (images.size() == 1) {
		if (!images.front().upload_texture2D(0, GL_TEXTURE_2D))
			throw content_error("Could not upload atlas " + manifest.name);
	} else {
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, manifest.mipLevels, images.front().get_internal_format(), manifest.width, manifest.height, images.size());
		for (size_t page = 0; page < images.size(); ++page) {
			const auto& image = images[page];
			for (uint32_t level = 0; level < manifest.mipLevels; ++level) {
				const auto& surface = level == 0 ? static_cast<const nv_dds::CSurface&>(image.get_image()) : image.get_mipmap(level - 1);
				if (image.is_compressed())
					glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, 0, 0, page, surface.get_width(), surface.get_height(), 1, image.get_internal_format(), surface.get_size(), surface);
				else
					glTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, 0, 0, page, surface.get_width(), surface.get_height(), 1, image.get_external_format(), GL_UNSIGNED_BYTE, surface);
			}
		}
	}
	glTexParameteri(GetTexTarget(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GetTexTarget(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GetTexTarget(), GL_TEXTURE_MIN_FILTER, manifest.mipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GetTexTarget(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
}

void CMetadataTextureAtlas::BuildModifiedPixels()
{
#ifdef HEADLESS
	return;
#else
	committedPixels.assign(manifest.pages.size(), std::vector<uint8_t>(static_cast<size_t>(manifest.width) * manifest.height * 4));
	for (size_t page = 0; page < variant.files.size(); ++page) {
		nv_dds::CDDSImage image;
		if (!image.load(variant.files[page], nv_dds::DDSOrientation::Preserve))
			throw content_error("Could not load base DDS page " + variant.files[page]);
		uint32_t temporary = 0;
		glGenTextures(1, &temporary);
		glBindTexture(GL_TEXTURE_2D, temporary);
		if (!image.upload_texture2D(0, GL_TEXTURE_2D)) {
			glDeleteTextures(1, &temporary);
			throw content_error("Could not upload base DDS page " + variant.files[page]);
		}
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, committedPixels[page].data());
		glDeleteTextures(1, &temporary);
	}
	std::unordered_map<std::string, const AtlasEntryDefinition*> originals;
	for (const auto& entry: manifest.entries)
		originals.emplace(entry.id, &entry);
	for (const auto& patch: committedPatches) {
		const auto old = originals.find(patch.definition.id);
		if (old != originals.end() && !patch.placement.reusedExisting) {
			const auto& rect = old->second->allocation;
			for (int y = rect.y; y < rect.Bottom64(); ++y)
				std::memset(committedPixels[old->second->page].data() + (static_cast<size_t>(y) * manifest.width + rect.x) * 4, 0, static_cast<size_t>(rect.width) * 4);
		}
		CBitmap bitmap;
		if (!bitmap.Load(patch.definition.source) || bitmap.compressed || bitmap.channels != 4 || bitmap.xsize != patch.size.x || bitmap.ysize != patch.size.y)
			throw content_error("Patch changed or failed to reload during finalization: " + patch.definition.source);
		CopyPatchPixels(committedPixels[patch.placement.page], manifest.width, bitmap, patch.placement, patch.padding);
	}
#endif
}

void CMetadataTextureAtlas::UploadModified()
{
	compressed = false;
#ifndef HEADLESS
	DeleteTexture();
	glGenTextures(1, &textureID);
	glBindTexture(GetTexTarget(), textureID);
	if (committedPixels.size() == 1) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, manifest.width, manifest.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, committedPixels.front().data());
	} else {
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, manifest.width, manifest.height, committedPixels.size(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		for (size_t page = 0; page < committedPixels.size(); ++page)
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, page, manifest.width, manifest.height, 1, GL_RGBA, GL_UNSIGNED_BYTE, committedPixels[page].data());
	}
	glGenerateMipmap(GetTexTarget());
	glTexParameteri(GetTexTarget(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GetTexTarget(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GetTexTarget(), GL_TEXTURE_MIN_FILTER, manifest.mipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GetTexTarget(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
}

void CMetadataTextureAtlas::ReloadTexture()
{
	if (state != State::FINALIZED)
		throw content_error("Cannot reload an atlas before finalization: " + manifest.name);
	if (patches.empty())
		UploadDirect();
	else
		UploadModified();
}
