/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>

#include "Sim/Projectiles/Projectile.h"
#include "System/Color.h"

class CUnit;

class CNanoProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CNanoProjectile)

public:
	CNanoProjectile();
	CNanoProjectile(float3 pos, float3 speed, int lifeTime, SColor color, const CUnit* homingTarget = nullptr, int homingTargetPiece = -1, bool inverse = false);
	~CNanoProjectile();

	void Update() override;
	void Draw() override;
	void DrawOnMinimap() const override;
	float GetDrawRadius() const override;

	int GetProjectilesCount() const override;

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

private:
	void InitHoming(const CUnit* target, int targetPiece, int lifeTime);
	void InitGroundClamp(bool inverse, int lifeTime);
	bool ResolveHomingTarget(float3& targetPos);
	bool UpdateGroundClamp(int frame);
	void UpdateHoming(int frame);
	void Reaim(const float3& targetPos, int remainingFrames);

	float rotAcc = 0.0f;

	float rotVal0x = 0.0f;
	float rotVel0x = 0.0f;
	float rotAcc0x = 0.0f;

	int homingTargetID = -1;
	std::int64_t homingTargetSyncID = -1;
	int homingTargetPiece = -1;
	float homingSpeedLimitSq = 0.0f;
	float3 homingOffset;
	float3 lastHomingTargetPos;

	float3 groundClampFinalPos;
	float3 groundClampWaypointPos;
	int groundClampWaypointFrame = -1;
	int groundClampNextFrame = -1;
	uint8_t updatePhase = 0;
	bool groundClampActive = false;
public:
	static inline float rotVal0 = 0.0f;
	static inline float rotVel0 = 0.0f;
	static inline float rotAcc0 = 0.0f;
	static inline float rotValRng0 = 0.0f;
	static inline float rotVelRng0 = 0.0f;
	static inline float rotAccRng0 = 0.0f;

	int deathFrame;
	SColor color;
};

