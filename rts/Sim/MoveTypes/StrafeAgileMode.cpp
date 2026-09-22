/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// The agile flight regime of CStrafeAirMoveType: hover-like control for short legs, the final
// approach, takeoff and landing of strafing aircraft that have agileFlight set (the fixed-wing
// model in StrafeAirMoveType.cpp flies everything else and every attack run)

#include "StrafeAirMoveType.h"

#include "Map/Ground.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Units/Unit.h"
#include "System/SpringMath.h"
#include "System/SpringHash.h"

#include "System/Misc/TracyDefs.h"

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

extern AAirMoveType::GetGroundHeightFunc amtGetGroundHeightFuncs[6];


// agile flight regime: defaults of the tunables a unit does not set
static constexpr float AGILE_SPEED_DEFAULT_SHARE = 0.4f; // of the unit's top speed
static constexpr float AGILE_TURN_RATE_DEFAULT_PER_SECOND = 8100.0f; // heading units (65536 is a full circle): half a turn in about four seconds

// keeps divisions by a rate finite
static constexpr float AGILE_MIN_RATE = 0.0001f;
// distances, speeds and vector lengths below this count as zero
static constexpr float AGILE_EPSILON = 0.01f;

// share of its acceleration limit the agile regime plans its braking with
static constexpr float GOVERNOR_ACC_SHARE = 0.8f;

// an agile move order is finished within the unit's radius of the goal, but never less than this
static constexpr float AGILE_MIN_GOAL_RADIUS = 2.0f * SQUARE_SIZE;
// altitude is held against the terrain this much travel time ahead
static constexpr float AGILE_TERRAIN_LOOKAHEAD_SECONDS = 2.0f / 3.0f;
// attitude of the agile regime (cosmetic): share of the difference to the wanted attitude still left after a second
static constexpr float ATTITUDE_KEPT_PER_SECOND = 0.0424f;
static constexpr float AGILE_NOSE_DIP = 0.12f; // frontdir.y at full forward speed
static constexpr float AGILE_MAX_BANK = 0.9f;
static constexpr float BANK_STRAFE_SHARE = 0.7f; // of the bank comes from strafing sideways, the rest from turning
static constexpr float BANK_FULL_TURN_SECONDS = 4.0f / 15.0f; // full turn bank while the nose has this much turning left


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


// speed from which a constant per-frame deceleration comes to rest within <dist>
// (the positive root of v * v + acc * v = 2 * acc * dist). Plans with a share of <acc>,
// a controller that is limited to the very deceleration its braking curve assumes
// can never get back onto the curve once above it
static float GovernedSpeed(float dist, float acc)
{
	acc *= GOVERNOR_ACC_SHARE;
	return (acc * (math::sqrt(0.25f + 2.0f * std::max(0.0f, dist) / acc) - 0.5f));
}


float CStrafeAirMoveType::GetAgileHeight() const
{
	// maneuvering can be given its own (lower) altitude, never above the cruise altitude
	// (orgWantedHeight: wantedHeight is the touchdown height while landing)
	return ((agileAltitude > 0.0f)? std::min(agileAltitude, orgWantedHeight): orgWantedHeight);
}


float CStrafeAirMoveType::GetAgileGoalRadius() const
{
	return (std::max(owner->radius, AGILE_MIN_GOAL_RADIUS));
}


float CStrafeAirMoveType::GetAgileSpeed() const
{
	const float speed = std::min(agileSpeed, maxSpeed);

	// zero means "no limit requested" here, StopMoving sets it while we still need to fly
	return ((maxWantedSpeed > 0.0f)? std::min(speed, maxWantedSpeed): speed);
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


void CStrafeAirMoveType::UpdateAgileFlight(const float3& targetPos, const float3& facePos, float targetHeight, bool landing)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	SyncedFloat3& rightdir = owner->rightdir;
	SyncedFloat3& frontdir = owner->frontdir;
	SyncedFloat3& updir    = owner->updir;

	const float agileAcc = std::max(agileAccRate, AGILE_MIN_RATE);
	const float goalRadius = GetAgileGoalRadius();

	// how much of the regime's top speed we are flying at
	const float speedFraction = std::min(1.0f, spd.Length2D() / std::max(GetAgileSpeed(), AGILE_EPSILON));

	// heading: the nose comes round toward <facePos> while we strafe there, at a rate that
	// grows with our speed; an aircraft that is not moving can not pivot on the spot
	float3 frontDir2D = (frontdir * XZVector).SafeNormalize();

	if (frontDir2D == ZeroVector)
		frontDir2D = FwdVector;

	const float3 faceVec = (facePos - pos) * XZVector;
	const float faceDist = faceVec.Length();

	float turnFraction = 0.0f;

	if (faceDist > goalRadius) {
		const float3 faceDir = faceVec / faceDist;
		const float3 flatRight = frontDir2D.cross(UpVector);

		const float maxYaw = std::max(agileTurnRate, 0.0f) * (math::TWOPI / SPRING_CIRCLE_DIVS) * speedFraction;
		const float faceAngle = math::acos(std::clamp(frontDir2D.dot(faceDir), -1.0f, 1.0f));
		const float turnSign = (flatRight.dot(faceDir) >= 0.0f)? 1.0f: -1.0f;

		if (faceAngle <= maxYaw) {
			frontDir2D = faceDir;
		} else {
			frontDir2D = (frontDir2D * math::cos(maxYaw) + flatRight * (turnSign * math::sin(maxYaw))).SafeNormalize();
		}

		turnFraction = turnSign * std::min(1.0f, faceAngle / std::max(maxYaw * (BANK_FULL_TURN_SECONDS * GAME_SPEED), AGILE_MIN_RATE));
	}

	// horizontal: steer the velocity vector at the target, slowing down in time to stop on it
	const float3 goalVec = (targetPos - pos) * XZVector;
	const float goalDist = goalVec.Length();

	float3 wantedVel;

	if (goalDist > AGILE_EPSILON) {
		const float3 goalDir = goalVec / goalDist;

		// straight at the target whichever way the nose points: strafing, not turn-then-fly
		const float wantedSpeed = std::min(GetAgileSpeed(), GovernedSpeed(goalDist, agileAcc));

		wantedVel = goalDir * std::min(wantedSpeed, goalDist);
	}

	float3 deltaVel = wantedVel - spd * XZVector;
	const float deltaVelLen = deltaVel.Length();

	if (deltaVelLen > agileAcc)
		deltaVel *= (agileAcc / deltaVelLen);

	// vertical: same governor, against the terrain ahead unless we are setting down
	float groundHeight = amtGetGroundHeightFuncs[canSubmerge](pos.x, pos.z);

	if (!landing) {
		groundHeight = amtGetGroundHeightFuncs[5 * UseSmoothMesh()](pos.x, pos.z);
		const float3 aheadPos = pos + spd * (AGILE_TERRAIN_LOOKAHEAD_SECONDS * GAME_SPEED);

		groundHeight = std::max(groundHeight, amtGetGroundHeightFuncs[5 * UseSmoothMesh()](aheadPos.x, aheadPos.z));
	}

	const float deltaHeight = (groundHeight + targetHeight) - pos.y;
	// (never further than what is left, or it overshoots by that much every frame)
	const float wantedClimb = Sign(deltaHeight) * std::min({altitudeRate, GovernedSpeed(math::fabs(deltaHeight), agileAcc), math::fabs(deltaHeight)});

	deltaVel.y = std::clamp(wantedClimb - spd.y, -agileAcc, agileAcc);

	owner->SetVelocityAndSpeed(spd + deltaVel);
	owner->Move(spd, true);
	// never sink into the ground
	owner->Move(UpVector * (std::max(pos.y, amtGetGroundHeightFuncs[canSubmerge](pos.x, pos.z)) - pos.y), true);

	// attitude (cosmetic, the velocity vector does not care): lean into the direction
	// we are strafing in and a little into the turn, dip the nose when moving forward
	const float3 flatRight = frontDir2D.cross(UpVector);
	const float3 flatVel = spd * XZVector;

	const float agileSpd = std::max(GetAgileSpeed(), AGILE_EPSILON);
	const float sideFraction = std::clamp(flatVel.dot(flatRight) / agileSpd, -1.0f, 1.0f);
	const float frontFraction = std::clamp(flatVel.dot(frontDir2D) / agileSpd, -1.0f, 1.0f);

	static const float attitudeBlend = 1.0f - math::pow(ATTITUDE_KEPT_PER_SECOND, INV_GAME_SPEED);

	const float frontY = mix(float(frontdir.y), -AGILE_NOSE_DIP * frontFraction, attitudeBlend);
	const float bankY = -(BANK_STRAFE_SHARE * sideFraction + (1.0f - BANK_STRAFE_SHARE) * turnFraction * speedFraction) * std::clamp(maxBank, 0.0f, AGILE_MAX_BANK);

	const float3 wantedRight = flatRight * math::sqrt(1.0f - bankY * bankY) + UpVector * bankY;

	frontdir = (frontDir2D * math::sqrt(1.0f - frontY * frontY) + UpVector * frontY).SafeNormalize();

	float3 newRight = mix(float3(rightdir), wantedRight, attitudeBlend);

	// remove the part along frontdir, fall back to the wanted vector if nothing is left
	newRight -= (float3(frontdir) * newRight.dot(frontdir));

	if (newRight.SqLength() < AGILE_EPSILON)
		newRight = wantedRight - (float3(frontdir) * wantedRight.dot(frontdir));

	rightdir = newRight.SafeNormalize();
	updir = (rightdir.cross(frontdir)).SafeNormalize();

	owner->SetHeadingFromDirection();
	owner->UpdateMidAndAimPos();
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
