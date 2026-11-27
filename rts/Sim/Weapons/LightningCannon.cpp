/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LightningCannon.h"
#include "PlasmaRepulser.h"
#include "WeaponDef.h"
#include "Game/GameHelper.h"
#include "Game/TraceRay.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"

#include <vector>

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CLightningCannon, CWeapon, )
CR_REG_METADATA(CLightningCannon, (
	CR_MEMBER(color)
))

CLightningCannon::CLightningCannon(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// null happens when loading
	if (def != nullptr)
		color = def->visuals.color;
}


bool CLightningCannon::TestRange(const float3& tgtPos, const SWeaponTarget& trg) const
{
	float3 aimDir = (rangeFromBase == 1) ? tgtPos - float3(owner->pos.x, aimFromPos.y, owner->pos.z) :
					(rangeFromBase == 2) ? tgtPos - owner->aimPos :
										   tgtPos - aimFromPos; // default case

	const float targetDist = aimDir.LengthNormalize();

	if (const auto shapedRange = GetShapedWeaponRange(aimDir, range); targetDist > shapedRange)
		return false;

	// NOTE: mainDir is in unit-space
	return (CheckTargetAngleConstraint(aimDir, owner->GetObjectSpaceVec(mainDir)));
}

void CLightningCannon::FireImpl(const bool scriptCall)
{
	RECOIL_DETAILED_TRACY_ZONE;
	float3 curPos = weaponMuzzlePos;
	float3 curDir = (currentTargetPos - weaponMuzzlePos).SafeNormalize();

	curDir += (gsRNG.NextVector() * SprayAngleExperience() + SalvoErrorExperience());
	curDir.Normalize();

	CUnit* hitUnit = nullptr;
	CFeature* hitFeature = nullptr;
	CPlasmaRepulser* hitShield = nullptr;
	CollisionQuery hitColQuery;

	float maxLength = range;
	if (rangeFromBase == 1) {
		// need to keep the laser from shooting past range as defined by rangeFromBase
		// and also need to stretch it if the muzzle is behind the rangefrom point
		// figure out the length of a ray that starts at weaponMuzzlePos, 
		// and terminates on a sphere centered on the rangefrom point with radius range * rangeMod
		const float3 weaponMuzzlePosOffset = weaponMuzzlePos - float3(owner->pos.x, aimFromPos.y, owner->pos.z);
		const float b = weaponMuzzlePosOffset.dot(curDir);
		const float c = weaponMuzzlePosOffset.dot(weaponMuzzlePosOffset) - maxLength * maxLength;
		if ( (b * b - c) > 0) {
			maxLength = (-b + math::sqrt(b * b - c));
		}
	}
	else if (rangeFromBase == 2) {
		const float3 weaponMuzzlePosOffset = weaponMuzzlePos - owner->aimPos;
		const float b = weaponMuzzlePosOffset.dot(curDir);
		const float c = weaponMuzzlePosOffset.dot(weaponMuzzlePosOffset) - maxLength * maxLength;
		if ( (b * b - c) > 0) {
			maxLength = (-b + math::sqrt(b * b - c));
		}
	}

	const auto shapedRange = GetShapedWeaponRange(curDir, maxLength);

	float boltLength = TraceRay::TraceRay(curPos, curDir, shapedRange, collisionFlags, owner, hitUnit, hitFeature, &hitColQuery);

	if (!weaponDef->waterweapon) {
		// terminate bolt at water surface if necessary
		if ((curDir.y < 0.0f) && ((curPos.y + curDir.y * boltLength) <= 0.0f)) {
			boltLength = curPos.y / -curDir.y;
			hitUnit = nullptr;
			hitFeature = nullptr;
		}
	}

	static std::vector<TraceRay::SShieldDist> hitShields;
	hitShields.clear();
	TraceRay::TraceRayShields(this, curPos, curDir, shapedRange, hitShields);
	for (const TraceRay::SShieldDist& sd: hitShields) {
		if (sd.dist < boltLength && sd.rep->IncomingBeam(this, curPos, curPos + (curDir * sd.dist), 1.0f)) {
			boltLength = sd.dist;
			hitUnit = nullptr;
			hitFeature = nullptr;
			hitShield = sd.rep;
			break;
		}
	}

	if (hitUnit != nullptr)
		hitUnit->SetLastHitPiece(hitColQuery.GetHitPiece(), gs->frameNum);

	assert(1 * (!!hitUnit) + 1 * (!!hitFeature) + 1 * (!!hitShield) <= 1);

	ProjectileParams pparams = GetProjectileParams();
	pparams.pos = curPos;
	pparams.end = curPos + curDir * (boltLength + 10.0f);
	pparams.ttl = weaponDef->beamLaserTTL;

	auto projID = WeaponProjectileFactory::LoadProjectile(pparams);

	const DamageArray& damageArray = damages->GetDynamicDamages(weaponMuzzlePos, currentTargetPos);
	const CExplosionParams params = {
		.pos                  = curPos + curDir * boltLength,
		.dir                  = curDir,
		.damages              = damageArray,
		.weaponDef            = weaponDef,
		.owner                = owner,
		.hitObject            = ExplosionHitObject(hitUnit, hitFeature, hitShield),
		.craterAreaOfEffect   = damages->craterAreaOfEffect,
		.damageAreaOfEffect   = damages->damageAreaOfEffect,
		.edgeEffectiveness    = damages->edgeEffectiveness,
		.explosionSpeed       = damages->explosionSpeed,
		.gfxMod               = 0.5f,
		.maxGroundDeformation = 0.0f,
		.impactOnly           = weaponDef->impactOnly,
		.ignoreOwner          = weaponDef->noExplode || weaponDef->noSelfDamage,
		.damageGround         = false,
		.projectileID         = projID
	};

	helper->Explosion(params);
}

