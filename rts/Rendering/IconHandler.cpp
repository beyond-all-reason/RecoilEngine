/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IconHandler.h"

#include <algorithm>
#include <cassert>
#include <locale>
#include <cctype>
#include <cmath>

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderBuffers.h"
#include "System/Log/ILog.h"
#include "System/UnorderedSet.hpp"
#include "System/UnorderedMap.hpp"
#include "System/StringUtil.h"
#include "Lua/LuaParser.h"
#include "Textures/Bitmap.h"
#include "Textures/TextureRenderAtlas.h"
#include "System/Exceptions.h"

namespace icon {

CIconHandler iconHandler;

/******************************************************************************/
//
//  CIconHandler
//

void CIconHandler::Kill()
{
	glDeleteTextures(2, atlasTextureIDs.data());

	iconsMap.clear();
	iconsData.clear();
}


void CIconHandler::DumpAtlasTextures() const
{
	if (atlasTextureIDs[0])
		glSaveTexture(atlasTextureIDs[0], "IconsAtlas1.png");
	if (atlasTextureIDs[1])
		glSaveTexture(atlasTextureIDs[1], "IconsAtlas2.png");
}

bool CIconHandler::UpdateAtlasData(size_t atlasIdx)
{
	auto& atlas = atlases[atlasIdx];
	if (atlas)
		return true;

	atlas = std::make_unique<CTextureRenderAtlas>(CTextureAtlas::ATLAS_ALLOC_LEGACY, 0, 0, GL_RGBA8, IntToString(atlasIdx, "IconsAtlas_%i"));

	spring::unordered_set<std::string> invalidIcons;
	for (const auto& [iconName, iconIndex] : iconsMap) {

		const auto& iconData = iconsData[iconIndex];

		if (iconData.GetAtlasIndex() != atlasIdx)
			continue;

		if (!atlas->AddTexFromFile(iconName, iconData.GetFileName(), iconData.GetSrcTexCoords())) {
			LOG_L(L_WARNING, "[CIconHandler::%s] Failed to load icon=%s, bitmap=%s", __func__, iconName.c_str(), iconData.GetFileName().c_str());
			invalidIcons.emplace(iconName);
		}
	}

	// everything above was loaded from a single image atlas, no need to run CTextureRenderAtlas machinery
	if (auto allFiles = atlas->GetAllFileNames(); allFiles.size() == 1) {
		const auto& uniqueSubTexMap = atlas->GetUniqueSubTexMap();
		for (const auto& [iconName, uniqSubTexName] : atlas->GetNameToUniqueSubTexMap()) {
			auto it = uniqueSubTexMap.find(uniqSubTexName);
			auto& item = iconsData[*iconsMap.try_get(iconName)];
			item.SetTexCoords(AtlasedTexture{ it->second.subTexCoords, 0 });
		}
	}

	if (auto it = invalidIcons.find("default"); it != invalidIcons.end()) {
		bool res = atlas->AddTexFromFile("default", "bitmaps/defaultradardot.png");
		assert(res);
		invalidIcons.erase(it);
	}

	if (!atlas->CalculateAtlas()) {
		atlas = nullptr;
		return false;
	}

	for (const auto& [iconName, _] : atlas->GetNameToUniqueSubTexMap()) {
		auto& item = iconsData[*iconsMap.try_get(iconName)];
		item.SetTexCoords(atlas->GetTexture(iconName));
	}

	const auto& defTC = iconsData[defaultIconIdx].GetTexCoords();
	for (const auto& iconName : invalidIcons) {
		auto& item = iconsData[*iconsMap.try_get(iconName)];
		item.SetTexCoords(defTC);
	}

	return true;
}

bool CIconHandler::CreateAtlasTexture(size_t atlasIdx)
{
	auto& atlas = atlases[atlasIdx];

	if (!atlas)
		return false;

	if (auto allFiles = atlas->GetAllFileNames(); allFiles.size() == 1) {
		CBitmap bm;

		if (!bm.Load(*allFiles.begin()))
			return false;

		glDeleteTextures(1, &atlasTextureIDs[atlasIdx]);
		atlasTextureIDs[atlasIdx] = bm.CreateMipMapTexture();
		atlasTextureSizes[atlasIdx] = int2(bm.xsize, bm.ysize);

		return true;
	}

	atlas->SetMaxTexLevel(DEFAULT_NUM_OF_TEXTURE_LEVELS);
	if (!atlas->CreateAtlasTexture())
		return false;

	atlasTextureSizes[atlasIdx] = atlas->GetAtlasSize();

	if (atlasTextureIDs[atlasIdx])
		glDeleteTextures(1, &atlasTextureIDs[atlasIdx]);

	atlasTextureIDs[atlasIdx] = atlas->DisownTexture();
	atlas = nullptr;

	return true;
}

void CIconHandler::LoadIcons(const std::string& filename)
{
	LuaParser luaParser(filename, SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);

	if (!luaParser.Execute())
		throw content_error(fmt::format("{}: {}", filename, luaParser.GetErrorLog()));

	const LuaTable iconTypes = luaParser.GetRoot();

	std::vector<std::string> iconNames;
	iconTypes.GetKeys(iconNames);

	for (const auto& iconName : iconNames) {
		const auto iconTable = iconTypes.SubTable(iconName);
		AddIcon(0,
			iconName,
			iconTable.GetString("bitmap", ""),
			iconTable.GetFloat("size", 1.0f),
			iconTable.GetFloat("distance", 1.0f),
			iconTable.GetBool("radiusAdjust", false),
			iconTable.GetFloat("u0", 0.0f),
			iconTable.GetFloat("v0", 0.0f),
			iconTable.GetFloat("u1", 1.0f),
			iconTable.GetFloat("v1", 1.0f)
		);
	}

	atlasNeedsUpdate.set(0);

	// force Update()
	Update();
}

bool CIconHandler::AddIcon(
	uint32_t atlasIndex,
	const std::string& iconName,
	const std::string& texFileName,
	float size,
	float distance,
	bool radAdj,
	float u0, float v0, float u1, float v1
) {
	auto it = iconsMap.find(iconName);
	if (it != iconsMap.end()) {
		FreeIcon(iconName);
	} else {
		it = iconsMap.emplace(iconName, iconsData.size()).first;
		iconsData.emplace_back();
	}

	iconsData[it->second] = IconData{
		iconName,
		texFileName,
		size,
		distance,
		atlasIndex,
		radAdj,
		float4{u0, v0, u1, v1}
	};

	// do basic check instead of the full load procedure
	return CFileHandler::FileExists(texFileName, SPRING_VFS_ALL);
}


bool CIconHandler::FreeIcon(const std::string& iconName)
{
	if (iconName == "default")
		return false;

	auto it = iconsMap.find(iconName);

	if (it == iconsMap.end())
		return false;

	if (const auto& ai = iconsData[it->second].GetAtlasIndex(); ai > 0) {
		atlasNeedsUpdate.set(ai);
	}

	iconsData[it->second] = IconData{};
	it->second = defaultIconIdx;

	return true;
}

void CIconHandler::Update()
{
	if (atlasNeedsUpdate.none())
		return;

	auto defIt = iconsMap.find("default");
	if (defIt == iconsMap.end()) {
		defIt = iconsMap.emplace("default", iconsData.size()).first;
		iconsData.emplace_back(
			"default",
			"bitmaps/defaultradardot.png",
			1.0f,
			1.0f,
			0,
			false,
			float4{
				0.0f,
				0.0f,
				1.0f,
				1.0f
			}
		);
	}

	defaultIconIdx = defIt->second;

	for (size_t i = 0; i < atlasNeedsUpdate.size(); ++i) {
		if (!atlasNeedsUpdate.test(i))
			continue;

		if (UpdateAtlasData(i) && CreateAtlasTexture(i))
			atlasNeedsUpdate.reset(i);
	}
}

std::pair<bool, spring::unordered_map<std::string, size_t>::const_iterator> CIconHandler::FindIconIdx(const std::string& iconName) const
{
	const auto it = iconsMap.find(iconName);
	return std::make_pair(it != iconsMap.end(), it);
}

const IconData& CIconHandler::GetIconData(const std::string& iconName) const
{
	const auto* ptrIconIdx = iconsMap.try_get(iconName);
	assert(ptrIconIdx != nullptr && (*ptrIconIdx) < iconsData.size());
	return iconsData[*ptrIconIdx];
}

const IconData& CIconHandler::GetIconData(size_t iconIdx) const
{
	assert(iconIdx < iconsData.size());
	return iconsData[iconIdx];
}

size_t CIconHandler::GetIconIdx(const std::string& iconName) const
{
	if (const auto& [found, it] = FindIconIdx(iconName); found)
		return it->second;
	else
		return INVALID_ICON_INDEX;
}

size_t CIconHandler::GetIconIdxOrDefault(const std::string& iconName) const
{
	if (const auto& [found, it] = FindIconIdx(iconName); found)
		return it->second;
	else
		return defaultIconIdx;
}

}