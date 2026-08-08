/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>

#include "System/Color.h"
#include "System/float3.h"

struct DirectNanoParticle {
	float3 startPos = ZeroVector;
	float3 velocity = ZeroVector;
	SColor color;
	int createFrame = 0;
	int deathFrame = 0;
	int lightID = -1;
	int allyTeam = -1;
	float builderBuildSpeed = 0.0f;
	bool lightSelected = false;
};

enum class DirectNanoParticleChangeType : std::uint8_t {
	Add,
	Remove,
};

struct DirectNanoParticleChange {
	DirectNanoParticle particle;
	DirectNanoParticleChangeType type = DirectNanoParticleChangeType::Add;
};

struct DirectNanoParticleExpiry {
	int deathFrame = 0;
	int lightID = -1;
};