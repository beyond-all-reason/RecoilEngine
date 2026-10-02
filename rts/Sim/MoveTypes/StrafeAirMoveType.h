/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "AAirMoveType.h"

struct float4;

/**
 * Air movement type definition
 */
class CStrafeAirMoveType: public AAirMoveType
{
	CR_DECLARE_DERIVED(CStrafeAirMoveType)
	CR_DECLARE_SUB(DrawLine)

public:
	enum {
		MANEUVER_FLY_STRAIGHT = 0,
		MANEUVER_IMMELMAN     = 1,
		MANEUVER_IMMELMAN_INV = 2,
	};
	enum {
		REGIME_CRUISE = 0, ///< fixed-wing flight model
		REGIME_AGILE  = 1, ///< hover-like control, only entered when agileFlight is set
	};

	CStrafeAirMoveType(CUnit* owner);

	void* GetPreallocContainer() { return owner; }  // creg

	bool Update() override;
	void SlowUpdate() override;

	bool SetMemberValue(unsigned int memberHash, void* memberValue) override;

	void UpdateManeuver();
	void UpdateAttack();
	bool UpdateFlying(float wantedHeight, float wantedThrottle);
	void UpdateLanding();
	bool UpdateAirPhysics(const float4& controlInputs, const float3& thrustVector);
	void SetState(AircraftState state) override;
	void UpdateTakeOff();

	void UpdateAgileRegime();
	void UpdateAgileTakeOff();
	void UpdateAgileLanding();
	/// hover-like control: fly to <targetPos> at <targetHeight> above ground while turning to face <facePos>
	void UpdateAgileFlight(const float3& targetPos, const float3& facePos, float targetHeight, bool landing);

	/// agileFlight, unless it is restricted to the idle mode "land" and the aircraft is set to fly
	bool UseAgileFlight() const { return (agileFlight && (!agileLandOnly || (autoLand && !dontLand))); }
	bool InAgileRegime() const { return (UseAgileFlight() && flightRegime == REGIME_AGILE); }
	/// how close an agile aircraft gets to a goal before its move order counts as finished
	float GetAgileGoalRadius() const;
	/// altitude flown in the agile regime
	float GetAgileHeight() const;
	/// the only place the regime changes, so that Lua hears about every change
	void SetFlightRegime(int regime);

	void SetAgileSpeed(float speed);
	void SetAgileTurnRate(float rate);
	void SetAgileAccRate(float rate);
	float GetAgileSpeed() const;
	float GetCruiseDistance() const;
	float GetTurnDiameter() const;
	float GetAgileApproachHeight(float goalDist2D, bool finalGoal) const;
	float GetCruiseBrakingDistance(float speed) const;
	/// distance needed to come to rest from <speed>, across both regimes
	float GetAgileStopDistance(float speed) const;

	float3 FindLandingPos(float3 landPos);
	/// nearest spot to <wantedPos> that no other aircraft has claimed, -OnesVector if there is none
	float3 FindAgileSpot(const float3& wantedPos, bool landable);
	bool CanSetDownAt(const float3& spot) const;
	/// the agile regime's share of StopMoving, StartMoving and SetMemberValue
	bool AgileStopMoving();
	void AgileStartMoving();
	bool SetAgileMemberValue(unsigned int memberHash, void* memberValue);

	void SetMaxSpeed(float speed) override;
	float BrakingDistance(float speed, float rate) const override;

	void KeepPointingTo(float3 pos, float distance, bool aggressive) override {}
	void StartMoving(float3 pos, float goalRadius) override;
	void StartMoving(float3 pos, float goalRadius, float speed) override;
	void StopMoving(bool callScript = false, bool hardStop = false, bool cancelRaw = false) override;

	void Takeoff() override;

private:
	bool HandleCollisions(bool checkCollisions);

public:
	int maneuverBlockTime = 0;
	int maneuverState = MANEUVER_FLY_STRAIGHT;
	int maneuverSubState = 0;

	bool loopbackAttack = false;
	bool isFighter = false;

	float wingDrag = 0.07f;
	float wingAngle = 0.1f;
	float invDrag = 0.995f;
	/// actually the invDrag of crashDrag
	float crashDrag = 0.995f;

	float frontToSpeed = 0.04f;
	float speedToFront = 0.01f;
	float myGravity = 0.8f;

	float maxBank = 0.55f;
	float maxPitch = 0.35f;
	float turnRadius = 150.0f;

	float maxAileron = 0.04f;
	float maxElevator = 0.02f;
	float maxRudder = 0.01f;
	// fighters abort dive toward target if within this distance and climb back to normal altitude
	float attackSafetyDistance = 0.0f;

	/// used while landing
	float crashAileron = 0.0f;
	float crashElevator = 0.0f;
	float crashRudder = 0.0f;

	float lastRudderPos[2] = {0.0f, 0.0f};
	float lastElevatorPos[2] = {0.0f, 0.0f};
	float lastAileronPos[2] = {0.0f, 0.0f};

	bool agileFlight = false;
	bool agileLandOnly = false;
	int flightRegime = REGIME_CRUISE;

	/// elmos/frame; top speed of the agile regime and the speed at which it hands over to cruise flight
	float agileSpeed = 0.0f;
	/// heading change per frame in the agile regime at its top speed (65536 is a full circle), scaled down with speed
	float agileTurnRate = 0.0f;
	/// acceleration and deceleration limit of the agile regime
	float agileAccRate = 0.0f;
	/// goals nearer than this are flown entirely in the agile regime; 0 derives it from the turn radius
	float cruiseDistance = 0.0f;

	/// altitude of the agile regime; 0 uses the cruise altitude (wantedHeight)
	float agileAltitude = 0.0f;

	/// where an agile aircraft sets down: the goal StopMoving was about to discard
	float3 landGoalPos = -OnesVector;

	/// frames between two searches for a landing spot, grows while they find nothing
	int spotSearchFrames = 1;
};
