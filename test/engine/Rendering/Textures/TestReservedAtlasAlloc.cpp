/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

#include "Rendering/Textures/ReservedAtlasAlloc.h"

namespace {
	AtlasPadding Clamp(uint32_t pixels)
	{
		return AtlasPadding{AtlasPaddingMode::CLAMP, pixels, 0, 0};
	}

	AtlasPadding Tile(uint32_t x, uint32_t y)
	{
		return AtlasPadding{AtlasPaddingMode::TILE, 0, x, y};
	}
}

TEST_CASE("Reserved atlas allocation is deterministic and transactional")
{
	AtlasPageDefinition page;
	page.reserves = {{0, 0, 32, 32}, {32, 0, 32, 32}};
	const ReservedAtlasAlloc allocator(64, 32, {page});
	const std::vector<ReservedAtlasRequest> requests {
		{"z", int2(8, 8), Clamp(2)},
		{"a", int2(16, 8), Clamp(0)},
		{"m", int2(4, 4), Tile(1, 1)},
	};

	const auto first = allocator.Plan(requests);
	REQUIRE(first);
	const std::vector<ReservedAtlasRequest> reversed(requests.rbegin(), requests.rend());
	const auto second = allocator.Plan(reversed);
	REQUIRE(second);
	CHECK(*first == *second);
	CHECK(first->size() == requests.size());
}

TEST_CASE("Reserved atlas allocator keeps fitting overrides in place")
{
	AtlasPageDefinition page;
	page.reserves = {{32, 0, 32, 32}};
	const ReservedAtlasAlloc allocator(64, 32, {page});

	ReservedAtlasRequest request {"existing", int2(8, 6), Clamp(2)};
	request.existingPage = 0;
	request.existingAllocation = AtlasPixelRect{4, 4, 20, 20};
	const auto result = allocator.Plan({request});

	REQUIRE(result);
	REQUIRE(result->size() == 1);
	CHECK(result->front().reusedExisting);
	CHECK(result->front().allocation == *request.existingAllocation);
	CHECK(result->front().content == AtlasPixelRect{6, 6, 8, 6});
}

TEST_CASE("Larger overrides relocate without recycling their old allocation")
{
	AtlasPageDefinition page;
	page.reserves = {{16, 0, 16, 16}};
	const ReservedAtlasAlloc allocator(32, 16, {page});

	ReservedAtlasRequest moved {"moved", int2(12, 12), Clamp(0)};
	moved.existingPage = 0;
	moved.existingAllocation = AtlasPixelRect{0, 0, 8, 8};
	const auto result = allocator.Plan({moved, {"new", int2(5, 5), Clamp(0)}});

	CHECK_FALSE(result);
	const auto retry = allocator.Plan({{"new", int2(5, 5), Clamp(0)}});
	REQUIRE(retry);
	CHECK(retry->front().allocation == AtlasPixelRect{16, 0, 5, 5});
}

TEST_CASE("Reserved atlas allocator supports pages and rejects invalid geometry")
{
	AtlasPageDefinition first;
	first.reserves = {{0, 0, 8, 8}};
	AtlasPageDefinition second;
	second.reserves = {{0, 0, 16, 16}};
	const ReservedAtlasAlloc allocator(16, 16, {first, second});

	const auto result = allocator.Plan({{"large", int2(12, 12), Clamp(0)}});
	REQUIRE(result);
	CHECK(result->front().page == 1);

	first.reserves.push_back({4, 4, 8, 8});
	CHECK_FALSE(ReservedAtlasAlloc(16, 16, {first}).Plan({}));
	AtlasPageDefinition invalidPage;
	invalidPage.reserves = {{-1, 0, 1, 1}};
	CHECK_FALSE(ReservedAtlasAlloc(16, 16, {invalidPage}).Plan({}));
	CHECK_FALSE(allocator.Plan({{"overflow", int2(1, 1), Clamp(UINT32_MAX)}}));
}

TEST_CASE("Tile padding places content in the central tile")
{
	AtlasPageDefinition page;
	page.reserves = {{0, 0, 64, 64}};
	const ReservedAtlasAlloc allocator(64, 64, {page});
	const auto result = allocator.Plan({{"tile", int2(5, 7), Tile(2, 1)}});

	REQUIRE(result);
	CHECK(result->front().allocation == AtlasPixelRect{0, 0, 25, 21});
	CHECK(result->front().content == AtlasPixelRect{10, 7, 5, 7});
}
