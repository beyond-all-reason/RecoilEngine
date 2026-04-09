/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>

#include "System/MathConstants.h"
#include "System/float3.h"
#include "Sim/IK/FABRIKSolverMath.hpp"

#include <catch_amalgamated.hpp>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static inline float randf()
{
	return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

static inline float srandf()
{
	return 2.0f * (randf() - 0.5f);
}

// Generate a uniformly random unit direction vector.
static float3 RandomDir()
{
	float3 d;
	do {
		d = float3{srandf(), srandf(), srandf()};
	} while (d.SqLength() < 1e-4f);
	d.Normalize();
	return d;
}



// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static constexpr int    NUM_TRIALS        = 10000; // Reduced for CI speed
static constexpr int    NUM_TRIALS_CONSTR = 1000;
static constexpr float  SOLVE_PRECISION   = 4.0f;
static constexpr int    MAX_ITERATIONS    = 40;
static constexpr float  SEG_LEN_TOL       = 0.1f;

TEST_CASE("FABRIKSingleJoint")
{
	std::vector<IK::Bone> chain;
	auto result = IK::SolveFABRIK(chain, float3{10, 0, 0}, MAX_ITERATIONS, SOLVE_PRECISION);
	CHECK(result == IK::FABRIKResult::ERR_INPUTS);
}

TEST_CASE("FABRIK2BoneReach")
{
	srand(42);
	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float L1 = 5.0f + randf() * 15.0f;
		const float L2 = 5.0f + randf() * 15.0f;

		std::vector<IK::Bone> chain(2);
		chain[0].length = L1;
		chain[0].orientation = CQuaternion::MakeFrom(float3(0,1,0), RandomDir());
		chain[1].length = L2;
		chain[1].orientation = CQuaternion::MakeFrom(float3(0,1,0), RandomDir());

		float3 goal = RandomDir() * ((L1 + L2) * 0.8f);

		auto result = IK::SolveFABRIK(chain, goal, MAX_ITERATIONS, SOLVE_PRECISION);

		CHECK(result == IK::FABRIKResult::FOUND);
		float3 effector = chain[0].orientation.Rotate(float3(0,1,0)) * L1 + chain[1].orientation.Rotate(float3(0,1,0)) * L2;
		CHECK(effector.distance(goal) < SOLVE_PRECISION);
	}
}

TEST_CASE("FABRIK2BoneStretch")
{
	srand(123);
	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float L1 = 5.0f + randf() * 15.0f;
		const float L2 = 5.0f + randf() * 15.0f;
		const float totalLen = L1 + L2;

		float3 goalDir = RandomDir();
		float3 goal = goalDir * (totalLen + 20.0f);

		std::vector<IK::Bone> chain(2);
		chain[0].length = L1;
		chain[0].orientation = CQuaternion::MakeFrom(float3(0,1,0), RandomDir());
		chain[1].length = L2;
		chain[1].orientation = CQuaternion::MakeFrom(float3(0,1,0), RandomDir());

		auto result = IK::SolveFABRIK(chain, goal, MAX_ITERATIONS, SOLVE_PRECISION);

		CHECK(result == IK::FABRIKResult::STRETCHING);
		float3 d0 = chain[0].orientation.Rotate(float3(0,1,0));
		float3 d1 = chain[1].orientation.Rotate(float3(0,1,0));
		CHECK(d0.dot(goalDir) > 0.99f);
		CHECK(d1.dot(goalDir) > 0.99f);
	}
}

TEST_CASE("FABRIKGoalAtRoot")
{
	srand(99);
	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float L = 10.0f;
		std::vector<IK::Bone> chain(2);
		chain[0].length = L;
		chain[0].orientation = CQuaternion::MakeFrom(float3(0,1,0), RandomDir());
		chain[1].length = L;
		chain[1].orientation = CQuaternion::MakeFrom(float3(0,1,0), RandomDir());

		float3 goal = float3(0, 0, 0);

		auto result = IK::SolveFABRIK(chain, goal, MAX_ITERATIONS * 2, SOLVE_PRECISION);

		CHECK(result == IK::FABRIKResult::FOUND);
		float3 effector = chain[0].orientation.Rotate(float3(0,1,0)) * L + chain[1].orientation.Rotate(float3(0,1,0)) * L;
		CHECK(effector.distance(goal) < SOLVE_PRECISION);
	}
}

TEST_CASE("FABRIKBallConstraint")
{
	srand(200);
	for (int trial = 0; trial < NUM_TRIALS_CONSTR; trial++) {
		const float coneAngle = math::PI * 0.4f;
		float3 localConeAxis = float3(0, 1, 0); // Along the bone

		std::vector<IK::Bone> chain(3);
		for (size_t i = 0; i < 3; i++) {
			chain[i].length = 10.0f;
			chain[i].orientation = CQuaternion::MakeFrom(float3(0,1,0), RandomDir());
			IK::BallJointConstraint bc;
			bc.coneAxis = localConeAxis;
			bc.coneAngle = coneAngle;
			chain[i].constraint = bc;
		}

		float3 goal = RandomDir() * 15.0f;
		auto result = IK::SolveFABRIK(chain, goal, MAX_ITERATIONS * 2, SOLVE_PRECISION);
		CHECK(result == IK::FABRIKResult::FOUND);
		if (result == IK::FABRIKResult::FOUND) {
			for (size_t i = 0; i < 3; i++) {
				float3 dirW = chain[i].orientation.Rotate(float3(0,1,0));
				CQuaternion parentOri = (i > 0) ? chain[i-1].orientation : CQuaternion();
				float3 coneAxisW = parentOri.Rotate(localConeAxis);
				CHECK(dirW.dot(coneAxisW) >= math::cos(coneAngle) - 0.05f);
			}
		}
	}
}

TEST_CASE("FABRIKHingeConstraint")
{
	srand(300);
	for (int trial = 0; trial < NUM_TRIALS_CONSTR; trial++) {
		const float minA = -math::PI * 0.25f;
		const float maxA =  math::PI * 0.25f;
		float3 localHingeAxis = float3(1, 0, 0); // Orthogonal to bone (0,1,0)

		std::vector<IK::Bone> chain(3);
		for (size_t i = 0; i < 3; i++) {
			chain[i].length = 10.0f;
			chain[i].orientation = CQuaternion();
			IK::HingeJointConstraint hc;
			hc.axis = localHingeAxis;
			hc.minAngle = minA;
			hc.maxAngle = maxA;
			chain[i].constraint = hc;
		}

		float3 goal = float3(0, 0, 0);
		{
			CQuaternion currentOri;
			for (size_t i = 0; i < 3; i++) {
				float angle = minA + randf() * (maxA - minA);
				CQuaternion localRot = CQuaternion::MakeFrom(angle, localHingeAxis);
				currentOri = (currentOri * localRot).Normalize();
				goal += currentOri.Rotate(float3(0,1,0)) * chain[i].length;
			}
		}
		auto result = IK::SolveFABRIK(chain, goal, MAX_ITERATIONS * 4, SOLVE_PRECISION * 2.0f);
		CHECK(result == IK::FABRIKResult::FOUND);
		if (result == IK::FABRIKResult::FOUND) {
			for (size_t i = 0; i < 3; i++) {
				float3 dirW = chain[i].orientation.Rotate(float3(0,1,0));
				CQuaternion parentOri = (i > 0) ? chain[i-1].orientation : CQuaternion();

				float3 dirL = parentOri.Inverse().Rotate(dirW);
				float3 restL = float3(0,1,0); // Our convention

				float3 projL = (dirL - localHingeAxis * dirL.dot(localHingeAxis)).SafeNormalize();
				float cosA = std::clamp(projL.dot(restL), -1.0f, 1.0f);
				float sinSign = restL.cross(projL).dot(localHingeAxis) >= 0.0f ? 1.0f : -1.0f;
				float angle = sinSign * math::acos(cosA);

				CHECK(angle >= minA - 0.1f);
				CHECK(angle <= maxA + 0.1f);
			}
		}
	}
}
TEST_CASE("FABRIKBenchmarks", "[!benchmark]")
{
	const int benchmarkTrials = 2000; 
	srand(1337);

	{
		uint64_t totalIters = 0;
		int successes = 0;
		for (int trial = 0; trial < benchmarkTrials; trial++) {
			const float L1 = 10.0f, L2 = 10.0f;
			std::vector<IK::Bone> chain(2);
			chain[0].length = L1;
			chain[0].orientation = CQuaternion::MakeFrom(float3(0,1,0), RandomDir());
			chain[1].length = L2;
			chain[1].orientation = CQuaternion::MakeFrom(float3(0,1,0), RandomDir());

			float3 goal = RandomDir() * 15.0f;
			uint32_t iters = 0;
			auto res = IK::SolveFABRIK(chain, goal, MAX_ITERATIONS, SOLVE_PRECISION, &iters);
			if (res == IK::FABRIKResult::FOUND) {
				totalIters += iters;
				successes++;
			}
		}
		printf("  2-Bone Unconstrained:    avg iters = %.2f (%d/%d successes)\n", (double)totalIters / (successes ? successes : 1), successes, benchmarkTrials);
	}

	{
		uint64_t totalIters = 0;
		int successes = 0;
		const float coneAngle = math::PI * 0.4f;
		for (int trial = 0; trial < benchmarkTrials; trial++) {
			std::vector<IK::Bone> chain(3);
			for (size_t i = 0; i < 3; i++) {
				chain[i].length = 10.0f;
				chain[i].orientation = CQuaternion::MakeFrom(float3(0,1,0), RandomDir());
				IK::BallJointConstraint bc;
				bc.coneAngle = coneAngle;
				chain[i].constraint = bc;
			}
			float3 goal = RandomDir() * 15.0f;
			uint32_t iters = 0;
			auto res = IK::SolveFABRIK(chain, goal, MAX_ITERATIONS * 2, SOLVE_PRECISION, &iters);
			if (res == IK::FABRIKResult::FOUND) {
				totalIters += iters;
				successes++;
			}
		}
		printf("  3-Bone Ball Constraints:  avg iters = %.2f (%d/%d successes)\n", (double)totalIters / (successes ? successes : 1), successes, benchmarkTrials);
	}

	{
		uint64_t totalIters = 0;
		int successes = 0;
		for (int trial = 0; trial < benchmarkTrials; trial++) {
			std::vector<IK::Bone> chain(3);
			for (size_t i = 0; i < 3; i++) {
				chain[i].length = 10.0f;
				chain[i].orientation = CQuaternion();
				IK::HingeJointConstraint hc;
				hc.axis = float3(1, 0, 0);
				hc.minAngle = -math::PI * 0.25f;
				hc.maxAngle =  math::PI * 0.25f;
				chain[i].constraint = hc;
			}
			float3 goal = float3(0, 0, 0);
			{
				CQuaternion currentOri;
				for (size_t i = 0; i < 3; i++) {
					float angle = -math::PI * 0.25f + randf() * (math::PI * 0.5f);
					CQuaternion localRot = CQuaternion::MakeFrom(angle, float3(1,0,0));
					currentOri = (currentOri * localRot).Normalize();
					goal += currentOri.Rotate(float3(0,1,0)) * chain[i].length;
				}
			}
			uint32_t iters = 0;
			auto res = IK::SolveFABRIK(chain, goal, MAX_ITERATIONS * 10, SOLVE_PRECISION * 0.5f, &iters);
			if (res == IK::FABRIKResult::FOUND) {
				totalIters += iters;
				successes++;
			}
		}
		printf("  3-Bone Hinge Constraints: avg iters = %.2f (%d/%d successes)\n", (double)totalIters / (successes ? successes : 1), successes, benchmarkTrials);
	}
	printf("Benchmarks Finished.\n");
}
