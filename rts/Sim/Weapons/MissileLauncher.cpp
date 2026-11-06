/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MissileLauncher.h"

#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/MissileProjectile.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/SpringMath.h"

#include "Rendering/GlobalRendering.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CMissileLauncher, CWeapon, )
CR_REG_METADATA(CMissileLauncher, )

float3 CMissileLauncher::GetStaticLaunchDir(const WeaponDef* wd, const float3& targetDir)
{
	return ((targetDir + UpVector * wd->trajectoryHeight).Normalize());
}

int CMissileLauncher::GetStaticTTL(const WeaponDef* wd, float targetDist, float range, float projectileSpeed)
{
	const float pathScale = 1.0f + 0.25f * std::max(0.0f, wd->trajectoryHeight);
	return math::ceil(std::max(targetDist, range) * pathScale / projectileSpeed + 25 * wd->selfExplode);
}

// path of the weaponDef's missile to a stationary target, without wobble or dance
void CMissileLauncher::GetStaticTrajectory(const WeaponDef& wd, float distanceXZ, float rise, std::vector<float3>& path)
{
	const float3 tgtPos = float3(distanceXZ, rise, 0.0f);
	const float maxSpeed = wd.projectilespeed;
	const float dist = tgtPos.Length();

	float3 pos;
	float3 dir = GetStaticLaunchDir(&wd, tgtPos / dist);
	float speed = wd.startvelocity;

	float extraHeight = 0.0f;
	int extraHeightTime = 0;
	float extraHeightDecay = 0.0f;

	CMissileProjectile::InitExtraHeight(&wd, dist, maxSpeed, extraHeight, extraHeightTime, extraHeightDecay);

	int ttl = (wd.flighttime == 0)? GetStaticTTL(&wd, dist, wd.range, maxSpeed): wd.flighttime;

	while (--ttl > 0) {
		speed += (wd.weaponacceleration * (speed < maxSpeed));
		dir = CMissileProjectile::StepSteering(&wd, pos, dir, tgtPos, ZeroVector, maxSpeed, extraHeight, extraHeightTime, extraHeightDecay);
		pos += (dir * speed);
		path.push_back(pos);

		if (pos.SqDistance(tgtPos) < Square(speed) || pos.x > distanceXZ)
			return;
	}
}

std::unique_ptr<const TrajectoryTable> CMissileLauncher::BuildTrajectoryTable(const WeaponDef& wd)
{
	if (wd.trajectoryHeight <= 0.0f)
		return nullptr;

	auto table = std::make_unique<TrajectoryTable>();

	table->Build(wd.range, [&wd](float distanceXZ, float rise, std::vector<float3>& path) {
		GetStaticTrajectory(wd, distanceXZ, rise, path);
	});

	return table;
}


void CMissileLauncher::UpdateWantedDir()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CWeapon::UpdateWantedDir();

	if (weaponDef->trajectoryHeight > 0.0f)
		wantedDir = GetStaticLaunchDir(weaponDef, wantedDir);
}

void CMissileLauncher::FireImpl(const bool scriptCall)
{
	RECOIL_DETAILED_TRACY_ZONE;
	float3 targetVec = currentTargetPos - weaponMuzzlePos;
	const float targetDist = targetVec.LengthNormalize();

	if (onlyForward) {
		targetVec = owner->frontdir;
	} else if (weaponDef->fixedLauncher) {
		targetVec = weaponDir;
	} else if (weaponDef->trajectoryHeight > 0.0f) {
		targetVec = GetStaticLaunchDir(weaponDef, targetVec);
	}

	targetVec += (gsRNG.NextVector() * SprayAngleExperience() + SalvoErrorExperience());
	targetVec.Normalize();

	float3 startSpeed = targetVec * weaponDef->startvelocity;

	// NOTE: why only for SAMT units?
	if (onlyForward && owner->unitDef->IsStrafingAirUnit())
		startSpeed += owner->speed;

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.end = currentTargetPos;
	params.speed = startSpeed;
	params.gravity = -weaponDef->myGravity;
	// arcing trajectories are longer than the straight-line distance
	params.ttl = (ttl == 0) ? GetStaticTTL(weaponDef, targetDist, range, projectileSpeed) : ttl;

	WeaponProjectileFactory::LoadProjectile(params);
}

bool CMissileLauncher::HaveFreeLineOfFire(const float3& srcPos, const float3& tgtPos, const SWeaponTarget& trg) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// high-trajectory missiles use curved path rather than linear ground intersection
	if (weaponDef->trajectoryTable == nullptr)
		return (CWeapon::HaveFreeLineOfFire(srcPos, tgtPos, trg));

	float3 targetVec = (tgtPos - srcPos) * XZVector;
	float3 launchDir = (tgtPos - srcPos).SafeNormalize();

	const float xzTargetDist = targetVec.LengthNormalize();

	if (xzTargetDist == 0.0f)
		return true;

	constexpr uint32_t numSteps = TrajectoryTable::NUM_SAMPLES;
	const float rise = tgtPos.y - srcPos.y;

	std::array<float, numSteps + 1> mdist = {}; //distance radially the missile has travelled
	std::array<float, numSteps + 1> mheight = {}; //distance vertically the missile has travelled
	mdist[numSteps] = xzTargetDist;
	mheight[numSteps] = rise;

	if (weaponDef->trajectoryTable->Covers(xzTargetDist, rise)) {
		weaponDef->trajectoryTable->Sample(xzTargetDist, rise, mdist.data(), mheight.data());
	} else {
		std::vector<float3> path(1);

		GetStaticTrajectory(*weaponDef, xzTargetDist, rise, path);
		path.emplace_back(xzTargetDist, rise, 0.0f);

		TrajectoryTable::TraceToSample(path, xzTargetDist, rise, mdist.data(), mheight.data());
	}

	// debug draw
	if (globalRendering->drawDebugTraceRay) {
		for (uint32_t i = 1; i <= numSteps; i++) {
			geometricObjects->SetColor(geometricObjects->AddLine(srcPos + targetVec * mdist[i - 1] + UpVector * mheight[i - 1], srcPos + targetVec * mdist[i] + UpVector * mheight[i], 3, 0, GAME_SPEED), 1.0f, 0.0f, 0.0f, 1.0f);
		}
	}

	// check for ground collision
	// might be better to spin this off into TraceRay.cpp 
	// but the curve (and the piecewise linear approximation used here)
	// that trajectoryheight missiles follow is singularly unique
	// so no need to spin it off until something else needs this
	uint32_t ii = 1;
	float delta1 = mdist[ii] - mdist[ii - 1];
	float delta2 = 0.0f;
	float ratio = 0.0f;
	float hitheight = 0.0f;
	if ((avoidFlags & Collision::NOGROUND) == 0) {
		// do not check last bit of trajectory, sized by damageAreaOfEffect
		// to avoid false positive values at very end of trajectory
		// this mimics CGround::TrajectoryGroundCol called by parabolic cannon shots 
		// GetApproximateHeight should already do map boundary checks
		for (float dd = 0; dd < xzTargetDist - damages->damageAreaOfEffect; dd += SQUARE_SIZE) {
			// make sure we are using correct part of the trajectory
			while (dd > mdist[ii]) {
				ii = ii + 1;
				delta1 = mdist[ii] - mdist[ii - 1];
			}
			delta2 = dd - mdist[ii - 1];
			ratio = delta2 / delta1;
			hitheight = mheight[ii - 1] + ratio * (mheight[ii] - mheight[ii - 1]);
			if (CGround::GetApproximateHeight(srcPos + targetVec*dd) > (srcPos.y + hitheight)) {
				return false;
			}
		}
	}

	// check for object collision
	// might be better to spin this off into TraceRay.cpp 
	// but the curve (and the piecewise linear approximation used here)
	// that trajectoryheight missiles follow is singularly unique
	// so no need to spin it off until something else needs this
	QuadFieldQuery qfQuery;
	quadField.GetQuadsOnRay(qfQuery, srcPos, targetVec, xzTargetDist);

	if (qfQuery.quads->empty())
		return true;

	CollisionQuery cq;

	const bool scanForAllies = ((avoidFlags & Collision::NOFRIENDLIES) == 0);
	const bool scanForNeutrals = ((avoidFlags & Collision::NONEUTRALS) == 0);
	const bool scanForFeatures = ((avoidFlags & Collision::NOFEATURES) == 0);
	for (const int quadIdx : *qfQuery.quads) {
		const CQuadField::Quad& quad = quadField.GetQuad(quadIdx);

		// friendly units in this quad
		if (scanForAllies) {
			for (const CUnit* u : quad.teamUnits[owner->allyteam]) {
				if (u == owner)
					continue;
				if (!u->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
					continue;

				// chord check here
				const CollisionVolume* cv = &u->collisionVolume;
				const float3 cvRelVec = cv->GetWorldSpacePos(u) - srcPos;
				const float  cvRelDst = std::clamp(cvRelVec.dot(targetVec), 0.0f, xzTargetDist);
				const CMatrix44f objTransform = u->GetTransformMatrix(true);
				for (uint32_t i = 1; i <= numSteps; i++) {
					if (cvRelDst < mdist[i]) {
						// find the relevant linear segment
						// interpolate the location
						delta1 = mdist[i] - mdist[i - 1];
						delta2 = cvRelDst - mdist[i - 1];
						ratio = delta2 / delta1;
						hitheight = mheight[i - 1] + ratio*(mheight[i] - mheight[i - 1]);
						const float3 hitPos = srcPos + targetVec * cvRelDst + UpVector * hitheight;
						if (mheight[i] > mheight[i - 1]) {
							// do chord check backwards
							if (CCollisionHandler::DetectHit(u, objTransform, srcPos, hitPos, &cq, true)) {
								return false;
							}
						} else {
							// do chord check forwards
							if (CCollisionHandler::DetectHit(u, objTransform, hitPos, tgtPos, &cq, true)) {
								return false;
							}
							
						}
						break;
					}
				}
			}
		}

		
		// neutral units in this quad
		if (scanForNeutrals) {
			for (const CUnit* u : quad.units) {
				if (!u->IsNeutral())
					continue;
				if (u == owner)
					continue;
				if (!u->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
					continue;

				// chord check here
				const CollisionVolume* cv = &u->collisionVolume;
				const float3 cvRelVec = cv->GetWorldSpacePos(u) - srcPos;
				const float  cvRelDst = std::clamp(cvRelVec.dot(targetVec), 0.0f, xzTargetDist);
				const CMatrix44f objTransform = u->GetTransformMatrix(true);
				for (uint32_t i = 1; i <= numSteps; i++) {
					if (cvRelDst < mdist[i]) {
						// find the relevant linear segment
						// interpolate the location
						delta1 = mdist[i] - mdist[i - 1];
						delta2 = cvRelDst - mdist[i - 1];
						ratio = delta2 / delta1;
						hitheight = mheight[i - 1] + ratio * (mheight[i] - mheight[i - 1]);
						const float3 hitPos = srcPos + targetVec * cvRelDst + UpVector * hitheight;
						if (mheight[i] > mheight[i - 1]) {
							// do chord check backwards
							if (CCollisionHandler::DetectHit(u, objTransform, srcPos, hitPos, &cq, true)) {
								return false;
							}
						}
						else {
							// do chord check forwards
							if (CCollisionHandler::DetectHit(u, objTransform, hitPos, tgtPos, &cq, true)) {
								return false;
							}

						}
						break;
					}
				}
			}
		}

		// features in this quad
		if (scanForFeatures) {
			for (const CFeature* f : quad.features) {
				if (!f->HasCollidableStateBit(CSolidObject::CSTATE_BIT_QUADMAPRAYS))
					continue;

				// chord check here
				const CollisionVolume* cv = &f->collisionVolume;
				const float3 cvRelVec = cv->GetWorldSpacePos(f) - srcPos;
				const float  cvRelDst = std::clamp(cvRelVec.dot(targetVec), 0.0f, xzTargetDist);
				const CMatrix44f objTransform = f->GetTransformMatrix(true);
				for (uint32_t i = 1; i <= numSteps; i++) {
					if (cvRelDst < mdist[i]) {
						// find the relevant linear segment
						// interpolate the location
						delta1 = mdist[i] - mdist[i - 1];
						delta2 = cvRelDst - mdist[i - 1];
						ratio = delta2 / delta1;
						hitheight = mheight[i - 1] + ratio * (mheight[i] - mheight[i - 1]);
						const float3 hitPos = srcPos + targetVec * cvRelDst + UpVector * hitheight;
						if (mheight[i] > mheight[i - 1]) {
							// do chord check backwards
							if (CCollisionHandler::DetectHit(f, objTransform, srcPos, hitPos, &cq, true)) {
								return false;
							}
						}
						else {
							// do chord check forwards
							if (CCollisionHandler::DetectHit(f, objTransform, hitPos, tgtPos, &cq, true)) {
								return false;
							}

						}
						break;
					}
				}
			}
		}
	}

	return true;
}

