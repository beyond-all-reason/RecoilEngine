/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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

// Generate a random unit direction within a cone of half-angle `coneAngle`
// around `axis` (which must be normalized).
static float3 RandomDirInCone(const float3& axis, float coneAngle)
{
	// Build perpendicular basis
	float3 perp = axis.cross(float3(1.0f, 0.0f, 0.0f));
	if (perp.SqLength() < 0.01f)
		perp = axis.cross(float3(0.0f, 1.0f, 0.0f));
	perp.Normalize();
	float3 perp2 = axis.cross(perp);
	perp2.Normalize();

	float angle   = randf() * coneAngle;
	float azimuth = randf() * 2.0f * math::PI;

	float3 result = axis * std::cos(angle)
	              + (perp * std::cos(azimuth) + perp2 * std::sin(azimuth)) * std::sin(angle);
	result.SafeNormalize();
	return result;
}

// Generate a random unit direction in the hinge plane at an angle within
// [minAngle, maxAngle] relative to `restDir`, around `hingeAxis`.
// `restDir` and `hingeAxis` must be normalized and perpendicular-ish.
static float3 RandomDirInHinge(const float3& hingeAxis, const float3& restDir,
                               float minAngle, float maxAngle)
{
	float3 restProj = restDir - hingeAxis * restDir.dot(hingeAxis);
	restProj.SafeNormalize();

	float angle = minAngle + randf() * (maxAngle - minAngle);
	float3 result = restProj * std::cos(angle) + hingeAxis.cross(restProj) * std::sin(angle);
	result.SafeNormalize();
	return result;
}

// Scramble all positions except root, spread randomly in a sphere.
static void ScramblePositions(std::vector<float3>& positions,
                              const std::vector<float>& segLengths)
{
	for (size_t i = 1; i < positions.size(); i++) {
		float3 dir = RandomDir();
		positions[i] = positions[i - 1] + dir * segLengths[i - 1];
	}
}

// Build a valid chain reaching a goal. Returns positions with the effector
// exactly at a reachable point. Segment lengths are randomised.
struct ChainSetup {
	std::vector<float3> positions;
	std::vector<float>  segLengths;
	float3 goal;
};

static ChainSetup MakeValidChain(size_t numJoints, const float3& root,
                                 float minSeg = 5.0f, float maxSeg = 20.0f)
{
	ChainSetup cs;
	cs.positions.resize(numJoints);
	cs.segLengths.resize(numJoints - 1);

	cs.positions[0] = root;
	for (size_t i = 0; i < numJoints - 1; i++) {
		cs.segLengths[i] = minSeg + randf() * (maxSeg - minSeg);
		float3 dir = RandomDir();
		cs.positions[i + 1] = cs.positions[i] + dir * cs.segLengths[i];
	}
	cs.goal = cs.positions.back();
	return cs;
}

// Verify that segment lengths are preserved after solving.
static void CheckSegmentLengths(const std::vector<float3>& positions,
                                const std::vector<float>& segLengths,
                                float tolerance = 0.1f)
{
	for (size_t i = 0; i < segLengths.size(); i++) {
		const float actual = positions[i].distance(positions[i + 1]);
		CHECK(std::abs(actual - segLengths[i]) < tolerance);
	}
}

// Verify ball constraint satisfaction for bone direction at joint i.
static bool IsBallConstraintSatisfied(const float3& boneDir,
                                      const IK::BallJointConstraint& bc,
                                      float tolerance = 0.02f)
{
	float3 axis = bc.coneAxis;
	axis.SafeNormalize();
	const float cosActual = boneDir.dot(axis);
	const float cosLimit  = std::cos(bc.coneAngle);
	return cosActual >= (cosLimit - tolerance);
}

// Verify hinge constraint satisfaction for bone direction at joint i.
static bool IsHingeConstraintSatisfied(const float3& boneDir,
                                       const float3& restDir,
                                       const IK::HingeJointConstraint& hc,
                                       float tolerance = 0.05f)
{
	float3 axis = hc.axis;
	axis.SafeNormalize();

	float3 dirProj  = boneDir - axis * boneDir.dot(axis);
	float3 restProj = restDir - axis * restDir.dot(axis);
	dirProj.SafeNormalize();
	restProj.SafeNormalize();

	const float cosA    = std::clamp(dirProj.dot(restProj), -1.0f, 1.0f);
	const float sinSign = restProj.cross(dirProj).dot(axis) >= 0.0f ? 1.0f : -1.0f;
	const float angle   = sinSign * std::acos(cosA);

	return angle >= (hc.minAngle - tolerance) && angle <= (hc.maxAngle + tolerance);
}


// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static constexpr int    NUM_TRIALS       = 100000;
static constexpr float  SOLVE_PRECISION  = 4.0f;
static constexpr int    MAX_ITERATIONS   = 40;
static constexpr float  SEG_LEN_TOL      = 0.1f;

// ---- Test 1: Single joint chain returns FAILED ----

TEST_CASE("FABRIKSingleJoint")
{
	std::vector<float3> positions = {float3{0, 0, 0}};
	std::vector<float>  segLengths;
	std::vector<IK::Constraint> constraints;
	float3 rootDir{0, 1, 0};

	auto result = IK::SolveFABRIK(positions, segLengths, constraints, rootDir,
	                               float3{10, 0, 0}, MAX_ITERATIONS, SOLVE_PRECISION);
	CHECK(result == IK::FABRIKResult::FAILED);
}


// ---- Test 2: 2-bone reachable goal ----

TEST_CASE("FABRIK2BoneReach")
{
	srand(42);

	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float3 root{srandf() * 100.0f, srandf() * 100.0f, srandf() * 100.0f};
		auto cs = MakeValidChain(3, root);

		// Scramble and solve
		ScramblePositions(cs.positions, cs.segLengths);
		float3 rootDir = RandomDir();
		auto result = IK::SolveFABRIK(cs.positions, cs.segLengths, {}, rootDir,
		                               cs.goal, MAX_ITERATIONS, SOLVE_PRECISION);

		CHECK(result == IK::FABRIKResult::FOUND);
		CHECK(cs.positions[0].distance(root) < 0.01f);  // root pinned
		CHECK(cs.positions.back().distance(cs.goal) < SOLVE_PRECISION);
		CheckSegmentLengths(cs.positions, cs.segLengths, SEG_LEN_TOL);
	}
}


// ---- Test 3: 2-bone unreachable (stretch) ----

TEST_CASE("FABRIK2BoneStretch")
{
	srand(123);

	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float3 root{srandf() * 50.0f, srandf() * 50.0f, srandf() * 50.0f};
		const float L1 = 5.0f + randf() * 15.0f;
		const float L2 = 5.0f + randf() * 15.0f;
		const float totalLen = L1 + L2;

		// Goal well beyond reach
		float3 goalDir = RandomDir();
		float3 goal = root + goalDir * (totalLen + 10.0f + randf() * 50.0f);

		std::vector<float3> positions = {root, root + RandomDir() * L1, root + RandomDir() * (L1 + L2)};
		std::vector<float>  segLengths = {L1, L2};

		auto result = IK::SolveFABRIK(positions, segLengths, {}, float3{0, 1, 0},
		                               goal, MAX_ITERATIONS, SOLVE_PRECISION);

		CHECK(result == IK::FABRIKResult::STRETCHING);
		CHECK(positions[0].distance(root) < 0.01f);  // root pinned
		CheckSegmentLengths(positions, segLengths, SEG_LEN_TOL);

		// All joints should be roughly collinear toward goal
		float3 toGoal = (goal - root);
		toGoal.SafeNormalize();
		for (size_t i = 1; i < positions.size(); i++) {
			float3 toJoint = (positions[i] - root);
			toJoint.SafeNormalize();
			CHECK(toJoint.dot(toGoal) > 0.95f);
		}
	}
}


// ---- Test 4: 3-bone reachable goal ----

TEST_CASE("FABRIK3BoneReach")
{
	srand(7);

	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float3 root{srandf() * 100.0f, srandf() * 100.0f, srandf() * 100.0f};
		auto cs = MakeValidChain(4, root);

		ScramblePositions(cs.positions, cs.segLengths);
		float3 rootDir = RandomDir();
		auto result = IK::SolveFABRIK(cs.positions, cs.segLengths, {}, rootDir,
		                               cs.goal, MAX_ITERATIONS, SOLVE_PRECISION);

		CHECK(result == IK::FABRIKResult::FOUND);
		CHECK(cs.positions[0].distance(root) < 0.01f);
		CHECK(cs.positions.back().distance(cs.goal) < SOLVE_PRECISION);
		CheckSegmentLengths(cs.positions, cs.segLengths, SEG_LEN_TOL);
	}
}


// ---- Test 5: Goal at root position ----

TEST_CASE("FABRIKGoalAtRoot")
{
	srand(99);

	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float3 root{srandf() * 50.0f, srandf() * 50.0f, srandf() * 50.0f};
		// Use equal segment lengths to guarantee root is always reachable
		const float segLen = 5.0f + randf() * 10.0f;
		const size_t numJoints = 3u + static_cast<size_t>(rand() % 3); // 3-5 joints

		std::vector<float3> positions(numJoints);
		std::vector<float>  segLengths(numJoints - 1, segLen);
		positions[0] = root;
		for (size_t i = 1; i < numJoints; i++)
			positions[i] = root + RandomDir() * segLen * static_cast<float>(i);

		const float3 goal = root; // goal == root

		ScramblePositions(positions, segLengths);
		positions[0] = root; // keep root pinned for scramble

		auto result = IK::SolveFABRIK(positions, segLengths, {}, float3{0, 1, 0},
		                               goal, MAX_ITERATIONS * 2, SOLVE_PRECISION);

		CHECK(result == IK::FABRIKResult::FOUND);
		CHECK(positions[0].distance(root) < 0.01f);
		// With equal segments and >=2 bones, the effector can reach the root
		CHECK(positions.back().distance(goal) < SOLVE_PRECISION);
		CheckSegmentLengths(positions, segLengths, SEG_LEN_TOL);
	}
}


// ---- Test 6: Goal at current effector (no-op) ----

TEST_CASE("FABRIKGoalAtEffector")
{
	srand(55);

	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float3 root{srandf() * 100.0f, srandf() * 100.0f, srandf() * 100.0f};
		auto cs = MakeValidChain(4, root);

		// Goal is already at the effector
		const float3 goal = cs.positions.back();

		// Save original positions
		auto origPositions = cs.positions;

		float3 rootDir = RandomDir();
		auto result = IK::SolveFABRIK(cs.positions, cs.segLengths, {}, rootDir,
		                               goal, MAX_ITERATIONS, SOLVE_PRECISION);

		CHECK(result == IK::FABRIKResult::FOUND);
		CHECK(cs.positions.back().distance(goal) < SOLVE_PRECISION);
		CheckSegmentLengths(cs.positions, cs.segLengths, SEG_LEN_TOL);
	}
}


// ---- Test 7: Ball joint constraints ----

TEST_CASE("FABRIKBallConstraint")
{
	srand(200);

	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float3 root{srandf() * 50.0f, srandf() * 50.0f, srandf() * 50.0f};
		const size_t numJoints = 4;

		// Cone axis along Y, generous cone angle
		const float coneAngle = math::PI * 0.3f + randf() * math::PI * 0.35f; // 54-117 deg

		// Build valid config: each bone direction is within the cone
		float3 coneAxis = RandomDir();
		std::vector<float3> validPositions(numJoints);
		std::vector<float>  segLengths(numJoints - 1);
		validPositions[0] = root;

		for (size_t i = 0; i < numJoints - 1; i++) {
			segLengths[i] = 8.0f + randf() * 12.0f;
			float3 dir = RandomDirInCone(coneAxis, coneAngle * 0.8f); // stay inside
			validPositions[i + 1] = validPositions[i] + dir * segLengths[i];
		}
		float3 goal = validPositions.back();

		// Set up constraints: ball constraint at each joint
		std::vector<IK::Constraint> constraints(numJoints);
		for (size_t i = 0; i < numJoints; i++) {
			IK::BallJointConstraint bc;
			bc.coneAxis  = coneAxis;
			bc.coneAngle = coneAngle;
			constraints[i] = bc;
		}

		// Scramble and solve
		auto positions = validPositions;
		ScramblePositions(positions, segLengths);
		float3 rootDir = RandomDirInCone(coneAxis, coneAngle);

		auto result = IK::SolveFABRIK(positions, segLengths, constraints, rootDir,
		                               goal, MAX_ITERATIONS * 2, SOLVE_PRECISION * 2.0f);

		CHECK(result == IK::FABRIKResult::FOUND);
		CHECK(positions[0].distance(root) < 0.01f);
		CheckSegmentLengths(positions, segLengths, SEG_LEN_TOL);

		// Verify ball constraints are satisfied on each bone
		for (size_t i = 0; i < numJoints - 1; i++) {
			float3 boneDir = (positions[i + 1] - positions[i]);
			boneDir.SafeNormalize();
			const auto& bc = std::get<IK::BallJointConstraint>(constraints[i]);
			CHECK(IsBallConstraintSatisfied(boneDir, bc, 0.05f));
		}
	}
}


// ---- Test 8: Hinge joint constraints ----

TEST_CASE("FABRIKHingeConstraint")
{
	srand(300);

	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float3 root{srandf() * 50.0f, srandf() * 50.0f, srandf() * 50.0f};
		const size_t numJoints = 4;

		// Hinge axis (e.g., pointing sideways)
		float3 hingeAxis = RandomDir();
		const float minAngle = -(math::PI * 0.1f + randf() * math::PI * 0.3f); // -18 to -72 deg
		const float maxAngle =  (math::PI * 0.1f + randf() * math::PI * 0.3f); //  18 to  72 deg

		// Build valid config: each bone direction satisfies the hinge constraint
		std::vector<float3> validPositions(numJoints);
		std::vector<float>  segLengths(numJoints - 1);
		validPositions[0] = root;

		// First bone: arbitrary direction (will serve as rest dir for joint 1)
		float3 prevDir = RandomDir();
		segLengths[0] = 8.0f + randf() * 12.0f;
		validPositions[1] = validPositions[0] + prevDir * segLengths[0];

		for (size_t i = 1; i < numJoints - 1; i++) {
			segLengths[i] = 8.0f + randf() * 12.0f;
			// restDir for joint i is the previous bone direction
			float3 dir = RandomDirInHinge(hingeAxis, prevDir, minAngle * 0.5f, maxAngle * 0.5f);
			validPositions[i + 1] = validPositions[i] + dir * segLengths[i];
			prevDir = dir;
		}
		float3 goal = validPositions.back();

		// Constraints: hinge at joints 1..n-2, none at root and effector
		std::vector<IK::Constraint> constraints(numJoints, std::monostate{});
		for (size_t i = 1; i < numJoints - 1; i++) {
			IK::HingeJointConstraint hc;
			hc.axis     = hingeAxis;
			hc.minAngle = minAngle;
			hc.maxAngle = maxAngle;
			constraints[i] = hc;
		}

		// Scramble and solve
		auto positions = validPositions;
		ScramblePositions(positions, segLengths);

		// bindPoseRootDir = initial first bone direction
		float3 initBoneDir = (validPositions[1] - validPositions[0]);
		initBoneDir.SafeNormalize();

		auto result = IK::SolveFABRIK(positions, segLengths, constraints, initBoneDir,
		                               goal, MAX_ITERATIONS * 2, SOLVE_PRECISION * 2.0f);

		CHECK(result == IK::FABRIKResult::FOUND);
		CHECK(positions[0].distance(root) < 0.01f);
		CheckSegmentLengths(positions, segLengths, SEG_LEN_TOL);

		// Verify hinge constraints on interior joints
		for (size_t i = 1; i < numJoints - 1; i++) {
			float3 boneDir = (positions[i + 1] - positions[i]);
			boneDir.SafeNormalize();
			float3 restDir = (positions[i] - positions[i - 1]);
			restDir.SafeNormalize();
			const auto& hc = std::get<IK::HingeJointConstraint>(constraints[i]);
			CHECK(IsHingeConstraintSatisfied(boneDir, restDir, hc, 0.1f));
		}
	}
}


// ---- Test 9: Zero-length segment (degenerate case) ----

TEST_CASE("FABRIKZeroLengthSegment")
{
	srand(400);

	for (int trial = 0; trial < 100; trial++) {
		const float3 root{srandf() * 50.0f, srandf() * 50.0f, srandf() * 50.0f};

		// 3 joints, middle segment has zero length
		const float L1 = 10.0f + randf() * 10.0f;
		const float L2 = 0.0f;  // zero-length
		const float L3 = 10.0f + randf() * 10.0f;

		float3 dir1 = RandomDir();
		float3 dir2 = RandomDir();
		std::vector<float3> positions = {
			root,
			root + dir1 * L1,
			root + dir1 * L1,           // same as joint 1 (zero-length bone)
			root + dir1 * L1 + dir2 * L3
		};
		std::vector<float> segLengths = {L1, L2, L3};
		float3 goal = positions.back();

		ScramblePositions(positions, segLengths);
		auto result = IK::SolveFABRIK(positions, segLengths, {}, float3{0, 1, 0},
		                               goal, MAX_ITERATIONS, SOLVE_PRECISION);

		// Should not crash; may or may not converge perfectly
		CHECK((result == IK::FABRIKResult::FOUND || result == IK::FABRIKResult::STRETCHING));
		CHECK(positions[0].distance(root) < 0.01f);
		CheckSegmentLengths(positions, segLengths, SEG_LEN_TOL);
	}
}


// ---- Test 10: Many-bone chain (10-20 segments) ----

TEST_CASE("FABRIKManyBones")
{
	srand(500);

	for (int trial = 0; trial < NUM_TRIALS; trial++) {
		const float3 root{srandf() * 100.0f, srandf() * 100.0f, srandf() * 100.0f};
		const size_t numJoints = 11u + static_cast<size_t>(rand() % 10); // 11-20 joints

		auto cs = MakeValidChain(numJoints, root, 3.0f, 10.0f);

		ScramblePositions(cs.positions, cs.segLengths);
		float3 rootDir = RandomDir();
		auto result = IK::SolveFABRIK(cs.positions, cs.segLengths, {}, rootDir,
		                               cs.goal, MAX_ITERATIONS * 3, SOLVE_PRECISION);

		CHECK(result == IK::FABRIKResult::FOUND);
		CHECK(cs.positions[0].distance(root) < 0.01f);
		CHECK(cs.positions.back().distance(cs.goal) < SOLVE_PRECISION);
		CheckSegmentLengths(cs.positions, cs.segLengths, SEG_LEN_TOL);
	}
}


// ---- Test 11: Segment length preservation across solve types ----

TEST_CASE("FABRIKSegmentLengthPreservation")
{
	srand(600);

	SECTION("Reachable") {
		for (int trial = 0; trial < NUM_TRIALS; trial++) {
			auto cs = MakeValidChain(5, float3{0, 0, 0});
			auto origLengths = cs.segLengths;
			ScramblePositions(cs.positions, cs.segLengths);
			IK::SolveFABRIK(cs.positions, cs.segLengths, {}, RandomDir(),
			                 cs.goal, MAX_ITERATIONS, SOLVE_PRECISION);
			// Segment lengths (the constraint) must be exact to within tolerance
			for (size_t i = 0; i < origLengths.size(); i++) {
				const float actual = cs.positions[i].distance(cs.positions[i + 1]);
				CHECK(std::abs(actual - origLengths[i]) < 0.05f);
			}
		}
	}

	SECTION("Unreachable") {
		for (int trial = 0; trial < NUM_TRIALS; trial++) {
			auto cs = MakeValidChain(5, float3{0, 0, 0}, 5.0f, 10.0f);
			auto origLengths = cs.segLengths;
			float totalLen = 0.0f;
			for (auto l : origLengths) totalLen += l;

			// Goal beyond reach
			float3 goal = float3{0, 0, 0} + RandomDir() * (totalLen + 50.0f);
			ScramblePositions(cs.positions, cs.segLengths);
			IK::SolveFABRIK(cs.positions, cs.segLengths, {}, RandomDir(),
			                 goal, MAX_ITERATIONS, SOLVE_PRECISION);
			for (size_t i = 0; i < origLengths.size(); i++) {
				const float actual = cs.positions[i].distance(cs.positions[i + 1]);
				CHECK(std::abs(actual - origLengths[i]) < 0.05f);
			}
		}
	}
}
