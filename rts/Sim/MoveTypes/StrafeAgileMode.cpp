/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// The agile flight regime of CStrafeAirMoveType: hover-like control for short legs, the final
// approach, takeoff and landing of strafing aircraft that have agileFlight set (the fixed-wing
// model in StrafeAirMoveType.cpp flies everything else and every attack run)

#include "StrafeAirMoveType.h"

#include <vector>

#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "System/EventHandler.h"
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


// steepest ground an aircraft sets down on
static constexpr float MAX_LANDING_SLOPE = 0.03f;

// agile flight regime: defaults of the tunables a unit does not set
static constexpr float AGILE_SPEED_DEFAULT_SHARE = 0.4f; // of the unit's top speed
static constexpr float AGILE_TURN_RATE_DEFAULT_PER_SECOND = 8100.0f; // heading units (65536 is a full circle): half a turn in about four seconds

// keeps divisions by a rate finite
static constexpr float AGILE_MIN_RATE = 0.0001f;
// distances, speeds and vector lengths below this count as zero
static constexpr float AGILE_EPSILON = 0.01f;

// share of its acceleration limit the agile regime plans its braking with
static constexpr float GOVERNOR_ACC_SHARE = 0.8f;
// BrakingDistance divides by log(invDrag), above this the aircraft counts as free of drag
static constexpr float DRAG_FREE_INV_DRAG = 0.9999f;

// an agile move order is finished within the unit's radius of the goal, but never less than this
static constexpr float AGILE_MIN_GOAL_RADIUS = 2.0f * SQUARE_SIZE;
// an aircraft told to stop this many goal radii from its goal has arrived and holds or lands there
static constexpr float ARRIVED_GOAL_RADII = 4.0f;
// lowest the approach descent goes before a landing spot is reserved, as a share of the cruise altitude
static constexpr float APPROACH_MIN_HEIGHT_SHARE = 0.3f;

// cruise flight is worth it for goals further than cruiseDistance times this
static constexpr float CRUISE_DIST_FINAL_GOAL_MULT = 1.2f; // goal we stop on
static constexpr float CRUISE_DIST_WAYPOINT_MULT = 0.5f; // goal we fly through
// handover to cruise flight wants this share of the agile altitude and of the agile speed,
// a climb rate the regime levels off within this time, and the nose this close to the goal
static constexpr float HANDOVER_MIN_HEIGHT_SHARE = 0.8f;
static constexpr float HANDOVER_MIN_SPEED_SHARE = 0.9f;
static constexpr float HANDOVER_MAX_CLIMB_SECONDS = 1.0f / 15.0f;
static constexpr float HANDOVER_MIN_GOAL_DOT = 0.95f;
// braking starts this much travel time before the stop distance
static constexpr float STOP_MARGIN_SECONDS = 1.0f / 15.0f;
// a goal closer than a turn diameter and further off the nose than this (60 degrees) is not turned onto at speed
static constexpr float MUST_TURN_MAX_GOAL_DOT = 0.5f;

// takeoff climbs straight up until clear of the ground by the lesser of these
static constexpr float TAKEOFF_CLEARANCE_HEIGHT_SHARE = 0.5f; // of the agile altitude
static constexpr float TAKEOFF_CLEARANCE_RADII = 2.0f;

// an aircraft without a landing spot searches for one four times a second; a search that finds nothing
// went all the way out (there is no ground to land on, say over water), so each one doubles the wait
static constexpr int SPOT_SEARCH_FRAMES = GAME_SPEED / 4;
static constexpr int SPOT_SEARCH_MAX_FRAMES = 8 * GAME_SPEED;
// touchdown happens within this share of the goal radius of the spot,
// at no more than this share of the speed change one frame allows
static constexpr float TOUCHDOWN_RADIUS_SHARE = 0.25f;
static constexpr float TOUCHDOWN_MAX_SPEED_SHARE = 0.5f;
// distance between neighboring holding and landing spots, in unit radii
static constexpr float SPOT_SPACING_RADII = 2.2f;
// how far out the spot search goes. A handful of rings serves nearly every call, only the stragglers
// of a very large group or a goal deep inside a base search far out
static constexpr int SPOT_SEARCH_MAX_RINGS = 64;

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


void CStrafeAirMoveType::SetFlightRegime(int regime)
{
	if (regime == flightRegime)
		return;

	flightRegime = regime;
	eventHandler.UnitFlightRegimeChanged(owner, regime == REGIME_AGILE);
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


float CStrafeAirMoveType::GetCruiseBrakingDistance(float speed) const
{
	if (speed <= 0.0f)
		return 0.0f;

	const float rate = std::max(decRate, AGILE_MIN_RATE);

	if (invDrag > DRAG_FREE_INV_DRAG)
		return (speed * speed / (2.0f * rate));

	return (BrakingDistance(speed, rate));
}


float CStrafeAirMoveType::GetAgileStopDistance(float speed) const
{
	const float agileSpd = GetAgileSpeed();
	const float agileAcc = std::max(agileAccRate, AGILE_MIN_RATE);

	// fixed-wing braking leg down to the handover speed, then the agile regime stops us
	const float cruiseLeg = (speed > agileSpd)? (GetCruiseBrakingDistance(speed) - GetCruiseBrakingDistance(agileSpd)): 0.0f;
	const float agileLeg = Square(std::min(speed, agileSpd)) / (2.0f * agileAcc) + std::min(speed, agileSpd);

	return (std::max(0.0f, cruiseLeg) + agileLeg);
}


float CStrafeAirMoveType::GetAgileApproachHeight(float goalDist2D, bool finalGoal) const
{
	// about to set down at the goal: come down a 1:1 slope on the way in, but stay
	// well clear of the ground until the landing spot has actually been reserved
	const float agileHeight = GetAgileHeight();

	if (!finalGoal || dontLand || !autoLand)
		return agileHeight;

	return (std::clamp(goalDist2D, std::min(agileHeight, orgWantedHeight * APPROACH_MIN_HEIGHT_SHARE), agileHeight));
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


void CStrafeAirMoveType::UpdateAgileRegime()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3& pos = owner->pos;

	const float goalDist2D = pos.distance2D(goalPos);
	const float speed2D = owner->speed.Length2D();
	const float agileSpd = GetAgileSpeed();
	const float cruiseDist = GetCruiseDistance();

	// only the last goal of a queue is captured exactly, earlier waypoints are flown through;
	// anything but a plain move (guard, fight, patrol, ...) keeps its stock arrival behavior
	const CCommandQueue& cmdQue = owner->commandAI->commandQue;

	const bool plainMove = (cmdQue.empty() || cmdQue.front().GetID() == CMD_MOVE);
	const bool finalGoal = (plainMove && !owner->commandAI->HasMoreMoveCommands());

	// brake when the goal is as far off as we need to stop, or when it is close
	// but not ahead (a fixed-wing turn onto it would be wider than flying there agile)
	const float3 goalDir2D = ((goalPos - pos) * XZVector).SafeNormalize();

	const bool mustStop = (goalDist2D < (GetAgileStopDistance(speed2D) + speed2D * (STOP_MARGIN_SECONDS * GAME_SPEED)));
	// (never less than a turn diameter, or a small cruiseDistance has us orbit the goal)
	const bool mustTurn = (goalDist2D < std::max(cruiseDist, GetTurnDiameter()) && owner->frontdir.dot(goalDir2D) < MUST_TURN_MAX_GOAL_DOT);
	const bool brake = (finalGoal && (mustStop || mustTurn));

	if (flightRegime == REGIME_AGILE) {
		const float curHeight = pos.y - amtGetGroundHeightFuncs[5 * UseSmoothMesh()](pos.x, pos.z);

		const bool farGoal = (goalDist2D > cruiseDist * (finalGoal? CRUISE_DIST_FINAL_GOAL_MULT: CRUISE_DIST_WAYPOINT_MULT));
		// the fixed-wing model turns climb rate into forward speed, so finish climbing first
		// (measured against the ground passing below, following a slope is level flight too)
		const float groundClimbRate = amtGetGroundHeightFuncs[5 * UseSmoothMesh()](pos.x + owner->speed.x, pos.z + owner->speed.z) - (pos.y - curHeight);
		const bool levelFlight = (curHeight > GetAgileHeight() * HANDOVER_MIN_HEIGHT_SHARE && math::fabs(owner->speed.y - groundClimbRate) < agileAccRate * (HANDOVER_MAX_CLIMB_SECONDS * GAME_SPEED));
		const bool canHandOver = (levelFlight && speed2D >= agileSpd * HANDOVER_MIN_SPEED_SHARE && owner->frontdir.dot(goalDir2D) > HANDOVER_MIN_GOAL_DOT);

		// (not into a cruise leg that would brake straight back into this regime)
		if (farGoal && canHandOver && !brake && agileSpd < maxSpeed) {
			SetFlightRegime(REGIME_CRUISE);
		} else {
			UpdateAgileFlight(goalPos, goalPos, GetAgileApproachHeight(goalDist2D, finalGoal), false);
			return;
		}
	}

	if (brake && speed2D <= agileSpd) {
		SetFlightRegime(REGIME_AGILE);
		UpdateAgileFlight(goalPos, goalPos, GetAgileApproachHeight(goalDist2D, finalGoal), false);
		return;
	}

	UpdateFlying(wantedHeight, brake? -(decRate / std::max(accRate, AGILE_MIN_RATE)): 1.0f);
}


void CStrafeAirMoveType::UpdateAgileTakeOff()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3& pos = owner->pos;

	const CCommandQueue& cmdQue = owner->commandAI->commandQue;

	// attack runs are flown fixed-wing, which wants the height and speed of a stock takeoff
	if (!cmdQue.empty() && cmdQue.front().GetID() == CMD_ATTACK) {
		SetFlightRegime(REGIME_CRUISE);
		UpdateTakeOff();
		return;
	}

	wantedHeight = orgWantedHeight;

	const float curHeight = pos.y - amtGetGroundHeightFuncs[canSubmerge](pos.x, pos.z);
	const float clearance = std::min(GetAgileHeight() * TAKEOFF_CLEARANCE_HEIGHT_SHARE, owner->radius * TAKEOFF_CLEARANCE_RADII);

	if (curHeight > clearance) {
		// also reached when a new order arrives in mid-air, keep whatever regime we were in
		SetState(AIRCRAFT_FLYING);
		UpdateAgileRegime();
		return;
	}

	SetFlightRegime(REGIME_AGILE);

	// climb straight up (the nose only comes round once we move)
	UpdateAgileFlight(pos, goalPos, GetAgileHeight(), false);
}


void CStrafeAirMoveType::UpdateAgileLanding()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3& pos = owner->pos;

	// LandAt (Spring.SetUnitLandGoal) can hand us a spot, possibly raised above the ground
	const float3 landTarget = HaveLandingPos()? reservedLandingPos: ((landGoalPos.x != -1.0f)? landGoalPos: float3(pos));

	if (flightRegime == REGIME_CRUISE) {
		// still too fast for the agile regime, brake fixed-wing first
		if (owner->speed.Length2D() > GetAgileSpeed()) {
			const float tempWantedHeight = wantedHeight;

			SetGoal(landTarget);
			UpdateFlying(wantedHeight = orgWantedHeight, -(decRate / std::max(accRate, AGILE_MIN_RATE)));

			wantedHeight = tempWantedHeight;
			return;
		}

		SetFlightRegime(REGIME_AGILE);
	}

	if (!HaveLandingPos()) {
		// the search is not free, so not every frame; we are still flying to the target meanwhile
		if (((gs->frameNum + owner->id) % spotSearchFrames) == 0) {
			reservedLandingPos = FindAgileSpot(landTarget, true);
			spotSearchFrames = HaveLandingPos()? SPOT_SEARCH_FRAMES: std::min(spotSearchFrames * 2, SPOT_SEARCH_MAX_FRAMES);
		}

		if (HaveLandingPos()) {
			const float3 originalPos = pos;

			wantedHeight = 0.0f;

			// mark the spot on the blocking-map so other aircraft can not claim it
			owner->Move(reservedLandingPos, false);
			owner->Block();
			owner->Move(originalPos, false);
		} else {
			// nowhere to set down (yet), hold over the target
			UpdateAgileFlight(landTarget, landTarget, GetAgileHeight(), false);
			return;
		}
	}

	SetGoal(reservedLandingPos);
	UpdateLandingHeight(wantedHeight);

	const float touchRadius = GetAgileGoalRadius() * TOUCHDOWN_RADIUS_SHARE;
	const float landDist2D = pos.distance2D(reservedLandingPos);

	// descend along a 1:1 slope, ending right above the spot
	UpdateAgileFlight(reservedLandingPos, reservedLandingPos, std::clamp(wantedHeight + landDist2D - touchRadius, wantedHeight, std::max(wantedHeight, GetAgileHeight())), true);

	const float localAltitude = pos.y - amtGetGroundHeightFuncs[canSubmerge](pos.x, pos.z);

	// wait for the governor to bleed off the sink rate, touchdown should not be a velocity step
	if (pos.distance2D(reservedLandingPos) <= touchRadius && localAltitude <= (wantedHeight + agileAccRate) && owner->speed.SqLength() <= Square(agileAccRate * TOUCHDOWN_MAX_SPEED_SHARE)) {
		SetState(AIRCRAFT_LANDED);
		owner->SetVelocityAndSpeed(ZeroVector);
		return;
	}

	// a raised spot is a pad some game logic owns (and which may well push us around),
	// leave the decision that we have arrived to the stock test and its caller's radius
	if (wantedHeight > 0.0f)
		AAirMoveType::UpdateLanding();
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


bool CStrafeAirMoveType::CanSetDownAt(const float3& spot) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// same rules as FindLandingPos, whose blocking-map test looks at the squares below the
	// aircraft (fixed-wing landings touch down short of the position they reserve); an agile
	// aircraft touches down on the spot itself, so that is where the ground has to be free
	if (CGround::GetHeightReal(spot.x, spot.z) < CGround::GetWaterLevel(spot.x, spot.z) && ((mapInfo->water.damage > 0.0f) || !(floatOnWater || canSubmerge)))
		return false;

	const int2 mp = CSolidObject::GetMapPosStatic(spot, owner->xsize, owner->zsize);

	for (int z = mp.y; z < (mp.y + owner->zsize); z++) {
		for (int x = mp.x; x < (mp.x + owner->xsize); x++) {
			if (groundBlockingObjectMap.GroundBlocked(x, z, owner))
				return false;
		}
	}

	return (CGround::GetSlope(spot.x, spot.z) <= MAX_LANDING_SLOPE);
}


float3 CStrafeAirMoveType::FindAgileSpot(const float3& wantedPos, bool landable)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// aircraft that stop on a goal would all stop on the same one, so each takes
	// the free spot nearest to it: not where another aircraft sits, holds or will land
	const float ringStep = owner->radius * SPOT_SPACING_RADII;

	struct Claim { float3 pos; float radius; };
	std::vector<Claim> claims;

	{
		// an aircraft can be as far from the spot it claimed as the search reaches (it reserves
		// on arriving at the goal, the spot may be rings away), so look twice that far for claimants.
		// Scoped: queries are pooled, and the spot tests need not hold on to this one
		const float searchReach = ringStep * (SPOT_SEARCH_MAX_RINGS + 2);

		QuadFieldQuery qfQuery;
		quadField.GetUnitsExact(qfQuery, wantedPos, searchReach * 2.0f, false);

		for (const CUnit* unit: *qfQuery.units) {
			if (unit == owner)
				continue;

			// a building we collide with pushes us out of its sphere, no setting down inside that
			if (landable && collide && unit->immobile) {
				claims.push_back({unit->pos, unit->radius});
				continue;
			}

			if (!unit->unitDef->IsStrafingAirUnit() || unit->UsingScriptMoveType())
				continue;

			const CStrafeAirMoveType* mt = static_cast<const CStrafeAirMoveType*>(unit->moveType);

			if (mt->HaveLandingPos()) {
				claims.push_back({mt->reservedLandingPos, unit->radius});
			} else if (mt->aircraftState == AIRCRAFT_LANDED) {
				claims.push_back({unit->pos, unit->radius});
			} else if (mt->UseAgileFlight() && mt->landGoalPos.x != -1.0f) {
				claims.push_back({mt->landGoalPos, unit->radius});
			}
		}
	}

	const auto TestSpot = [&](const float3& spot) -> float3 {
		if (!spot.IsInBounds())
			return -OnesVector;

		for (const Claim& claim: claims) {
			if (spot.SqDistance2D(claim.pos) < Square(owner->radius + claim.radius))
				return -OnesVector;
		}

		if (!landable)
			return spot;

		return (CanSetDownAt(spot)? float3(spot.x, CGround::GetHeightReal(spot.x, spot.z), spot.z): -OnesVector);
	};

	// hexagonal rings, neighboring spots are just over two radii apart
	float3 foundPos = TestSpot(wantedPos);

	for (int ring = 1; ring <= SPOT_SEARCH_MAX_RINGS && foundPos.x == -1.0f; ring++) {
		for (int n = 0; n < (6 * ring) && foundPos.x == -1.0f; n++) {
			const float angle = n * (math::TWOPI / (6 * ring));

			foundPos = TestSpot(wantedPos + float3(math::sin(angle), 0.0f, math::cos(angle)) * (ring * ringStep));
		}
	}

	// touchdown is exact
	if (landable && foundPos.x != -1.0f)
		landRadiusSq = Square(GetAgileGoalRadius());

	return foundPos;
}


// an agile aircraft told to stop holds or sets down on the goal it was about to discard;
// true when it took care of the stop
bool CStrafeAirMoveType::AgileStopMoving()
{
	if (!UseAgileFlight() || aircraftState == AIRCRAFT_LANDED || aircraftState == AIRCRAFT_CRASHING)
		return false;

	// when told to stop right after arriving, the goal about to be discarded is the
	// spot that was asked for: hold or set down there, otherwise come to a halt ahead
	const float3 flatVel = owner->speed * XZVector;
	const float flatSpeed = flatVel.Length();

	if (goalPos.SqDistance2D(owner->pos) < Square(GetAgileGoalRadius() * ARRIVED_GOAL_RADII)) {
		landGoalPos = FindAgileSpot(goalPos, false);

		if (landGoalPos.x == -1.0f)
			landGoalPos = goalPos;
	} else if (landGoalPos.x == -1.0f || aircraftState != AIRCRAFT_LANDING) {
		landGoalPos = owner->pos;

		if (flatSpeed > AGILE_EPSILON)
			landGoalPos += (flatVel / flatSpeed) * GetAgileStopDistance(flatSpeed);
	}

	landGoalPos.ClampInBounds();

	SetGoal(landGoalPos);
	ClearLandingPos();
	SetWantedMaxSpeed(0.0f);

	// a takeoff in progress carries on into flight and holds over the goal
	if (dontLand || !autoLand) {
		if (aircraftState != AIRCRAFT_TAKEOFF)
			SetState(AIRCRAFT_FLYING);

		return true;
	}

	SetState(AIRCRAFT_LANDING);
	return true;
}


// a new order forgets the spot the last one was going to end on
void CStrafeAirMoveType::AgileStartMoving()
{
	landGoalPos = -OnesVector;
	spotSearchFrames = SPOT_SEARCH_FRAMES;
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
