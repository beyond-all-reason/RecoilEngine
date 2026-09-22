/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// The agile flight regime of CStrafeAirMoveType: hover-like control for short legs, the final
// approach, takeoff and landing of strafing aircraft that have agileFlight set (the fixed-wing
// model in StrafeAirMoveType.cpp flies everything else and every attack run)

#include "StrafeAirMoveType.h"

#include "Sim/Misc/GlobalConstants.h"
#include "System/SpringHash.h"

#define MEMBER_LITERAL_HASH(memberName) spring::LiteHash(memberName, sizeof(memberName) - 1, 0)

// not plain assignments: zero derives a default, agileSpeed is given in elmos per second
static const unsigned int AGILE_MEMBER_HASHES[] = {
	MEMBER_LITERAL_HASH(    "agileSpeed"),
	MEMBER_LITERAL_HASH( "agileTurnRate"),
	MEMBER_LITERAL_HASH(  "agileAccRate"),
	MEMBER_LITERAL_HASH("cruiseDistance"),
	MEMBER_LITERAL_HASH( "agileAltitude"),
};


#undef MEMBER_LITERAL_HASH

// agile flight regime: defaults of the tunables a unit does not set
static constexpr float AGILE_SPEED_DEFAULT_SHARE = 0.4f; // of the unit's top speed
static constexpr float AGILE_TURN_RATE_DEFAULT_PER_SECOND = 8100.0f; // heading units (65536 is a full circle): half a turn in about four seconds

// keeps divisions by a rate finite
static constexpr float AGILE_MIN_RATE = 0.0001f;



// for all three, a value that is not positive asks for the default derived from the stock tags
void CStrafeAirMoveType::SetAgileSpeed(float speed)
{
	agileSpeed = (speed > 0.0f)? speed: maxSpeedDef * AGILE_SPEED_DEFAULT_SHARE;
}

void CStrafeAirMoveType::SetAgileAccRate(float rate)
{
	agileAccRate = (rate > 0.0f)? rate: std::max(accRate, AGILE_MIN_RATE);
}

void CStrafeAirMoveType::SetAgileTurnRate(float rate)
{
	// applies at the regime's top speed, proportionally less below it (never on the spot)
	agileTurnRate = (rate > 0.0f)? rate: (AGILE_TURN_RATE_DEFAULT_PER_SECOND / GAME_SPEED);
}


float CStrafeAirMoveType::GetAgileHeight() const
{
	// maneuvering can be given its own (lower) altitude, never above the cruise altitude
	// (orgWantedHeight: wantedHeight is the touchdown height while landing)
	return ((agileAltitude > 0.0f)? std::min(agileAltitude, orgWantedHeight): orgWantedHeight);
}


float CStrafeAirMoveType::GetCruiseDistance() const
{
	if (cruiseDistance > 0.0f)
		return cruiseDistance;

	// a goal nearer than one full turn is not worth lining up on. Braking
	// needs no room here, it is planned from the speed actually reached
	return (GetTurnDiameter());
}


float CStrafeAirMoveType::GetTurnDiameter() const
{
	// the yaw radius is 1 / maxRudder at any speed
	return (2.0f / std::max(maxRudder, AGILE_MIN_RATE));
}


// the agile tunables: zero derives a default, agileSpeed is given in elmos per second
bool CStrafeAirMoveType::SetAgileMemberValue(unsigned int memberHash, void* memberValue)
{
	if (memberHash == AGILE_MEMBER_HASHES[0]) {
		SetAgileSpeed(*(reinterpret_cast<float*>(memberValue)) / GAME_SPEED);
		return true;
	}
	if (memberHash == AGILE_MEMBER_HASHES[1]) {
		SetAgileTurnRate(*(reinterpret_cast<float*>(memberValue)));
		return true;
	}
	if (memberHash == AGILE_MEMBER_HASHES[2]) {
		SetAgileAccRate(*(reinterpret_cast<float*>(memberValue)));
		return true;
	}
	if (memberHash == AGILE_MEMBER_HASHES[3]) {
		cruiseDistance = std::max(0.0f, *(reinterpret_cast<float*>(memberValue)));
		return true;
	}
	if (memberHash == AGILE_MEMBER_HASHES[4]) {
		agileAltitude = std::max(0.0f, *(reinterpret_cast<float*>(memberValue)));
		return true;
	}

	return false;
}
