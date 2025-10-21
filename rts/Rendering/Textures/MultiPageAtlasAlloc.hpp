#pragma once

#include <type_traits>
#include <memory>
#include <tuple>
#include <vector>
#include <algorithm>
#include <set>
#include <limits>
#include <cmath>

#include "IAtlasAllocator.h"

// Concept requiring T to inherit from Base
template <typename T>
concept DerivedFromIAtlasAllocator = std::is_base_of_v<IAtlasAllocator, T>;

template<DerivedFromIAtlasAllocator AtlasAlloc>
class MultiPageAtlasAlloc : public IAtlasAllocator {
public:
	MultiPageAtlasAlloc(uint32_t maxPages_)
		: maxPages(maxPages_)
	{}
public:
	// Inherited via IAtlasAllocator
	bool Allocate() override
	{
		if (maxsize.x * maxsize.y <= 0)
			return false;

		std::vector<const SAtlasEntry*> memtextures;
		memtextures.reserve(entries.size());

		std::set<std::string> sortedNames;
		for (auto& entry : entries) {
			sortedNames.insert(entry.first);
		}

		size_t totalAreaSum = 0;
		for (auto& name : sortedNames) {
			const auto& entry = entries[name];
			memtextures.push_back(&entry);
			totalAreaSum += (entry.size.x * entry.size.y);
		}

		std::stable_sort(memtextures.begin(), memtextures.end(), [](const auto* lhs, const auto* rhs) {
		#if 0
			return std::forward_as_tuple(lhs->size.y, lhs->size.x, lhs->name) > std::forward_as_tuple(rhs->size.y, rhs->size.x, rhs->name);
		#else
			return std::forward_as_tuple(lhs->size.y * lhs->size.x, lhs->name) > std::forward_as_tuple(rhs->size.y * rhs->size.x, rhs->name);
		#endif
		});

		// guestimate the initial number of pages
		auto numPages = static_cast<uint32_t>(::ceilf(
			totalAreaSum / static_cast<float>(maxsize.x * maxsize.y)
		));

		std::vector<size_t> areaSums;

		bool done = false;
		while (!done) {
			if (numPages > maxPages)
				break;

			allocators.clear();
			allocators.resize(numPages);
			areaSums.clear();
			areaSums.resize(numPages, 0);

			for (auto& allocator : allocators) {
				allocator = std::move(std::make_unique<AtlasAlloc>());
				allocator->SetMaxSize(maxsize.x, maxsize.y);
				allocator->SetMaxTexLevel(numLevels);
			}

			for (const auto* memtexture : memtextures) {
				auto minAreaIdx = std::distance(areaSums.begin(), std::min_element(areaSums.begin(), areaSums.end()));
				allocators[minAreaIdx]->AddEntry(*memtexture);
				areaSums[minAreaIdx] += (memtexture->size.x * memtexture->size.y);
			}

			done = true;
			for (auto& allocator : allocators) {
				done &= allocator->Allocate();
			}

			++numPages;
		}

		if (!done)
			return false;

		// figure out tex coords and atlasSize
		for (size_t i = 0; i < allocators.size(); ++i) {
			const auto& allocator = allocators[i];

			const auto& allocEntries = allocator->GetEntries();
			for (const auto& [name, ae] : allocEntries) {
				assert(entries.contains(name));
				auto& myEntry = entries[name];
				myEntry.texCoords = ae.texCoords;
				myEntry.texCoords.pageNum = static_cast<uint32_t>(i);
			}

			const auto& as = allocator->GetAtlasSize();
			atlasSize.x = std::max(atlasSize.x, as.x);
			atlasSize.y = std::max(atlasSize.y, as.y);
		}

		return true;
	}

	int GetNumTexLevels() const override
	{
		int minLevels = std::numeric_limits<int>::max();
		for (auto& allocator : allocators) {
			minLevels = std::min(minLevels, allocator->GetNumTexLevels());
		}

		return (minLevels == std::numeric_limits<int>::max()) ? 0 : minLevels;
	}

	int GetReqNumTexLevels() const override
	{
		int reqLevels = std::numeric_limits<int>::max();
		for (auto& allocator : allocators) {
			reqLevels = std::min(reqLevels, allocator->GetNumTexLevels());
		}

		return (reqLevels == std::numeric_limits<int>::max()) ? 0 : reqLevels;
	}
	uint32_t GetNumPages() const override { return allocators.size(); }
private:
	uint32_t maxPages;
	std::vector<std::unique_ptr<AtlasAlloc>> allocators;
};