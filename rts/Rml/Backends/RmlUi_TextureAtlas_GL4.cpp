/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RmlUi_TextureAtlas_GL4.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <functional>
#include <iomanip>
#include <sstream>
#include <utility>

#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/TextureRenderAtlas.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

CONFIG(int, RmlUiGL4AtlasSize)
	.defaultValue(4096)
	.minimumValue(1024)
	.description("Maximum width and height of each RmlUi static texture-atlas page.");

namespace RmlGui {

std::string TextureAtlasManifest::CanonicalizePath(const std::string& path)
{
	std::vector<std::string> components;
	std::string component;
	std::string normalized;
	normalized.reserve(path.size() + 1);

	auto FlushComponent = [&]() {
		if (component.empty() || component == ".") {
			component.clear();
			return;
		}
		if (component == "..") {
			if (!components.empty())
				components.pop_back();
		} else {
			std::ranges::transform(component, component.begin(), [](unsigned char c) { return char(std::tolower(c)); });
			components.push_back(std::move(component));
		}
		component.clear();
	};

	for (char c : path) {
		if (c == '/' || c == '\\')
			FlushComponent();
		else
			component.push_back(c);
	}
	FlushComponent();

	for (const std::string& item : components) {
		normalized.push_back('/');
		normalized.append(item);
	}
	return normalized;
}

bool TextureAtlasManifest::Register(const std::string& path)
{
	if (stats.finalized) {
		LOG_L(L_ERROR, "[RmlUi GL4 atlas] Registration attempted after finalization: %s", path.c_str());
		return false;
	}

	const std::string canonical = CanonicalizePath(path);
	if (canonical.empty()) {
		LOG_L(L_WARNING, "[RmlUi GL4 atlas] Ignoring empty texture path.");
		return false;
	}

	auto [_, inserted] = originals.emplace(canonical, path);
	if (!inserted)
		stats.duplicate_count += 1;
	return true;
}

bool TextureAtlasManifest::Register(const std::vector<std::string>& paths)
{
	bool result = true;
	for (const std::string& path : paths)
		result = Register(path) && result;
	return result;
}

bool TextureAtlasManifest::Finalize()
{
	if (stats.finalized)
		return stats.failure_reason.empty();

	sources.reserve(originals.size());
	for (const auto& [canonical, _] : originals)
		sources.push_back(canonical);
	std::ranges::sort(sources);

	uint64_t hash = 14695981039346656037ull;
	for (const std::string& source : sources) {
		for (unsigned char c : source) {
			hash ^= uint64_t(c);
			hash *= 1099511628211ull;
		}
		hash ^= 0;
		hash *= 1099511628211ull;
	}

	std::ostringstream stream;
	stream << std::hex << std::setfill('0') << std::setw(16) << hash;
	stats.manifest_hash = stream.str();
	stats.source_count = sources.size();
	stats.finalized = true;
	return true;
}

void TextureAtlasManifest::Reset()
{
	originals.clear();
	sources.clear();
	stats = {};
}

StaticTextureAtlasGL4::StaticTextureAtlasGL4() = default;
StaticTextureAtlasGL4::~StaticTextureAtlasGL4() = default;

bool StaticTextureAtlasGL4::Build(TextureAtlasManifest& manifest)
{
	using Clock = std::chrono::steady_clock;
	const auto start = Clock::now();
	if (!manifest.Finalize()) {
		stats = manifest.GetStats();
		stats.build_duration_ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
		return false;
	}

	stats = manifest.GetStats();
	stats.mip_count = 5;
	const int atlas_size = std::min(configHandler->GetInt("RmlUiGL4AtlasSize"), globalRendering->maxTextureSize);
	if (atlas_size <= 0) {
		stats.failure_reason = "OpenGL reports no usable texture size for the static atlas.";
		stats.build_duration_ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
		LOG_L(L_ERROR, "[RmlUi GL4 atlas] %s", stats.failure_reason.c_str());
		return false;
	}
	stats.page_width = atlas_size;
	stats.page_height = atlas_size;
	atlas = std::make_unique<CTextureRenderAtlas>(
		CTextureAtlas::ATLAS_ALLOC_MP_QUADTREE, atlas_size, atlas_size, 5, GL_RGBA8, "RmlUiStaticAtlas", true, 16);
	auto Fail = [&](std::string reason) {
		stats.failure_reason = std::move(reason);
		stats.build_duration_ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
		LOG_L(L_ERROR, "[RmlUi GL4 atlas] %s", stats.failure_reason.c_str());
		atlas.reset();
		return false;
	};

	struct SourceSize {
		std::string source;
		uint64_t pixels;
	};
	std::vector<SourceSize> largest;
	largest.reserve(manifest.GetSources().size());

	for (const std::string& canonical : manifest.GetSources()) {
		const std::string vfs_path = canonical.substr(1);
		CBitmap bitmap;
		if (!bitmap.Load(vfs_path)) {
			return Fail("Registered texture is missing or undecodable: " + canonical);
		}
		if (bitmap.xsize <= 0 || bitmap.ysize <= 0) {
			return Fail("Registered texture has zero dimensions: " + canonical);
		}
		if (bitmap.xsize + 16 > atlas_size || bitmap.ysize + 16 > atlas_size) {
			return Fail("Registered texture exceeds the hardware-clamped atlas page size: " + canonical + " (" +
				std::to_string(bitmap.xsize) + "x" + std::to_string(bitmap.ysize) + " > " + std::to_string(atlas_size) + ")");
		}
		if (!atlas->AddTexFromBitmap(canonical, bitmap, vfs_path)) {
			return Fail("Could not add registered texture to atlas: " + canonical);
		}

		const uint64_t pixels = uint64_t(bitmap.xsize) * uint64_t(bitmap.ysize);
		stats.total_source_pixels += pixels;
		largest.push_back({canonical, pixels});
		records.emplace(canonical, StaticAtlasRecordGL4{{bitmap.xsize, bitmap.ysize}, {}, 0});
	}

	if (!atlas->Finalize()) {
		return Fail("Static atlas packing or GPU creation failed within the 16-page limit.");
	}
	if (atlas->GetNumTexLevels() != 5)
		return Fail("Static atlas could not preserve the required five mip levels; smallest packed source is too small.");

	for (auto& [source, record] : records) {
		if (!atlas->TextureExists(source)) {
			return Fail("Packed atlas has no record for registered texture: " + source);
		}
		const AtlasedTexture texture = atlas->GetTexture(source);
		record.uv_rect = {texture.x1, texture.y1, texture.x2, texture.y2};
		record.page = texture.pageNum;
	}

	const uint2 atlas_dimensions = atlas->GetAtlasSize();
	stats.page_width = atlas_dimensions.x;
	stats.page_height = atlas_dimensions.y;
	stats.page_count = atlas->GetNumPages();
	stats.mip_count = atlas->GetNumTexLevels();
	stats.page_occupancy.assign(stats.page_count, 0.f);
	const double page_pixels = double(stats.page_width) * double(stats.page_height);
	for (const auto& [_, record] : records) {
		if (record.page < stats.page_occupancy.size() && page_pixels > 0.0)
			stats.page_occupancy[record.page] += float(
				double(record.dimensions.x + 16) * double(record.dimensions.y + 16) / page_pixels);
	}

	uint64_t mip_pixels = 0;
	for (uint32_t level = 0; level < stats.mip_count; ++level)
		mip_pixels += uint64_t(std::max(stats.page_width >> level, 1u)) * uint64_t(std::max(stats.page_height >> level, 1u));
	stats.allocated_gpu_bytes = mip_pixels * stats.page_count * 4;

	std::ranges::sort(largest, std::greater{}, &SourceSize::pixels);
	for (size_t i = 0; i < std::min<size_t>(largest.size(), 10); ++i)
		stats.largest_sources.push_back(largest[i].source + " (" + std::to_string(largest[i].pixels) + " px)");

	stats.gpu_atlas_created = true;
	stats.build_duration_ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
	LOG_L(L_INFO, "[RmlUi GL4 atlas] built hash=%s sources=%llu pages=%u size=%ux%u mips=%u bytes=%llu in %.2fms",
		stats.manifest_hash.c_str(), static_cast<unsigned long long>(stats.source_count), stats.page_count,
		stats.page_width, stats.page_height, stats.mip_count,
		static_cast<unsigned long long>(stats.allocated_gpu_bytes), stats.build_duration_ms);
	return true;
}

const StaticAtlasRecordGL4* StaticTextureAtlasGL4::Find(const std::string& canonical_path) const
{
	const auto it = records.find(canonical_path);
	return it == records.end() ? nullptr : &it->second;
}

uint32_t StaticTextureAtlasGL4::GetTextureId() const
{
	return atlas ? atlas->GetTexID() : 0;
}

uint32_t StaticTextureAtlasGL4::GetTextureTarget() const
{
	return atlas ? atlas->GetTexTarget() : GL_TEXTURE_2D_ARRAY;
}

} // namespace RmlGui
