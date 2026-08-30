/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ReservedAtlasAlloc.h"

#include <algorithm>
#include <limits>
#include <tuple>
#include <unordered_set>

namespace {
	struct FreeRect {
		uint32_t page;
		AtlasPixelRect rect;
	};

	void PruneFreeList(std::vector<FreeRect>& freeRects)
	{
		for (size_t i = 0; i < freeRects.size();) {
			bool remove = !freeRects[i].rect.IsValid();
			for (size_t j = 0; !remove && j < freeRects.size(); ++j) {
				if (i != j && freeRects[i].page == freeRects[j].page && freeRects[j].rect.Contains(freeRects[i].rect))
					remove = true;
			}
			if (remove)
				freeRects.erase(freeRects.begin() + i);
			else
				++i;
		}
	}

	void SplitFreeList(std::vector<FreeRect>& freeRects, const FreeRect& used)
	{
		std::vector<FreeRect> split;
		for (const FreeRect& freeRect: freeRects) {
			if (freeRect.page != used.page || !freeRect.rect.Overlaps(used.rect)) {
				split.push_back(freeRect);
				continue;
			}

			const auto& f = freeRect.rect;
			const auto& u = used.rect;
			if (u.x > f.x)
				split.push_back({freeRect.page, {f.x, f.y, u.x - f.x, f.height}});
			if (u.Right64() < f.Right64())
				split.push_back({freeRect.page, {static_cast<int32_t>(u.Right64()), f.y, static_cast<int32_t>(f.Right64() - u.Right64()), f.height}});
			if (u.y > f.y)
				split.push_back({freeRect.page, {f.x, f.y, f.width, u.y - f.y}});
			if (u.Bottom64() < f.Bottom64())
				split.push_back({freeRect.page, {f.x, static_cast<int32_t>(u.Bottom64()), f.width, static_cast<int32_t>(f.Bottom64() - u.Bottom64())}});
		}
		freeRects = std::move(split);
		PruneFreeList(freeRects);
	}
}

ReservedAtlasAlloc::ReservedAtlasAlloc(int32_t width_, int32_t height_, std::vector<AtlasPageDefinition> pages_)
	: width(width_)
	, height(height_)
	, pages(std::move(pages_))
{}

std::optional<std::vector<ReservedAtlasPlacement>> ReservedAtlasAlloc::Plan(const std::vector<ReservedAtlasRequest>& requests) const
{
	if (width <= 0 || height <= 0 || pages.empty())
		return std::nullopt;

	std::vector<FreeRect> freeRects;
	for (uint32_t page = 0; page < pages.size(); ++page) {
		for (const auto& reserve: pages[page].reserves) {
			if (!reserve.FitsInside(width, height))
				return std::nullopt;
			for (const auto& freeRect: freeRects) {
				if (freeRect.page == page && freeRect.rect.Overlaps(reserve))
					return std::nullopt;
			}
			freeRects.push_back({page, reserve});
		}
	}

	std::unordered_set<std::string> ids;
	std::vector<ReservedAtlasPlacement> result;
	std::vector<const ReservedAtlasRequest*> pending;
	result.reserve(requests.size());
	pending.reserve(requests.size());

	for (const auto& request: requests) {
		if (request.id.empty() || !ids.emplace(request.id).second)
			return std::nullopt;
		const auto required = GetPaddedAtlasSize(request.contentSize, request.padding);
		if (!required)
			return std::nullopt;

		if (request.existingPage && request.existingAllocation) {
			if (*request.existingPage >= pages.size() || !request.existingAllocation->FitsInside(width, height))
				return std::nullopt;
			if (required->x <= request.existingAllocation->width && required->y <= request.existingAllocation->height) {
				const auto content = GetContentRect(*request.existingAllocation, request.contentSize, request.padding);
				if (!content)
					return std::nullopt;
				result.push_back({request.id, *request.existingPage, *content, *request.existingAllocation, true});
				continue;
			}
		}
		pending.push_back(&request);
	}

	std::sort(pending.begin(), pending.end(), [](const auto* lhs, const auto* rhs) {
		const auto l = *GetPaddedAtlasSize(lhs->contentSize, lhs->padding);
		const auto r = *GetPaddedAtlasSize(rhs->contentSize, rhs->padding);
		return std::tuple{-std::max(l.x, l.y), -int64_t{l.x} * l.y, lhs->id}
			< std::tuple{-std::max(r.x, r.y), -int64_t{r.x} * r.y, rhs->id};
	});

	for (const auto* request: pending) {
		const int2 required = *GetPaddedAtlasSize(request->contentSize, request->padding);
		std::optional<size_t> bestIndex;
		auto bestScore = std::tuple{std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), std::numeric_limits<uint32_t>::max(), std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max()};
		for (size_t i = 0; i < freeRects.size(); ++i) {
			const auto& freeRect = freeRects[i];
			if (required.x > freeRect.rect.width || required.y > freeRect.rect.height)
				continue;
			const int32_t remainingX = freeRect.rect.width - required.x;
			const int32_t remainingY = freeRect.rect.height - required.y;
			const auto score = std::tuple{std::min(remainingX, remainingY), std::max(remainingX, remainingY), freeRect.page, freeRect.rect.y, freeRect.rect.x};
			if (score < bestScore) {
				bestScore = score;
				bestIndex = i;
			}
		}

		if (!bestIndex)
			return std::nullopt;

		const auto& chosen = freeRects[*bestIndex];
		const AtlasPixelRect allocation {chosen.rect.x, chosen.rect.y, required.x, required.y};
		const auto content = GetContentRect(allocation, request->contentSize, request->padding);
		if (!content)
			return std::nullopt;
		result.push_back({request->id, chosen.page, *content, allocation, false});
		SplitFreeList(freeRects, {chosen.page, allocation});
	}

	std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) { return lhs.id < rhs.id; });
	return result;
}
