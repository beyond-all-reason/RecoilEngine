/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <cmath>
#include <limits>

#include "Projectile.h"
#include "ProjectileHandler.h"
#include "ProjectileMemPool.h"
#include "Game/GlobalUnsynced.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GroundFlash.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/Env/Particles/Classes/NanoProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/Cpp11Compat.hpp"
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"

#include "System/Misc/TracyDefs.h"


// reserve 5% of maxNanoParticles for important stuff such as capture and reclaim other teams' units
#define NORMAL_NANO_PRIO 0.95f
#define HIGH_NANO_PRIO 1.0f

CONFIG(int, MaxParticles).defaultValue(10000).headlessValue(0).minimumValue(0);
CONFIG(int, MaxNanoParticles).defaultValue(2000).headlessValue(0).minimumValue(0);
CONFIG(bool, NanoParticlesHoming).defaultValue(false).safemodeValue(false).headlessValue(false).description("Allow nano particles to follow moving unit targets and builder nano pieces");
CONFIG(bool, NanoParticlesGroundClamp).defaultValue(false).safemodeValue(false).headlessValue(false).description("Route nano particles above intervening terrain");
CONFIG(bool, NanoParticlesReclaimBurst).defaultValue(false).safemodeValue(false).headlessValue(false).description("Emit a nano burst when reclaiming a unit finishes");
CONFIG(float, NanoParticlesRate).defaultValue(0.32f).description("Global build-power-scaled nano particle emission multiplier");
CONFIG(bool, NanoParticleLights).defaultValue(false).deprecated(true).description("Deprecated: use NanoParticlesUpdateLuaUI");
CONFIG(float, NanoParticlesUpdateLuaUISampleRate).defaultValue(0.32f).minimumValue(0.0f).maximumValue(1.0f).description("Fraction multiplier for native nano particles sent to LuaUI (used for deferred lights)");
CONFIG(bool, NanoParticlesUpdateLuaUI).defaultValue(false).safemodeValue(false).headlessValue(false).description("Send batched nano particle lifecycle updates to LuaUI");

namespace {
	constexpr float NANO_PARTICLE_SPEED = 4.0f;
	constexpr float NANO_EMIT_REF_BUILDSPEED = 100.0f;
	constexpr int NANO_FEEDBACK_EMIT_MIN_GAP = 60;
	constexpr int RECLAIM_BURST_BASE = 1;
	constexpr float RECLAIM_BURST_LOG_K = 40.0f;
	constexpr float RECLAIM_BURST_LOG_NORM = 250.0f;
	constexpr float RECLAIM_BURST_BUILDER_EXPONENT = 0.5f;
	constexpr int RECLAIM_BURST_MAX = 1500;
	constexpr double NANO_PARTICLE_UPDATE_HASH_MULTIPLIER = 2654435761.0;
	constexpr double NANO_PARTICLE_UPDATE_HASH_RANGE = 1000000.0;
	constexpr int NANO_PARTICLE_LOS_CACHE_FRAMES = 7;
	constexpr float NANO_PARTICLE_LOS_CACHE_CELL_SIZE = 64.0f;
	constexpr std::size_t NANO_PARTICLE_LOS_CACHE_SLOTS = 1024;

	bool DirectNanoParticleExpiryCompare(const DirectNanoParticleExpiry& lhs, const DirectNanoParticleExpiry& rhs)
	{
		return lhs.deathFrame > rhs.deathFrame;
	}

	struct NanoParticleLosCacheEntry {
		int x = 0;
		int y = 0;
		int z = 0;
		int allyTeam = -1;
		int expiresFrame = -1;
		bool visible = false;
	};

	struct NanoParticleLosCache {
		std::array<NanoParticleLosCacheEntry, NANO_PARTICLE_LOS_CACHE_SLOTS> entries = {};
		int lastFrame = -1;
	};

	bool IsNanoParticleEventVisible(const CProjectile* projectile, int allyTeam)
	{
		thread_local NanoParticleLosCache cache;
		const int frame = gs->frameNum;
		if (frame < cache.lastFrame) {
			for (NanoParticleLosCacheEntry& entry : cache.entries)
				entry.expiresFrame = -1;
		}
		cache.lastFrame = frame;

		const auto quantize = [](float value) {
			return static_cast<int>(std::floor(value / NANO_PARTICLE_LOS_CACHE_CELL_SIZE + 0.5f));
		};
		const int x = quantize(projectile->pos.x);
		const int y = quantize(projectile->pos.y);
		const int z = quantize(projectile->pos.z);
		const std::size_t slot = (
			static_cast<std::uint32_t>(x) * 73856093u ^
			static_cast<std::uint32_t>(y) * 19349663u ^
			static_cast<std::uint32_t>(z) * 83492791u ^
			static_cast<std::uint32_t>(allyTeam) * 2654435761u
		) & (NANO_PARTICLE_LOS_CACHE_SLOTS - 1);

		NanoParticleLosCacheEntry& entry = cache.entries[slot];
		if (entry.expiresFrame > frame && entry.x == x && entry.y == y && entry.z == z && entry.allyTeam == allyTeam)
			return entry.visible;

		entry.x = x;
		entry.y = y;
		entry.z = z;
		entry.allyTeam = allyTeam;
		entry.expiresFrame = frame + NANO_PARTICLE_LOS_CACHE_FRAMES;
		entry.visible = losHandler->InLos(projectile, allyTeam);
		return entry.visible;
	}

	void MigrateNanoParticleConfigNames()
	{
		const auto configData = configHandler->GetDataWithoutDefaults();
		const auto migrate = [&](const char* oldKey, const char* newKey) {
			const auto oldValue = configData.find(oldKey);
			if (oldValue == configData.end())
				return;

			if (configData.find(newKey) == configData.end())
				configHandler->SetString(newKey, oldValue->second);

			configHandler->Delete(oldKey);
		};

		migrate("NanoParticleLights", "NanoParticlesUpdateLuaUI");
		migrate("NanoParticleUpdateLuaUI", "NanoParticlesUpdateLuaUI");
		migrate("NanoParticleRate", "NanoParticlesRate");
		migrate("NanoParticleUpdateLuaUISampleRate", "NanoParticlesUpdateLuaUISampleRate");
	}
}


CR_BIND(CProjectileHandler, )
CR_REG_METADATA(CProjectileHandler, (
	CR_MEMBER(projectiles),
	CR_MEMBER_UN(flyingPieces),
	CR_MEMBER_UN(groundFlashes),
	CR_MEMBER_UN(resortFlyingPieces),

	CR_MEMBER(maxParticles),
	CR_MEMBER(maxNanoParticles),
	CR_MEMBER(currentNanoParticles),
	CR_MEMBER_UN(frameCurrentParticles),
	CR_MEMBER_UN(frameProjectileCounts)
))



// note: stores all ExpGenSpawnable types, not just projectiles
ProjMemPool projMemPool;

CProjectileHandler projectileHandler;



void CProjectileHandler::Init()
{
	RECOIL_DETAILED_TRACY_ZONE;
	currentNanoParticles = 0;
	frameCurrentParticles = 0;
	frameProjectileCounts[false] = 0;
	frameProjectileCounts[ true] = 0;

	resortFlyingPieces.fill(false);

	MigrateNanoParticleConfigNames();

	maxParticles     = configHandler->GetInt("MaxParticles");
	maxNanoParticles = configHandler->GetInt("MaxNanoParticles");
	nanoParticlesHoming = configHandler->GetBool("NanoParticlesHoming");
	nanoParticlesGroundClamp = configHandler->GetBool("NanoParticlesGroundClamp");
	nanoParticlesReclaimBurst = configHandler->GetBool("NanoParticlesReclaimBurst");
	nanoParticlesRate = std::clamp(configHandler->GetFloat("NanoParticlesRate"), 0.0f, 1.0f);
	nanoParticlesUpdateLuaUISampleRate = std::clamp(configHandler->GetFloat("NanoParticlesUpdateLuaUISampleRate"), 0.0f, 1.0f);
	nanoParticleUpdateLuaUI = configHandler->GetBool("NanoParticlesUpdateLuaUI");
	nanoParticleUpdateClientActive = eventHandler.HasNanoParticleUpdateClients();

	projMemPool.clear();
	projMemPool.reserve(1024);

	for (int modelType = 0; modelType < MODELTYPE_CNT; ++modelType) {
		flyingPieces[modelType].clear();
		flyingPieces[modelType].reserve(1000);
	}
	for (auto& events: nanoParticleUpdateThreadEvents) {
		events.clear();
		events.reserve(64);
	}
	nanoParticleUpdateEvents.clear();
	nanoParticleUpdateEvents.reserve(maxNanoParticles);
	directNanoParticles.clear();
	directNanoParticles.reserve(maxNanoParticles);
	directNanoParticleChanges.clear();
	directNanoParticleChanges.reserve(maxNanoParticles);
	directNanoParticleExpiries.clear();
	directNanoParticleExpiries.reserve(maxNanoParticles);
	directNanoParticleIndices.clear();
	directNanoParticleIndices.reserve(maxNanoParticles);
	nextDirectNanoParticleLightID = -1;
	directNanoParticleGeneration = 1;
	directNanoParticleLightGeneration = nanoParticleLightGeneration;

	projectiles[true ].SeedFreeKeys(0, 1 << 14, true); //seed only synced free ids.
	projectiles[false].reserve(static_cast<size_t>(maxParticles) * 2);

	CExpGenSpawnable::InitSpawnables();

	// register ConfigNotify()
	configHandler->NotifyOnChange(this, {"MaxParticles", "MaxNanoParticles", "NanoParticlesHoming", "NanoParticlesGroundClamp", "NanoParticlesReclaimBurst", "NanoParticlesRate", "NanoParticlesUpdateLuaUISampleRate", "NanoParticlesUpdateLuaUI"});
}

void CProjectileHandler::Kill()
{
	RECOIL_DETAILED_TRACY_ZONE;
	configHandler->RemoveObserver(this);

	{
		// synced first, to avoid callback crashes
		for (CProjectile* p: projectiles[true])
			projMemPool.free(p);

		projectiles[true].clear();
	}

	{
		for (CProjectile* p: projectiles[false])
			projMemPool.free(p);

		projectiles[false].clear();
	}

	{
		for (CGroundFlash* gf: groundFlashes)
			projMemPool.free(gf);

		groundFlashes.clear();
	}

	{
		for (auto& fpc: flyingPieces) {
			fpc.clear();
		}
	}
	for (auto& events: nanoParticleUpdateThreadEvents)
		events.clear();
	nanoParticleUpdateEvents.clear();
	directNanoParticles.clear();
	directNanoParticleChanges.clear();
	directNanoParticleExpiries.clear();
	directNanoParticleIndices.clear();
	nanoParticleUpdateClientActive = false;

	CCollisionHandler::PrintStats();
}


void CProjectileHandler::ConfigNotify(const std::string& key, const std::string& value)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const bool oldNanoParticleUpdateLuaUI = nanoParticleUpdateLuaUI;
	const float oldNanoParticlesUpdateLuaUISampleRate = nanoParticlesUpdateLuaUISampleRate;
	maxParticles     = configHandler->GetInt("MaxParticles");
	maxNanoParticles = configHandler->GetInt("MaxNanoParticles");
	nanoParticlesHoming = configHandler->GetBool("NanoParticlesHoming");
	nanoParticlesGroundClamp = configHandler->GetBool("NanoParticlesGroundClamp");
	nanoParticlesReclaimBurst = configHandler->GetBool("NanoParticlesReclaimBurst");
	nanoParticlesRate = std::clamp(configHandler->GetFloat("NanoParticlesRate"), 0.0f, 1.0f);
	nanoParticlesUpdateLuaUISampleRate = std::clamp(configHandler->GetFloat("NanoParticlesUpdateLuaUISampleRate"), 0.0f, 1.0f);
	nanoParticleUpdateLuaUI = configHandler->GetBool("NanoParticlesUpdateLuaUI");
	if (nanoParticleUpdateLuaUI != oldNanoParticleUpdateLuaUI || nanoParticlesUpdateLuaUISampleRate != oldNanoParticlesUpdateLuaUISampleRate) {
		++nanoParticleLightGeneration;
		NanoParticleEvent event;
		event.type = NanoParticleEventType::Reset;
		QueueNanoParticleUpdateEvent(std::move(event));
	}

	projectiles[false].reserve(static_cast<size_t>(maxParticles) * 2);
}


static void MAPPOS_SANITY_CHECK(const float3 v)
{
	RECOIL_DETAILED_TRACY_ZONE;
	v.AssertNaNs();
	assert(v.x >= -(float3::maxxpos * 16.0f));
	assert(v.x <=  (float3::maxxpos * 16.0f));
	assert(v.z >= -(float3::maxzpos * 16.0f));
	assert(v.z <=  (float3::maxzpos * 16.0f));
	assert(v.y >= -MAX_PROJECTILE_HEIGHT);
	assert(v.y <=  MAX_PROJECTILE_HEIGHT);
}

template<bool synced>
void CProjectileHandler::UpdateProjectilesImpl()
{
	SCOPED_TIMER("Sim::Projectiles::Update");

	auto& pc = projectiles[synced];
	// WARNING:
	//   we can't use iterators here because ProjectileCreated
	//   and ProjectileDestroyed events may add new projectiles
	//   to the container!
	for (size_t i = 0; i < pc.size(); /*no-op*/) {
		CProjectile* p = pc[i];

		assert(p != nullptr);
		assert(p->synced == synced);
#ifdef USING_CREG
		assert(p->synced == !!(p->GetClass()->flags & creg::CF_Synced));
#endif

		// (delayed) creation for projectiles added after CheckCollisions()
		if (p->createMe)
			CreateProjectile(p);

		// deletion (FIXME: move outside of loop)
		if (p->deleteMe) {
			DestroyProjectile(p);
			continue;
		}

		// neither
		++i;
	}

	// WARNING: same as above but for p->Update()
	if constexpr (synced) {

		SCOPED_TIMER("Sim::Projectiles::UpdateSyncedST");
		for (size_t i = 0; i < pc.size(); ++i) {
			CProjectile* p = pc[i];
			assert(p != nullptr);

			MAPPOS_SANITY_CHECK(p->pos);
			p->PreUpdate();
			p->Update();
			quadField.MovedProjectile(p);

			MAPPOS_SANITY_CHECK(p->pos);
		}
	}
	else {
		SCOPED_TIMER("Sim::Projectiles::UpdateUnsyncedMT");
		for_mt_chunk(0, pc.size(), [&pc](int i) {
			CProjectile* p = pc[i];
			assert(p != nullptr);

			MAPPOS_SANITY_CHECK(p->pos);
			p->PreUpdate();
			p->Update();
			MAPPOS_SANITY_CHECK(p->pos);
		});
	}
}


template<class T>
static void UPDATE_PTR_CONTAINER(T& cont) {
	if (cont.empty())
		return;

#ifndef NDEBUG
	const size_t origSize = cont.size();
#endif
	size_t size = cont.size();

	for (size_t i = 0; i < size; /*no-op*/) {
		CGroundFlash*& gf = cont[i];

		if (!gf->Update()) {
			projMemPool.free(gf);
			gf = cont[size -= 1];
			continue;
		}

		++i;
	}

	// WARNING:
	//   check if the vector was enlarged while iterating, in
	//   which case we will have missed updating newest items
	assert(cont.size() == origSize);

	cont.erase(cont.begin() + size, cont.end());
}

template<class T>
static void UPDATE_REF_CONTAINER(T& cont) {
	if (cont.empty())
		return;

#ifndef NDEBUG
	const size_t origSize = cont.size();
#endif
	size_t size = cont.size();

	for (size_t i = 0; i < size; /*no-op*/) {
		auto& p = cont[i];

		if (!p.Update()) {
			p = std::move(cont[size -= 1]);
			continue;
		}

		++i;
	}

	// WARNING: see UPDATE_PTR_CONTAINER
	assert(cont.size() == origSize);

	cont.erase(cont.begin() + size, cont.end());
}



void CProjectileHandler::CreateProjectile(CProjectile* p)
{
	RECOIL_DETAILED_TRACY_ZONE;
	p->createMe = false;

	if (p->synced || PH_UNSYNCED_PROJECTILE_EVENTS == 1)
		eventHandler.ProjectileCreated(p, p->GetAllyteamID());

	eventHandler.RenderProjectileCreated(p);
}

void CProjectileHandler::DestroyProjectile(CProjectile* p)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(!p->createMe);

	eventHandler.RenderProjectileDestroyed(p);

	if (p->synced) {
		//modelUniformsStorage.DelObject(p);

		eventHandler.ProjectileDestroyed(p, p->GetAllyteamID());

		projectiles[true].Del(p->id);

		ASSERT_SYNCED(p->pos);
		ASSERT_SYNCED(p->id);
	} else {
	#if (PH_UNSYNCED_PROJECTILE_EVENTS == 1)
		eventHandler.ProjectileDestroyed(p, p->GetAllyteamID());
	#endif
		projectiles[false].Del(p->id);
	}

	projMemPool.free(p);
}

uint32_t CProjectileHandler::UnsyncedRandInt(uint32_t N) { return guRNG.NextInt(N); }
uint32_t CProjectileHandler::SyncedRandInt  (uint32_t N) { return gsRNG.NextInt(N); }

void CProjectileHandler::Update()
{
	{
		SCOPED_TIMER("Sim::Projectiles");
		const bool updateClientActive = eventHandler.HasNanoParticleUpdateClients();
		if (updateClientActive != nanoParticleUpdateClientActive) {
			nanoParticleUpdateClientActive = updateClientActive;
			++nanoParticleLightGeneration;
			NanoParticleEvent event;
			event.type = NanoParticleEventType::Reset;
			QueueNanoParticleUpdateEvent(std::move(event));
		}

		// check if any projectiles have collided since the previous update
		CheckCollisions();
		UpdateProjectiles();
		UpdateDirectNanoParticles();
		DispatchNanoParticleUpdates();

		UPDATE_PTR_CONTAINER(groundFlashes);

		// flying pieces; sort these every now and then
		for (int modelType = 0; modelType < MODELTYPE_CNT; ++modelType) {
			auto& fpc = flyingPieces[modelType];

			UPDATE_REF_CONTAINER(fpc);

			if (resortFlyingPieces[modelType]) {
				std::stable_sort(fpc.begin(), fpc.end());
			}
		}
	}

	// precache part of particles count calculation that else becomes very heavy
	{
		ZoneScopedNC("ProjectileHandler::CountParticles", tracy::Color::Goldenrod);
		frameCurrentParticles = 0;

		for (const CProjectile* p : projectiles[true]) {
			frameCurrentParticles += p->GetProjectilesCount();
		}
		for (const CProjectile* p : projectiles[false]) {
			frameCurrentParticles += p->GetProjectilesCount();
		}

		frameProjectileCounts[true] = projectiles[true].size();
		frameProjectileCounts[false] = projectiles[false].size();
	}
}

void CProjectileHandler::QueueNanoParticleUpdateEvent(NanoParticleEvent&& event)
{
	const int threadNum = ThreadPool::GetThreadNum();
	assert(threadNum >= 0 && threadNum < ThreadPool::MAX_THREADS);
	nanoParticleUpdateThreadEvents[threadNum].emplace_back(std::move(event));
}

void CProjectileHandler::DispatchNanoParticleUpdates()
{
	ZoneScopedN("NanoParticles::Updates");

	size_t eventCount = 0;
	for (const auto& events: nanoParticleUpdateThreadEvents)
		eventCount += events.size();
	if (eventCount == 0)
		return;

	nanoParticleUpdateEvents.clear();
	if (nanoParticleUpdateEvents.capacity() < eventCount)
		nanoParticleUpdateEvents.reserve(eventCount);

	for (auto& events: nanoParticleUpdateThreadEvents) {
		for (NanoParticleEvent& event: events) {
			if (event.type == NanoParticleEventType::Reset) {
			} else if (event.type == NanoParticleEventType::Remove) {
				continue;
			} else if (event.direct) {
				const bool visible = gu->spectatingFullView ||
					(teamHandler.IsValidAllyTeam(event.allyTeam) && teamHandler.Ally(event.allyTeam, gu->myAllyTeam)) ||
					(teamHandler.IsValidAllyTeam(gu->myAllyTeam) &&
						(losHandler->InLos(event.pos, gu->myAllyTeam) || losHandler->InLos(event.pos + event.velocity, gu->myAllyTeam))
					);
				if (!visible)
					continue;
			} else {
				const CProjectile* projectile = GetProjectileByUnsyncedID(event.projectileID);
				if (projectile == nullptr || projectile->GetSyncID() != event.projectileSyncID)
					continue;

				const int projectileAllyTeam = projectile->GetAllyteamID();
				const bool visible = gu->spectatingFullView ||
					(teamHandler.IsValidAllyTeam(projectileAllyTeam) && teamHandler.Ally(projectileAllyTeam, gu->myAllyTeam)) ||
					(teamHandler.IsValidAllyTeam(gu->myAllyTeam) && IsNanoParticleEventVisible(projectile, gu->myAllyTeam));
				if (!visible)
					continue;
				event.pos = projectile->pos;
				event.velocity = projectile->speed;
			}

			nanoParticleUpdateEvents.emplace_back(std::move(event));
		}
		events.clear();
	}

	if (!nanoParticleUpdateEvents.empty())
		eventHandler.NanoParticleUpdate(nanoParticleUpdateEvents);
}

void CProjectileHandler::AddProjectile(CProjectile* p)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// already initialized?
	assert(p->id < 0);
	assert(p->createMe);

	if (p->synced)
		p->id = static_cast<int>(projectiles[true ].Add(p, rngFuncs[true]));
	else
		p->id = static_cast<int>(projectiles[false].Add(p)); //don't bother with shuffling unsynced ids 

	if (p->synced) {
		ASSERT_SYNCED(freeIDs.size());
		ASSERT_SYNCED(p->id);
	}

	CreateProjectile(p);
}




static bool CheckProjectileCollisionFlags(const CProjectile* p, const CUnit* u)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const unsigned int collFlags = p->GetCollisionFlags() * p->weapon;

	// only weapon-projectiles can have non-zero flags
	if (collFlags == 0)
		return true;

	// disregard everything else when this bit is set
	// (ground and feature flags are tested elsewhere)
	if ((collFlags & Collision::NONONTARGETS) != 0)
		return (static_cast<const CWeaponProjectile*>(p)->GetTargetObject() == u);

	if ((collFlags & Collision::NOCLOAKED) != 0 && u->IsCloaked())
		return false;
	if ((collFlags & Collision::NONEUTRALS) != 0 && u->IsNeutral())
		return false;

	if ((collFlags & Collision::NOFIREBASES) != 0) {
		const CUnit* owner = p->owner();
		const CUnit* trans = (owner != nullptr)? owner->GetTransporter(): nullptr;

		// check if the unit being collided with is occupied by p's owner
		if (u == trans && trans->unitDef->isFirePlatform)
			return false;
	}

	if (teamHandler.IsValidAllyTeam(p->GetAllyteamID())) {
		const bool noFriendsBit = ((collFlags & Collision::NOFRIENDLIES) != 0);
		const bool noEnemiesBit = ((collFlags & Collision::NOENEMIES   ) != 0);
		const bool friendlyFire = teamHandler.AlliedAllyTeams(p->GetAllyteamID(), u->allyteam);

		if (noFriendsBit && friendlyFire)
			return false;
		if (noEnemiesBit && !friendlyFire)
			return false;
	}

	return true;
}


void CProjectileHandler::CheckUnitCollisions(
	CProjectile* p,
	std::vector<CUnit*>& tempUnits,
	const float3 ppos0,
	const float3 ppos1
) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (!p->checkCol)
		return;

	CollisionQuery cq;

	for (CUnit* unit: tempUnits) {
		assert(unit != nullptr);

		// if this unit fired this projectile, always ignore
		if (unit == p->owner())
			continue;
		if (!unit->HasCollidableStateBit(CSolidObject::CSTATE_BIT_PROJECTILES))
			continue;

		if (!CheckProjectileCollisionFlags(p, unit))
			continue;

		if (CCollisionHandler::DetectHit(unit, unit->GetTransformMatrix(true), ppos0, ppos1, &cq)) {
			if (cq.GetHitPiece() != nullptr)
				unit->SetLastHitPiece(cq.GetHitPiece(), gs->frameNum, p->synced);

			if (!cq.InsideHit()) {
				p->SetPosition(cq.GetHitPos());
				p->Collision(unit);
				p->SetPosition(ppos0);
			} else {
				p->Collision(unit);
			}

			break;
		}
	}
}

void CProjectileHandler::CheckFeatureCollisions(
	CProjectile* p,
	std::vector<CFeature*>& tempFeatures,
	const float3 ppos0,
	const float3 ppos1
) {
	RECOIL_DETAILED_TRACY_ZONE;
	// already collided with unit?
	if (!p->checkCol)
		return;

	if ((p->GetCollisionFlags() & Collision::NOFEATURES) != 0)
		return;

	CollisionQuery cq;

	for (CFeature* feature: tempFeatures) {
		assert(feature != nullptr);

		if (!feature->HasCollidableStateBit(CSolidObject::CSTATE_BIT_PROJECTILES))
			continue;

		if (CCollisionHandler::DetectHit(feature, feature->GetTransformMatrix(true), ppos0, ppos1, &cq)) {
			if (cq.GetHitPiece() != nullptr)
				feature->SetLastHitPiece(cq.GetHitPiece(), gs->frameNum, p->synced);

			if (!cq.InsideHit()) {
				p->SetPosition(cq.GetHitPos());
				p->Collision(feature);
				p->SetPosition(ppos0);
			} else {
				p->Collision(feature);
			}

			break;
		}
	}
}


void CProjectileHandler::CheckShieldCollisions(
	CProjectile* p,
	std::vector<CPlasmaRepulser*>& tempRepulsers,
	const float3 ppos0,
	const float3 ppos1
) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (!p->checkCol)
		return;
	// skip unsynced and non-weapon projectiles
	if (!p->weapon)
		return;

	CWeaponProjectile* wpro = static_cast<CWeaponProjectile*>(p);
	const WeaponDef* wdef = wpro->GetWeaponDef();

	const unsigned int interceptType = wdef->interceptedByShieldType;
	const unsigned int projAllyTeam = p->GetAllyteamID();

	// bail early
	if (interceptType == 0)
		return;

	CollisionQuery cq;

	for (CPlasmaRepulser* repulser: tempRepulsers) {
		assert(repulser != nullptr);

		if (!repulser->CanIntercept(interceptType, projAllyTeam))
			continue;

		// we sometimes get false inside hits due to the movement of the shield
		// a very hacky solution is to nudge the start of the intersecting ray
		// back (proportional to how far the shield moved last frame) so as to
		// increase its length.
		// it's not 100% accurate so there's a bit of a FIXME here to do a real
		// solution (keep track in the projectile which shields it's in)
		const float3 rpvec  = ppos0 - ppos1;
		const float3 rppos0 = ppos0 + rpvec * repulser->GetDeltaDist();
		const float3 cvpos  = repulser->weaponMuzzlePos - repulser->owner->relMidPos;

		// shield volumes are always spherical, transform directly
		// (CollisionHandler will cancel out the relmidpos offset)
		if (!CCollisionHandler::DetectHit(repulser->owner, &repulser->collisionVolume, CMatrix44f{cvpos}, rppos0, ppos1, &cq))
			continue;

		if (cq.InsideHit() && repulser->IgnoreInteriorHit(wpro))
			continue;

		if (repulser->IncomingProjectile(wpro, cq.GetHitPos()))
			return;
	}
}

void CProjectileHandler::CheckUnitFeatureCollisions(bool synced)
{
	RECOIL_DETAILED_TRACY_ZONE;
	static std::vector<CUnit*> tempUnits;
	static std::vector<CFeature*> tempFeatures;
	static std::vector<CPlasmaRepulser*> tempRepulsers;

	//can't use iterators here, because instructions inside the loop modify projectiles[synced]
	for (size_t i = 0; i < projectiles[synced].size(); ++i) {
		CProjectile* p = projectiles[synced][i];

		if (!p->checkCol) continue;
		if ( p->deleteMe) continue;

		const float3 ppos0 = p->pos;
		const float3 ppos1 = p->pos + p->speed;
		// const float3 ppos1 = p->pos + p->dir * (p->speed.w + p->radius);

		quadField.GetUnitsAndFeaturesColVol(p->pos, p->speed.w + p->radius, tempUnits, tempFeatures, &tempRepulsers);

		CheckShieldCollisions (p, tempRepulsers, ppos0, ppos1); tempRepulsers.clear();
		CheckUnitCollisions   (p, tempUnits    , ppos0, ppos1); tempUnits.clear();
		CheckFeatureCollisions(p, tempFeatures , ppos0, ppos1); tempFeatures.clear();
	}
}

void CProjectileHandler::CheckGroundCollisions(bool synced)
{
	RECOIL_DETAILED_TRACY_ZONE;
	//can't use iterators here, because instructions inside the loop modify projectiles[synced]
	for (size_t i = 0; i < projectiles[synced].size(); ++i) {
		CProjectile* p = projectiles[synced][i];

		if (!p->checkCol)
			continue;

		// NOTE:
		//   if <p> is a MissileProjectile and does not have
		//   selfExplode set, tbis will cause it to never be
		//   removed (!)
		if (p->GetCollisionFlags() & Collision::NOGROUND)
			continue;

		// don't collide with ground yet if last update scheduled a bounce
		if (p->weapon && static_cast<const CWeaponProjectile*>(p)->HasScheduledBounce())
			continue;

		// NOTE:
		//   don't add p->radius to groundHeight, or most (esp. modelled)
		//   projectiles will collide with the ground one or more frames
		//   too early
		const float& px = p->pos.x;
		const float& py = p->pos.y;
		const float& pz = p->pos.z;

		const float gy = CGround::GetHeightReal(px, pz);

		const bool belowGround = (py < gy);
		const bool insideWater = (py <= CGround::GetWaterLevel(px, pz));

		if (!belowGround && (!insideWater || p->ignoreWater))
			continue;

		if likely(belowGround) {
			//ZoneScopedN("CheckGroundCollisions::BG");
			if likely(p->speed.w > 0 && !p->blockPreciseCol) {
				const auto& prePos = p->preFrameTra.t;
				const auto groundDistance = std::clamp(CGround::LineGroundCol(prePos, p->pos, synced), 0.0f, p->speed.w);
				p->SetPosition(prePos + static_cast<float3>(p->speed) * groundDistance / p->speed.w);
			}
			else {
				p->pos.y = gy;
			}
		}

		p->Collision();
	}
}

void CProjectileHandler::CheckCollisions()
{
	SCOPED_TIMER("Sim::Projectiles::Collisions");

	CheckUnitFeatureCollisions(true ); // changes simulation state
	CheckUnitFeatureCollisions(false); // does not change simulation state

	CheckGroundCollisions(true ); // changes simulation state
	CheckGroundCollisions(false); // does not change simulation state
}



void CProjectileHandler::AddFlyingPiece(
	int modelType,
	const S3DModelPiece* piece,
	const CMatrix44f& m,
	const float3 pos,
	const float3 speed,
	const float2 pieceParams,
	const int2 renderParams
) {
	RECOIL_DETAILED_TRACY_ZONE;
	flyingPieces[modelType].emplace_back(piece, m, pos, speed, pieceParams, renderParams);
	resortFlyingPieces[modelType] = true;
}

bool CProjectileHandler::CanUseDirectNanoParticles(bool needsTrajectoryCorrection) const
{
	return !needsTrajectoryCorrection && projectileDrawer != nullptr && projectileDrawer->UseDirectNanoParticles();
}

void CProjectileHandler::AddDirectNanoParticle(const float3& startPos, const float3& initialVelocity, int lifeTime, const SColor& color, int allyTeam, float builderBuildSpeed)
{
	// Preserve the unsynced RNG consumption of CNanoProjectile's legacy rotation.
	guRNG.NextFloat();
	guRNG.NextFloat();
	guRNG.NextFloat();

	const float3 endPos = startPos + initialVelocity * lifeTime;
	const float speedMultiplier = 1.0f + NanoParticle::GL4SpeedVariation * (guRNG.NextFloat() * 2.0f - 1.0f);
	const int adjustedLifeTime = std::max(1, static_cast<int>(std::ceil(lifeTime / speedMultiplier)));
	const float3 velocity = (endPos - startPos) / adjustedLifeTime;

	if (nextDirectNanoParticleLightID == std::numeric_limits<int>::min())
		nextDirectNanoParticleLightID = -1;

	DirectNanoParticle particle;
	particle.startPos = startPos;
	particle.velocity = velocity;
	particle.color = color;
	particle.createFrame = gs->frameNum;
	particle.deathFrame = gs->frameNum + adjustedLifeTime;
	particle.lightID = nextDirectNanoParticleLightID--;
	particle.allyTeam = allyTeam;
	particle.builderBuildSpeed = builderBuildSpeed;
	particle.lightSelected = ShouldSendNanoParticleUpdate(particle.lightID, builderBuildSpeed, velocity.Length());

	directNanoParticleIndices[particle.lightID] = directNanoParticles.size();
	directNanoParticles.emplace_back(particle);
	directNanoParticleChanges.emplace_back(DirectNanoParticleChange{particle, DirectNanoParticleChangeType::Add});
	directNanoParticleExpiries.emplace_back(DirectNanoParticleExpiry{particle.deathFrame, particle.lightID});
	std::push_heap(directNanoParticleExpiries.begin(), directNanoParticleExpiries.end(), DirectNanoParticleExpiryCompare);
	if (++directNanoParticleGeneration == 0)
		directNanoParticleGeneration = 1;
	currentNanoParticles += 1;

	if (NanoParticleUpdatesEnabled() && particle.lightSelected)
		QueueDirectNanoParticleUpdate(directNanoParticles.back());
}

void CProjectileHandler::QueueDirectNanoParticleUpdate(const DirectNanoParticle& particle)
{
	const int frame = gs->frameNum;
	NanoParticleEvent event;
	event.type = NanoParticleEventType::Spawn;
	event.direct = true;
	event.allyTeam = particle.allyTeam;
	event.lightID = particle.lightID;
	event.pos = particle.startPos + particle.velocity * (frame - particle.createFrame);
	event.velocity = particle.velocity;
	event.remainingLife = std::max(particle.deathFrame - frame, 0);
	event.color = float3(particle.color[0], particle.color[1], particle.color[2]) * (1.0f / 255.0f);
	event.builderBuildSpeed = particle.builderBuildSpeed;
	QueueNanoParticleUpdateEvent(std::move(event));
}

void CProjectileHandler::RemoveDirectNanoParticle(std::size_t index)
{
	const DirectNanoParticle particle = directNanoParticles[index];
	const int lightID = particle.lightID;
	directNanoParticleIndices.erase(lightID);

	const std::size_t lastIndex = directNanoParticles.size() - 1;
	if (index != lastIndex) {
		directNanoParticles[index] = directNanoParticles.back();
		directNanoParticleIndices[directNanoParticles[index].lightID] = index;
	}
	directNanoParticles.pop_back();
	directNanoParticleChanges.emplace_back(DirectNanoParticleChange{particle, DirectNanoParticleChangeType::Remove});
	if (++directNanoParticleGeneration == 0)
		directNanoParticleGeneration = 1;
	currentNanoParticles -= 1;
}

void CProjectileHandler::UpdateDirectNanoParticles()
{
	ZoneScopedN("NanoParticles::DirectUpdate");
	if (directNanoParticleLightGeneration != nanoParticleLightGeneration) {
		for (DirectNanoParticle& particle : directNanoParticles) {
			particle.lightSelected = ShouldSendNanoParticleUpdate(particle.lightID, particle.builderBuildSpeed, particle.velocity.Length());
			if (NanoParticleUpdatesEnabled() && particle.lightSelected)
				QueueDirectNanoParticleUpdate(particle);
		}
		directNanoParticleLightGeneration = nanoParticleLightGeneration;
	}

	const int frame = gs->frameNum;
	while (!directNanoParticleExpiries.empty() && directNanoParticleExpiries.front().deathFrame <= frame) {
		std::pop_heap(directNanoParticleExpiries.begin(), directNanoParticleExpiries.end(), DirectNanoParticleExpiryCompare);
		const DirectNanoParticleExpiry expiry = directNanoParticleExpiries.back();
		directNanoParticleExpiries.pop_back();

		const auto indexIt = directNanoParticleIndices.find(expiry.lightID);
		if (indexIt == directNanoParticleIndices.end())
			continue;
		if (directNanoParticles[indexIt->second].deathFrame != expiry.deathFrame)
			continue;

		RemoveDirectNanoParticle(indexIt->second);
	}
}


void CProjectileHandler::AddNanoParticle(
	const float3 startPos,
	const float3 endPos,
	const UnitDef* unitDef,
	int teamNum,
	bool highPriority
) {
	RECOIL_DETAILED_TRACY_ZONE;
	const float priority = mix(NORMAL_NANO_PRIO, HIGH_NANO_PRIO, highPriority);
	const float emitProb = 1.0f - GetNanoParticleSaturation(priority);

	if (emitProb < guRNG.NextFloat())
		return;
	if (!unitDef->showNanoSpray)
		return;

	float3 dif = endPos - startPos;
	const float l = fastmath::apxsqrt2(dif.SqLength());
	const int lifeTime = std::max(1, static_cast<int>(std::ceil(l / NANO_PARTICLE_SPEED)));

	dif /= l;
	dif += (guRNG.NextVector() * 0.15f);

	const     float3 udColor = unitDef->nanoColor;
	constexpr float  udAlpha = 20 / 256.0f; // denom=255 is not constexpr-able

	const     uint8_t* tColor = (teamHandler.Team(teamNum))->color;
	constexpr uint8_t  tAlpha = udAlpha * 256;

	const SColor colors[2] = {
		{udColor.r, udColor.g, udColor.b, udAlpha},
		{tColor[0], tColor[1], tColor[2],  tAlpha},
	};

	const float3 velocity = dif * NANO_PARTICLE_SPEED;
	const int allyTeam = teamHandler.IsValidTeam(teamNum) ? teamHandler.AllyTeam(teamNum) : -1;
	const bool needsGroundClamp = nanoParticlesGroundClamp && CNanoProjectile::NeedsGroundClamp(startPos, velocity, lifeTime);
	if (CanUseDirectNanoParticles(needsGroundClamp)) {
		AddDirectNanoParticle(startPos, velocity, lifeTime, colors[globalRendering->teamNanospray], allyTeam, unitDef->buildSpeed);
		return;
	}

	projMemPool.alloc<CNanoProjectile>(startPos, velocity, lifeTime, colors[globalRendering->teamNanospray], unitDef->buildSpeed);
}

void CProjectileHandler::AddNanoParticle(
	const float3 startPos,
	const float3 endPos,
	const UnitDef* unitDef,
	int teamNum,
	float radius,
	bool inverse,
	bool highPriority,
	const CUnit* homingTarget,
	int homingTargetPiece
) {
	RECOIL_DETAILED_TRACY_ZONE;
	const float priority = mix(NORMAL_NANO_PRIO, HIGH_NANO_PRIO, highPriority);
	const float emitProb = 1.0f - GetNanoParticleSaturation(priority);

	if (emitProb < guRNG.NextFloat())
		return;
	if (!unitDef->showNanoSpray)
		return;

	float3 dif = endPos - startPos;
	const float len = fastmath::apxsqrt2(dif.SqLength());
	const int lifeTime = std::max(1, static_cast<int>(std::ceil(len / NANO_PARTICLE_SPEED)));

	dif /= len;
	dif += (guRNG.NextVector() * (radius / len));

	const     float3 udColor = unitDef->nanoColor;
	constexpr float  udAlpha = 20 / 256.0f;

	const     uint8_t* tColor = (teamHandler.Team(teamNum))->color;
	constexpr uint8_t  tAlpha = udAlpha * 256;

	const SColor colors[2] = {
		{udColor.r, udColor.g, udColor.b, udAlpha},
		{tColor[0], tColor[1], tColor[2],  tAlpha},
	};
	const bool needsHoming = nanoParticlesHoming && homingTarget != nullptr && (inverse || !homingTarget->beingBuilt);
	const float3 directStartPos = inverse ? (startPos + dif * len) : startPos;
	const float3 directVelocity = (inverse ? -dif : dif) * NANO_PARTICLE_SPEED;
	const int allyTeam = teamHandler.IsValidTeam(teamNum) ? teamHandler.AllyTeam(teamNum) : -1;
	const bool needsGroundClamp = nanoParticlesGroundClamp && CNanoProjectile::NeedsGroundClamp(directStartPos, directVelocity, lifeTime);

	if (CanUseDirectNanoParticles(needsHoming || needsGroundClamp)) {
		AddDirectNanoParticle(directStartPos, directVelocity, lifeTime, colors[globalRendering->teamNanospray], allyTeam, unitDef->buildSpeed);
		return;
	}

	if (!inverse) {
		projMemPool.alloc<CNanoProjectile>(startPos, dif * NANO_PARTICLE_SPEED, lifeTime, colors[globalRendering->teamNanospray], unitDef->buildSpeed, homingTarget, homingTargetPiece, false);
	} else {
		projMemPool.alloc<CNanoProjectile>(startPos + dif * len, -dif * NANO_PARTICLE_SPEED, lifeTime, colors[globalRendering->teamNanospray], unitDef->buildSpeed, homingTarget, homingTargetPiece, true);
	}
}

int CProjectileHandler::GetNanoParticleEmitCount(float builderBuildSpeed, float buildPower, float& accumulator, int& lastEmitFrame) const
{
	if (maxNanoParticles <= 0 || nanoParticlesRate <= 0.0f) {
		accumulator = 0.0f;
		return 0;
	}

	const float rate = std::max(0.0f, builderBuildSpeed)
		* std::clamp(buildPower, 0.0f, 1.0f)
		* (nanoParticlesRate / NANO_EMIT_REF_BUILDSPEED);
	const float accumulated = accumulator + rate;
	int emitCount = static_cast<int>(std::floor(accumulated));
	accumulator = accumulated - emitCount;
	if (emitCount == 0 && buildPower > 0.0f && (gs->frameNum - lastEmitFrame) >= NANO_FEEDBACK_EMIT_MIN_GAP) {
		emitCount = 1;
		accumulator -= 1.0f;
	}
	if (emitCount > 0)
		lastEmitFrame = gs->frameNum;

	return emitCount;
}

int CProjectileHandler::GetReclaimCompletionNanoBurstCount(float reclaimedMetal, int contributorCount) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!nanoParticlesReclaimBurst)
		return 0;

	const float scaledMetal = std::max(0.0f, reclaimedMetal);
	const int perBuilder = RECLAIM_BURST_BASE + static_cast<int>(std::floor(RECLAIM_BURST_LOG_K * std::log(1.0f + scaledMetal / RECLAIM_BURST_LOG_NORM) + 0.5f));
	const float contributorScale = std::pow(std::max(1, contributorCount), RECLAIM_BURST_BUILDER_EXPONENT);
	return std::clamp(
		static_cast<int>(std::floor(perBuilder * contributorScale + 0.5f)),
		1,
		RECLAIM_BURST_MAX
	);
}

bool CProjectileHandler::ShouldSendNanoParticleUpdate(int particleID, float builderBuildSpeed, float particleSpeed) const
{
	const float sampleFraction = nanoParticlesRate
		* nanoParticlesUpdateLuaUISampleRate
		* (std::max(0.0f, builderBuildSpeed) / NANO_EMIT_REF_BUILDSPEED)
		* (std::max(0.0f, particleSpeed) / NANO_PARTICLE_SPEED);
	if (sampleFraction <= 0.0f)
		return false;
	if (sampleFraction >= 1.0f)
		return true;

	const double hash = std::fmod(static_cast<double>(static_cast<std::uint32_t>(particleID)) * NANO_PARTICLE_UPDATE_HASH_MULTIPLIER, NANO_PARTICLE_UPDATE_HASH_RANGE);
	return hash < (static_cast<double>(sampleFraction) * NANO_PARTICLE_UPDATE_HASH_RANGE);
}

float CProjectileHandler::GetParticleSaturation(bool randomized) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int curParticles = GetCurrentParticles();

	// use the random mult to weaken the max limit a little
	// so the chance is better spread when being close to the limit
	// i.e. when there are rockets that spam CEGs this gives smaller CEGs still a chance
	const float total = std::max(1.0f, maxParticles * 1.0f);
	const float fract = curParticles / total;
	const float rmult = 1.0f + (int(randomized) * 0.3f * guRNG.NextFloat());

	return (fract * rmult);
}

int CProjectileHandler::GetCurrentParticles() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// use precached part of particles count calculation that else becomes very heavy
	// example where it matters: (in ZK) /cheat /give 20 armraven -> shoot ground
	for (size_t i = frameProjectileCounts[true], e = projectiles[true].size(); i < e; ++i) {
		frameCurrentParticles += projectiles[true][i]->GetProjectilesCount();
	}
	frameProjectileCounts[true ] = projectiles[true ].size();

	for (size_t i = frameProjectileCounts[false], e = projectiles[false].size(); i < e; ++i) {
		frameCurrentParticles += projectiles[false][i]->GetProjectilesCount();
	}
	frameProjectileCounts[false] = projectiles[false].size();

	int partCount = frameCurrentParticles;
	for (const auto& c: flyingPieces) {
		for (const auto& fp: c) {
			partCount += fp.GetDrawCallCount();
		}
	}
	partCount += groundFlashes.size();
	return partCount;
}

