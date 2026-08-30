/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

#include "Rendering/Textures/TextureAtlasTypes.h"

TEST_CASE("Atlas pixel rectangles are top-left, half-open, and checked")
{
	const AtlasPixelRect rect {10, 20, 30, 40};
	CHECK(rect.Right64() == 40);
	CHECK(rect.Bottom64() == 60);
	CHECK(rect.Overlaps({39, 59, 1, 1}));
	CHECK_FALSE(rect.Overlaps({40, 20, 1, 1}));
	CHECK(rect.Contains({10, 20, 30, 40}));
	CHECK_FALSE(AtlasPixelRect{-1, 0, 1, 1}.IsValid());
	CHECK_FALSE(AtlasPixelRect{INT32_MAX, 0, 1, 1}.IsValid());
}

TEST_CASE("Atlas padding dimensions are checked")
{
	CHECK(GetPaddedAtlasSize(int2(8, 4), {AtlasPaddingMode::CLAMP, 2, 0, 0}) == int2(12, 8));
	CHECK(GetPaddedAtlasSize(int2(8, 4), {AtlasPaddingMode::TILE, 0, 1, 2}) == int2(24, 20));
	CHECK_FALSE(GetPaddedAtlasSize(int2(0, 4), {}));
	CHECK_FALSE(GetPaddedAtlasSize(int2(INT32_MAX, 4), {AtlasPaddingMode::CLAMP, 1, 0, 0}));
}

TEST_CASE("Pixel to UV conversion preserves top-left coordinates")
{
	const AtlasedTexture uv = AtlasPixelRectToUV({25, 10, 50, 20}, 100, 100, 2);
	CHECK(uv.x1 == Catch::Approx(0.25f));
	CHECK(uv.y1 == Catch::Approx(0.10f));
	CHECK(uv.x2 == Catch::Approx(0.75f));
	CHECK(uv.y2 == Catch::Approx(0.30f));
	CHECK(uv.pageNum == 2);
}
