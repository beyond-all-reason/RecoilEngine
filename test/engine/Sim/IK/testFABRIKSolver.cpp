/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>

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

static float ComputeHingeAngleLocal(const float3& dirL, const IK::HingeJointConstraint& hc)
{
	float3 axisL = hc.axis;
	axisL.SafeNormalize();
	float3 restL = GetHingeRestDir(axisL);
	float3 projL = dirL - axisL * dirL.dot(axisL);
	if (projL.SqLength() < 1e-5f) {
		projL = restL;
	} else {
		projL.Normalize();
	}

	const float dot = std::clamp(projL.dot(restL), -1.0f, 1.0f);
	const float det = restL.cross(projL).dot(axisL);
	return math::atan2(det, dot);
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
		printf("  2-Bone Unconstrained:    avg iters = %.2f (%d/%d successes)\n", (double)totalIters / (successes ? successes : 1), successes, benchmarkTrials);
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
		printf("  3-Bone Ball Constraints:  avg iters = %.2f (%d/%d successes)\n", (double)totalIters / (successes ? successes : 1), successes, benchmarkTrials);
	}

	{
		uint64_t totalIters = 0;
		int successes = 0;
		bool dumpedFirstFailure = false;
		const bool debugHingeFail = []() {
			const char* env = std::getenv("FABRIK_DEBUG_HINGE_FAIL");
			return (env != nullptr) && (env[0] != '\0') && (env[0] != '0');
		}();
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

			const std::vector<IK::Bone> chainInitial = chain;
			uint32_t iters = 0;
			auto res = IK::SolveFABRIK(chain, goal, 30000, benchHingePrecision, &iters);
			if (res == IK::FABRIKResult::FOUND) {
				totalIters += iters;
				successes++;
			} else if (debugHingeFail && !dumpedFirstFailure) {
				dumpedFirstFailure = true;
				auto EvalEffector = [](const std::vector<IK::Bone>& ch) {
					float3 eff = ZeroVector;
					for (const auto& b: ch)
						eff += b.orientation.Rotate(FwdVector) * b.length;
					return eff;
				};

				const float3 startEff = EvalEffector(chainInitial);
				const float3 endEff = EvalEffector(chain);
				std::printf(
					"[FABRIK_DEBUG_HINGE_FAIL] trial=%d result=%d iters=%u precision=%.6f\n"
					"  goal=(%.6f, %.6f, %.6f)\n"
					"  startEff=(%.6f, %.6f, %.6f) startErr=%.6f\n"
					"  endEff=(%.6f, %.6f, %.6f) endErr=%.6f\n",
					trial, int(res), iters, benchHingePrecision,
					goal.x, goal.y, goal.z,
					startEff.x, startEff.y, startEff.z, startEff.distance(goal),
					endEff.x, endEff.y, endEff.z, endEff.distance(goal)
				);

				for (int i = 0; i < numBones; i++) {
					const auto& hc = std::get<IK::HingeJointConstraint>(chainInitial[i].constraint);
					const CQuaternion parentStart = (i > 0) ? chainInitial[i - 1].orientation : CQuaternion();
					const CQuaternion parentEnd = (i > 0) ? chain[i - 1].orientation : CQuaternion();
					const float3 startDirL = parentStart.Inverse().Rotate(chainInitial[i].orientation.Rotate(FwdVector));
					const float3 endDirL = parentEnd.Inverse().Rotate(chain[i].orientation.Rotate(FwdVector));
					const float startAngle = ComputeHingeAngleLocal(startDirL, hc);
					const float endAngle = ComputeHingeAngleLocal(endDirL, hc);

					std::printf(
						"  bone[%d] len=%.6f axis=(%.6f, %.6f, %.6f) min=%.6f max=%.6f "
						"startAngle=%.6f endAngle=%.6f\n",
						i, chainInitial[i].length,
						hc.axis.x, hc.axis.y, hc.axis.z, hc.minAngle, hc.maxAngle,
						startAngle, endAngle
					);
				}

				const char* oldTrace = std::getenv("FABRIK_DEBUG_TRACE");
				const bool hadTrace = (oldTrace != nullptr) && (oldTrace[0] != '\0') && (oldTrace[0] != '0');
				if (!hadTrace)
					setenv("FABRIK_DEBUG_TRACE", "1", 1);

				std::vector<IK::Bone> rerunChain = chainInitial;
				uint32_t rerunIters = 0;
				const auto rerunRes = IK::SolveFABRIK(rerunChain, goal, 30000, benchHingePrecision, &rerunIters);
				const float3 rerunEff = EvalEffector(rerunChain);
				std::printf(
					"[FABRIK_DEBUG_HINGE_FAIL] rerun result=%d iters=%u endErr=%.6f\n",
					int(rerunRes), rerunIters, rerunEff.distance(goal)
				);

				if (!hadTrace)
					unsetenv("FABRIK_DEBUG_TRACE");
			}
		}
		printf("  3-Bone Hinge Constraints: avg iters = %.2f (%d/%d successes)\n", (double)totalIters / (successes ? successes : 1), successes, benchmarkTrials);
	}
	printf("Benchmarks Finished.\n");
}
