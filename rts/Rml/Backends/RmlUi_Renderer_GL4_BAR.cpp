/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RmlUi_Renderer_GL4_BAR.h"

// The compatibility calls below explicitly target shared infrastructure, never
// a GL3 renderer instance or runtime backend substitute.
#define RenderInterface_GL3_Recoil RenderInterface_GL_Recoil_Common

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <ranges>
#include <string>
#include <utility>

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/DecorationTypes.h>
#include <RmlUi/Core/Dictionary.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Math.h>
#include <RmlUi/Core/SystemInterface.h>

#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/IncrementalTextureAtlas.h"
#include "Rendering/UniformConstants.h"
#include "System/Log/ILog.h"
#include "System/Misc/TracyDefs.h"

namespace {

constexpr size_t MAX_PATCH_INSTANCES = 8192;
constexpr size_t MAX_CLIP_RECORDS = 65536;

enum MaterialMode : uint32_t {
	MATERIAL_SOLID = 0,
	MATERIAL_STATIC_IMAGE = 1,
	MATERIAL_EXTERNAL_IMAGE = 2,
	MATERIAL_BACKGROUND_BORDER = 3,
	MATERIAL_LINEAR_GRADIENT = 4,
	MATERIAL_RADIAL_GRADIENT = 5,
	MATERIAL_MISSING_TEXTURE = 6,
	MATERIAL_GENERATED_IMAGE = 7,
};

static constexpr const char* GL4_VERTEX_SHADER = R"GLSL(
#version 430 core

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor0;
layout(location = 2) in vec2 inTexCoord0;

struct PatchInstance {
	mat4 transform;
	vec4 destination_rect;
	vec4 destination_insets;
	vec4 texture_uv_rect;
	vec4 texture_uv_insets;
	vec4 atlas_uv_rect;
	vec4 color;
	vec4 scissor_rect;
	uvec4 data0;
	uvec4 data1;
	vec4 render_state; // brightness, translation x/y, reserved
};

layout(std430, binding = 0) readonly buffer InstanceBuffer { PatchInstance instances[]; };

uniform mat4 uProjection;
uniform bool uGenericMode;
uniform mat4 uGenericTransform;
uniform vec2 uGenericTranslation;
uniform vec4 uGenericUvRect;
uniform vec4 uGenericAtlasUvRect;
uniform vec4 uGenericScissor;
uniform uvec4 uGenericData;
uniform float uGenericBrightness;

out vec2 fragTexCoord;
out vec4 fragColor;
out vec2 fragLocal;
flat out vec4 fragPatchRect;
flat out vec4 fragScissor;
flat out uint fragTextureKind;
flat out uint fragTexturePage;
flat out uint fragMaterialIndex;
flat out uint fragClipIndex;
flat out uint fragClipCount;
flat out float fragBrightness;
flat out vec4 fragAtlasUvRect;
flat out uint fragWrapAtlas;

float GridCoordinate(int grid_index, float p0, float p1, float edge0, float edge1)
{
	if (grid_index == 0) return p0;
	if (grid_index == 1) return p0 + edge0;
	if (grid_index == 2) return p1 - edge1;
	return p1;
}

void main()
{
	if (uGenericMode) {
		vec2 position = inPosition + uGenericTranslation;
		gl_Position = uProjection * uGenericTransform * vec4(position, 0.0, 1.0);
		fragTexCoord = mix(uGenericUvRect.xy, uGenericUvRect.zw, inTexCoord0);
		fragColor = inColor0;
		fragLocal = inPosition;
		fragPatchRect = vec4(-1.0);
		fragScissor = uGenericScissor;
		fragTextureKind = uGenericData.x;
		fragTexturePage = uGenericData.y;
		fragMaterialIndex = uGenericData.z;
		fragClipIndex = 0u;
		fragClipCount = uGenericData.w & 0x7fffffffu;
		fragBrightness = uGenericBrightness;
		fragAtlasUvRect = uGenericAtlasUvRect;
		fragWrapAtlas = uGenericData.w >> 31;
		return;
	}

	PatchInstance instance = instances[gl_InstanceID];
	int grid_x = gl_VertexID % 4;
	int grid_y = gl_VertexID / 4;
	vec2 position = vec2(
		GridCoordinate(grid_x, instance.destination_rect.x, instance.destination_rect.z,
			instance.destination_insets.x, instance.destination_insets.z),
		GridCoordinate(grid_y, instance.destination_rect.y, instance.destination_rect.w,
			instance.destination_insets.y, instance.destination_insets.w));
	vec2 uv = vec2(
		GridCoordinate(grid_x, instance.texture_uv_rect.x, instance.texture_uv_rect.z,
			instance.texture_uv_insets.x, instance.texture_uv_insets.z),
		GridCoordinate(grid_y, instance.texture_uv_rect.y, instance.texture_uv_rect.w,
			instance.texture_uv_insets.y, instance.texture_uv_insets.w));

	vec2 translated_position = position + instance.render_state.yz;
	gl_Position = uProjection * instance.transform * vec4(translated_position, 0.0, 1.0);
	fragTexCoord = uv;
	fragColor = instance.color;
	fragLocal = position;
	fragPatchRect = instance.destination_rect;
	fragScissor = instance.scissor_rect;
	fragTextureKind = instance.data0.x;
	fragTexturePage = instance.data0.y;
	fragMaterialIndex = instance.data0.z;
	fragClipIndex = instance.data0.w;
	fragClipCount = instance.data1.x;
	fragBrightness = instance.render_state.x;
	fragAtlasUvRect = instance.atlas_uv_rect;
	fragWrapAtlas = instance.data1.y;
}
)GLSL";

static constexpr const char* GL4_FRAGMENT_SHADER = R"GLSL(
struct Material {
	uvec4 header;
	vec4 fill_color;
	vec4 border_colors[4];
	vec4 gradient_colors[4];
	vec4 stop_positions;
	vec4 parameters;
	vec4 corner_radii;
	vec4 border_widths;
};

struct ClipRecord {
	mat4 inverse_transform;
	vec4 local_rect;
	vec4 corner_radii;
};

layout(std430, binding = 1) readonly buffer MaterialBuffer { Material materials[]; };
layout(std430, binding = 2) readonly buffer ClipBuffer { ClipRecord clips[]; };

layout(binding = 0) uniform sampler2DArray uStaticAtlas;
layout(binding = 1) uniform sampler2DArray uGeneratedAtlas;
layout(binding = 2) uniform sampler2D uExternalTexture;

in vec2 fragTexCoord;
in vec4 fragColor;
in vec2 fragLocal;
flat in vec4 fragPatchRect;
flat in vec4 fragScissor;
flat in uint fragTextureKind;
flat in uint fragTexturePage;
flat in uint fragMaterialIndex;
flat in uint fragClipIndex;
flat in uint fragClipCount;
flat in float fragBrightness;
flat in vec4 fragAtlasUvRect;
flat in uint fragWrapAtlas;
layout(location = 0) out vec4 finalColor;

// UniformParamsBuffer supplies viewGeometry, timeInfo, mouseScreenPos, and the
// engine mouseStatus bitmask. Engine mouse Y is bottom-up; RmlUi is top-down.
vec2 GetRmlMousePosition()
{
	return vec2(mouseScreenPos.x, viewGeometry.y - mouseScreenPos.y);
}

bool InsideRoundedRect(vec2 point, vec4 rect, vec4 radii)
{
	if (point.x < rect.x || point.y < rect.y || point.x > rect.z || point.y > rect.w)
		return false;
	vec2 center = 0.5 * (rect.xy + rect.zw);
	float radius = point.y < center.y ? (point.x < center.x ? radii.x : radii.y) :
		(point.x < center.x ? radii.w : radii.z);
	radius = max(radius, 0.0);
	vec2 half_size = 0.5 * (rect.zw - rect.xy);
	vec2 q = abs(point - center) - max(half_size - vec2(radius), vec2(0.0));
	float distance_to_edge = length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - radius;
	return distance_to_edge <= 0.0;
}

vec4 EvaluateBackgroundBorder(Material material)
{
	if (!InsideRoundedRect(fragLocal, fragPatchRect, material.corner_radii))
		discard;
	vec4 inner = fragPatchRect + vec4(material.border_widths.x, material.border_widths.y,
		-material.border_widths.z, -material.border_widths.w);
	vec4 inner_radii = max(material.corner_radii - vec4(
		max(material.border_widths.x, material.border_widths.y),
		max(material.border_widths.z, material.border_widths.y),
		max(material.border_widths.z, material.border_widths.w),
		max(material.border_widths.x, material.border_widths.w)), vec4(0.0));
	if (inner.z >= inner.x && inner.w >= inner.y && InsideRoundedRect(fragLocal, inner, inner_radii))
		return material.fill_color;

	vec4 distances = vec4(fragLocal.x - fragPatchRect.x, fragLocal.y - fragPatchRect.y,
		fragPatchRect.z - fragLocal.x, fragPatchRect.w - fragLocal.y);
	vec4 scaled = distances / max(material.border_widths, vec4(0.0001));
	int side = 0;
	float nearest = scaled.x;
	for (int i = 1; i < 4; ++i) {
		if (scaled[i] < nearest) { nearest = scaled[i]; side = i; }
	}
	return material.border_colors[side];
}

vec4 EvaluateGradient(Material material)
{
	float t = 0.0;
	if (material.header.x == 4u) {
		vec2 v = material.parameters.zw;
		t = dot(v, fragTexCoord - material.parameters.xy) / max(dot(v, v), 0.000001);
	} else {
		t = length(material.parameters.zw * (fragTexCoord - material.parameters.xy));
	}
	int count = int(material.header.y);
	if (material.header.z != 0u && count > 1) {
		float t0 = material.stop_positions[0];
		float t1 = material.stop_positions[count - 1];
		t = t0 + mod(t - t0, max(t1 - t0, 0.000001));
	}
	vec4 color = material.gradient_colors[0];
	for (int i = 1; i < count; ++i)
		color = mix(color, material.gradient_colors[i], smoothstep(material.stop_positions[i - 1], material.stop_positions[i], t));
	return color;
}

vec2 GetAtlasTexCoord()
{
	if (fragWrapAtlas == 0u)
		return fragTexCoord;
	return mix(fragAtlasUvRect.xy, fragAtlasUvRect.zw, fract(fragTexCoord));
}

vec4 SampleAtlas(sampler2DArray atlas)
{
	vec2 uv = GetAtlasTexCoord();
	if (fragWrapAtlas == 0u)
		return texture(atlas, vec3(uv, float(fragTexturePage)));
	vec2 atlas_size = fragAtlasUvRect.zw - fragAtlasUvRect.xy;
	return textureGrad(atlas, vec3(uv, float(fragTexturePage)),
		dFdx(fragTexCoord) * atlas_size, dFdy(fragTexCoord) * atlas_size);
}

void main()
{
	vec2 screen = vec2(gl_FragCoord.x, viewGeometry.y - gl_FragCoord.y);
	if (fragScissor.x >= 0.0 &&
		(screen.x < fragScissor.x || screen.y < fragScissor.y || screen.x >= fragScissor.z || screen.y >= fragScissor.w))
		discard;

	for (uint i = 0u; i < fragClipCount; ++i) {
		ClipRecord clip = clips[fragClipIndex + i];
		vec2 local = (clip.inverse_transform * vec4(screen, 0.0, 1.0)).xy;
		if (!InsideRoundedRect(local, clip.local_rect, clip.corner_radii))
			discard;
	}

	Material material = materials[fragMaterialIndex];
	vec4 color;
	if (material.header.x == 3u)
		color = EvaluateBackgroundBorder(material);
	else if (material.header.x == 4u || material.header.x == 5u)
		color = EvaluateGradient(material) * fragColor;
	else if (material.header.x == 1u)
		color = SampleAtlas(uStaticAtlas) * fragColor;
	else if (material.header.x == 2u)
		color = texture(uExternalTexture, fragTexCoord) * fragColor;
	else if (material.header.x == 7u)
		color = SampleAtlas(uGeneratedAtlas) * fragColor;
	else if (material.header.x == 6u) {
		vec2 checker = floor(fragLocal / 8.0);
		color = mod(checker.x + checker.y, 2.0) < 1.0 ? vec4(1.0, 0.0, 1.0, 1.0) : vec4(0.1, 0.0, 0.1, 1.0);
	} else
		color = fragColor;

	if (material.header.w != 0u && !InsideRoundedRect(fragLocal, fragPatchRect, material.corner_radii))
		discard;
	color.rgb *= fragBrightness;
	finalColor = color;
}
)GLSL";

static GLuint CompileShaderStage(GLenum type, const char* source)
{
	const GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);
	GLint success = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success == GL_TRUE)
		return shader;

	GLint length = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
	std::string log(std::max(length, 1), '\0');
	glGetShaderInfoLog(shader, length, nullptr, log.data());
	LOG_L(L_ERROR, "[RmlUi GL4] Shader compilation failed: %s", log.c_str());
	glDeleteShader(shader);
	return 0;
}

static std::array<float, 4> ColorToFloat(Rml::ColourbPremultiplied color)
{
	return {color.red / 255.f, color.green / 255.f, color.blue / 255.f, color.alpha / 255.f};
}

static bool IsUniformColor(Rml::Span<const Rml::Vertex> vertices)
{
	if (vertices.empty())
		return false;
	return std::ranges::all_of(vertices, [&](const Rml::Vertex& vertex) { return vertex.colour == vertices[0].colour; });
}

} // namespace

struct RenderInterface_GL4_BAR::PatchTemplate {
	Rml::Vector4f destination_rect = {};
	Rml::Vector4f destination_insets = {};
	Rml::Vector4f uv_rect = {0.f, 0.f, 1.f, 1.f};
	Rml::Vector4f uv_insets = {};
	Rml::ColourbPremultiplied color = {};
};

struct RenderInterface_GL4_BAR::GeometryGL4 {
	CompiledGeometryKindGL4 kind = CompiledGeometryKindGL4::GenericTriangles;
	Rml::RenderGeometryMetadata metadata;
	std::vector<PatchTemplate> patches;
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ibo = 0;
	GLsizei index_count = 0;
	bool wraps_atlas = false;
	Rml::CompiledGeometryHandle compatibility_geometry = {};
};

struct alignas(16) RenderInterface_GL4_BAR::PatchInstanceGPU {
	float transform[16] = {};
	float destination_rect[4] = {};
	float destination_insets[4] = {};
	float texture_uv_rect[4] = {};
	float texture_uv_insets[4] = {};
	float atlas_uv_rect[4] = {0.f, 0.f, 1.f, 1.f};
	float color[4] = {};
	float scissor_rect[4] = {};
	uint32_t data0[4] = {};
	uint32_t data1[4] = {};
	float render_state[4] = {1.f, 0.f, 0.f, 0.f};
};

struct alignas(16) RenderInterface_GL4_BAR::MaterialGPU {
	uint32_t header[4] = {};
	float fill_color[4] = {};
	float border_colors[4][4] = {};
	float gradient_colors[4][4] = {};
	float stop_positions[4] = {};
	float parameters[4] = {};
	float corner_radii[4] = {};
	float border_widths[4] = {};
};

struct alignas(16) RenderInterface_GL4_BAR::ClipRecordGPU {
	float inverse_transform[16] = {};
	float local_rect[4] = {};
	float corner_radii[4] = {};
};

struct RenderInterface_GL4_BAR::ShaderGL4 {
	enum class Type { Linear, Radial, Fallback } type = Type::Fallback;
	bool repeating = false;
	Rml::Vector2f p = {};
	Rml::Vector2f v = {};
	std::vector<float> positions;
	std::vector<Rml::ColourbPremultiplied> colors;
	Rml::CompiledShaderHandle fallback = {};
};

RenderInterface_GL4_BAR::RenderInterface_GL4_BAR()
{
	stats.renderer = "GL4_BAR";
#if RECOIL_RMLUI_GL4_TEXT_RUN_ATLAS
	LOG_L(L_WARNING, "[RmlUi GL4] Whole-text-run atlas experiment enabled; glyph instancing remains the fallback baseline.");
#else
	LOG_L(L_INFO, "[RmlUi GL4] Whole-text-run atlas disabled; using glyph instancing.");
#endif
	brightness_stack.push_back(1.f);
	valid = static_cast<bool>(static_cast<RenderInterface_GL3_Recoil&>(*this)) && ValidateCapabilities() && CreateProgram() && CreateBuffers();
	if (valid) {
		generated_atlas = std::make_unique<CIncrementalTextureAtlas>(std::min(globalRendering->maxTextureSize, 2048), 16, 2);
		valid = static_cast<bool>(*generated_atlas);
		if (!valid)
			LOG_L(L_ERROR, "[RmlUi GL4] Generated texture-atlas creation failed.");
	}
	if (!valid) {
		LOG_L(L_ERROR, "[RmlUi GL4] Fatal renderer initialization failure; GL3 fallback is intentionally disabled.");
		DestroyResources();
	}
}

RenderInterface_GL4_BAR::~RenderInterface_GL4_BAR()
{
	if (!warning_keys.empty())
		LOG_L(L_WARNING, "[RmlUi GL4] Renderer shut down with %llu unique fallback warnings.",
			static_cast<unsigned long long>(warning_keys.size()));
	for (auto& [_, texture] : textures) {
		if (texture.owns_texture && texture.gl_texture)
			glDeleteTextures(1, &texture.gl_texture);
	}
	textures.clear();
	static_atlas.reset();
	DestroyResources();
}

RenderInterface_GL4_BAR::operator bool() const
{
	return valid;
}

bool RenderInterface_GL4_BAR::ValidateCapabilities()
{
	std::vector<std::string> missing;
	if (!globalRendering->haveGL4)
		missing.emplace_back("engine GL4 capability set");
	if (!(GLAD_GL_VERSION_4_3 || GLAD_GL_ARB_shader_storage_buffer_object))
		missing.emplace_back("GLSL 4.30 / shader storage buffers");
	if (!(GLAD_GL_VERSION_3_1 || GLAD_GL_ARB_draw_instanced))
		missing.emplace_back("instanced indexed drawing");
	if (!(GLAD_GL_VERSION_3_0 || GLAD_GL_EXT_texture_array))
		missing.emplace_back("2D texture arrays");
	if (!(GLAD_GL_VERSION_3_0 || GLAD_GL_ARB_framebuffer_object))
		missing.emplace_back("layered framebuffer attachments");

	GLint texture_units = 0;
	GLint fragment_texture_units = 0;
	GLint array_layers = 0;
	GLint storage_bindings = 0;
	GLint vertex_storage_blocks = 0;
	GLint fragment_storage_blocks = 0;
	GLint color_attachments = 0;
	GLint rgba8_texture_supported = GL_FALSE;
	GLint rgba8_framebuffer_supported = GL_FALSE;
	GLint64 storage_size = 0;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &texture_units);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &fragment_texture_units);
	glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &array_layers);
	glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &storage_bindings);
	glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &vertex_storage_blocks);
	glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &fragment_storage_blocks);
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &color_attachments);
	glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &storage_size);
	glGetInternalformativ(GL_TEXTURE_2D_ARRAY, GL_RGBA8, GL_INTERNALFORMAT_SUPPORTED, 1, &rgba8_texture_supported);
	glGetInternalformativ(GL_RENDERBUFFER, GL_RGBA8, GL_FRAMEBUFFER_RENDERABLE, 1, &rgba8_framebuffer_supported);
	if (texture_units < 3)
		missing.emplace_back("at least three combined texture units");
	if (fragment_texture_units < 3)
		missing.emplace_back("at least three fragment-stage texture units");
	if (array_layers < 16)
		missing.emplace_back("at least sixteen texture-array layers");
	if (storage_bindings < 3)
		missing.emplace_back("at least three shader-storage buffer bindings");
	if (vertex_storage_blocks < 1)
		missing.emplace_back("one vertex-stage shader-storage block");
	if (fragment_storage_blocks < 2)
		missing.emplace_back("two fragment-stage shader-storage blocks");
	if (color_attachments < 1)
		missing.emplace_back("RGBA8 framebuffer color attachment support");
	if (rgba8_texture_supported != GL_TRUE)
		missing.emplace_back("RGBA8 2D-array texture format");
	if (rgba8_framebuffer_supported == GL_NONE || rgba8_framebuffer_supported == GL_FALSE)
		missing.emplace_back("RGBA8 framebuffer-renderable format");
	const size_t required_storage_size = std::max({
		MAX_PATCH_INSTANCES * sizeof(PatchInstanceGPU),
		MAX_PATCH_INSTANCES * sizeof(MaterialGPU),
		MAX_CLIP_RECORDS * sizeof(ClipRecordGPU),
	});
	if (storage_size < GLint64(required_storage_size))
		missing.emplace_back("sufficient shader-storage block size for instance, material, and clip rings");

	for (const std::string& feature : missing)
		LOG_L(L_ERROR, "[RmlUi GL4] Missing required capability: %s", feature.c_str());
	return missing.empty();
}

bool RenderInterface_GL4_BAR::CreateProgram()
{
	const std::string& uniform_params_definition = UniformConstants::GetInstance().GetGLSLDefinition(1);
	if (uniform_params_definition.empty()) {
		LOG_L(L_ERROR, "[RmlUi GL4] Engine UniformParamsBuffer definition is unavailable.");
		return false;
	}
	const std::string fragment_source = "#version 430 core\n" + uniform_params_definition + GL4_FRAGMENT_SHADER;
	const GLuint vertex = CompileShaderStage(GL_VERTEX_SHADER, GL4_VERTEX_SHADER);
	const GLuint fragment = CompileShaderStage(GL_FRAGMENT_SHADER, fragment_source.c_str());
	if (!vertex || !fragment) {
		if (vertex) glDeleteShader(vertex);
		if (fragment) glDeleteShader(fragment);
		return false;
	}

	program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	GLint success = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (success == GL_TRUE)
		return true;

	GLint length = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
	std::string log(std::max(length, 1), '\0');
	glGetProgramInfoLog(program, length, nullptr, log.data());
	LOG_L(L_ERROR, "[RmlUi GL4] Megashader link failed: %s", log.c_str());
	return false;
}

bool RenderInterface_GL4_BAR::CreateBuffers()
{
	static constexpr std::array<uint32_t, 54> indices = {
		0,4,1, 1,4,5, 1,5,2, 2,5,6, 2,6,3, 3,6,7,
		4,8,5, 5,8,9, 5,9,6, 6,9,10, 6,10,7, 7,10,11,
		8,12,9, 9,12,13, 9,13,10, 10,13,14, 10,14,11, 11,14,15,
	};

	glGenVertexArrays(1, &patch_vao);
	glGenBuffers(1, &patch_index_buffer);
	glGenBuffers(GLsizei(STREAM_RING_SIZE), instance_buffers.data());
	glGenBuffers(GLsizei(STREAM_RING_SIZE), material_buffers.data());
	glGenBuffers(GLsizei(STREAM_RING_SIZE), clip_buffers.data());
	const bool stream_buffers_valid = std::ranges::all_of(instance_buffers, [](uint32_t buffer) { return buffer != 0; }) &&
		std::ranges::all_of(material_buffers, [](uint32_t buffer) { return buffer != 0; }) &&
		std::ranges::all_of(clip_buffers, [](uint32_t buffer) { return buffer != 0; });
	if (!patch_vao || !patch_index_buffer || !stream_buffers_valid) {
		LOG_L(L_ERROR, "[RmlUi GL4] Core buffer creation failed.");
		return false;
	}

	glBindVertexArray(patch_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, patch_index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices.data(), GL_STATIC_DRAW);
	glBindVertexArray(0);
	instances.reserve(MAX_PATCH_INSTANCES);
	materials.reserve(MAX_PATCH_INSTANCES);
	clips.reserve(MAX_PATCH_INSTANCES);
	return true;
}

void RenderInterface_GL4_BAR::DestroyResources()
{
	glDeleteBuffers(GLsizei(STREAM_RING_SIZE), clip_buffers.data());
	glDeleteBuffers(GLsizei(STREAM_RING_SIZE), material_buffers.data());
	glDeleteBuffers(GLsizei(STREAM_RING_SIZE), instance_buffers.data());
	if (patch_index_buffer) glDeleteBuffers(1, &patch_index_buffer);
	if (patch_vao) glDeleteVertexArrays(1, &patch_vao);
	if (program) glDeleteProgram(program);
	clip_buffers.fill(0);
	material_buffers.fill(0);
	instance_buffers.fill(0);
	patch_index_buffer = patch_vao = program = 0;
}

void RenderInterface_GL4_BAR::PrepareCompatibilityScissor()
{
	RenderInterface_GL3_Recoil::EnableScissorRegion(false);
	if (gl4_scissor_region.Valid())
		RenderInterface_GL3_Recoil::SetScissorRegion(gl4_scissor_region);
}

void RenderInterface_GL4_BAR::RestoreShaderScissor()
{
	RenderInterface_GL3_Recoil::EnableScissorRegion(false);
	TrackScissorRegion(gl4_scissor_region);
}

void RenderInterface_GL4_BAR::SetViewport(int width, int height, int offset_x, int offset_y)
{
	RenderInterface_GL3_Recoil::SetViewport(width, height, offset_x, offset_y);
	viewport_width = std::max(width, 1);
	viewport_height = std::max(height, 1);
	viewport_offset_x = offset_x;
	viewport_offset_y = offset_y;
	projection = Rml::Matrix4f::ProjectOrtho(0.f, float(viewport_width), float(viewport_height), 0.f, -10000.f, 10000.f);
}

void RenderInterface_GL4_BAR::BeginFrame()
{
	ZoneScopedN("RmlUi_GL4::BeginFrame");
	RenderInterface_GL3_Recoil::BeginFrame();
	instances.clear();
	materials.clear();
	clips.clear();
	active_clips.clear();
	batch_external_texture = 0;
	gl4_scissor_region = Rml::Rectanglei::MakeInvalid();
	compatibility_clip_active = false;
	TrackScissorRegion(gl4_scissor_region);
	current_transform = Rml::Matrix4f::Identity();
	brightness_stack.assign(1, 1.f);
	brightness = 1.f;
	frame_patch_instances = 0;
	frame_glyph_instances = 0;
	frame_patch_batches = 0;
	frame_generic_batches = 0;
}

void RenderInterface_GL4_BAR::EndFrame()
{
	ZoneScopedN("RmlUi_GL4::EndFrame");
	FlushPatchBatch();
	TracyPlot("RmlUi_GL4::PatchInstances", int64_t(frame_patch_instances));
	TracyPlot("RmlUi_GL4::GlyphInstances", int64_t(frame_glyph_instances));
	TracyPlot("RmlUi_GL4::PatchBatches", int64_t(frame_patch_batches));
	TracyPlot("RmlUi_GL4::GenericBatches", int64_t(frame_generic_batches));
	RenderInterface_GL3_Recoil::EndFrame();
}

Rml::CompiledGeometryHandle RenderInterface_GL4_BAR::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	return CompileGeometryWithMetadata(vertices, indices, nullptr);
}

Rml::CompiledGeometryHandle RenderInterface_GL4_BAR::CompileGeometryWithMetadata(Rml::Span<const Rml::Vertex> vertices,
	Rml::Span<const int> indices, const Rml::RenderGeometryMetadata* metadata)
{
	ZoneScopedN("RmlUi_GL4::CompileGeometry");
	auto geometry = std::make_unique<GeometryGL4>();
	if (metadata)
		geometry->metadata = *metadata;
	if (geometry->metadata.document_source.empty())
		geometry->metadata.document_source = current_document_source;
	geometry->index_count = GLsizei(indices.size());
	geometry->compatibility_geometry = RenderInterface_GL3_Recoil::CompileGeometry(vertices, indices);

	while (glGetError() != GL_NO_ERROR) {}
	glGenVertexArrays(1, &geometry->vao);
	glGenBuffers(1, &geometry->vbo);
	glGenBuffers(1, &geometry->ibo);
	glBindVertexArray(geometry->vao);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vbo);
	glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vertices.size() * sizeof(Rml::Vertex)), vertices.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex), reinterpret_cast<void*>(offsetof(Rml::Vertex, position)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Rml::Vertex), reinterpret_cast<void*>(offsetof(Rml::Vertex, colour)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex), reinterpret_cast<void*>(offsetof(Rml::Vertex, tex_coord)));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(indices.size() * sizeof(int)), indices.data(), GL_STATIC_DRAW);
	glBindVertexArray(0);
	geometry->wraps_atlas = std::ranges::any_of(vertices, [](const Rml::Vertex& vertex) {
		return vertex.tex_coord.x < 0.f || vertex.tex_coord.y < 0.f || vertex.tex_coord.x > 1.f || vertex.tex_coord.y > 1.f;
	});
	if (!geometry->vao || !geometry->vbo || !geometry->ibo || glGetError() != GL_NO_ERROR) {
		LOG_L(L_ERROR, "[RmlUi GL4] Geometry buffer creation failed for source '%s'.",
			geometry->metadata.source_identifier.c_str());
		if (geometry->ibo) glDeleteBuffers(1, &geometry->ibo);
		if (geometry->vbo) glDeleteBuffers(1, &geometry->vbo);
		if (geometry->vao) glDeleteVertexArrays(1, &geometry->vao);
		if (geometry->compatibility_geometry)
			RenderInterface_GL3_Recoil::ReleaseGeometry(geometry->compatibility_geometry);
		return {};
	}

	auto AddQuad = [&](size_t first) -> bool {
		if (first + 4 > vertices.size())
			return false;
		const Rml::Span<const Rml::Vertex> quad(vertices.data() + first, 4);
		if (!IsUniformColor(quad))
			return false;
		PatchTemplate patch;
		patch.destination_rect = {quad[0].position.x, quad[0].position.y, quad[2].position.x, quad[2].position.y};
		patch.uv_rect = {quad[0].tex_coord.x, quad[0].tex_coord.y, quad[2].tex_coord.x, quad[2].tex_coord.y};
		patch.color = quad[0].colour;
		geometry->patches.push_back(patch);
		return true;
	};

	using Semantic = Rml::RenderGeometrySemantic;
	switch (geometry->metadata.semantic) {
		case Semantic::Quad:
		case Semantic::LayerComposite: {
			bool valid_quads = !vertices.empty() && vertices.size() % 4 == 0 && indices.size() == (vertices.size() / 4) * 6;
			for (size_t first = 0; valid_quads && first < vertices.size(); first += 4)
				valid_quads = AddQuad(first);
			if (valid_quads)
				geometry->kind = CompiledGeometryKindGL4::PatchTemplates;
			else
				geometry->patches.clear();
			break;
		}
		case Semantic::GlyphRun: {
			bool valid_glyphs = !vertices.empty() && vertices.size() % 4 == 0;
			for (size_t first = 0; valid_glyphs && first < vertices.size(); first += 4)
				valid_glyphs = AddQuad(first);
			if (valid_glyphs)
				geometry->kind = CompiledGeometryKindGL4::GlyphTemplates;
			else
				geometry->patches.clear();
			break;
		}
		case Semantic::NinePatch:
			if (vertices.size() == 16 && indices.size() == 54 && IsUniformColor(vertices)) {
				PatchTemplate patch;
				patch.destination_rect = geometry->metadata.local_bounds;
				patch.destination_insets = geometry->metadata.destination_edge_widths;
				patch.uv_rect = {vertices[0].tex_coord.x, vertices[0].tex_coord.y, vertices[15].tex_coord.x, vertices[15].tex_coord.y};
				patch.uv_insets = geometry->metadata.texture_source_insets;
				patch.color = vertices[0].colour;
				geometry->patches.push_back(patch);
				geometry->kind = CompiledGeometryKindGL4::PatchTemplates;
			}
			break;
		case Semantic::BackgroundBorder:
		case Semantic::RoundedClip: {
			PatchTemplate patch;
			patch.destination_rect = geometry->metadata.local_bounds;
			patch.destination_insets = geometry->metadata.destination_edge_widths;
			patch.color = Rml::ColourbPremultiplied(255);
			geometry->patches.push_back(patch);
			geometry->kind = geometry->metadata.semantic == Semantic::RoundedClip ?
				CompiledGeometryKindGL4::RoundedClip : CompiledGeometryKindGL4::PatchTemplates;
			break;
		}
		default:
			break;
	}

	if (geometry->kind == CompiledGeometryKindGL4::GenericTriangles) {
		stats.unsupported_geometry += 1;
	}
	return reinterpret_cast<Rml::CompiledGeometryHandle>(geometry.release());
}

void RenderInterface_GL4_BAR::ReleaseGeometry(Rml::CompiledGeometryHandle handle)
{
	if (!handle)
		return;
	auto* geometry = reinterpret_cast<GeometryGL4*>(handle);
	if (geometry->compatibility_geometry)
		RenderInterface_GL3_Recoil::ReleaseGeometry(geometry->compatibility_geometry);
	if (geometry->ibo) glDeleteBuffers(1, &geometry->ibo);
	if (geometry->vbo) glDeleteBuffers(1, &geometry->vbo);
	if (geometry->vao) glDeleteVertexArrays(1, &geometry->vao);
	delete geometry;
}

void RenderInterface_GL4_BAR::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation,
	Rml::TextureHandle texture_handle)
{
	ZoneScopedN("RmlUi_GL4::RenderGeometry");
	if (texture_handle == TexturePostprocess || texture_handle == TextureEnableWithoutBinding) {
		FlushPatchBatch();
		RenderInterface_GL3_Recoil::RenderGeometry(handle, translation, texture_handle);
		return;
	}

	const auto* geometry = reinterpret_cast<const GeometryGL4*>(handle);
	const TextureHandleGL4* texture = FindTexture(texture_handle);
#if !RECOIL_RMLUI_GL4_TEXT_RUN_ATLAS
	stats.text_run_disabled_submissions += geometry->kind == CompiledGeometryKindGL4::GlyphTemplates;
#endif
	MaterialGPU missing_material;
	const MaterialGPU* material_override = nullptr;
	if (texture_handle && !texture) {
		missing_material.header[0] = MATERIAL_MISSING_TEXTURE;
		material_override = &missing_material;
		WarnOnce("unknown_texture_handle", std::to_string(texture_handle));
	}
	if (geometry->kind == CompiledGeometryKindGL4::GenericTriangles) {
		WarnOnce("unsupported_geometry", geometry->metadata.source_identifier);
		DrawGeneric(*geometry, translation, texture, material_override);
	} else
		AppendGeometry(*geometry, translation, texture, material_override);
}

Rml::TextureHandle RenderInterface_GL4_BAR::StoreTexture(TextureHandleGL4 texture)
{
	const Rml::TextureHandle handle = next_virtual_texture++;
	texture.generation = next_texture_generation++;
	textures.insert_or_assign(handle, std::move(texture));
	return handle;
}

TextureHandleGL4* RenderInterface_GL4_BAR::FindTexture(Rml::TextureHandle texture)
{
	const auto it = textures.find(texture);
	return it == textures.end() ? nullptr : &it->second;
}

const TextureHandleGL4* RenderInterface_GL4_BAR::FindTexture(Rml::TextureHandle texture) const
{
	const auto it = textures.find(texture);
	return it == textures.end() ? nullptr : &it->second;
}

Rml::TextureHandle RenderInterface_GL4_BAR::LoadTexture(Rml::Vector2i& dimensions, const Rml::String& source)
{
	const std::string canonical = TextureAtlasManifest::CanonicalizePath(source);
	if (static_atlas) {
		if (const StaticAtlasRecordGL4* record = static_atlas->Find(canonical)) {
			dimensions = record->dimensions;
			if (const auto it = static_texture_handles.find(canonical); it != static_texture_handles.end())
				return it->second;
			TextureHandleGL4 texture;
			texture.kind = TextureSourceKindGL4::StaticAtlas;
			texture.dimensions = record->dimensions;
			texture.uv_rect = record->uv_rect;
			texture.atlas_page = record->page;
			texture.gl_texture = static_atlas->GetTextureId();
			texture.source = canonical;
			const Rml::TextureHandle handle = StoreTexture(std::move(texture));
			static_texture_handles.emplace(canonical, handle);
			return handle;
		}
	}

	CBitmap bitmap;
	std::string vfs_source = canonical.empty() ? source : canonical.substr(1);
	if (!bitmap.Load(vfs_source)) {
		stats.late_file_textures += 1;
		WarnOnce("late_file_texture_missing", canonical);
		return {};
	}
	dimensions = {bitmap.xsize, bitmap.ysize};
	stats.late_file_textures += 1;
	WarnOnce("late_file_texture", canonical);
#if RECOIL_RMLUI_GL4_STRICT_VALIDATION
	LOG_L(L_ERROR, "[RmlUi GL4 strict] Late production texture rejected: %s", canonical.c_str());
	return {};
#endif
	const GLuint gl_texture = bitmap.CreateTexture();
	if (!gl_texture) {
		WarnOnce("late_texture_gpu_creation_failed", canonical);
		return {};
	}
	TextureHandleGL4 texture;
	texture.kind = TextureSourceKindGL4::External2D;
	texture.dimensions = dimensions;
	texture.gl_texture = gl_texture;
	texture.owns_texture = true;
	texture.source = canonical;
	return StoreTexture(std::move(texture));
}

Rml::TextureHandle RenderInterface_GL4_BAR::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i dimensions)
{
	if (dimensions.x <= 0 || dimensions.y <= 0)
		return {};
	const size_t required_bytes = size_t(dimensions.x) * size_t(dimensions.y) * 4;
	if (!source.empty() && source.size() < required_bytes) {
		WarnOnce("generated_texture_data_too_small", std::to_string(dimensions.x) + "x" + std::to_string(dimensions.y));
		return {};
	}
	if (!source.empty() && generated_atlas) {
		const CIncrementalTextureAtlas::Entry entry = generated_atlas->Allocate(source.data(), dimensions.x, dimensions.y);
		if (entry) {
			const uint32_t atlas_texture = generated_atlas->GetTextureId();
			for (auto& [_, existing_texture] : textures) {
				if (existing_texture.kind == TextureSourceKindGL4::GeneratedAtlas)
					existing_texture.gl_texture = atlas_texture;
			}
			TextureHandleGL4 texture;
			texture.kind = TextureSourceKindGL4::GeneratedAtlas;
			texture.dimensions = dimensions;
			texture.uv_rect = {entry.u0, entry.v0, entry.u1, entry.v1};
			texture.atlas_page = entry.page;
			texture.gl_texture = atlas_texture;
			texture.atlas_entry = entry.id;
			texture.atlas_generation = entry.generation;
			texture.source = "generated-callback";
			return StoreTexture(std::move(texture));
		}
		WarnOnce("generated_atlas_full", std::to_string(dimensions.x) + "x" + std::to_string(dimensions.y));
	}

	GLuint gl_texture = 0;
	glGenTextures(1, &gl_texture);
	if (!gl_texture)
		return {};
	glBindTexture(GL_TEXTURE_2D, gl_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, source.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);

	TextureHandleGL4 texture;
	texture.kind = source.empty() ? TextureSourceKindGL4::RenderTarget : TextureSourceKindGL4::External2D;
	texture.dimensions = dimensions;
	texture.gl_texture = gl_texture;
	texture.owns_texture = true;
	texture.source = source.empty() ? "render-target" : "generated-callback";
	return StoreTexture(std::move(texture));
}

void RenderInterface_GL4_BAR::ReleaseTexture(Rml::TextureHandle handle)
{
	const auto it = textures.find(handle);
	if (it == textures.end())
		return;
	if (it->second.kind == TextureSourceKindGL4::GeneratedAtlas && generated_atlas)
		generated_atlas->Release(it->second.atlas_entry, it->second.atlas_generation);
	if (it->second.owns_texture && it->second.gl_texture)
		glDeleteTextures(1, &it->second.gl_texture);
	if (it->second.kind == TextureSourceKindGL4::StaticAtlas)
		static_texture_handles.erase(it->second.source);
	if (it->second.kind == TextureSourceKindGL4::External2D && !it->second.owns_texture)
		external_texture_handles.erase(it->second.gl_texture);
	textures.erase(it);
}

void RenderInterface_GL4_BAR::EnableScissorRegion(bool enable)
{
	if (!enable) {
		gl4_scissor_region = Rml::Rectanglei::MakeInvalid();
		TrackScissorRegion(gl4_scissor_region);
	}
}

void RenderInterface_GL4_BAR::SetScissorRegion(Rml::Rectanglei region)
{
	gl4_scissor_region = region;
	TrackScissorRegion(region);
}

void RenderInterface_GL4_BAR::EnableClipMask(bool /*enable*/)
{
	FlushPatchBatch();
	active_clips.clear();
	compatibility_clip_active = false;
	RenderInterface_GL3_Recoil::EnableClipMask(false);
}

void RenderInterface_GL4_BAR::RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle handle,
	Rml::Vector2f translation)
{
	const auto* geometry = reinterpret_cast<const GeometryGL4*>(handle);
	const bool analytic = operation != Rml::ClipMaskOperation::SetInverse &&
		(operation == Rml::ClipMaskOperation::Set || active_clips.size() < MAX_CLIP_RECORDS) &&
		(geometry->metadata.semantic == Rml::RenderGeometrySemantic::RoundedClip ||
		 geometry->metadata.semantic == Rml::RenderGeometrySemantic::BackgroundBorder);
	if (!analytic) {
		FlushPatchBatch();
		if (operation == Rml::ClipMaskOperation::Set || operation == Rml::ClipMaskOperation::SetInverse)
			active_clips.clear();
		stats.unsupported_clip_masks += 1;
		WarnOnce("unsupported_clip_mask", geometry->metadata.source_identifier);
		PrepareCompatibilityScissor();
		RenderInterface_GL3_Recoil::EnableClipMask(true);
		const Rml::ClipMaskOperation compatibility_operation =
			(operation == Rml::ClipMaskOperation::Intersect && !compatibility_clip_active) ?
			Rml::ClipMaskOperation::Set : operation;
		RenderInterface_GL3_Recoil::RenderToClipMask(compatibility_operation, geometry->compatibility_geometry, translation);
		compatibility_clip_active = true;
		RestoreShaderScissor();
		return;
	}

	if (operation == Rml::ClipMaskOperation::Set) {
		active_clips.clear();
		compatibility_clip_active = false;
	}
	ClipRecordGPU clip;
	Rml::Matrix4f inverse = current_transform;
	if (!inverse.Invert()) {
		stats.unsupported_clip_masks += 1;
		return;
	}
	std::memcpy(clip.inverse_transform, inverse.data(), sizeof(clip.inverse_transform));
	const Rml::Vector4f bounds = geometry->metadata.local_bounds + Rml::Vector4f(translation.x, translation.y, translation.x, translation.y);
	std::memcpy(clip.local_rect, &bounds[0], sizeof(clip.local_rect));
	std::memcpy(clip.corner_radii, &geometry->metadata.corner_radii[0], sizeof(clip.corner_radii));
	active_clips.push_back(clip);
}

void RenderInterface_GL4_BAR::SetTransform(const Rml::Matrix4f* transform)
{
	current_transform = transform ? *transform : Rml::Matrix4f::Identity();
	RenderInterface_GL3_Recoil::SetTransform(transform);
}

void RenderInterface_GL4_BAR::PushInlineBrightness(float value)
{
	brightness_stack.push_back(brightness);
	brightness *= value;
	stats.shader_brightness_operations += 1;
}

void RenderInterface_GL4_BAR::PopInlineBrightness()
{
	if (brightness_stack.size() <= 1) {
		brightness = 1.f;
		return;
	}
	brightness = brightness_stack.back();
	brightness_stack.pop_back();
}

void RenderInterface_GL4_BAR::BeginDocument(const Rml::String& source_url)
{
	current_document_source = source_url;
#if RECOIL_RMLUI_GL4_DOCUMENT_BATCH_BARRIERS
	FlushPatchBatch();
	stats.document_batch_boundaries += 1;
#endif
}

void RenderInterface_GL4_BAR::EndDocument()
{
#if RECOIL_RMLUI_GL4_DOCUMENT_BATCH_BARRIERS
	FlushPatchBatch();
#endif
	current_document_source.clear();
}

Rml::LayerHandle RenderInterface_GL4_BAR::PushLayer()
{
	FlushPatchBatch();
	stats.offscreen_layer_operations += 1;
	PrepareCompatibilityScissor();
	const Rml::LayerHandle result = RenderInterface_GL3_Recoil::PushLayer();
	RestoreShaderScissor();
	return result;
}

void RenderInterface_GL4_BAR::CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode,
	Rml::Span<const Rml::CompiledFilterHandle> filters)
{
	FlushPatchBatch();
	PrepareCompatibilityScissor();
	RenderInterface_GL3_Recoil::CompositeLayers(source, destination, blend_mode, filters);
	RestoreShaderScissor();
}

void RenderInterface_GL4_BAR::PopLayer()
{
	FlushPatchBatch();
	PrepareCompatibilityScissor();
	RenderInterface_GL3_Recoil::PopLayer();
	RestoreShaderScissor();
}

Rml::TextureHandle RenderInterface_GL4_BAR::SaveLayerAsTexture()
{
	FlushPatchBatch();
	const Rml::Rectanglei saved_scissor = gl4_scissor_region;
	PrepareCompatibilityScissor();
	const Rml::TextureHandle raw_texture = RenderInterface_GL3_Recoil::SaveLayerAsTexture();
	gl4_scissor_region = saved_scissor;
	RestoreShaderScissor();
	if (!raw_texture)
		return {};
	TextureHandleGL4 texture;
	texture.kind = TextureSourceKindGL4::RenderTarget;
	texture.dimensions = saved_scissor.Size();
	texture.gl_texture = uint32_t(raw_texture);
	texture.owns_texture = true;
	texture.source = "saved-layer";
	return StoreTexture(std::move(texture));
}

Rml::CompiledFilterHandle RenderInterface_GL4_BAR::SaveLayerAsMaskImage()
{
	FlushPatchBatch();
	PrepareCompatibilityScissor();
	const Rml::CompiledFilterHandle result = RenderInterface_GL3_Recoil::SaveLayerAsMaskImage();
	RestoreShaderScissor();
	return result;
}

Rml::CompiledFilterHandle RenderInterface_GL4_BAR::CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters)
{
	if (name == "blur" || name == "drop-shadow")
		stats.box_shadow_operations += name == "drop-shadow";
	return RenderInterface_GL3_Recoil::CompileFilter(name, parameters);
}

Rml::CompiledShaderHandle RenderInterface_GL4_BAR::CompileShader(const Rml::String& name, const Rml::Dictionary& parameters)
{
	auto shader = std::make_unique<ShaderGL4>();
	if (name == "linear-gradient" || name == "radial-gradient") {
		shader->type = name == "linear-gradient" ? ShaderGL4::Type::Linear : ShaderGL4::Type::Radial;
		shader->repeating = Rml::Get(parameters, "repeating", false);
		if (shader->type == ShaderGL4::Type::Linear) {
			shader->p = Rml::Get(parameters, "p0", Rml::Vector2f{});
			shader->v = Rml::Get(parameters, "p1", Rml::Vector2f{}) - shader->p;
		} else {
			shader->p = Rml::Get(parameters, "center", Rml::Vector2f{});
			shader->v = Rml::Vector2f(1.f) / Rml::Get(parameters, "radius", Rml::Vector2f(1.f));
		}
		const auto it = parameters.find("color_stop_list");
		if (it != parameters.end() && it->second.GetType() == Rml::Variant::COLORSTOPLIST) {
			const Rml::ColorStopList& stops = it->second.GetReference<Rml::ColorStopList>();
			const bool supported_stops = !stops.empty() && stops.size() <= 4 && std::ranges::all_of(stops, [](const Rml::ColorStop& stop) {
				return stop.position.unit == Rml::Unit::NUMBER;
			});
			if (supported_stops) {
				for (const Rml::ColorStop& stop : stops) {
					shader->positions.push_back(stop.position.number);
					shader->colors.push_back(stop.color);
				}
				return reinterpret_cast<Rml::CompiledShaderHandle>(shader.release());
			}
		}
		stats.unsupported_gradients += 1;
		WarnOnce("unsupported_gradient", name);
#if RECOIL_RMLUI_GL4_STRICT_VALIDATION
		LOG_L(L_ERROR, "[RmlUi GL4 strict] Unsupported gradient rejected: %s", name.c_str());
		return {};
#endif
	}

	shader->type = ShaderGL4::Type::Fallback;
	shader->fallback = RenderInterface_GL3_Recoil::CompileShader(name, parameters);
	if (!shader->fallback)
		return {};
	return reinterpret_cast<Rml::CompiledShaderHandle>(shader.release());
}

void RenderInterface_GL4_BAR::RenderShader(Rml::CompiledShaderHandle handle, Rml::CompiledGeometryHandle geometry_handle,
	Rml::Vector2f translation, Rml::TextureHandle texture_handle)
{
	ZoneScopedN("RmlUi_GL4::RenderShader");
	auto* shader = reinterpret_cast<ShaderGL4*>(handle);
	const auto* geometry = reinterpret_cast<const GeometryGL4*>(geometry_handle);
	if (shader->type == ShaderGL4::Type::Fallback) {
		FlushPatchBatch();
		const TextureHandleGL4* texture = FindTexture(texture_handle);
		PrepareCompatibilityScissor();
		RenderInterface_GL3_Recoil::RenderShader(shader->fallback, geometry->compatibility_geometry, translation,
			texture ? Rml::TextureHandle(texture->gl_texture) : Rml::TextureHandle{});
		RestoreShaderScissor();
		return;
	}

	MaterialGPU material;
	material.header[0] = shader->type == ShaderGL4::Type::Linear ? MATERIAL_LINEAR_GRADIENT : MATERIAL_RADIAL_GRADIENT;
	material.header[1] = uint32_t(shader->positions.size());
	material.header[2] = shader->repeating;
	material.parameters[0] = shader->p.x;
	material.parameters[1] = shader->p.y;
	material.parameters[2] = shader->v.x;
	material.parameters[3] = shader->v.y;
	for (size_t i = 0; i < shader->positions.size(); ++i) {
		material.stop_positions[i] = shader->positions[i];
		const auto color = ColorToFloat(shader->colors[i]);
		std::memcpy(material.gradient_colors[i], color.data(), sizeof(material.gradient_colors[i]));
	}
	const TextureHandleGL4* texture = FindTexture(texture_handle);
	if (geometry->kind == CompiledGeometryKindGL4::GenericTriangles)
		DrawGeneric(*geometry, translation, texture, &material);
	else
		AppendGeometry(*geometry, translation, texture, &material);
}

void RenderInterface_GL4_BAR::ReleaseShader(Rml::CompiledShaderHandle handle)
{
	auto* shader = reinterpret_cast<ShaderGL4*>(handle);
	if (shader->fallback)
		RenderInterface_GL3_Recoil::ReleaseShader(shader->fallback);
	delete shader;
}

void RenderInterface_GL4_BAR::AppendGeometry(const GeometryGL4& geometry, Rml::Vector2f translation,
	const TextureHandleGL4* texture, const MaterialGPU* material_override)
{
	const bool uses_external_texture = texture &&
		(texture->kind == TextureSourceKindGL4::External2D || texture->kind == TextureSourceKindGL4::RenderTarget);
	const uint32_t external_texture = uses_external_texture ? texture->gl_texture : 0;
	if (!instances.empty() && external_texture != batch_external_texture)
		FlushPatchBatch();
	batch_external_texture = external_texture;

	for (const PatchTemplate& patch : geometry.patches) {
		if (instances.size() >= MAX_PATCH_INSTANCES || clips.size() + active_clips.size() > MAX_CLIP_RECORDS) {
			FlushPatchBatch();
			batch_external_texture = external_texture;
		}

		MaterialGPU material = material_override ? *material_override : MaterialGPU{};
		if (!material_override) {
			if (texture)
				material.header[0] = texture->kind == TextureSourceKindGL4::StaticAtlas ? MATERIAL_STATIC_IMAGE :
					(texture->kind == TextureSourceKindGL4::GeneratedAtlas ? MATERIAL_GENERATED_IMAGE : MATERIAL_EXTERNAL_IMAGE);
			else if (geometry.metadata.semantic == Rml::RenderGeometrySemantic::BackgroundBorder)
				material.header[0] = MATERIAL_BACKGROUND_BORDER;
			else
				material.header[0] = MATERIAL_SOLID;
		}

		if (geometry.metadata.semantic == Rml::RenderGeometrySemantic::BackgroundBorder) {
			const auto fill = ColorToFloat(geometry.metadata.background_color);
			std::memcpy(material.fill_color, fill.data(), sizeof(material.fill_color));
			for (size_t i = 0; i < 4; ++i) {
				const auto border = ColorToFloat(geometry.metadata.border_colors[i]);
				std::memcpy(material.border_colors[i], border.data(), sizeof(material.border_colors[i]));
			}
			std::memcpy(material.corner_radii, &geometry.metadata.corner_radii[0], sizeof(material.corner_radii));
			std::memcpy(material.border_widths, &geometry.metadata.border_widths[0], sizeof(material.border_widths));
			const Rml::Vector4f& radii = geometry.metadata.corner_radii;
			material.header[3] = radii.x > 0.f || radii.y > 0.f || radii.z > 0.f || radii.w > 0.f;
		}

		PatchInstanceGPU instance;
		std::memcpy(instance.transform, current_transform.data(), sizeof(instance.transform));
		const Rml::Vector4f local_rect = patch.destination_rect;
		std::memcpy(instance.destination_rect, &local_rect[0], sizeof(instance.destination_rect));
		Rml::Vector4f destination_insets = patch.destination_insets;
		const float available_width = std::abs(local_rect.z - local_rect.x);
		const float available_height = std::abs(local_rect.w - local_rect.y);
		const float horizontal_insets = destination_insets.x + destination_insets.z;
		const float vertical_insets = destination_insets.y + destination_insets.w;
		if (horizontal_insets > available_width && horizontal_insets > 0.f) {
			const float scale = available_width / horizontal_insets;
			destination_insets.x *= scale;
			destination_insets.z *= scale;
		}
		if (vertical_insets > available_height && vertical_insets > 0.f) {
			const float scale = available_height / vertical_insets;
			destination_insets.y *= scale;
			destination_insets.w *= scale;
		}
		if (geometry.metadata.semantic == Rml::RenderGeometrySemantic::BackgroundBorder)
			std::memcpy(material.border_widths, &destination_insets[0], sizeof(material.border_widths));
		std::memcpy(instance.destination_insets, &destination_insets[0], sizeof(instance.destination_insets));
		const uint32_t material_index = uint32_t(materials.size());
		materials.push_back(material);

		Rml::Vector4f uv_rect = patch.uv_rect;
		Rml::Vector4f uv_insets = patch.uv_insets;
		const bool atlased_texture = texture && (texture->kind == TextureSourceKindGL4::StaticAtlas ||
			texture->kind == TextureSourceKindGL4::GeneratedAtlas);
		const bool wraps_atlas = atlased_texture && (patch.uv_rect.x < 0.f || patch.uv_rect.y < 0.f ||
			patch.uv_rect.z > 1.f || patch.uv_rect.w > 1.f);
		if (atlased_texture)
			std::memcpy(instance.atlas_uv_rect, &texture->uv_rect[0], sizeof(instance.atlas_uv_rect));
		if (atlased_texture && !wraps_atlas) {
			const Rml::Vector2f atlas_min(texture->uv_rect.x, texture->uv_rect.y);
			const Rml::Vector2f atlas_size(texture->uv_rect.z - texture->uv_rect.x, texture->uv_rect.w - texture->uv_rect.y);
			uv_rect = {
				atlas_min.x + patch.uv_rect.x * atlas_size.x,
				atlas_min.y + patch.uv_rect.y * atlas_size.y,
				atlas_min.x + patch.uv_rect.z * atlas_size.x,
				atlas_min.y + patch.uv_rect.w * atlas_size.y,
			};
			uv_insets = {patch.uv_insets.x * atlas_size.x, patch.uv_insets.y * atlas_size.y,
				patch.uv_insets.z * atlas_size.x, patch.uv_insets.w * atlas_size.y};
		}
		std::memcpy(instance.texture_uv_rect, &uv_rect[0], sizeof(instance.texture_uv_rect));
		std::memcpy(instance.texture_uv_insets, &uv_insets[0], sizeof(instance.texture_uv_insets));
		const Rml::ColourbPremultiplied instance_color =
			(material_override && geometry.metadata.semantic == Rml::RenderGeometrySemantic::BackgroundBorder) ?
			geometry.metadata.background_color : patch.color;
		const auto color = ColorToFloat(instance_color);
		std::memcpy(instance.color, color.data(), sizeof(instance.color));
		if (gl4_scissor_region.Valid()) {
			instance.scissor_rect[0] = float(gl4_scissor_region.Left());
			instance.scissor_rect[1] = float(gl4_scissor_region.Top());
			instance.scissor_rect[2] = float(gl4_scissor_region.Right());
			instance.scissor_rect[3] = float(gl4_scissor_region.Bottom());
		} else {
			std::ranges::fill(instance.scissor_rect, -1.f);
		}

		instance.data0[0] = texture ? uint32_t(texture->kind) : 0;
		instance.data0[1] = texture ? texture->atlas_page : 0;
		instance.data0[2] = material_index;
		instance.data0[3] = uint32_t(clips.size());
		instance.data1[0] = uint32_t(active_clips.size());
		instance.data1[1] = wraps_atlas;
		clips.insert(clips.end(), active_clips.begin(), active_clips.end());
		instance.render_state[0] = brightness;
		instance.render_state[1] = translation.x;
		instance.render_state[2] = translation.y;
		instances.push_back(instance);

		stats.patch_instances += 1;
		stats.glyph_instances += geometry.kind == CompiledGeometryKindGL4::GlyphTemplates;
		frame_patch_instances += 1;
		frame_glyph_instances += geometry.kind == CompiledGeometryKindGL4::GlyphTemplates;
	}
}

void RenderInterface_GL4_BAR::FlushPatchBatch()
{
	if (instances.empty())
		return;
	ZoneScopedN("RmlUi_GL4::FlushPatchBatch");

	auto Upload = [](GLenum target, GLuint buffer, const void* data, size_t size) {
		glBindBuffer(target, buffer);
		glBufferData(target, GLsizeiptr(std::max<size_t>(size, 16)), nullptr, GL_STREAM_DRAW);
		if (size == 0)
			return;
		void* destination = glMapBufferRange(target, 0, GLsizeiptr(size), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
		if (destination) {
			std::memcpy(destination, data, size);
			glUnmapBuffer(target);
		} else {
			glBufferSubData(target, 0, GLsizeiptr(size), data);
		}
	};

	const size_t ring_index = stream_ring_cursor++ % STREAM_RING_SIZE;
	const GLuint instance_buffer = instance_buffers[ring_index];
	const GLuint material_buffer = material_buffers[ring_index];
	const GLuint clip_buffer = clip_buffers[ring_index];
	Upload(GL_SHADER_STORAGE_BUFFER, instance_buffer, instances.data(), instances.size() * sizeof(PatchInstanceGPU));
	Upload(GL_SHADER_STORAGE_BUFFER, material_buffer, materials.data(), materials.size() * sizeof(MaterialGPU));
	Upload(GL_SHADER_STORAGE_BUFFER, clip_buffer, clips.data(), clips.size() * sizeof(ClipRecordGPU));
	stats.instance_buffer_bytes += instances.size() * sizeof(PatchInstanceGPU);
	stats.material_buffer_bytes += materials.size() * sizeof(MaterialGPU);
	stats.clip_buffer_bytes += clips.size() * sizeof(ClipRecordGPU);

	glUseProgram(program);
	UniformConstants::GetInstance().Bind();
	glUniform1i(glGetUniformLocation(program, "uGenericMode"), GL_FALSE);
	glUniformMatrix4fv(glGetUniformLocation(program, "uProjection"), 1, GL_FALSE, projection.data());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, instance_buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, material_buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, clip_buffer);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, static_atlas ? static_atlas->GetTextureId() : 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D_ARRAY, generated_atlas ? generated_atlas->GetTextureId() : 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, batch_external_texture);
	glBindVertexArray(patch_vao);
	glDrawElementsInstanced(GL_TRIANGLES, 54, GL_UNSIGNED_INT, nullptr, GLsizei(instances.size()));
	glBindVertexArray(0);
	glUseProgram(0);
	ResetCompatibilityProgram();
	glActiveTexture(GL_TEXTURE0);

	stats.patch_batches += 1;
	frame_patch_batches += 1;
	stats.external_texture_batches += batch_external_texture != 0;
	stats.static_atlas_draws += std::ranges::any_of(instances, [](const PatchInstanceGPU& instance) {
		return instance.data0[0] == uint32_t(TextureSourceKindGL4::StaticAtlas);
	});
	stats.generated_atlas_draws += std::ranges::any_of(instances, [](const PatchInstanceGPU& instance) {
		return instance.data0[0] == uint32_t(TextureSourceKindGL4::GeneratedAtlas);
	});
	instances.clear();
	materials.clear();
	clips.clear();
	batch_external_texture = 0;
}

void RenderInterface_GL4_BAR::DrawGeneric(const GeometryGL4& geometry, Rml::Vector2f translation,
	const TextureHandleGL4* texture, const MaterialGPU* material_override)
{
	ZoneScopedN("RmlUi_GL4::DrawGeneric");
	FlushPatchBatch();
	const size_t ring_index = stream_ring_cursor++ % STREAM_RING_SIZE;
	const GLuint material_buffer = material_buffers[ring_index];
	const GLuint clip_buffer = clip_buffers[ring_index];
	MaterialGPU material = material_override ? *material_override : MaterialGPU{};
	if (!material_override)
		material.header[0] = texture ? (texture->kind == TextureSourceKindGL4::StaticAtlas ? MATERIAL_STATIC_IMAGE :
			(texture->kind == TextureSourceKindGL4::GeneratedAtlas ? MATERIAL_GENERATED_IMAGE : MATERIAL_EXTERNAL_IMAGE)) : MATERIAL_SOLID;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, material_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(material), &material, GL_STREAM_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, material_buffer);
	stats.material_buffer_bytes += sizeof(material);

	std::vector<ClipRecordGPU> generic_clips(active_clips);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, clip_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, GLsizeiptr(std::max<size_t>(generic_clips.size() * sizeof(ClipRecordGPU), 16)),
		generic_clips.empty() ? nullptr : generic_clips.data(), GL_STREAM_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, clip_buffer);
	stats.clip_buffer_bytes += generic_clips.size() * sizeof(ClipRecordGPU);

	glUseProgram(program);
	UniformConstants::GetInstance().Bind();
	glUniform1i(glGetUniformLocation(program, "uGenericMode"), GL_TRUE);
	glUniformMatrix4fv(glGetUniformLocation(program, "uProjection"), 1, GL_FALSE, projection.data());
	glUniformMatrix4fv(glGetUniformLocation(program, "uGenericTransform"), 1, GL_FALSE, current_transform.data());
	glUniform2f(glGetUniformLocation(program, "uGenericTranslation"), translation.x, translation.y);
	glUniform1f(glGetUniformLocation(program, "uGenericBrightness"), brightness);

	const bool atlased_texture = texture && (texture->kind == TextureSourceKindGL4::StaticAtlas ||
		texture->kind == TextureSourceKindGL4::GeneratedAtlas);
	const bool wraps_atlas = atlased_texture && geometry.wraps_atlas;
	Rml::Vector4f uv_rect = texture && !wraps_atlas ? texture->uv_rect : Rml::Vector4f(0.f, 0.f, 1.f, 1.f);
	glUniform4fv(glGetUniformLocation(program, "uGenericUvRect"), 1, &uv_rect[0]);
	const Rml::Vector4f atlas_uv_rect = texture ? texture->uv_rect : Rml::Vector4f(0.f, 0.f, 1.f, 1.f);
	glUniform4fv(glGetUniformLocation(program, "uGenericAtlasUvRect"), 1, &atlas_uv_rect[0]);
	const std::array<float, 4> scissor = gl4_scissor_region.Valid() ?
		std::array<float, 4>{float(gl4_scissor_region.Left()), float(gl4_scissor_region.Top()), float(gl4_scissor_region.Right()), float(gl4_scissor_region.Bottom())} :
		std::array<float, 4>{-1.f, -1.f, -1.f, -1.f};
	glUniform4fv(glGetUniformLocation(program, "uGenericScissor"), 1, scissor.data());
	glUniform4ui(glGetUniformLocation(program, "uGenericData"), texture ? uint32_t(texture->kind) : 0,
		texture ? texture->atlas_page : 0, 0, uint32_t(generic_clips.size()) | (uint32_t(wraps_atlas) << 31));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, static_atlas ? static_atlas->GetTextureId() : 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D_ARRAY, generated_atlas ? generated_atlas->GetTextureId() : 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texture &&
		(texture->kind == TextureSourceKindGL4::External2D || texture->kind == TextureSourceKindGL4::RenderTarget) ? texture->gl_texture : 0);
	glBindVertexArray(geometry.vao);
	glDrawElements(GL_TRIANGLES, geometry.index_count, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	glUseProgram(0);
	ResetCompatibilityProgram();
	glActiveTexture(GL_TEXTURE0);
	stats.generic_batches += 1;
	frame_generic_batches += 1;
	stats.static_atlas_draws += texture && texture->kind == TextureSourceKindGL4::StaticAtlas;
	stats.generated_atlas_draws += texture && texture->kind == TextureSourceKindGL4::GeneratedAtlas;
	stats.external_texture_batches += texture &&
		(texture->kind == TextureSourceKindGL4::External2D || texture->kind == TextureSourceKindGL4::RenderTarget);
}

bool RenderInterface_GL4_BAR::FinalizeTextureAtlas(TextureAtlasManifest& manifest)
{
	ZoneScopedN("RmlUi_GL4::FinalizeTextureAtlas");
	if (!manifest.Finalize())
		return false;
	stats.manifest_hash = manifest.GetStats().manifest_hash;
	static_atlas = std::make_unique<StaticTextureAtlasGL4>();
	if (!static_atlas->Build(manifest)) {
		atlas_stats = static_atlas->GetStats();
		static_atlas.reset();
		return false;
	}
	atlas_stats = static_atlas->GetStats();
	return true;
}

TextureAtlasStats RenderInterface_GL4_BAR::GetTextureAtlasStats() const
{
	return atlas_stats;
}

RendererStats RenderInterface_GL4_BAR::GetRendererStats() const
{
	return stats;
}

void RenderInterface_GL4_BAR::SetManifestHash(std::string hash)
{
	stats.manifest_hash = std::move(hash);
}

Rml::TextureHandle RenderInterface_GL4_BAR::GetExternalTexture(uint32_t gl_texture, Rml::Vector2i dimensions, const std::string& source)
{
	if (const auto handle_it = external_texture_handles.find(gl_texture); handle_it != external_texture_handles.end()) {
		const Rml::TextureHandle handle = handle_it->second;
		auto it = textures.find(handle);
		if (it == textures.end()) {
			external_texture_handles.erase(handle_it);
		} else {
			if (!it->second.owns_texture) {
				it->second.dimensions = dimensions;
				it->second.source = source;
			}
			return handle;
		}
	}
	TextureHandleGL4 texture;
	texture.kind = TextureSourceKindGL4::External2D;
	texture.dimensions = dimensions;
	texture.gl_texture = gl_texture;
	texture.owns_texture = false;
	texture.source = source;
	stats.lua_external_textures += 1;
	const Rml::TextureHandle handle = StoreTexture(std::move(texture));
	external_texture_handles.insert_or_assign(gl_texture, handle);
	return handle;
}

void RenderInterface_GL4_BAR::RenderExternalTexture(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation,
	uint32_t gl_texture, Rml::Vector2i dimensions, const std::string& source)
{
	RenderGeometry(geometry, translation, GetExternalTexture(gl_texture, dimensions, source));
}

void RenderInterface_GL4_BAR::WarnOnce(const std::string& category, const std::string& source)
{
	const std::string effective_source = source.empty() ? "<unknown>" : source;
	const std::string document = current_document_source.empty() ? "<not-rendering-document>" : current_document_source;
	const std::string key = category + ':' + effective_source + ':' + document;
	if (!warning_keys.emplace(key).second)
		return;
	LOG_L(L_WARNING, "[RmlUi GL4 fallback] category=%s source=%s document=%s strict=%d fallback_batch=%llu",
		category.c_str(), effective_source.c_str(), document.c_str(), RECOIL_RMLUI_GL4_STRICT_VALIDATION,
		static_cast<unsigned long long>(stats.generic_batches + stats.external_texture_batches));
	if ((warning_keys.size() % 64) == 0)
		LOG_L(L_WARNING, "[RmlUi GL4 fallback] %llu unique warnings observed so far.",
			static_cast<unsigned long long>(warning_keys.size()));
}
