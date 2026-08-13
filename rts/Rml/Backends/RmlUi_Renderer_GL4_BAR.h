/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "RmlUi_Renderer_GL3_Recoil.h"
#include "RmlUi_Renderer_GL4_Types.h"
#include "RmlUi_TextureAtlas_GL4.h"

#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class CIncrementalTextureAtlas;

class RenderInterface_GL4_BAR final : public RenderInterface_GL_Recoil_Common
{
public:
	RenderInterface_GL4_BAR();
	~RenderInterface_GL4_BAR() override;

	explicit operator bool() const;

	void SetViewport(int width, int height, int offset_x = 0, int offset_y = 0);
	void BeginFrame();
	void EndFrame();

	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
	Rml::CompiledGeometryHandle CompileGeometryWithMetadata(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices,
		const Rml::RenderGeometryMetadata* metadata) override;
	void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

	Rml::TextureHandle LoadTexture(Rml::Vector2i& dimensions, const Rml::String& source) override;
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;
	void EnableClipMask(bool enable) override;
	void RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation) override;
	void SetTransform(const Rml::Matrix4f* transform) override;

	Rml::LayerHandle PushLayer() override;
	void CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode,
		Rml::Span<const Rml::CompiledFilterHandle> filters) override;
	void PopLayer() override;
	Rml::TextureHandle SaveLayerAsTexture() override;
	Rml::CompiledFilterHandle SaveLayerAsMaskImage() override;
	Rml::CompiledFilterHandle CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters) override;

	Rml::CompiledShaderHandle CompileShader(const Rml::String& name, const Rml::Dictionary& parameters) override;
	void RenderShader(Rml::CompiledShaderHandle shader, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation,
		Rml::TextureHandle texture) override;
	void ReleaseShader(Rml::CompiledShaderHandle shader) override;

	bool SupportsInlineBrightness() const override { return true; }
	void PushInlineBrightness(float value) override;
	void PopInlineBrightness() override;
	void BeginDocument(const Rml::String& source_url) override;
	void EndDocument() override;
	void NotifyBoxShadowOperation() override { stats.box_shadow_operations += 1; }
	void NotifyBackdropFilterOperation() override { stats.backdrop_filter_operations += 1; }

	bool FinalizeTextureAtlas(TextureAtlasManifest& manifest);
	TextureAtlasStats GetTextureAtlasStats() const;
	RendererStats GetRendererStats() const;
	void SetManifestHash(std::string hash);
	void RenderExternalTexture(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, uint32_t gl_texture,
		Rml::Vector2i dimensions, const std::string& source);

private:
	struct GeometryGL4;
	struct ShaderGL4;
	struct PatchTemplate;
	struct PatchInstanceGPU;
	struct MaterialGPU;
	struct ClipRecordGPU;

	bool ValidateCapabilities();
	bool CreateProgram();
	bool CreateBuffers();
	void DestroyResources();
	void PrepareCompatibilityScissor();
	void RestoreShaderScissor();
	void FlushPatchBatch();
	void DrawGeneric(const GeometryGL4& geometry, Rml::Vector2f translation, const TextureHandleGL4* texture,
		const MaterialGPU* material = nullptr);
	void AppendGeometry(const GeometryGL4& geometry, Rml::Vector2f translation, const TextureHandleGL4* texture,
		const MaterialGPU* material_override = nullptr);
	TextureHandleGL4* FindTexture(Rml::TextureHandle texture);
	const TextureHandleGL4* FindTexture(Rml::TextureHandle texture) const;
	Rml::TextureHandle StoreTexture(TextureHandleGL4 texture);
	Rml::TextureHandle GetExternalTexture(uint32_t gl_texture, Rml::Vector2i dimensions, const std::string& source);
	void WarnOnce(const std::string& category, const std::string& source);

	bool valid = false;
	int viewport_width = 1;
	int viewport_height = 1;
	int viewport_offset_x = 0;
	int viewport_offset_y = 0;
	Rml::Matrix4f projection = Rml::Matrix4f::Identity();
	Rml::Matrix4f current_transform = Rml::Matrix4f::Identity();
	Rml::Rectanglei gl4_scissor_region = Rml::Rectanglei::MakeInvalid();
	bool compatibility_clip_active = false;
	float brightness = 1.f;
	std::vector<float> brightness_stack;

	uint32_t program = 0;
	uint32_t patch_vao = 0;
	uint32_t patch_index_buffer = 0;
	static constexpr size_t STREAM_RING_SIZE = 3;
	std::array<uint32_t, STREAM_RING_SIZE> instance_buffers = {};
	std::array<uint32_t, STREAM_RING_SIZE> material_buffers = {};
	std::array<uint32_t, STREAM_RING_SIZE> clip_buffers = {};
	size_t stream_ring_cursor = 0;
	uint32_t next_texture_generation = 1;
	Rml::TextureHandle next_virtual_texture = Rml::TextureHandle(1) << (sizeof(Rml::TextureHandle) * 8 - 1);

	std::vector<PatchInstanceGPU> instances;
	std::vector<MaterialGPU> materials;
	std::vector<ClipRecordGPU> clips;
	std::vector<ClipRecordGPU> active_clips;
	uint32_t batch_external_texture = 0;

	std::unordered_map<Rml::TextureHandle, TextureHandleGL4> textures;
	std::unordered_map<uint32_t, Rml::TextureHandle> external_texture_handles;
	std::unordered_map<std::string, Rml::TextureHandle> static_texture_handles;
	std::unordered_set<std::string> warning_keys;
	std::string current_document_source;
	std::unique_ptr<StaticTextureAtlasGL4> static_atlas;
	std::unique_ptr<CIncrementalTextureAtlas> generated_atlas;
	TextureAtlasStats atlas_stats;
	RendererStats stats;
	uint64_t frame_patch_instances = 0;
	uint64_t frame_glyph_instances = 0;
	uint64_t frame_patch_batches = 0;
	uint64_t frame_generic_batches = 0;
};
