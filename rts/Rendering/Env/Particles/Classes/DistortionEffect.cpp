#include "DistortionEffect.hpp"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Game/Camera.h"
#include "System/SpringMath.h"
#include "Sim/Projectiles/ExpGenSpawnableMemberInfo.h"
#include "Sim/Misc/GlobalSynced.h"

CR_BIND_DERIVED(CDistortionEffect, CProjectile, )

CR_REG_METADATA(CDistortionEffect,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
	CR_IGNORED(texture),
	CR_MEMBER(sizeParams),
	CR_MEMBER(time),
	CR_MEMBER(xSize),
	CR_MEMBER(ySize),
	CR_MEMBER(strength),
	CR_MEMBER(directional),
	CR_MEMBER_ENDFLAG(CM_Config),

	CR_MEMBER(createTime),

	CR_SERIALIZER(Serialize)
))


CDistortionEffect::CDistortionEffect()
	: CProjectile()
	, createTime(gs->frameNum)
{
}

void CDistortionEffect::Init(const CUnit* owner, const float3& offset)
{
	CProjectile::Init(owner, offset);

	SetDrawRadius(std::max(xSize, ySize));

	validTextures[1] = IsValidTexture(texture);
	validTextures[0] = validTextures[1];

	drawSorted = false;
	castShadow = false;
}

void CDistortionEffect::Serialize(creg::ISerializer* s)
{
	std::string texName;
	if (s->IsWriting()) {
		texName = projectileDrawer->textureAtlas->GetTextureName(texture);
	}
	creg::GetType(texName)->Serialize(s, &texName);
	if (!s->IsWriting()) {
		texture = projectileDrawer->textureAtlas->GetTexturePtr(texName);
	}
}


void CDistortionEffect::Draw()
{
	if (!UpdateAnimParams())
		return;

	const float3 drawPos = pos + speed * globalRendering->timeOffset;

	float3 xdir, ydir;
	if (directional) {
		const float3 zdir = (drawPos - camera->GetPos());
		xdir = (dir.cross(zdir)).SafeANormalize();
		ydir = (dir.cross(xdir)).SafeANormalize();
	}
	else {
		xdir = camera->GetRight();
		ydir = camera->GetUp();
	}

	float currDuration = (gs->frameNum - createTime) * INV_GAME_SPEED + globalRendering->timeOffset;

	float currStrength = strength;
	// fade-in
	if (currDuration < time[0]) {
		currStrength *= smoothstep(0.0f, time[0], currDuration);
	}
	// fade-out
	else if (currDuration > time[0] + time[1]) {
		currStrength *= smoothstep(time[0] + time[1] + time[2], time[0] + time[1], currDuration);
	}

	currStrength = std::clamp(currStrength, 0.0f, 1.0f);

	SColor col { currStrength, 0.0f, 0.0f, 1.0f };

	// S = (V + A * t) * t;
	float currXS = xSize + (sizeParams[0] + sizeParams[1] * currDuration) * currDuration;
	float currYS = xSize + (sizeParams[2] + sizeParams[3] * currDuration) * currDuration;

	SetDrawRadius(std::max(currXS, currYS));

	std::array<float3, 4> bounds = {
		 -xdir * currXS + ydir * currYS,
		  xdir * currXS + ydir * currYS,
		  xdir * currXS - ydir * currYS,
		 -xdir * currXS - ydir * currYS
	};

	AddEffectsQuad<1>(
		texture->pageNum,
		{ drawPos + bounds[0], texture->xstart, texture->ystart, col },
		{ drawPos + bounds[1], texture->xend  , texture->ystart, col },
		{ drawPos + bounds[2], texture->xend  , texture->yend  , col },
		{ drawPos + bounds[3], texture->xstart, texture->yend  , col }
	);
}

void CDistortionEffect::Update()
{
	pos += speed;
	//speed += gravity;
	//speed *= airdrag;
	deleteMe = (gs->frameNum - createTime) * INV_GAME_SPEED >= time[0] + time[1] + time[2];
}


int CDistortionEffect::GetProjectilesCount() const
{
	return 1;
}

bool CDistortionEffect::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	if (CProjectile::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_PTR(CDistortionEffect, texture, projectileDrawer->textureAtlas->GetTexturePtr);
	CHECK_MEMBER_INFO_FLOAT4(CDistortionEffect, sizeParams);
	CHECK_MEMBER_INFO_FLOAT3(CDistortionEffect, time);
	CHECK_MEMBER_INFO_FLOAT(CDistortionEffect, xSize);
	CHECK_MEMBER_INFO_FLOAT(CDistortionEffect, ySize);
	CHECK_MEMBER_INFO_FLOAT(CDistortionEffect, strength);

	return false;
}