/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

#include "System/Input/PointerWarp.h"

TEST_CASE("PointerWarp")
{
	using PointerWarp::MovesSystemPointer;

	CHECK(MovesSystemPointer(true, true));
	CHECK(MovesSystemPointer(true, false));
	CHECK(MovesSystemPointer(false, true));
	CHECK_FALSE(MovesSystemPointer(false, false));
}
