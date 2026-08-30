/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TextureAtlasRegistry.h"

#include <algorithm>

#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"

CONFIG(std::string, TextureAtlasVariant)
	.defaultValue("auto")
	.description("Forces game metadata atlases to use a named texture format variant");

CTextureAtlasRegistry* textureAtlasRegistry = nullptr;

namespace {
	bool IsSupported(const std::string& format)
	{
#ifdef HEADLESS
		return true;
#else
		if (format == "rgba8") return true;
		if (format == "bc1" || format == "bc1a" || format == "bc2" || format == "bc3") return globalRendering->SupportsS3TC();
		if (format == "bc4" || format == "bc5") return globalRendering->SupportsRGTC();
		if (format == "bc7") return globalRendering->SupportsBPTC();
		return false;
#endif
	}
}

std::string CTextureAtlasRegistry::NormalizeAtlasName(const std::string& name)
{
	std::string normalized = name;
	if (normalized.starts_with("$atlas:"))
		normalized.erase(0, 7);
	return TextureAtlasManifestLoader::NormalizeEntryID(normalized);
}

CTextureAtlasRegistry::Entry& CTextureAtlasRegistry::RequireEntry(const std::string& name)
{
	const auto it = entries.find(NormalizeAtlasName(name));
	if (it == entries.end())
		throw content_error("Unknown engine texture atlas: " + name);
	return it->second;
}

const CTextureAtlasRegistry::Entry& CTextureAtlasRegistry::RequireEntry(const std::string& name) const
{
	const auto it = entries.find(NormalizeAtlasName(name));
	if (it == entries.end())
		throw content_error("Unknown engine texture atlas: " + name);
	return it->second;
}

void CTextureAtlasRegistry::LoadDefinitions()
{
	if (!entries.empty())
		throw content_error("Texture atlas registry definitions were loaded more than once");
	std::string forced = configHandler != nullptr ? configHandler->GetString("TextureAtlasVariant") : "auto";
	std::transform(forced.begin(), forced.end(), forced.begin(), [](unsigned char c) { return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c; });
	for (const auto& definition: TextureAtlasManifestLoader::LoadRegistry()) {
		TextureAtlasManifest manifest = TextureAtlasManifestLoader::LoadManifest(definition.manifest);
		if (manifest.name != definition.id)
			throw content_error("Registry ID '" + definition.id + "' does not match manifest name '" + manifest.name + "'");
		const AtlasVariant* selected = nullptr;
		if (forced == "auto") {
			for (const auto& candidate: manifest.variants) {
				if (IsSupported(candidate.format)) {
					selected = &candidate;
					break;
				}
			}
		} else {
			selected = TextureAtlasManifestLoader::ResolveVariantFiles(manifest, forced);
			if (selected != nullptr && !IsSupported(selected->format))
				selected = nullptr;
		}
		if (selected == nullptr)
			throw content_error("No compatible DDS variant for atlas " + definition.id + " (requested " + forced + ")");

		const AtlasVariant selectedVariant = *selected;
		auto atlas = std::make_unique<CMetadataTextureAtlas>(std::move(manifest), selectedVariant);
		std::vector<AtlasPatchDefinition> allPatches;
		for (const auto& sidecar: definition.patches) {
			auto patches = TextureAtlasManifestLoader::LoadPatchSidecar(sidecar);
			allPatches.insert(allPatches.end(), std::make_move_iterator(patches.begin()), std::make_move_iterator(patches.end()));
		}
		TextureAtlasManifestLoader::ValidatePatchSet(atlas->GetManifest(), allPatches);
		for (auto& patch: allPatches)
			atlas->QueuePatch(std::move(patch));

		Entry entry;
		entry.atlas = atlas.get();
		entry.role = definition.role;
		entry.owned = std::move(atlas);
		if (!entry.role.empty())
			roles.emplace(entry.role, entry.atlas);
		LOG_L(L_INFO, "Defined metadata atlas \"%s\" variant=%s pages=%u size=%dx%d mips=%d", definition.id.c_str(), selectedVariant.format.c_str(), entry.atlas->GetNumPages(), entry.atlas->GetSize().x, entry.atlas->GetSize().y, entry.atlas->GetNumTexLevels());
		entries.emplace(definition.id, std::move(entry));
	}
}

ITextureAtlas* CTextureAtlasRegistry::GetAtlas(const std::string& name) const
{
	const auto it = entries.find(NormalizeAtlasName(name));
	return it != entries.end() ? it->second.atlas : nullptr;
}

ITextureAtlas* CTextureAtlasRegistry::GetAtlasByRole(const std::string& role) const
{
	std::string normalized = role;
	std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) { return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c; });
	const auto it = roles.find(normalized);
	return it != roles.end() ? it->second : nullptr;
}

void CTextureAtlasRegistry::QueueStartupPatch(const std::string& name, AtlasPatchDefinition patch)
{
	auto& entry = RequireEntry(name);
	if (!entry.owned)
		throw content_error("Cannot patch procedural atlas " + name);
	entry.owned->QueuePatch(std::move(patch));
}

void CTextureAtlasRegistry::PlanAtlas(const std::string& name)
{
	auto& entry = RequireEntry(name);
	if (entry.owned && entry.owned->GetState() == CMetadataTextureAtlas::State::DEFINED)
		entry.owned->Plan();
}

void CTextureAtlasRegistry::FinalizeAtlas(const std::string& name)
{
	auto& entry = RequireEntry(name);
	if (!entry.owned)
		return;
	if (entry.owned->GetState() == CMetadataTextureAtlas::State::DEFINED)
		entry.owned->Plan();
	if (entry.owned->GetState() == CMetadataTextureAtlas::State::PLANNED)
		entry.owned->Finalize();
}

void CTextureAtlasRegistry::FinalizeRemainingAtlases()
{
	for (auto& [name, entry]: entries) {
		if (!entry.owned)
			continue;
		if (entry.owned->GetState() == CMetadataTextureAtlas::State::DEFINED)
			entry.owned->Plan();
		if (entry.owned->GetState() == CMetadataTextureAtlas::State::PLANNED)
			entry.owned->Finalize();
	}
}

void CTextureAtlasRegistry::RegisterProceduralAtlas(const std::string& name, const std::string& role, ITextureAtlas* atlas)
{
	if (atlas == nullptr)
		throw content_error("Cannot register a null procedural texture atlas");
	const std::string normalized = NormalizeAtlasName(name);
	if (entries.contains(normalized))
		throw content_error("Texture atlas is already registered: " + name);
	Entry entry {.atlas = atlas, .role = role, .procedural = true};
	entries.emplace(normalized, std::move(entry));
	if (!role.empty() && !roles.emplace(role, atlas).second) {
		entries.erase(normalized);
		throw content_error("Texture atlas role is already registered: " + role);
	}
}

void CTextureAtlasRegistry::UnregisterProceduralAtlas(ITextureAtlas* atlas)
{
	for (auto it = entries.begin(); it != entries.end();) {
		if (it->second.procedural && it->second.atlas == atlas) {
			if (!it->second.role.empty())
				roles.erase(it->second.role);
			it = entries.erase(it);
		} else {
			++it;
		}
	}
}

void CTextureAtlasRegistry::ReloadAtlas(const std::string& name)
{
	RequireEntry(name).atlas->ReloadTexture();
}

void CTextureAtlasRegistry::Kill()
{
	roles.clear();
	entries.clear();
}
