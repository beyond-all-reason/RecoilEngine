/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>

class CRasRNG
{
public:
	explicit CRasRNG(uint32_t seed)
		: state(seed ? seed : 1u)
	{}

	uint32_t Next()
	{
		uint32_t x = state;
		x ^= x << 13;
		x ^= x >> 17;
		x ^= x << 5;
		state = x;
		return x;
	}

	int NextInt(int n)
	{
		return static_cast<int>(Next() % static_cast<uint32_t>(n));
	}

	uint32_t state;
};
