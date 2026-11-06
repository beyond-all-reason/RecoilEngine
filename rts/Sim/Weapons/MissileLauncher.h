/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MISSILELAUNCHER_H
#define MISSILELAUNCHER_H

#include <memory>

#include "Weapon.h"
#include "TrajectoryTable.h"

class CMissileLauncher: public CWeapon
{
	CR_DECLARE_DERIVED(CMissileLauncher)
public:
	CMissileLauncher(CUnit* owner = nullptr, const WeaponDef* def = nullptr): CWeapon(owner, def) {}

	static std::unique_ptr<const TrajectoryTable> BuildTrajectoryTable(const WeaponDef& wd);

	void UpdateWantedDir() override final;

private:
	static float3 GetStaticLaunchDir(const WeaponDef* wd, const float3& targetDir);
	static int GetStaticTTL(const WeaponDef* wd, float targetDist, float range, float projectileSpeed);
	static void GetStaticTrajectory(const WeaponDef& wd, float distanceXZ, float rise, std::vector<float3>& path);

	const float3& GetAimFromPos(bool useMuzzle = false) const override { return weaponMuzzlePos; }

	bool HaveFreeLineOfFire(const float3& srcPos, const float3& tgtPos, const SWeaponTarget& trg) const override final;
	void FireImpl(const bool scriptCall) override final;
};


#endif /* MISSILELAUNCHER_H */
