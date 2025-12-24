/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXP_GEN_SPAWNABLE_H
#define EXP_GEN_SPAWNABLE_H

#include <memory>
#include <array>
#include <tuple>

#include "Sim/Objects/WorldObject.h"
#include "System/Threading/ThreadPool.h"
#include "Rendering/GL/RenderBuffersFwd.h"

struct SExpGenSpawnableMemberInfo;
class CUnit;

class CExpGenSpawnable : public CWorldObject
{
	CR_DECLARE(CExpGenSpawnable)
public:
	using AllocFunc = CExpGenSpawnable*(*)();
	using GetMemberInfoFunc = bool(*)(SExpGenSpawnableMemberInfo&);
	using SpawnableTuple = std::tuple<std::string, GetMemberInfoFunc, AllocFunc>;

	CExpGenSpawnable(const float3& pos, const float3& spd);

	~CExpGenSpawnable() override;
	virtual void Init(const CUnit* owner, const float3& offset);

	static bool GetSpawnableMemberInfo(const std::string& spawnableName, SExpGenSpawnableMemberInfo& memberInfo);
	static int GetSpawnableID(const std::string& spawnableName);

	static void InitSpawnables();

	//Memory handled in projectileHandler
	static CExpGenSpawnable* CreateSpawnable(int spawnableID);
	static TypedRenderBuffer<VA_TYPE_PROJ>& GetPrimaryRenderBuffer();
protected:
	struct EffectAuxParams {
		const float3* ap = nullptr;
		float p = 0.0f;
		const float2* distMag = nullptr;
		const float4* distPar = nullptr;
	};

	CExpGenSpawnable();

	//update in Draw() of CGroundFlash or CProjectile
	void UpdateRotation();
	virtual bool UpdateAnimParams() = 0;

	void UpdateAnimParamsImpl(const float3& ap, float& p) const;

	template <uint32_t texIdx>
	void AddEffectsQuad(uint32_t pageNum, const VA_TYPE_TC& tl, const VA_TYPE_TC& tr, const VA_TYPE_TC& br, const VA_TYPE_TC& bl) const;

	static void AddEffectsQuadImpl(uint32_t pageNum, const VA_TYPE_TC& tl, const VA_TYPE_TC& tr, const VA_TYPE_TC& br, const VA_TYPE_TC& bl, const EffectAuxParams& aux);

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

	// anim params for 4 textures (max) in an effect
	float3 animParams1 = { 1.0f, 1.0f, 30.0f }; // numX, numY, animLength, 
	float animProgress1 = 0.0f; // animProgress = (gf_dt % animLength) / animLength
	float3 animParams2 = { 1.0f, 1.0f, 30.0f }; // numX, numY, animLength, 
	float animProgress2 = 0.0f; // animProgress = (gf_dt % animLength) / animLength
	float3 animParams3 = { 1.0f, 1.0f, 30.0f }; // numX, numY, animLength, 
	float animProgress3 = 0.0f; // animProgress = (gf_dt % animLength) / animLength
	float3 animParams4 = { 1.0f, 1.0f, 30.0f }; // numX, numY, animLength, 
	float animProgress4 = 0.0f; // animProgress = (gf_dt % animLength) / animLength

	float3 rotParams = { 0.0f, 0.0f, 0.0f }; // speed, accel, startRot |deg/s, deg/s2, deg|

	float rotVal;
	float rotVel;

	int createFrame;

	float2 distMag = { 0.0f, 0.0f };
	float4 distPar = { 0.0f, 0.0f, 0.0f, 0.0f };

	static std::array<SpawnableTuple, 15> spawnables;
};

template <>
inline void CExpGenSpawnable::AddEffectsQuad<0>(uint32_t pageNum, const VA_TYPE_TC& tl, const VA_TYPE_TC& tr, const VA_TYPE_TC& br, const VA_TYPE_TC& bl) const {
	// no animation
	AddEffectsQuadImpl(pageNum, tl, tr, br, bl, EffectAuxParams{ .ap = nullptr, .p = 0.0f, .distMag = &distMag, .distPar = &distPar });
}

template <>
inline void CExpGenSpawnable::AddEffectsQuad<1>(uint32_t pageNum, const VA_TYPE_TC& tl, const VA_TYPE_TC& tr, const VA_TYPE_TC& br, const VA_TYPE_TC& bl) const {
	AddEffectsQuadImpl(pageNum, tl, tr, br, bl, EffectAuxParams{ .ap = &animParams1, .p = animProgress1, .distMag = &distMag, .distPar = &distPar });
}

template <>
inline void CExpGenSpawnable::AddEffectsQuad<2>(uint32_t pageNum, const VA_TYPE_TC& tl, const VA_TYPE_TC& tr, const VA_TYPE_TC& br, const VA_TYPE_TC& bl) const {
	AddEffectsQuadImpl(pageNum, tl, tr, br, bl, EffectAuxParams{ .ap = &animParams2, .p = animProgress2, .distMag = &distMag, .distPar = &distPar });
}

template <>
inline void CExpGenSpawnable::AddEffectsQuad<3>(uint32_t pageNum, const VA_TYPE_TC& tl, const VA_TYPE_TC& tr, const VA_TYPE_TC& br, const VA_TYPE_TC& bl) const {
	AddEffectsQuadImpl(pageNum, tl, tr, br, bl, EffectAuxParams{ .ap = &animParams3, .p = animProgress3, .distMag = &distMag, .distPar = &distPar });
}

template <>
inline void CExpGenSpawnable::AddEffectsQuad<4>(uint32_t pageNum, const VA_TYPE_TC& tl, const VA_TYPE_TC& tr, const VA_TYPE_TC& br, const VA_TYPE_TC& bl) const {
	AddEffectsQuadImpl(pageNum, tl, tr, br, bl, EffectAuxParams{ .ap = &animParams4, .p = animProgress4, .distMag = &distMag, .distPar = &distPar });
}

#endif //EXP_GEN_SPAWNABLE_H
