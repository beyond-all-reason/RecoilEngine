#pragma once

#include "Sim/Projectiles/Projectile.h"

struct AtlasedTexture;
class CDistortionEffect : public CProjectile
{
	CR_DECLARE_DERIVED(CDistortionEffect)

public:
	CDistortionEffect();

	void Serialize(creg::ISerializer* s);

	void Draw() override;
	void Update() override;

	void Init(const CUnit* owner, const float3& offset) override;

	int GetProjectilesCount() const override;

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);
private:
	AtlasedTexture* texture = nullptr;
	float4 sizeParams{}; // xSizeV, xSizeA, ySizeV, ySizeA
	float3 time{}; // fadeInTime, stayTime, fadeOutTime
	float xSize = 0.0f;
	float ySize = 0.0f;
	float strength = 0.0f;
	int createTime = 0;
	bool directional = false;
};