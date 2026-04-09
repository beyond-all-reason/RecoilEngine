/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>

#include "System/MathConstants.h"
#include "System/float3.h"
#include "Sim/IK/IKSolverMath.hpp"
#include "Sim/IK/CCDSolverMath.hpp"

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
	} while (d.SqLength() < float3::apx_eps());
	d.Normalize();
	return d;
}

/**
 * @brief Get the reference "zero-angle" direction for a hinge axis.
 *
 * In standard orientation, a bone points at (0, 0, 1). For a hinge, we need
 * to project this "rest" direction onto the plane orthogonal to the hinge axis
 * to find the direction that corresponds to an angle of 0.
 */
static float3 GetHingeRestDir(const float3& axisL)
{
	static constexpr float3 defaultAxis = FwdVector;
	// Project the default forward vector onto the hinge plane
	float3 restProjL = defaultAxis - axisL * axisL.z;

	// If the axis is parallel to the bone, the projection is zero.
	// Fall back to an arbitrary orthogonal vector in the hinge plane.
	if (restProjL.SqLength() < float3::apx_eps()) {
		float3 alt = (std::abs(axisL.x) < 0.9f) ? RgtVector : UpVector;
		restProjL = axisL.cross(alt);
	}
	restProjL.Normalize();
	return restProjL;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static constexpr int    NUM_TRIALS        = 10000; // Reduced for CI speed
static constexpr int    NUM_TRIALS_CONSTR = 1000;
static constexpr float  SOLVE_PRECISION   = 4.0f;
static constexpr int    MAX_ITERATIONS    = 40;
static constexpr float  SEG_LEN_TOL       = 0.1f;

static IK::FABRIKResult SolveByName(
	const IK::IIKSolver& solver,
	std::vector<IK::Bone>& chain,
	const float3& goal,
	uint32_t maxIterations,
	float precision,
	uint32_t* iterCount = nullptr)
{
	return solver.Solve(chain, goal, maxIterations, precision, iterCount);
}

TEST_CASE("IKVirtualSolverInterfaceParity")
{
	std::vector<const IK::IIKSolver*> solvers = {
		&IK::GetFABRIKSolver(),
		&IK::GetCCDSolver(),
	};

	for (const IK::IIKSolver* solver: solvers) {
		INFO("solver = " << ((solver == &IK::GetFABRIKSolver()) ? "FABRIK" : "CCD"));

		std::vector<IK::Bone> chain(2);
		chain[0].length = 11.0f;
		chain[0].orientation = CQuaternion::MakeFrom(FwdVector, float3{0.2f, 0.1f, 1.0f}.SafeNormalize());
		chain[1].length = 8.0f;
		chain[1].orientation = CQuaternion::MakeFrom(FwdVector, float3{-0.3f, 0.4f, 1.0f}.SafeNormalize());

		const float3 goal{4.0f, 2.0f, 10.0f};
		uint32_t iters = 0;
		const auto result = solver->Solve(chain, goal, 120, 0.5f, &iters);

		CHECK(result == IK::FABRIKResult::FOUND);
		CHECK(iters > 0);

		const float3 effector =
			chain[0].orientation.Rotate(FwdVector) * chain[0].length +
			chain[1].orientation.Rotate(FwdVector) * chain[1].length;

		CHECK(effector.distance(goal) < 0.5f);
	}
}

TEST_CASE("CCDInputValidation")
{
	std::vector<IK::Bone> chain;
	auto result = IK::GetCCDSolver().Solve(chain, float3{10, 0, 0}, MAX_ITERATIONS, SOLVE_PRECISION);
	CHECK(result == IK::FABRIKResult::ERR_INPUTS);
}

TEST_CASE("CCD2BoneReach")
{
	srand(451);
	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float L1 = 5.0f + randf() * 15.0f;
		const float L2 = 5.0f + randf() * 15.0f;

		std::vector<IK::Bone> chain(2);
		chain[0].length = L1;
		chain[0].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());
		chain[1].length = L2;
		chain[1].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());

		const float3 goal = RandomDir() * ((L1 + L2) * 0.8f);

		const auto result = SolveByName(IK::GetCCDSolver(), chain, goal, 200, SOLVE_PRECISION);
		CHECK(result == IK::FABRIKResult::FOUND);

		const float3 effector = chain[0].orientation.Rotate(FwdVector) * L1 + chain[1].orientation.Rotate(FwdVector) * L2;
		CHECK(effector.distance(goal) < SOLVE_PRECISION);
	}
}

TEST_CASE("CCD2BoneStretch")
{
	srand(452);
	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float L1 = 5.0f + randf() * 15.0f;
		const float L2 = 5.0f + randf() * 15.0f;
		const float totalLen = L1 + L2;

		const float3 goalDir = RandomDir();
		const float3 goal = goalDir * (totalLen + 20.0f);

		std::vector<IK::Bone> chain(2);
		chain[0].length = L1;
		chain[0].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());
		chain[1].length = L2;
		chain[1].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());

		const auto result = SolveByName(IK::GetCCDSolver(), chain, goal, 200, SOLVE_PRECISION);
		CHECK(result == IK::FABRIKResult::STRETCHING);

		const float3 d0 = chain[0].orientation.Rotate(FwdVector);
		const float3 d1 = chain[1].orientation.Rotate(FwdVector);
		CHECK(d0.dot(goalDir) > 0.99f);
		CHECK(d1.dot(goalDir) > 0.99f);
	}
}

TEST_CASE("CCDVsFABRIK2BoneReachComparison")
{
	srand(453);
	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float L1 = 5.0f + randf() * 15.0f;
		const float L2 = 5.0f + randf() * 15.0f;

		std::vector<IK::Bone> seedChain(2);
		seedChain[0].length = L1;
		seedChain[0].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());
		seedChain[1].length = L2;
		seedChain[1].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());

		const float3 goal = RandomDir() * ((L1 + L2) * 0.8f);

		auto chainFABRIK = seedChain;
		auto chainCCD = seedChain;

		const auto resultFABRIK = SolveByName(IK::GetFABRIKSolver(), chainFABRIK, goal, 200, SOLVE_PRECISION);
		const auto resultCCD = SolveByName(IK::GetCCDSolver(), chainCCD, goal, 200, SOLVE_PRECISION);

		CHECK(resultFABRIK == IK::FABRIKResult::FOUND);
		CHECK(resultCCD == IK::FABRIKResult::FOUND);

		const float3 effectorFABRIK = chainFABRIK[0].orientation.Rotate(FwdVector) * L1 + chainFABRIK[1].orientation.Rotate(FwdVector) * L2;
		const float3 effectorCCD = chainCCD[0].orientation.Rotate(FwdVector) * L1 + chainCCD[1].orientation.Rotate(FwdVector) * L2;

		CHECK(effectorFABRIK.distance(goal) < SOLVE_PRECISION);
		CHECK(effectorCCD.distance(goal) < SOLVE_PRECISION);
	}
}

TEST_CASE("FABRIKSingleJoint")
{
	std::vector<IK::Bone> chain;
	auto result = IK::SolveFABRIK(chain, float3{10, 0, 0}, MAX_ITERATIONS, SOLVE_PRECISION);
	CHECK(result == IK::FABRIKResult::ERR_INPUTS);
}

TEST_CASE("FABRIKWrapperAxisContract")
{
	srand(777);
	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float L1 = 5.0f + randf() * 15.0f;
		const float L2 = 5.0f + randf() * 15.0f;

		const float3 dir0 = RandomDir();
		const float3 dir1 = RandomDir();

		const CQuaternion q0 = IK::MakeOrientationFromBoneDir(dir0);
		const CQuaternion q1 = IK::MakeOrientationFromBoneDir(dir1);
		// Validate wrapper conversion against the explicit expected rest axis.
		CHECK(q0.Rotate(FwdVector).distance(dir0) < 1e-3f);
		CHECK(q1.Rotate(FwdVector).distance(dir1) < 1e-3f);

		std::vector<IK::Bone> chain(2);
		chain[0].length = L1;
		chain[0].orientation = q0;
		chain[1].length = L2;
		chain[1].orientation = q1;

		float3 goal = RandomDir() * ((L1 + L2) * 0.8f);
		auto result = IK::SolveFABRIK(chain, goal, MAX_ITERATIONS, SOLVE_PRECISION);
		CHECK(result == IK::FABRIKResult::FOUND);

		if (result == IK::FABRIKResult::FOUND) {
			float3 effector =
				chain[0].orientation.Rotate(FwdVector) * L1 +
				chain[1].orientation.Rotate(FwdVector) * L2;
			CHECK(effector.distance(goal) < SOLVE_PRECISION);
		}
	}
}

TEST_CASE("FABRIK2BoneReach")
{
	srand(42);
	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float L1 = 5.0f + randf() * 15.0f;
		const float L2 = 5.0f + randf() * 15.0f;

		std::vector<IK::Bone> chain(2);
		chain[0].length = L1;
		chain[0].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());
		chain[1].length = L2;
		chain[1].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());

		float3 goal = RandomDir() * ((L1 + L2) * 0.8f);

		auto result = IK::SolveFABRIK(chain, goal, MAX_ITERATIONS, SOLVE_PRECISION);

		CHECK(result == IK::FABRIKResult::FOUND);
		float3 effector = chain[0].orientation.Rotate(FwdVector) * L1 + chain[1].orientation.Rotate(FwdVector) * L2;
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
		chain[0].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());
		chain[1].length = L2;
		chain[1].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());

		auto result = IK::SolveFABRIK(chain, goal, MAX_ITERATIONS, SOLVE_PRECISION);

		CHECK(result == IK::FABRIKResult::STRETCHING);
		float3 d0 = chain[0].orientation.Rotate(FwdVector);
		float3 d1 = chain[1].orientation.Rotate(FwdVector);
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
		chain[0].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());
		chain[1].length = L;
		chain[1].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());

		float3 goal = ZeroVector;

		auto result = IK::SolveFABRIK(chain, goal, 1000, SOLVE_PRECISION);

		CHECK(result == IK::FABRIKResult::FOUND);
		float3 effector = chain[0].orientation.Rotate(FwdVector) * L + chain[1].orientation.Rotate(FwdVector) * L;
		CHECK(effector.distance(goal) < SOLVE_PRECISION);
	}
}

TEST_CASE("FABRIKBallConstraint")
{
	srand(200);
	for (int trial = 0; trial < NUM_TRIALS_CONSTR; trial++) {
		const float coneAngle = math::PI * 0.4f;
		const float3 localConeAxis = FwdVector; // Along the bone

		std::vector<IK::Bone> chain(3);
		for (size_t i = 0; i < 3; i++) {
			chain[i].length = 10.0f;
			IK::BallJointConstraint bc;
			bc.coneAxis = localConeAxis;
			bc.coneAngle = coneAngle;
			chain[i].constraint = bc;
		}

		// Calculate a reachable goal via FK
		float3 goal;
		{
			CQuaternion cur;
			for (int i = 0; i < 3; i++) {
				// Random direction within cone
				float3 dir;
				float cosLim = math::cos(coneAngle);
				do {
					dir = RandomDir();
				} while (dir.dot(FwdVector) < cosLim);

				float3 dirW = cur.Rotate(dir);
				float3 oldW = cur.Rotate(FwdVector);
				cur = (CQuaternion::MakeFrom(oldW, dirW) * cur).Normalize();
				goal += dirW * chain[i].length;
			}
		}

		// Randomize initial chain state (within constraints)
		{
			CQuaternion cur;
			for (int i = 0; i < 3; i++) {
				float3 dir;
				float cosLim = math::cos(coneAngle);
				do {
					dir = RandomDir();
				} while (dir.dot(FwdVector) < cosLim);

				float3 dirW = cur.Rotate(dir);
				float3 oldW = cur.Rotate(FwdVector);
				cur = (CQuaternion::MakeFrom(oldW, dirW) * cur).Normalize();
				chain[i].orientation = cur;
			}
		}

		auto result = IK::SolveFABRIK(chain, goal, 1000, SOLVE_PRECISION);
		CHECK(result == IK::FABRIKResult::FOUND);
		if (result == IK::FABRIKResult::FOUND) {
			float3 effector = ZeroVector;
			for (size_t i = 0; i < 3; i++) {
				effector += chain[i].orientation.Rotate(FwdVector) * chain[i].length;
			}
			CHECK(effector.distance(goal) < SOLVE_PRECISION);

			for (size_t i = 0; i < 3; i++) {
				float3 dirW = chain[i].orientation.Rotate(FwdVector);
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
		const int numBones = 2 + rand() % 2; // 2 to 3 bones
		std::vector<IK::Bone> chain(numBones);

		for (int i = 0; i < numBones; i++) {
			chain[i].length = 5.0f + randf() * 5.0f;

			// Randomized axis - avoid being nearly parallel to bone (pure roll)
			float3 axis;
			do {
				axis = RandomDir();
			} while (std::abs(axis.z) > 0.8f); // ~36 degrees min angle

			IK::HingeJointConstraint hc;
			hc.axis = axis;
			hc.minAngle = -randf() * math::PI * 0.5f;
			hc.maxAngle =  randf() * math::PI * 0.5f;
			if (hc.minAngle > hc.maxAngle) std::swap(hc.minAngle, hc.maxAngle);
			chain[i].constraint = hc;
		}

		// Calculate a reachable goal
		float3 goal;
		{
			CQuaternion currentGoalOri;
			for (int i = 0; i < numBones; i++) {
				const auto& hc = std::get<IK::HingeJointConstraint>(chain[i].constraint);
				float angle = hc.minAngle + randf() * (hc.maxAngle - hc.minAngle);

				float3 restL = GetHingeRestDir(hc.axis);
				float3 resL  = restL.rotate<true>(angle, hc.axis);
				float3 resW  = currentGoalOri.Rotate(resL);

				float3 oldW = currentGoalOri.Rotate(FwdVector);
				currentGoalOri = (CQuaternion::MakeFrom(oldW, resW) * currentGoalOri).Normalize();
				goal += resW * chain[i].length;
			}
		}

		// Randomize initial chain orientation (within hinge constraints)
		{
			CQuaternion currentStartOri;
			for (int i = 0; i < numBones; i++) {
				const auto& hc = std::get<IK::HingeJointConstraint>(chain[i].constraint);
				float angle = hc.minAngle + randf() * (hc.maxAngle - hc.minAngle);

				float3 restL = GetHingeRestDir(hc.axis);
				float3 resL  = restL.rotate<true>(angle, hc.axis);
				float3 resW  = currentStartOri.Rotate(resL);

				float3 oldW = currentStartOri.Rotate(FwdVector);
				currentStartOri = (CQuaternion::MakeFrom(oldW, resW) * currentStartOri).Normalize();
				chain[i].orientation = currentStartOri;
			}
		}

		auto result = IK::SolveFABRIK(chain, goal, 10000, SOLVE_PRECISION * 4.0f);
		CHECK(result == IK::FABRIKResult::FOUND);

		for (size_t i = 0; i < numBones; i++) {
			const auto& hc = std::get<IK::HingeJointConstraint>(chain[i].constraint);
			float3 dirW = chain[i].orientation.Rotate(FwdVector);
			CQuaternion parentOri = (i > 0) ? chain[i - 1].orientation : CQuaternion();

			float3 dirL = parentOri.Inverse().Rotate(dirW);
			float3 axisL = hc.axis;
			float3 restL = GetHingeRestDir(axisL);

			float3 projL = dirL - axisL * dirL.dot(axisL);
			if (projL.SqLength() < 1e-4f) {
				projL = restL;
			} else {
				projL.Normalize();
			}
			float cosA = std::clamp(projL.dot(restL), -1.0f, 1.0f);
			float sinSign = restL.cross(projL).dot(axisL) >= 0.0f ? 1.0f : -1.0f;
			float angle = sinSign * math::acos(cosA);

			CHECK(angle >= hc.minAngle - 0.15f);
			CHECK(angle <= hc.maxAngle + 0.15f);
		}

		if (result == IK::FABRIKResult::FOUND) {
			float3 effector = ZeroVector;
			for (int i = 0; i < numBones; i++) {
				effector += chain[i].orientation.Rotate(FwdVector) * chain[i].length;
			}
			CHECK(effector.distance(goal) < SOLVE_PRECISION * 4.0f);
		}
	}
}
TEST_CASE("FABRIKBenchmarks", "[!benchmark]")
{
	const int benchmarkTrials = 2000;
	const float benchUnconstrainedPrecision = 1.0f;
	const float benchBallPrecision = 1.0f;
	const float benchHingePrecision = 2.0f;
	srand(1337);

	{
		uint64_t totalIters = 0;
		int successes = 0;
		for (int trial = 0; trial < benchmarkTrials; trial++) {
			const float L1 = 10.0f, L2 = 10.0f;
			std::vector<IK::Bone> chain(2);
			chain[0].length = L1;
			chain[0].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());
			chain[1].length = L2;
			chain[1].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());

			float3 goal = RandomDir() * 15.0f;
			uint32_t iters = 0;
			auto res = IK::SolveFABRIK(chain, goal, MAX_ITERATIONS, benchUnconstrainedPrecision, &iters);
			if (res == IK::FABRIKResult::FOUND) {
				totalIters += iters;
				successes++;
			}
		}
		printf("FABRIK 2-Bone Unconstrained:    avg iters = %.2f (%d/%d successes)\n", (double)totalIters / (successes ? successes : 1), successes, benchmarkTrials);
	}

	{
		uint64_t totalIters = 0;
		int successes = 0;
		const float coneAngle = math::PI * 0.4f;
		const float3 localConeAxis = FwdVector;
		for (int trial = 0; trial < benchmarkTrials; trial++) {
			std::vector<IK::Bone> chain(3);
			for (size_t i = 0; i < 3; i++) {
				chain[i].length = 10.0f;
				IK::BallJointConstraint bc;
				bc.coneAxis = localConeAxis;
				bc.coneAngle = coneAngle;
				chain[i].constraint = bc;
			}

			float3 goal;
			{
				CQuaternion cur;
				for (int i = 0; i < 3; i++) {
					float3 dir;
					float cosLim = math::cos(coneAngle);
					do {
						dir = RandomDir();
					} while (dir.dot(FwdVector) < cosLim);

					float3 dirW = cur.Rotate(dir);
					float3 oldW = cur.Rotate(FwdVector);
					cur = (CQuaternion::MakeFrom(oldW, dirW) * cur).Normalize();
					goal += dirW * chain[i].length;
				}
			}

			{
				CQuaternion cur;
				for (int i = 0; i < 3; i++) {
					float3 dir;
					float cosLim = math::cos(coneAngle);
					do {
						dir = RandomDir();
					} while (dir.dot(FwdVector) < cosLim);

					float3 dirW = cur.Rotate(dir);
					float3 oldW = cur.Rotate(FwdVector);
					cur = (CQuaternion::MakeFrom(oldW, dirW) * cur).Normalize();
					chain[i].orientation = cur;
				}
			}

			uint32_t iters = 0;
			auto res = IK::SolveFABRIK(chain, goal, 5000, benchBallPrecision, &iters);
			if (res == IK::FABRIKResult::FOUND) {
				totalIters += iters;
				successes++;
			}
		}
		printf("FABRIK 3-Bone Ball Constraints:  avg iters = %.2f (%d/%d successes)\n", (double)totalIters / (successes ? successes : 1), successes, benchmarkTrials);
	}

	{
		uint64_t totalIters = 0;
		int successes = 0;
		for (int trial = 0; trial < benchmarkTrials; trial++) {
			const int numBones = 3;
			std::vector<IK::Bone> chain(numBones);
			for (int i = 0; i < numBones; i++) {
				chain[i].length = 5.0f + randf() * 5.0f;
				float3 axis;
				do {
					axis = RandomDir();
				} while (std::abs(axis.z) > 0.8f);

				IK::HingeJointConstraint hc;
				hc.axis = axis;
				hc.minAngle = -randf() * math::PI * 0.5f;
				hc.maxAngle =  randf() * math::PI * 0.5f;
				if (hc.minAngle > hc.maxAngle) std::swap(hc.minAngle, hc.maxAngle);
				chain[i].constraint = hc;
			}

			float3 goal(0, 0, 0);
			{
				CQuaternion cur;
				for (int i = 0; i < numBones; i++) {
					const auto& hc = std::get<IK::HingeJointConstraint>(chain[i].constraint);
					float a = hc.minAngle + randf() * (hc.maxAngle - hc.minAngle);

					float3 restL = GetHingeRestDir(hc.axis);
					float3 resL  = restL.rotate<true>(a, hc.axis);
					float3 resW  = cur.Rotate(resL);
					float3 oldW  = cur.Rotate(FwdVector);
					cur = (CQuaternion::MakeFrom(oldW, resW) * cur).Normalize();

					goal += resW * chain[i].length;
				}
			}

			// Pre-scramble initial state within constraints
			{
				CQuaternion cur;
				for (int i = 0; i < numBones; i++) {
					const auto& hc = std::get<IK::HingeJointConstraint>(chain[i].constraint);
					float a = hc.minAngle + randf() * (hc.maxAngle - hc.minAngle);

					float3 restL = GetHingeRestDir(hc.axis);
					float3 resL  = restL.rotate<true>(a, hc.axis);
					float3 resW  = cur.Rotate(resL);
					float3 oldW  = cur.Rotate(FwdVector);
					cur = (CQuaternion::MakeFrom(oldW, resW) * cur).Normalize();

					chain[i].orientation = cur;
				}
			}

			uint32_t iters = 0;
			auto res = IK::SolveFABRIK(chain, goal, 30000, benchHingePrecision, &iters);
			if (res == IK::FABRIKResult::FOUND) {
				totalIters += iters;
				successes++;
			}
		}
		printf("FABRIK 3-Bone Hinge Constraints: avg iters = %.2f (%d/%d successes)\n", (double)totalIters / (successes ? successes : 1), successes, benchmarkTrials);
	}
	printf("FABRIK Benchmarks Finished.\n");
}

TEST_CASE("CCDBenchmarks", "[!benchmark]")
{
	const int benchmarkTrials = 2000;
	const float benchUnconstrainedPrecision = 1.0f;
	const float benchBallPrecision = 1.0f;
	const float benchHingePrecision = 2.0f;
	const IK::IIKSolver& solver = IK::GetCCDSolver();
	srand(2337);

	{
		uint64_t totalIters = 0;
		int successes = 0;
		for (int trial = 0; trial < benchmarkTrials; trial++) {
			const float L1 = 10.0f, L2 = 10.0f;
			std::vector<IK::Bone> chain(2);
			chain[0].length = L1;
			chain[0].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());
			chain[1].length = L2;
			chain[1].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());

			const float3 goal = RandomDir() * 15.0f;
			uint32_t iters = 0;
			const auto res = SolveByName(solver, chain, goal, MAX_ITERATIONS * 5, benchUnconstrainedPrecision, &iters);
			if (res == IK::FABRIKResult::FOUND) {
				totalIters += iters;
				successes++;
			}
		}
		printf("CCD 2-Bone Unconstrained:    avg iters = %.2f (%d/%d successes)\n", (double)totalIters / (successes ? successes : 1), successes, benchmarkTrials);
	}

	{
		uint64_t totalIters = 0;
		int successes = 0;
		const float coneAngle = math::PI * 0.4f;
		const float3 localConeAxis = FwdVector;
		for (int trial = 0; trial < benchmarkTrials; trial++) {
			std::vector<IK::Bone> chain(3);
			for (size_t i = 0; i < 3; i++) {
				chain[i].length = 10.0f;
				IK::BallJointConstraint bc;
				bc.coneAxis = localConeAxis;
				bc.coneAngle = coneAngle;
				chain[i].constraint = bc;
			}

			float3 goal;
			{
				CQuaternion cur;
				for (int i = 0; i < 3; i++) {
					float3 dir;
					const float cosLim = math::cos(coneAngle);
					do {
						dir = RandomDir();
					} while (dir.dot(FwdVector) < cosLim);

					const float3 dirW = cur.Rotate(dir);
					const float3 oldW = cur.Rotate(FwdVector);
					cur = (CQuaternion::MakeFrom(oldW, dirW) * cur).Normalize();
					goal += dirW * chain[i].length;
				}
			}

			{
				CQuaternion cur;
				for (int i = 0; i < 3; i++) {
					float3 dir;
					const float cosLim = math::cos(coneAngle);
					do {
						dir = RandomDir();
					} while (dir.dot(FwdVector) < cosLim);

					const float3 dirW = cur.Rotate(dir);
					const float3 oldW = cur.Rotate(FwdVector);
					cur = (CQuaternion::MakeFrom(oldW, dirW) * cur).Normalize();
					chain[i].orientation = cur;
				}
			}

			uint32_t iters = 0;
			const auto res = SolveByName(solver, chain, goal, 5000, benchBallPrecision, &iters);
			if (res == IK::FABRIKResult::FOUND) {
				totalIters += iters;
				successes++;
			}
		}
		printf("CCD 3-Bone Ball Constraints:  avg iters = %.2f (%d/%d successes)\n", (double)totalIters / (successes ? successes : 1), successes, benchmarkTrials);
	}

	{
		uint64_t totalIters = 0;
		int successes = 0;
		for (int trial = 0; trial < benchmarkTrials; trial++) {
			const int numBones = 3;
			std::vector<IK::Bone> chain(numBones);
			for (int i = 0; i < numBones; i++) {
				chain[i].length = 5.0f + randf() * 5.0f;
				float3 axis;
				do {
					axis = RandomDir();
				} while (std::abs(axis.z) > 0.8f);

				IK::HingeJointConstraint hc;
				hc.axis = axis;
				hc.minAngle = -randf() * math::PI * 0.5f;
				hc.maxAngle =  randf() * math::PI * 0.5f;
				if (hc.minAngle > hc.maxAngle)
					std::swap(hc.minAngle, hc.maxAngle);
				chain[i].constraint = hc;
			}

			float3 goal(0, 0, 0);
			{
				CQuaternion cur;
				for (int i = 0; i < numBones; i++) {
					const auto& hc = std::get<IK::HingeJointConstraint>(chain[i].constraint);
					const float a = hc.minAngle + randf() * (hc.maxAngle - hc.minAngle);

					const float3 restL = GetHingeRestDir(hc.axis);
					const float3 resL  = restL.rotate<true>(a, hc.axis);
					const float3 resW  = cur.Rotate(resL);
					const float3 oldW  = cur.Rotate(FwdVector);
					cur = (CQuaternion::MakeFrom(oldW, resW) * cur).Normalize();

					goal += resW * chain[i].length;
				}
			}

			{
				CQuaternion cur;
				for (int i = 0; i < numBones; i++) {
					const auto& hc = std::get<IK::HingeJointConstraint>(chain[i].constraint);
					const float a = hc.minAngle + randf() * (hc.maxAngle - hc.minAngle);

					const float3 restL = GetHingeRestDir(hc.axis);
					const float3 resL  = restL.rotate<true>(a, hc.axis);
					const float3 resW  = cur.Rotate(resL);
					const float3 oldW  = cur.Rotate(FwdVector);
					cur = (CQuaternion::MakeFrom(oldW, resW) * cur).Normalize();

					chain[i].orientation = cur;
				}
			}

			uint32_t iters = 0;
			const auto res = SolveByName(solver, chain, goal, 30000, benchHingePrecision, &iters);
			if (res == IK::FABRIKResult::FOUND) {
				totalIters += iters;
				successes++;
			}
		}
		printf("CCD 3-Bone Hinge Constraints: avg iters = %.2f (%d/%d successes)\n", (double)totalIters / (successes ? successes : 1), successes, benchmarkTrials);
	}

	printf("CCD Benchmarks Finished.\n");
}

TEST_CASE("CCDVsFABRIKBenchmarks", "[!benchmark]")
{
	const int benchmarkTrials = 2000;
	const float precision = 1.0f;
	const IK::IIKSolver& fabrik = IK::GetFABRIKSolver();
	const IK::IIKSolver& ccd = IK::GetCCDSolver();
	srand(3347);

	uint64_t fabrikIters = 0;
	uint64_t ccdIters = 0;
	int fabrikSuccesses = 0;
	int ccdSuccesses = 0;
	clock_t fabrikTicks = 0;
	clock_t ccdTicks = 0;

	for (int trial = 0; trial < benchmarkTrials; ++trial) {
		const float L1 = 10.0f, L2 = 10.0f;
		std::vector<IK::Bone> seedChain(2);
		seedChain[0].length = L1;
		seedChain[0].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());
		seedChain[1].length = L2;
		seedChain[1].orientation = CQuaternion::MakeFrom(FwdVector, RandomDir());
		const float3 goal = RandomDir() * 15.0f;

		{
			auto chain = seedChain;
			uint32_t iters = 0;
			const clock_t t0 = std::clock();
			const auto res = SolveByName(fabrik, chain, goal, MAX_ITERATIONS, precision, &iters);
			fabrikTicks += (std::clock() - t0);
			if (res == IK::FABRIKResult::FOUND) {
				fabrikIters += iters;
				fabrikSuccesses++;
			}
		}

		{
			auto chain = seedChain;
			uint32_t iters = 0;
			const clock_t t0 = std::clock();
			const auto res = SolveByName(ccd, chain, goal, MAX_ITERATIONS * 5, precision, &iters);
			ccdTicks += (std::clock() - t0);
			if (res == IK::FABRIKResult::FOUND) {
				ccdIters += iters;
				ccdSuccesses++;
			}
		}
	}

	const double fabrikAvgIters = static_cast<double>(fabrikIters) / static_cast<double>(fabrikSuccesses ? fabrikSuccesses : 1);
	const double ccdAvgIters = static_cast<double>(ccdIters) / static_cast<double>(ccdSuccesses ? ccdSuccesses : 1);
	const double fabrikMs = 1000.0 * static_cast<double>(fabrikTicks) / static_cast<double>(CLOCKS_PER_SEC);
	const double ccdMs = 1000.0 * static_cast<double>(ccdTicks) / static_cast<double>(CLOCKS_PER_SEC);

	printf("FABRIK: successes=%d/%d avgIters=%.2f cpuMs=%.2f\n", fabrikSuccesses, benchmarkTrials, fabrikAvgIters, fabrikMs);
	printf("CCD:    successes=%d/%d avgIters=%.2f cpuMs=%.2f\n", ccdSuccesses, benchmarkTrials, ccdAvgIters, ccdMs);
	printf(
		"CCD vs FABRIK: iterRatio=%.3f timeRatio=%.3f\n",
		ccdAvgIters / std::max(fabrikAvgIters, static_cast<double>(1e-6f)),
		ccdMs / std::max(fabrikMs, static_cast<double>(1e-6f))
	);
}
