/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "NanoProjectile.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Colors.h"
#include "Rendering/GlobalRendering.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/ExpGenSpawnableMemberInfo.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CNanoProjectile, CProjectile, )

CR_REG_METADATA(CNanoProjectile,
(
	CR_MEMBER(rotAcc),
	CR_MEMBER(rotVal0x),
	CR_MEMBER(rotVel0x),
	CR_MEMBER(rotAcc0x),
	CR_MEMBER(homingTargetID),
	CR_MEMBER(homingTargetSyncID),
	CR_MEMBER(homingTargetPiece),
	CR_MEMBER(homingSpeedLimitSq),
	CR_MEMBER(homingOffset),
	CR_MEMBER(lastHomingTargetPos),
	CR_MEMBER(groundClampFinalPos),
	CR_MEMBER(groundClampWaypointPos),
	CR_MEMBER(groundClampWaypointFrame),
	CR_MEMBER(groundClampNextFrame),
	CR_IGNORED(nanoParticleLightGeneration),
	CR_MEMBER(nanoParticleLightBuildSpeed),
	CR_MEMBER(updatePhase),
	CR_MEMBER(groundClampActive),
	CR_IGNORED(nanoParticleLightSpawned),
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(deathFrame),
		CR_MEMBER(color),
	CR_MEMBER_ENDFLAG(CM_Config)
))

namespace {
	constexpr int HOMING_RUN_EVERY = 4;
	constexpr int GROUND_CLAMP_RECHECK_HIT = 6;
	constexpr int GROUND_CLAMP_RECHECK_MISS = 12;
	constexpr float NANO_SPEED_VARIATION = 0.14f;
	constexpr float GROUND_CLAMP_MARGIN = 11.0f;
	constexpr float GROUND_CLAMP_SMART_DELTA = 4.0f;
	constexpr float GROUND_CACHE_CELL_SIZE = 16.0f;
	constexpr std::size_t GROUND_CACHE_SLOTS = 1024;
	constexpr std::size_t HOMING_CACHE_SLOTS = 256;
	constexpr std::array<float, 3> GROUND_CLAMP_SHORT_SAMPLES = {0.35f, 0.50f, 0.65f};
	constexpr std::array<float, 8> GROUND_CLAMP_LONG_SAMPLES = {0.12f, 0.22f, 0.35f, 0.50f, 0.65f, 0.78f, 0.90f, 0.96f};

	struct GroundHeightCache {
		std::array<std::uint64_t, GROUND_CACHE_SLOTS> keys = {};
		std::array<std::uint32_t, GROUND_CACHE_SLOTS> stamps = {};
		std::array<float, GROUND_CACHE_SLOTS> heights = {};
		const CReadMap* map = nullptr;
		int lastFrame = -1;

		GroundHeightCache()
		{
			stamps.fill(std::numeric_limits<std::uint32_t>::max());
		}
	};

	float GetGroundYMargin(float x, float z, int frame)
	{
		thread_local GroundHeightCache cache;
		if (cache.map != readMap || frame < cache.lastFrame) {
			cache.stamps.fill(std::numeric_limits<std::uint32_t>::max());
			cache.map = readMap;
		}
		cache.lastFrame = frame;

		const int quantizedX = static_cast<int>(std::floor(x / GROUND_CACHE_CELL_SIZE + 0.5f));
		const int quantizedZ = static_cast<int>(std::floor(z / GROUND_CACHE_CELL_SIZE + 0.5f));
		const std::uint64_t key = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(quantizedX)) << 32u) | static_cast<std::uint32_t>(quantizedZ);
		const std::size_t slot = (static_cast<std::uint32_t>(quantizedX) * 73856093u ^ static_cast<std::uint32_t>(quantizedZ) * 19349663u) & (GROUND_CACHE_SLOTS - 1);
		const std::uint32_t stamp = static_cast<std::uint32_t>(frame);

		if (cache.stamps[slot] == stamp && cache.keys[slot] == key)
			return cache.heights[slot];

		const float height = CGround::GetHeightReal(x, z, false) + GROUND_CLAMP_MARGIN;
		cache.keys[slot] = key;
		cache.stamps[slot] = stamp;
		cache.heights[slot] = height;
		return height;
	}

	struct HomingTargetCacheEntry {
		int frame = -1;
		int targetID = -1;
		std::int64_t targetSyncID = -1;
		int targetPiece = -2;
		float3 pos;
		bool valid = false;
	};

	bool GetHomingTargetPos(int targetID, std::int64_t targetSyncID, int targetPiece, float3& targetPos)
	{
		thread_local std::array<HomingTargetCacheEntry, HOMING_CACHE_SLOTS> cache;
		const std::size_t slot = (static_cast<std::uint32_t>(targetID) * 73856093u ^ static_cast<std::uint32_t>(targetPiece + 2) * 19349663u) & (HOMING_CACHE_SLOTS - 1);
		HomingTargetCacheEntry& entry = cache[slot];
		const int frame = gs->frameNum;

		if (entry.frame == frame && entry.targetID == targetID && entry.targetSyncID == targetSyncID && entry.targetPiece == targetPiece) {
			if (entry.valid)
				targetPos = entry.pos;
			return entry.valid;
		}

		entry.frame = frame;
		entry.targetID = targetID;
		entry.targetSyncID = targetSyncID;
		entry.targetPiece = targetPiece;
		entry.valid = false;

		const CUnit* target = unitHandler.GetUnit(targetID);
		if (target == nullptr || target->GetSyncID() != targetSyncID || target->isDead || target->IsCrashing())
			return false;

		if (targetPiece >= 0) {
			if (!target->localModel.Initialized() || !target->localModel.HasPiece(targetPiece))
				return false;

			entry.pos = target->GetObjectSpacePos(target->localModel.GetRawPiecePos(targetPiece));
		} else {
			entry.pos = target->midPos;
		}

		entry.valid = true;
		targetPos = entry.pos;
		return true;
	}
}


CNanoProjectile::CNanoProjectile()
{
	RECOIL_DETAILED_TRACY_ZONE;
	deathFrame = 0;
	color[0] = color[1] = color[2] = color[3] = 255;

	checkCol = false;
	drawSorted = false;
}

CNanoProjectile::CNanoProjectile(float3 pos, float3 speed, int lifeTime, SColor c, float builderBuildSpeed, const CUnit* homingTarget, int targetPiece, bool inverse)
	: CProjectile(pos, speed, nullptr, false, false, false)
	, deathFrame(gs->frameNum + lifeTime)
	, color(c)
{
	RECOIL_DETAILED_TRACY_ZONE;
	checkCol = false;
	drawSorted = false;
	drawRadius = 3;

	projectileHandler.currentNanoParticles += 1;

	rotVal0x = rotValRng0 * (guRNG.NextFloat() * 2.0 - 1.0);
	rotVel0x = rotVelRng0 * (guRNG.NextFloat() * 2.0 - 1.0);
	rotAcc0x = rotAccRng0 * (guRNG.NextFloat() * 2.0 - 1.0);

	rotVal = rotVal0 + rotVal0x;
	rotVel = rotVel0 + rotVel0x;
	rotAcc = rotAcc0 + rotAcc0x;

	if (projectileDrawer != nullptr && projectileDrawer->UseNanoParticleShader() && lifeTime > 0 && this->speed.w > 0.0f) {
		const float3 endPos = pos + static_cast<float3>(this->speed) * lifeTime;
		const float speedMult = 1.0f + NANO_SPEED_VARIATION * (guRNG.NextFloat() * 2.0f - 1.0f);
		lifeTime = std::max(1, static_cast<int>(std::ceil(lifeTime / speedMult)));
		SetVelocityAndSpeed((endPos - pos) / lifeTime);
		deathFrame = gs->frameNum + lifeTime;
	}

	updatePhase = static_cast<uint8_t>(id);

	if (projectileHandler.nanoParticlesHoming && homingTarget != nullptr && (inverse || !homingTarget->beingBuilt))
		InitHoming(homingTarget, targetPiece, lifeTime);

	if (projectileHandler.nanoParticlesGroundClamp)
		InitGroundClamp(inverse, lifeTime);

	nanoParticleLightBuildSpeed = builderBuildSpeed;
	nanoParticleLightGeneration = projectileHandler.nanoParticleLightGeneration;
	if (projectileHandler.NanoParticleUpdatesEnabled()) {
		QueueNanoParticleUpdateEvent(NanoParticleEventType::Spawn);
		nanoParticleLightSpawned = true;
	}
}

CNanoProjectile::~CNanoProjectile()
{
	RECOIL_DETAILED_TRACY_ZONE;
	projectileHandler.currentNanoParticles -= 1;
}

void CNanoProjectile::Update()
{
	ZoneScopedN("NanoParticles::Update");
	RECOIL_DETAILED_TRACY_ZONE;
	const int frame = gs->frameNum;
	const float3 oldPos = pos;
	const float3 oldVelocity = speed;
	bool reaimed = false;

	if (groundClampActive)
		reaimed = UpdateGroundClamp(frame);

	if (!reaimed && groundClampWaypointFrame < 0 && homingTargetID >= 0 && projectileHandler.nanoParticlesHoming && ((frame + (updatePhase % HOMING_RUN_EVERY)) % HOMING_RUN_EVERY) == 0)
		UpdateHoming(frame);

	const bool trajectoryChanged =
		pos.x != oldPos.x || pos.y != oldPos.y || pos.z != oldPos.z ||
		speed.x != oldVelocity.x || speed.y != oldVelocity.y || speed.z != oldVelocity.z;

	pos += speed;

	if (nanoParticleLightGeneration != projectileHandler.nanoParticleLightGeneration) {
		nanoParticleLightGeneration = projectileHandler.nanoParticleLightGeneration;
		nanoParticleLightSpawned = false;
	}
	if (projectileHandler.NanoParticleUpdatesEnabled()) {
		if (!nanoParticleLightSpawned) {
			QueueNanoParticleUpdateEvent(NanoParticleEventType::Spawn);
			nanoParticleLightSpawned = true;
		} else if (trajectoryChanged) {
			QueueNanoParticleUpdateEvent(NanoParticleEventType::Update);
		}
	}

	deleteMe |= (frame >= deathFrame);
}

void CNanoProjectile::QueueNanoParticleUpdateEvent(NanoParticleEventType type)
{
	NanoParticleEvent event;
	event.type = type;
	event.lightID = id;
	event.projectileID = id;
	event.projectileSyncID = GetSyncID();
	event.pos = pos;
	event.velocity = speed;
	event.remainingLife = std::max(deathFrame - gs->frameNum, 0);
	event.color = float3(color[0], color[1], color[2]) * (1.0f / 255.0f);
	event.builderBuildSpeed = nanoParticleLightBuildSpeed;
	projectileHandler.QueueNanoParticleUpdateEvent(std::move(event));
}

void CNanoProjectile::InitHoming(const CUnit* target, int targetPiece, int lifeTime)
{
	ZoneScopedN("NanoParticles::Homing");
	const float initialSpeed = speed.w;
	homingTargetID = target->id;
	homingTargetSyncID = target->GetSyncID();
	homingTargetPiece = targetPiece;

	if (targetPiece >= 0 && (!target->localModel.Initialized() || !target->localModel.HasPiece(targetPiece))) {
		homingTargetID = -1;
		return;
	}

	float3 targetPos = (targetPiece >= 0)
		? target->GetObjectSpacePos(target->localModel.GetRawPiecePos(targetPiece))
		: static_cast<float3>(target->midPos);

	if (homingTargetPiece < 0) {
		const float3 initialEndPos = pos + static_cast<float3>(speed) * lifeTime;
		homingOffset = initialEndPos - targetPos;
		targetPos += homingOffset;
	}

	lastHomingTargetPos = targetPos;
	if (homingTargetPiece < 0) {
		const float speedMultiplier = (target->unitDef != nullptr && target->unitDef->canfly) ? 1.35f : 2.0f;
		const float maxHomingSpeed = std::max(initialSpeed * speedMultiplier, 0.1f);
		homingSpeedLimitSq = maxHomingSpeed * maxHomingSpeed;
	}
	Reaim(targetPos, lifeTime);
}

void CNanoProjectile::InitGroundClamp(bool inverse, int lifeTime)
{
	ZoneScopedN("NanoParticles::GroundClamp");
	if (lifeTime <= 1)
		return;

	groundClampFinalPos = pos + static_cast<float3>(speed) * lifeTime;
	const float3 path = groundClampFinalPos - pos;
	const float horizontalLengthSq = path.x * path.x + path.z * path.z;
	const bool longPath = (horizontalLengthSq > 4096.0f);
	const std::size_t sampleCount = longPath ? GROUND_CLAMP_LONG_SAMPLES.size() : GROUND_CLAMP_SHORT_SAMPLES.size();

	float guideY = -1.0e9f;
	float maxPenetration = -1.0e9f;
	float peakT = 0.5f;
	for (std::size_t sample = 0; sample < sampleCount; ++sample) {
		const float t = longPath ? GROUND_CLAMP_LONG_SAMPLES[sample] : GROUND_CLAMP_SHORT_SAMPLES[sample];
		const float3 samplePos = pos + path * t;
		const float groundY = GetGroundYMargin(samplePos.x, samplePos.z, gs->frameNum);
		guideY = std::max(guideY, groundY);

		const float penetration = groundY - samplePos.y;
		if (penetration > maxPenetration) {
			maxPenetration = penetration;
			peakT = t;
		}
	}

	if (maxPenetration <= GROUND_CLAMP_SMART_DELTA)
		return;

	groundClampActive = true;
	groundClampNextFrame = createFrame + (updatePhase % GROUND_CLAMP_RECHECK_HIT);
	groundClampFinalPos.y = std::max(
		groundClampFinalPos.y,
		GetGroundYMargin(groundClampFinalPos.x, groundClampFinalPos.z, gs->frameNum)
	);

	if (inverse)
		return;

	peakT = std::clamp(peakT, 0.15f, 0.85f);
	const int firstLegFrames = std::clamp(static_cast<int>(lifeTime * peakT), 1, lifeTime - 1);
	groundClampWaypointPos = pos + (groundClampFinalPos - pos) * peakT;
	groundClampWaypointPos.y = std::max(groundClampWaypointPos.y, guideY);
	groundClampWaypointFrame = createFrame + firstLegFrames;
	Reaim(groundClampWaypointPos, firstLegFrames);
}

bool CNanoProjectile::ResolveHomingTarget(float3& targetPos)
{
	if (homingTargetID < 0)
		return false;

	// Unit deletion/movement and script GameFrame animation finish before the
	// multithreaded unsynced projectile pass; script Tick resumes afterward.
	// The unit table and local-model transforms are read-only in this phase.
	if (!GetHomingTargetPos(homingTargetID, homingTargetSyncID, homingTargetPiece, targetPos)) {
		homingTargetID = -1;
		return false;
	}

	if (homingTargetPiece < 0)
		targetPos += homingOffset;

	return true;
}

bool CNanoProjectile::UpdateGroundClamp(int frame)
{
	ZoneScopedN("NanoParticles::GroundClamp");
	bool reaimed = false;
	const int remainingLife = deathFrame - frame;
	if (remainingLife <= 0)
		return false;

	if (groundClampWaypointFrame >= 0 && frame >= groundClampWaypointFrame) {
		groundClampWaypointFrame = -1;
		float3 targetPos = groundClampFinalPos;
		if (projectileHandler.nanoParticlesHoming && ResolveHomingTarget(targetPos))
			lastHomingTargetPos = targetPos;

		targetPos.y = std::max(targetPos.y, GetGroundYMargin(targetPos.x, targetPos.z, frame));
		Reaim(targetPos, remainingLife);
		reaimed = true;
	}

	if (!projectileHandler.nanoParticlesGroundClamp || frame < groundClampNextFrame)
		return reaimed;

	const float groundY = GetGroundYMargin(pos.x, pos.z, frame);
	if (pos.y >= groundY) {
		groundClampNextFrame = frame + GROUND_CLAMP_RECHECK_MISS;
		return reaimed;
	}

	pos.y = groundY;
	float3 targetPos = (groundClampWaypointFrame >= 0) ? groundClampWaypointPos : groundClampFinalPos;
	int remainingFrames = (groundClampWaypointFrame >= 0) ? (groundClampWaypointFrame - frame) : remainingLife;
	if (groundClampWaypointFrame < 0 && projectileHandler.nanoParticlesHoming && ResolveHomingTarget(targetPos))
		lastHomingTargetPos = targetPos;

	targetPos.y = std::max(targetPos.y, GetGroundYMargin(targetPos.x, targetPos.z, frame));
	Reaim(targetPos, remainingFrames);
	groundClampNextFrame = frame + GROUND_CLAMP_RECHECK_HIT;
	return true;
}

void CNanoProjectile::UpdateHoming(int frame)
{
	ZoneScopedN("NanoParticles::Homing");
	float3 targetPos;
	if (!ResolveHomingTarget(targetPos))
		return;

	const float3 targetMove = targetPos - lastHomingTargetPos;
	if (targetMove.SqLength() < 1.0f)
		return;

	lastHomingTargetPos = targetPos;
	const int remainingFrames = deathFrame - frame;
	if (remainingFrames <= 1)
		return;

	const float3 newVelocity = (targetPos - pos) / remainingFrames;
	if (homingSpeedLimitSq > 0.0f && newVelocity.SqLength() > homingSpeedLimitSq) {
		homingTargetID = -1;
		return;
	}

	SetVelocityAndSpeed(newVelocity);
}

void CNanoProjectile::Reaim(const float3& targetPos, int remainingFrames)
{
	if (remainingFrames <= 0)
		return;

	SetVelocityAndSpeed((targetPos - pos) / remainingFrames);
}

void CNanoProjectile::Draw()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (projectileDrawer->DrawNanoParticle(drawPos, speed, createFrame, deathFrame, color))
		return;

	{
		const float t = (gs->frameNum - createFrame + globalRendering->timeOffset);
		// rotParams.y is acceleration in angle per frame^2
		rotVel = rotVel0 + rotAcc * t;
		rotVal = rotVal0 + rotVel * t;
	}

	const float3 ri = camera->GetRight() * drawRadius;
	const float3 up = camera->GetUp() * drawRadius;
	std::array<float3, 4> bounds = {
		-ri - up,
		 ri - up,
		 ri + up,
		-ri + up
	};

	if (math::fabs(rotVal) > 0.01f) {
		float3::rotate<false>(rotVal, camera->GetForward(), bounds);
	}

	const auto* gfxt = projectileDrawer->gfxtex;
	AddEffectsQuad<0>(
		gfxt->pageNum,
		{ drawPos + bounds[0], gfxt->xstart, gfxt->ystart, color },
		{ drawPos + bounds[1], gfxt->xend  , gfxt->ystart, color },
		{ drawPos + bounds[2], gfxt->xend  , gfxt->yend  , color },
		{ drawPos + bounds[3], gfxt->xstart, gfxt->yend  , color }
	);
}

void CNanoProjectile::DrawOnMinimap() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	AddMiniMapVertices({ pos        , color4::green }, { pos + speed, color4::green });
}

float CNanoProjectile::GetDrawRadius() const
{
	// The shader halo extends well beyond the 3-elmo legacy billboard.
	return projectileDrawer != nullptr && projectileDrawer->UseNanoParticleShader() ? 22.0f : CProjectile::GetDrawRadius();
}

int CNanoProjectile::GetProjectilesCount() const
{
	return 0; // nano particles use their own counter
}


bool CNanoProjectile::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (CProjectile::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_INT   (CNanoProjectile, deathFrame);
	CHECK_MEMBER_INFO_SCOLOR(CNanoProjectile, color     );

	return false;
}
