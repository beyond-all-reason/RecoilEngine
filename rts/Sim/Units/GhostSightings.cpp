/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "GhostSightings.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/creg/STL_Map.h"

GhostSightings ghostSightings;

CR_BIND(GhostSighting, )
CR_REG_METADATA(GhostSighting, (CR_MEMBER(pos), CR_MEMBER(midPos), CR_MEMBER(aimPos), CR_MEMBER(unitID), CR_MEMBER(sonarTargetAllyTeam), CR_MEMBER(sonarDetectable)))
CR_BIND(GhostSightings::Observer, )
CR_REG_METADATA(GhostSightings::Observer, (CR_MEMBER(sightings), CR_MEMBER(liveUnits), CR_MEMBER(nextKey)))
CR_BIND(GhostSightings, )
CR_REG_METADATA(GhostSightings, (CR_MEMBER(observers)))

bool GhostSighting::IsInLos(int allyTeam) const
{
	if (!losHandler->InLos(pos, allyTeam))
		return false;
	if (sonarTargetAllyTeam < 0 || losHandler->GetGlobalLOS(allyTeam))
		return true;
	return sonarDetectable && losHandler->InSonar(pos, allyTeam, sonarTargetAllyTeam);
}

void GhostSightings::Init(int allyTeams)
{
	observers.clear();
	observers.resize(allyTeams);
	// Keep the lower object-ID space available for units, features, projectiles and future types.
	constexpr int firstGhostID = 1 << 20;
	static_assert(MAX_UNITS + MAX_FEATURES + MAX_PROJECTILES < firstGhostID);
	for (int at = 0; at < allyTeams; ++at)
		observers[at].nextKey = firstGhostID + at;
}

void GhostSightings::EnterLos(int allyTeam, int unitID)
{
	auto& observer = observers[allyTeam];
	if (const auto live = observer.liveUnits.find(unitID); live != observer.liveUnits.end()) {
		observer.sightings.erase(live->second);
		observer.liveUnits.erase(live);
	}
}

void GhostSightings::LeaveLos(int allyTeam, const CUnit& unit)
{
	auto& observer = observers[allyTeam];
	// Interleaved observer-local sequences reveal no other allyteam's activity.
	const Key key = observer.nextKey;
	if (std::isfinite(key) && observer.liveUnits.try_emplace(unit.id, key).second) {
		GhostSighting ghost{unit.pos, unit.midPos, unit.aimPos, unit.id};
		if (modInfo.requireSonarUnderWater && unit.IsUnderWater() && !unit.useAirLos) {
			ghost.sonarTargetAllyTeam = unit.allyteam;
			ghost.sonarDetectable = unit.CanBeDetectedBySonar();
		}
		observer.sightings.emplace(key, ghost);
		// Each observer advances by the same number of representable integer IDs.
		// Above 2^24 a float can skip integers, so adding the stride could alias IDs.
		constexpr Key consecutive = 1u << 24;
		if (key <= consecutive - observers.size()) {
			observer.nextKey += observers.size();
		} else {
			for (size_t i = 0; i < observers.size(); ++i) {
				Key& id = observer.nextKey;
				id = id < consecutive ? id + 1 : std::nextafter(id, std::numeric_limits<Key>::infinity());
			}
		}
	}
}

void GhostSightings::RemoveUnit(CUnit& unit, bool leaveDeadGhost)
{
	for (int at = 0; at < observers.size(); ++at) {
		auto& observer = observers[at];
		auto& status = unit.losStatus[at];
		// A hidden transfer can reset the unit's LOS history. The stored sighting
		// remains the observer's record until it sees the site or the removal.
		const bool keep = leaveDeadGhost && !(status & (LOS_INLOS | LOS_INRADAR | LOS_CONTRADAR));
		if (keep)
			status &= ~LOS_PREVLOS;

		const auto live = observer.liveUnits.find(unit.id);
		if (live == observer.liveUnits.end())
			continue;
		if (keep)
			observer.sightings.at(live->second).unitID = -1;
		else
			observer.sightings.erase(live->second);
		observer.liveUnits.erase(live);
	}
}

bool GhostSightings::ExpireVisibleSites()
{
	bool expired = false;
	for (int at = 0; at < observers.size(); ++at) {
		auto& observer = observers[at];
		const auto removed = std::erase_if(observer.sightings, [&](const auto& entry) {
			const auto& ghost = entry.second;
			if (!ghost.IsInLos(at))
				return false;
			if (ghost.unitID >= 0) {
				// A current radar contact at the remembered site still confirms the target.
				const CUnit* unit = unitHandler.GetUnit(ghost.unitID);
				if (unit != nullptr && (unit->losStatus[at] & LOS_INRADAR) && unit->pos.SqDistance2D(ghost.pos) == 0.0f)
					return false;
				observer.liveUnits.erase(ghost.unitID);
			}
			return true;
		});
		expired |= (removed != 0);
	}
	return expired;
}

GhostSightings::Key GhostSightings::LiveKey(int allyTeam, int unitID) const
{
	const auto& live = observers[allyTeam].liveUnits;
	const auto it = live.find(unitID);
	return it == live.end() ? 0 : it->second;
}

const GhostSighting* GhostSightings::Find(int allyTeam, Key key) const
{
	const auto& sightings = ForAllyTeam(allyTeam);
	const auto it = sightings.find(key);
	return it == sightings.end() ? nullptr : &it->second;
}
