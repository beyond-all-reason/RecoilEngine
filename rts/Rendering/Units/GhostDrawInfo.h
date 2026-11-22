/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once
#include "System/Matrix44f.h"
#include "System/float4.h"
struct S3DModel;

// Only observer snapshots belong here; never expose the hidden unit's current state.
struct GhostDrawInfo {
	const S3DModel* model = nullptr;
	CMatrix44f transform;
	float4 teamColor;
	float ghostID = 0;
	int allyTeam = -1;
};
