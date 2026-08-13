/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "RmlUi_Renderer_Config.h"

#include <cstdint>
#include <string>

#include <RmlUi/Core/RenderGeometry.h>
#include <RmlUi/Core/Types.h>

enum class TextureSourceKindGL4 : uint32_t {
	None,
	StaticAtlas,
	GeneratedAtlas,
	External2D,
	RenderTarget,
};

struct TextureHandleGL4 {
	TextureSourceKindGL4 kind = TextureSourceKindGL4::None;
	Rml::Vector2i dimensions = {};
	Rml::Vector4f uv_scale_bias = {1.f, 1.f, 0.f, 0.f};
	Rml::Vector4f uv_rect = {0.f, 0.f, 1.f, 1.f};
	uint32_t atlas_page = 0;
	uint32_t gl_texture = 0;
	uint32_t generation = 0;
	uint32_t atlas_entry = 0;
	uint32_t atlas_generation = 0;
	bool owns_texture = false;
	std::string source;
};

enum class CompiledGeometryKindGL4 : uint32_t {
	PatchTemplates,
	GlyphTemplates,
	GenericTriangles,
	RoundedClip,
};

struct TextRunKeyGL4 {
	std::string shaped_text;
	std::string font_face;
	uint64_t effects_hash = 0;
	float size = 0.f;
	float letter_spacing = 0.f;
	float dp_ratio = 1.f;
	bool operator==(const TextRunKeyGL4&) const = default;
};

struct TextRunAtlasEntryGL4 {
	Rml::TextureHandle texture = {};
	Rml::Vector2i dimensions = {};
	uint32_t generation = 0;
};

class ITextRunRasterizerGL4
{
public:
	virtual ~ITextRunRasterizerGL4() = default;
	virtual bool Rasterize(const TextRunKeyGL4& key, TextRunAtlasEntryGL4& result) = 0;
};

class ITextRunAtlasCacheGL4
{
public:
	virtual ~ITextRunAtlasCacheGL4() = default;
	virtual const TextRunAtlasEntryGL4* Find(const TextRunKeyGL4& key) const = 0;
	virtual void Insert(TextRunKeyGL4 key, TextRunAtlasEntryGL4 entry) = 0;
	virtual void InvalidateAll() = 0;
};
