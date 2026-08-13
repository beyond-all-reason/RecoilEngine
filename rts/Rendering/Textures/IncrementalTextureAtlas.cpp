/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IncrementalTextureAtlas.h"

#include <algorithm>
#include <cstring>

#include "Rendering/GL/myGL.h"
#include "System/Log/ILog.h"

CIncrementalTextureAtlas::CIncrementalTextureAtlas(int page_size_, uint32_t max_pages_, int padding_)
	: page_size(page_size_)
	, padding(padding_)
	, max_pages(max_pages_)
	, pages(max_pages_)
{
	if (page_size <= 0 || max_pages == 0)
		return;
	slots.emplace_back(); // Entry id zero is always invalid.
	GrowStorage(1);
}

bool CIncrementalTextureAtlas::GrowStorage(uint32_t required_pages)
{
	if (required_pages <= allocated_pages)
		return true;
	if (required_pages > max_pages)
		return false;

	const uint32_t new_page_capacity = std::min(max_pages, std::max(required_pages, std::max(allocated_pages * 2, 1u)));
	while (glGetError() != GL_NO_ERROR) {}
	GLuint new_texture = 0;
	glGenTextures(1, &new_texture);
	glBindTexture(GL_TEXTURE_2D_ARRAY, new_texture);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, page_size, page_size, GLsizei(new_page_capacity));
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if (texture && allocated_pages > 0) {
		glCopyImageSubData(texture, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0,
			new_texture, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0,
			page_size, page_size, GLsizei(allocated_pages));
	}
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	if (!new_texture || glGetError() != GL_NO_ERROR) {
		LOG_L(L_ERROR, "[IncrementalTextureAtlas] Failed to create %dx%d texture array with %u layers.",
			page_size, page_size, new_page_capacity);
		if (new_texture)
			glDeleteTextures(1, &new_texture);
		return false;
	}
	if (texture)
		glDeleteTextures(1, &texture);
	texture = new_texture;
	allocated_pages = new_page_capacity;
	return true;
}

CIncrementalTextureAtlas::~CIncrementalTextureAtlas()
{
	if (texture)
		glDeleteTextures(1, &texture);
}

bool CIncrementalTextureAtlas::FindSpace(int width, int height, uint32_t& out_page, int& out_x, int& out_y)
{
	for (auto it = free_rects.begin(); it != free_rects.end(); ++it) {
		if (width <= it->width && height <= it->height) {
			out_page = it->page;
			out_x = it->x;
			out_y = it->y;
			free_rects.erase(it);
			return true;
		}
	}

	for (uint32_t page_index = 0; page_index < max_pages; ++page_index) {
		Page& page = pages[page_index];
		if (page.cursor_x + width > page_size) {
			page.cursor_x = 0;
			page.cursor_y += page.row_height;
			page.row_height = 0;
		}
		if (page.cursor_y + height > page_size)
			continue;
		if (!GrowStorage(page_index + 1))
			return false;
		out_page = page_index;
		out_x = page.cursor_x;
		out_y = page.cursor_y;
		page.cursor_x += width;
		page.row_height = std::max(page.row_height, height);
		page_count = std::max(page_count, page_index + 1);
		return true;
	}
	return false;
}

void CIncrementalTextureAtlas::UploadWithPadding(uint32_t page, int x, int y, const uint8_t* rgba, int width, int height)
{
	const int padded_width = width + 2 * padding;
	const int padded_height = height + 2 * padding;
	std::vector<uint8_t> pixels(size_t(padded_width) * size_t(padded_height) * 4);
	for (int py = 0; py < padded_height; ++py) {
		const int source_y = std::clamp(py - padding, 0, height - 1);
		for (int px = 0; px < padded_width; ++px) {
			const int source_x = std::clamp(px - padding, 0, width - 1);
			std::memcpy(&pixels[(size_t(py) * padded_width + px) * 4], &rgba[(size_t(source_y) * width + source_x) * 4], 4);
		}
	}
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, x, y, GLint(page), padded_width, padded_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

CIncrementalTextureAtlas::Entry CIncrementalTextureAtlas::Allocate(const uint8_t* rgba, int width, int height)
{
	if (!texture || !rgba || width <= 0 || height <= 0)
		return {};
	const int allocation_width = width + 2 * padding;
	const int allocation_height = height + 2 * padding;
	if (allocation_width > page_size || allocation_height > page_size)
		return {};

	uint32_t page = 0;
	int x = 0;
	int y = 0;
	if (!FindSpace(allocation_width, allocation_height, page, x, y))
		return {};

	uint32_t id;
	if (!reusable_slot_ids.empty()) {
		id = reusable_slot_ids.back();
		reusable_slot_ids.pop_back();
	} else {
		id = uint32_t(slots.size());
		slots.emplace_back();
	}

	Slot& slot = slots[id];
	slot.entry.id = id;
	slot.entry.generation = next_generation++;
	slot.entry.page = page;
	slot.entry.width = width;
	slot.entry.height = height;
	slot.entry.u0 = float(x + padding) / float(page_size);
	slot.entry.v0 = float(y + padding) / float(page_size);
	slot.entry.u1 = float(x + padding + width) / float(page_size);
	slot.entry.v1 = float(y + padding + height) / float(page_size);
	slot.allocation_x = x;
	slot.allocation_y = y;
	slot.allocation_width = allocation_width;
	slot.allocation_height = allocation_height;
	slot.live = true;
	UploadWithPadding(page, x, y, rgba, width, height);
	return slot.entry;
}

bool CIncrementalTextureAtlas::Release(uint32_t id, uint32_t generation)
{
	if (id == 0 || id >= slots.size())
		return false;
	Slot& slot = slots[id];
	if (!slot.live || slot.entry.generation != generation)
		return false;
	free_rects.push_back({slot.entry.page, slot.allocation_x, slot.allocation_y, slot.allocation_width, slot.allocation_height});
	slot.live = false;
	reusable_slot_ids.push_back(id);
	return true;
}

const CIncrementalTextureAtlas::Entry* CIncrementalTextureAtlas::Get(uint32_t id, uint32_t generation) const
{
	if (id == 0 || id >= slots.size())
		return nullptr;
	const Slot& slot = slots[id];
	return slot.live && slot.entry.generation == generation ? &slot.entry : nullptr;
}

uint64_t CIncrementalTextureAtlas::GetAllocatedBytes() const
{
	return uint64_t(page_size) * uint64_t(page_size) * uint64_t(allocated_pages) * 4;
}
