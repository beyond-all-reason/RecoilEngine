/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * This source file is derived from the source code of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUI_BACKENDS_BACKEND_H
#define RMLUI_BACKENDS_BACKEND_H

#include "Rml/Rml_MathTypes_Conversions.h"
#include <RmlUi/Core.h>
#include <SDL.h>

#include "Game/UI/InputReceiver.h"
#include "lib/sol2/sol.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace RmlGui
{
	struct TextureAtlasStats {
		bool finalized = false;
		bool gpu_atlas_created = false;
		std::string manifest_hash;
		std::string failure_reason;
		uint64_t source_count = 0;
		uint64_t duplicate_count = 0;
		uint64_t total_source_pixels = 0;
		uint64_t allocated_gpu_bytes = 0;
		uint32_t page_width = 0;
		uint32_t page_height = 0;
		uint32_t page_count = 0;
		uint32_t mip_count = 0;
		double build_duration_ms = 0.0;
		std::vector<float> page_occupancy;
		std::vector<std::string> largest_sources;
	};

	struct RendererStats {
		std::string renderer;
		std::string manifest_hash;
		uint64_t patch_instances = 0;
		uint64_t glyph_instances = 0;
		uint64_t patch_batches = 0;
		uint64_t generic_batches = 0;
		uint64_t external_texture_batches = 0;
		uint64_t static_atlas_draws = 0;
		uint64_t generated_atlas_draws = 0;
		uint64_t late_file_textures = 0;
		uint64_t lua_external_textures = 0;
		uint64_t unsupported_geometry = 0;
		uint64_t unsupported_gradients = 0;
		uint64_t unsupported_clip_masks = 0;
		uint64_t box_shadow_operations = 0;
		uint64_t backdrop_filter_operations = 0;
		uint64_t offscreen_layer_operations = 0;
		uint64_t shader_brightness_operations = 0;
		uint64_t document_batch_boundaries = 0;
		uint64_t instance_buffer_bytes = 0;
		uint64_t material_buffer_bytes = 0;
		uint64_t clip_buffer_bytes = 0;
		uint64_t text_run_cache_hits = 0;
		uint64_t text_run_cache_misses = 0;
		uint64_t text_run_disabled_submissions = 0;
	};

	bool Initialize();
	bool InitializeLua(lua_State* lua_state);
	bool RemoveLua();

	void Shutdown();
	void Reload();

	Rml::SystemInterface* GetSystemInterface();
	Rml::RenderInterface* GetRenderInterface();

	bool ProcessEvent(const SDL_Event& event);

	bool ProcessKeyPressed(int keyCode, int scanCode, bool isRepeat);
	bool ProcessKeyReleased(int keyCode, int scanCode);
	bool ProcessTextInput(const std::string& text);
	bool ProcessMouseMove(int x, int y, int dx, int dy, int button);
	bool ProcessMousePress(int x, int y, int button);
	bool ProcessMouseRelease(int x, int y, int button);
	bool ProcessMouseWheel(float delta);

	void SetDebugContext(Rml::Context* context);

	bool IsMouseInteractingWith();
	const std::string& GetMouseCursor();
	void SetMouseCursorAlias(std::string from, std::string to);
	CInputReceiver* GetInputReceiver();
	lua_State* GetLuaState();

	void Update();
	void RenderFrame();

	void OnContextCreate(Rml::Context* context);
	void OnContextDestroy(Rml::Context* context);
	
	Rml::Context* GetOrCreateContext(const std::string& name);
	Rml::Context* GetContext(const std::string& name);
	void MarkContextForRemoval(Rml::Context* context);

	void BeginFrame();
	void PresentFrame();

	bool RegisterAtlasTexture(const std::string& path);
	bool RegisterAtlasTextures(const std::vector<std::string>& paths);
	bool FinalizeTextureAtlas();
	TextureAtlasStats GetTextureAtlasStats();
	RendererStats GetRendererStats();

	void RenderExternalTexture(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, uint32_t gl_texture,
		Rml::Vector2i dimensions, const std::string& source);

}  // namespace RmlGui

#endif
