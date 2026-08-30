/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TextureAtlasManifest.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <unordered_set>

#include "Lua/LuaParser.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSModes.h"

namespace {
	[[noreturn]] void Fail(const std::string& path, const std::string& message)
	{
		throw content_error("Texture atlas " + path + ": " + message);
	}

	std::string FoldASCII(std::string value)
	{
		std::replace(value.begin(), value.end(), '\\', '/');
		for (char& c: value) {
			if (c >= 'A' && c <= 'Z')
				c += 'a' - 'A';
		}
		return value;
	}

	void RequireType(const LuaTable& table, const std::string& key, LuaTable::DataType type, bool optional = false)
	{
		const auto actual = table.GetType(key);
		if (optional && actual == LuaTable::NIL)
			return;
		if (actual != type)
			Fail(table.GetPath(), "field '" + key + "' has the wrong type");
	}

	void CheckFields(const LuaTable& table, std::initializer_list<const char*> allowed)
	{
		std::set<std::string> allowedSet;
		for (const char* key: allowed)
			allowedSet.emplace(FoldASCII(key));
		std::vector<std::string> keys;
		table.GetKeys(keys);
		for (const auto& key: keys) {
			if (!allowedSet.contains(FoldASCII(key)))
				Fail(table.GetPath(), "unexpected field '" + key + "'");
		}
	}

	int32_t GetInt(const LuaTable& table, const std::string& key, bool optional = false, int32_t defaultValue = 0)
	{
		if (optional && table.GetType(key) == LuaTable::NIL)
			return defaultValue;
		RequireType(table, key, LuaTable::NUMBER);
		const float value = table.GetFloat(key, 0.0f);
		if (!std::isfinite(value) || std::floor(value) != value || value < std::numeric_limits<int32_t>::min() || value > std::numeric_limits<int32_t>::max())
			Fail(table.GetPath(), "field '" + key + "' must be a representable integer");
		return static_cast<int32_t>(value);
	}

	std::string GetString(const LuaTable& table, const std::string& key, bool optional = false, const std::string& defaultValue = {})
	{
		if (optional && table.GetType(key) == LuaTable::NIL)
			return defaultValue;
		RequireType(table, key, LuaTable::STRING);
		return table.GetString(key, defaultValue);
	}

	LuaTable GetTable(const LuaTable& table, const std::string& key, bool optional = false)
	{
		if (optional && table.GetType(key) == LuaTable::NIL)
			return LuaTable{};
		RequireType(table, key, LuaTable::TABLE);
		return table.SubTable(key);
	}

	void CheckArray(const LuaTable& table)
	{
		std::vector<std::string> stringKeys;
		table.GetKeys(stringKeys);
		if (!stringKeys.empty())
			Fail(table.GetPath(), "array contains named fields");
		std::vector<int> keys;
		table.GetKeys(keys);
		std::sort(keys.begin(), keys.end());
		for (int i = 0; i < keys.size(); ++i) {
			if (keys[i] != i + 1)
				Fail(table.GetPath(), "array keys must be contiguous and one-based");
		}
	}

	std::vector<std::string> GetStringArray(const LuaTable& parent, const std::string& key, bool optional = false)
	{
		const LuaTable table = GetTable(parent, key, optional);
		if (!table.IsValid())
			return {};
		CheckArray(table);
		std::vector<std::string> result;
		for (int i = 1; i <= table.GetLength(); ++i) {
			if (table.GetType(i) != LuaTable::STRING)
				Fail(table.GetPath(), "array values must be strings");
			result.push_back(table.GetString(i, ""));
		}
		return result;
	}

	AtlasPixelRect ParseRect(const LuaTable& table)
	{
		CheckFields(table, {"x", "y", "width", "height"});
		return {GetInt(table, "x"), GetInt(table, "y"), GetInt(table, "width"), GetInt(table, "height")};
	}

	AtlasPadding ParsePadding(const LuaTable& table)
	{
		CheckFields(table, {"mode", "pixels", "tilesx", "tilesy"});
		const std::string mode = FoldASCII(GetString(table, "mode"));
		AtlasPadding padding;
		if (mode == "clamp") {
			padding.mode = AtlasPaddingMode::CLAMP;
			const int32_t pixels = GetInt(table, "pixels");
			if (pixels < 0)
				Fail(table.GetPath(), "clamp pixels cannot be negative");
			padding.pixels = pixels;
			if (table.KeyExists("tilesx") || table.KeyExists("tilesy"))
				Fail(table.GetPath(), "clamp padding cannot specify tiles");
		} else if (mode == "tile") {
			padding.mode = AtlasPaddingMode::TILE;
			const int32_t tilesX = GetInt(table, "tilesx");
			const int32_t tilesY = GetInt(table, "tilesy");
			if (tilesX < 0 || tilesY < 0)
				Fail(table.GetPath(), "tile counts cannot be negative");
			padding.tilesX = tilesX;
			padding.tilesY = tilesY;
			if (table.KeyExists("pixels"))
				Fail(table.GetPath(), "tile padding cannot specify pixels");
		} else {
			Fail(table.GetPath(), "padding mode must be 'clamp' or 'tile'");
		}
		return padding;
	}

	LuaTable ParseFile(const std::string& fileName, LuaParser& parser)
	{
		if (!parser.Execute())
			Fail(fileName, parser.GetErrorLog());
		if (parser.NoTable())
			Fail(fileName, "file did not return a table");
		return parser.GetRoot();
	}
}

std::string TextureAtlasManifestLoader::NormalizeEntryID(const std::string& id)
{
	if (id.empty())
		throw content_error("Texture atlas entry ID cannot be empty");
	std::string result = FoldASCII(id);
	while (result.starts_with("./"))
		result.erase(0, 2);
	if (result.empty() || result.starts_with('/') || result == ".." || result.starts_with("../") || result.find("/../") != std::string::npos || result.ends_with("/.."))
		throw content_error("Texture atlas entry ID escapes its root: " + id);
	return result;
}

std::vector<TextureAtlasRegistryDefinition> TextureAtlasManifestLoader::LoadRegistry(const std::string& fileName)
{
	if (!CFileHandler::FileExists(fileName, SPRING_VFS_MOD_BASE))
		return {};
	LuaParser parser(fileName, SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	const LuaTable root = ParseFile(fileName, parser);
	std::vector<int> numericKeys;
	root.GetKeys(numericKeys);
	if (!numericKeys.empty())
		Fail(fileName, "registry must use atlas IDs as keys");

	std::vector<std::string> ids;
	root.GetKeys(ids);
	std::sort(ids.begin(), ids.end());
	std::unordered_set<std::string> normalizedIDs;
	std::unordered_set<std::string> roles;
	std::vector<TextureAtlasRegistryDefinition> result;
	for (const auto& id: ids) {
		if (root.GetType(id) != LuaTable::TABLE)
			Fail(fileName, "atlas '" + id + "' must be a table");
		const LuaTable table = root.SubTable(id);
		CheckFields(table, {"manifest", "patches", "role"});
		TextureAtlasRegistryDefinition definition;
		definition.id = NormalizeEntryID(id);
		definition.manifest = GetString(table, "manifest");
		definition.patches = GetStringArray(table, "patches", true);
		definition.role = FoldASCII(GetString(table, "role", true));
		if (!normalizedIDs.emplace(definition.id).second)
			Fail(fileName, "duplicate case-folded atlas ID '" + id + "'");
		if (!definition.role.empty()) {
			static const std::unordered_set<std::string> validRoles = {"explosions", "groundfx", "decals", "icons0"};
			if (!validRoles.contains(definition.role))
				Fail(fileName, "invalid role '" + definition.role + "'");
			if (!roles.emplace(definition.role).second)
				Fail(fileName, "role '" + definition.role + "' is assigned more than once");
		}
		result.push_back(std::move(definition));
	}
	return result;
}

TextureAtlasManifest TextureAtlasManifestLoader::LoadManifest(const std::string& fileName)
{
	LuaParser parser(fileName, SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	const LuaTable root = ParseFile(fileName, parser);
	CheckFields(root, {"schema", "version", "name", "target", "width", "height", "miplevels", "coordinateorigin", "variants", "pages", "defaultpadding", "entries"});
	if (GetString(root, "schema") != "recoil.texture-atlas" || GetInt(root, "version") != 2)
		Fail(fileName, "unsupported schema or version");

	TextureAtlasManifest manifest;
	manifest.name = NormalizeEntryID(GetString(root, "name"));
	manifest.target = FoldASCII(GetString(root, "target"));
	manifest.width = GetInt(root, "width");
	manifest.height = GetInt(root, "height");
	manifest.mipLevels = GetInt(root, "miplevels");
	if (FoldASCII(GetString(root, "coordinateorigin")) != "top-left")
		Fail(fileName, "coordinateOrigin must be 'top-left'");
	manifest.defaultPadding = ParsePadding(GetTable(root, "defaultpadding"));

	const LuaTable variants = GetTable(root, "variants");
	CheckArray(variants);
	for (int i = 1; i <= variants.GetLength(); ++i) {
		if (variants.GetType(i) != LuaTable::TABLE)
			Fail(variants.GetPath(), "variant must be a table");
		const LuaTable table = variants.SubTable(i);
		CheckFields(table, {"id", "format", "files", "sha256"});
		manifest.variants.push_back({GetString(table, "id"), FoldASCII(GetString(table, "format")), GetStringArray(table, "files"), GetStringArray(table, "sha256", true)});
	}

	const LuaTable pages = GetTable(root, "pages");
	CheckArray(pages);
	for (int i = 1; i <= pages.GetLength(); ++i) {
		if (pages.GetType(i) != LuaTable::TABLE)
			Fail(pages.GetPath(), "page must be a table");
		const LuaTable table = pages.SubTable(i);
		CheckFields(table, {"reserve"});
		const LuaTable reserves = GetTable(table, "reserve", true);
		AtlasPageDefinition page;
		if (reserves.IsValid()) {
			CheckArray(reserves);
			for (int j = 1; j <= reserves.GetLength(); ++j) {
				if (reserves.GetType(j) != LuaTable::TABLE)
					Fail(reserves.GetPath(), "reserve must be a rectangle table");
				page.reserves.push_back(ParseRect(reserves.SubTable(j)));
			}
		}
		manifest.pages.push_back(std::move(page));
	}

	const LuaTable entries = GetTable(root, "entries");
	std::vector<int> entryIndices;
	entries.GetKeys(entryIndices);
	if (!entryIndices.empty())
		Fail(entries.GetPath(), "entries must use string IDs");
	std::vector<std::string> entryIDs;
	entries.GetKeys(entryIDs);
	for (const auto& originalID: entryIDs) {
		if (entries.GetType(originalID) != LuaTable::TABLE)
			Fail(entries.GetPath(), "entry '" + originalID + "' must be a table");
		const LuaTable table = entries.SubTable(originalID);
		CheckFields(table, {"page", "source", "sourcewidth", "sourceheight", "content", "allocation", "padding"});
		AtlasEntryDefinition entry;
		entry.id = NormalizeEntryID(originalID);
		entry.source = GetString(table, "source");
		entry.page = GetInt(table, "page");
		entry.sourceSize = int2(GetInt(table, "sourcewidth"), GetInt(table, "sourceheight"));
		entry.content = ParseRect(GetTable(table, "content"));
		entry.allocation = ParseRect(GetTable(table, "allocation"));
		entry.padding = ParsePadding(GetTable(table, "padding"));
		manifest.entries.push_back(std::move(entry));
	}
	ValidateManifest(manifest);
	return manifest;
}

std::vector<AtlasPatchDefinition> TextureAtlasManifestLoader::LoadPatchSidecar(const std::string& fileName)
{
	LuaParser parser(fileName, SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	const LuaTable root = ParseFile(fileName, parser);
	CheckFields(root, {"schema", "version", "entries"});
	if (GetString(root, "schema") != "recoil.texture-atlas-patch" || GetInt(root, "version") != 1)
		Fail(fileName, "unsupported patch schema or version");
	const LuaTable entries = GetTable(root, "entries");
	std::vector<std::string> ids;
	entries.GetKeys(ids);
	std::vector<int> numericKeys;
	entries.GetKeys(numericKeys);
	if (!numericKeys.empty())
		Fail(fileName, "patch entries must use string IDs");
	std::vector<AtlasPatchDefinition> result;
	for (const auto& id: ids) {
		if (entries.GetType(id) != LuaTable::TABLE)
			Fail(entries.GetPath(), "patch entry must be a table");
		const LuaTable table = entries.SubTable(id);
		CheckFields(table, {"source", "padding"});
		AtlasPatchDefinition patch {NormalizeEntryID(id), GetString(table, "source"), std::nullopt};
		if (table.GetType("padding") != LuaTable::NIL)
			patch.padding = ParsePadding(GetTable(table, "padding"));
		result.push_back(std::move(patch));
	}
	std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) { return lhs.id < rhs.id; });
	return result;
}

void TextureAtlasManifestLoader::ValidateManifest(const TextureAtlasManifest& manifest)
{
	if (manifest.name.empty() || manifest.target != "2d" || manifest.width <= 0 || manifest.height <= 0 || manifest.mipLevels == 0 || manifest.pages.empty())
		throw content_error("Texture atlas manifest has invalid identity, target, dimensions, mip count, or pages");
	const uint32_t maxMipLevels = 1 + static_cast<uint32_t>(std::floor(std::log2(std::max(manifest.width, manifest.height))));
	if (manifest.mipLevels > maxMipLevels)
		throw content_error("Texture atlas manifest declares more mip levels than its dimensions allow: " + manifest.name);
	std::unordered_set<std::string> ids;
	std::vector<std::vector<AtlasPixelRect>> occupied(manifest.pages.size());
	for (const auto& entry: manifest.entries) {
		if (!ids.emplace(entry.id).second)
			throw content_error("Texture atlas has duplicate case-folded entry ID: " + entry.id);
		if (entry.page >= manifest.pages.size() || !entry.content.FitsInside(manifest.width, manifest.height) || !entry.allocation.FitsInside(manifest.width, manifest.height) || !entry.allocation.Contains(entry.content))
			throw content_error("Texture atlas entry has invalid geometry: " + entry.id);
		const auto expectedSize = GetPaddedAtlasSize(entry.sourceSize, entry.padding);
		const auto expectedContent = GetContentRect(entry.allocation, entry.sourceSize, entry.padding);
		if (!expectedSize || expectedSize->x != entry.allocation.width || expectedSize->y != entry.allocation.height || !expectedContent || *expectedContent != entry.content)
			throw content_error("Texture atlas entry padding does not match its rectangles: " + entry.id);
		for (const auto& rect: occupied[entry.page]) {
			if (rect.Overlaps(entry.allocation))
				throw content_error("Texture atlas entries overlap on page " + std::to_string(entry.page));
		}
		occupied[entry.page].push_back(entry.allocation);
	}
	for (uint32_t page = 0; page < manifest.pages.size(); ++page) {
		for (const auto& reserve: manifest.pages[page].reserves) {
			if (!reserve.FitsInside(manifest.width, manifest.height))
				throw content_error("Texture atlas reserve is outside its page");
			for (const auto& rect: occupied[page]) {
				if (rect.Overlaps(reserve))
					throw content_error("Texture atlas reserve overlaps occupied geometry");
			}
			occupied[page].push_back(reserve);
		}
	}
	if (manifest.variants.empty())
		throw content_error("Texture atlas manifest has no variants");
	static const std::unordered_set<std::string> formats = {"bc1", "bc1a", "bc2", "bc3", "bc4", "bc5", "bc7", "rgba8"};
	std::unordered_set<std::string> variantIDs;
	for (const auto& variant: manifest.variants) {
		if (variant.id.empty() || !variantIDs.emplace(FoldASCII(variant.id)).second || !formats.contains(variant.format) || variant.files.size() != manifest.pages.size() || (!variant.sha256.empty() && variant.sha256.size() != manifest.pages.size()))
			throw content_error("Texture atlas manifest has an invalid or duplicate variant: " + variant.id);
	}
}

void TextureAtlasManifestLoader::ValidatePatchSet(const TextureAtlasManifest&, const std::vector<AtlasPatchDefinition>& patches)
{
	std::unordered_set<std::string> ids;
	for (const auto& patch: patches) {
		if (patch.id.empty() || patch.source.empty() || !ids.emplace(NormalizeEntryID(patch.id)).second)
			throw content_error("Texture atlas patch set has an invalid or duplicate entry: " + patch.id);
	}
}

const AtlasVariant* TextureAtlasManifestLoader::ResolveVariantFiles(const TextureAtlasManifest& manifest, const std::string& requestedFormat)
{
	const std::string requested = FoldASCII(requestedFormat);
	if (requested == "auto")
		return manifest.variants.empty() ? nullptr : &manifest.variants.front();
	for (const auto& variant: manifest.variants) {
		if (FoldASCII(variant.id) == requested || variant.format == requested)
			return &variant;
	}
	return nullptr;
}
