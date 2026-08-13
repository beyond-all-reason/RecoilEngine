/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <vector>

class CIncrementalTextureAtlas
{
public:
	struct Entry {
		uint32_t id = 0;
		uint32_t generation = 0;
		uint32_t page = 0;
		int width = 0;
		int height = 0;
		float u0 = 0.f;
		float v0 = 0.f;
		float u1 = 0.f;
		float v1 = 0.f;
		explicit operator bool() const { return id != 0; }
	};

	CIncrementalTextureAtlas(int page_size, uint32_t max_pages, int padding = 2);
	~CIncrementalTextureAtlas();
	CIncrementalTextureAtlas(const CIncrementalTextureAtlas&) = delete;
	CIncrementalTextureAtlas& operator=(const CIncrementalTextureAtlas&) = delete;

	explicit operator bool() const { return texture != 0; }
	Entry Allocate(const uint8_t* rgba, int width, int height);
	bool Release(uint32_t id, uint32_t generation);
	const Entry* Get(uint32_t id, uint32_t generation) const;

	uint32_t GetTextureId() const { return texture; }
	uint32_t GetPageCount() const { return page_count; }
	uint64_t GetAllocatedBytes() const;

private:
	struct Slot {
		Entry entry;
		int allocation_x = 0;
		int allocation_y = 0;
		int allocation_width = 0;
		int allocation_height = 0;
		bool live = false;
	};
	struct FreeRect { uint32_t page; int x, y, width, height; };
	struct Page { int cursor_x = 0; int cursor_y = 0; int row_height = 0; };

	bool FindSpace(int width, int height, uint32_t& page, int& x, int& y);
	bool GrowStorage(uint32_t required_pages);
	void UploadWithPadding(uint32_t page, int x, int y, const uint8_t* rgba, int width, int height);

	uint32_t texture = 0;
	int page_size = 0;
	int padding = 0;
	uint32_t max_pages = 0;
	uint32_t allocated_pages = 0;
	uint32_t page_count = 0;
	uint32_t next_generation = 1;
	std::vector<Page> pages;
	std::vector<Slot> slots;
	std::vector<uint32_t> reusable_slot_ids;
	std::vector<FreeRect> free_rects;
};
