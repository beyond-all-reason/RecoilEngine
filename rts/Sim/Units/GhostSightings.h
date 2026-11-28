/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <map>
#include <vector>

#include "System/creg/creg_cond.h"
#include "System/float3.h"

class CUnit;

struct GhostSighting {
	CR_DECLARE_STRUCT(GhostSighting)
	float3 pos;
	float3 midPos;
	float3 aimPos;
	int unitID = -1; // Internal link, cleared on detach; only returned for a current contact.
	// A submerged site may require unjammed sonar from the observer.
	int sonarTargetAllyTeam = -1;
	bool sonarDetectable = false;
	bool IsInLos(int allyTeam) const;
};

class GhostSightings {
	CR_DECLARE_STRUCT(GhostSightings)
public:
	using Key = float; // Lua numbers and command parameters use this same representation.
	void Init(int allyTeams);
	void EnterLos(int allyTeam, int unitID);
	void LeaveLos(int allyTeam, const CUnit& unit);
	void RemoveUnit(CUnit& unit, bool leaveDeadGhost);
	bool ExpireVisibleSites();

	Key LiveKey(int allyTeam, int unitID) const;
	const GhostSighting* Find(int allyTeam, Key key) const;
	const auto& ForAllyTeam(int allyTeam) const { return observers[allyTeam].sightings; }

private:
	struct Observer {
		CR_DECLARE_STRUCT(Observer)
		// liveUnits indexes sightings still associated with a unit.
		std::map<Key, GhostSighting> sightings;
		std::map<int, Key> liveUnits;
		Key nextKey = 1;
	};
	std::vector<Observer> observers;
};

extern GhostSightings ghostSightings;
