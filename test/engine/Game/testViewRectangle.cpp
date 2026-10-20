/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include <catch_amalgamated.hpp>
#include <limits>
#include "Game/ViewRectangle.h"

TEST_CASE("ViewRectangle validates map coordinates")
{
	ViewRectangle unset;
	CHECK_FALSE(unset.IsSet());
	CHECK_FALSE(unset.ValidForMap(1024, 1024));
	const ViewRectangle rectangle{100, 200, 900, 600};
	REQUIRE(rectangle.ValidForMap(1024, 1024));
	CHECK(rectangle.Width() == 800);
	CHECK(rectangle.Height() == 400);
	CHECK(rectangle.CenterX() == 500);
	CHECK(rectangle.CenterZ() == 400);
	CHECK(rectangle.Contains(100, 200));
	CHECK(rectangle.Contains(900, 600));
	CHECK_FALSE(rectangle.Contains(99, 200));
	CHECK_FALSE(rectangle.Contains(900, 601));
	const auto nan = std::numeric_limits<float>::quiet_NaN();
	const auto inf = std::numeric_limits<float>::infinity();
	for (const auto invalid: {ViewRectangle{nan, 200, 900, 600}, {100, inf, 900, 600},
		{100, 200, inf, 600}, {100, 200, 900, nan}, {-1, 200, 900, 600},
		{100, 200, 1025, 600}, {100, 200, 900, 1025}, {900, 200, 100, 600},
		{100, 600, 900, 600}, {100, 200, 100, 600}}) {
		CHECK_FALSE(invalid.ValidForMap(1024, 1024));
	}
	CHECK(rectangle.ClampX(-100) > rectangle.x0);
	CHECK(rectangle.ClampX(2000) < rectangle.x1);
	CHECK(rectangle.ClampX(500) == 500);
	CHECK(rectangle.ClampZ(-100) > rectangle.z0);
	CHECK(rectangle.ClampZ(2000) < rectangle.z1);
	CHECK(rectangle.ClampZ(400) == 400);
}

TEST_CASE("ViewRectangle clamp supports narrow but valid regions")
{
	ViewRectangle rectangle{0, 0, 0.001f, 0.002f};
	REQUIRE(rectangle.ValidForMap(1024, 1024));
	CHECK(rectangle.ClampX(-1) > rectangle.x0);
	CHECK(rectangle.ClampX(1) < rectangle.x1);
	CHECK(rectangle.ClampZ(-1) > rectangle.z0);
	CHECK(rectangle.ClampZ(1) < rectangle.z1);
	rectangle = {0, 0, 1024, 1024};
	REQUIRE(rectangle.ValidForMap(1024, 1024));
	CHECK(rectangle.Contains(0, 0));
	CHECK(rectangle.Contains(1024, 1024));
}
