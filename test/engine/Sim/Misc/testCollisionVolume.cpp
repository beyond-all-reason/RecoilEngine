/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Units/Unit.h"
#include "Rendering/Models/LocalModelPiece.hpp"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"

#include <catch_amalgamated.hpp>

// Link seams. CollisionVolume.cpp calls these only on its surface-distance
// paths, which nothing here takes; defining them lets the file link without
// the rest of the engine. ~CUnit is CUnit's key function, so defining this
// member does not emit CUnit's vtable here.
CMatrix44f CUnit::GetTransformMatrix(bool, bool) const { return CMatrix44f(); }
const CMatrix44f& LocalModelPiece::GetModelSpaceMatrix() const { static const CMatrix44f m; return m; }

using CV = CollisionVolume;

static CV MakeVolume(int type, const float3& scales = ZeroVector)
{
	CV v;
	v.InitShape(scales, ZeroVector, type, CV::COLVOL_HITTEST_CONT, CV::COLVOL_AXIS_Z);
	return v;
}

TEST_CASE("CollisionVolume_PolygonPieceFlagSurvivesAssignmentAndInitDefault")
{
	// what ParseCollisionVolume produces for collisionVolumeType = "polygon":
	// an unscaled sphere that defers to the piece tree
	CV def = MakeVolume(CV::COLVOL_TYPE_SPHERE);
	def.SetUsePolygonPieceVolumes(true);
	def.SetDefaultToPieceTree(true);

	// CUnit::PreInit and CFeature::Initialize assign the def's volume to the
	// instance's (operator=, not the copy constructor), then call InitDefault
	CV inst;
	inst = def;
	CHECK(inst.UsePolygonPieceVolumes());

	REQUIRE(inst.DefaultToSphere());
	inst.InitDefault(float4(25.0f, 10.0f, 32.0f, 32.0f));

	CHECK(inst.UsePolygonPieceVolumes());
	CHECK(inst.DefaultToPieceTree());
}

TEST_CASE("CollisionVolume_PolygonTypeDoesNotSurviveInitDefault")
{
	// why the choice is recorded as a flag rather than the volume's type:
	// an unscaled polygon volume counts as default, and InitDefault replaces
	// its type with a sphere
	CV v = MakeVolume(CV::COLVOL_TYPE_POLYGON);
	REQUIRE(v.GetVolumeType() == CV::COLVOL_TYPE_POLYGON);
	REQUIRE(v.DefaultToSphere());

	v.InitDefault(float4(25.0f, 10.0f, 32.0f, 32.0f));

	CHECK(v.GetVolumeType() == CV::COLVOL_TYPE_SPHERE);
}

TEST_CASE("CollisionVolume_PolygonBoundingRadiusIsBoxHalfDiagonal")
{
	// a piece-less polygon volume is tested as a box of its scales, so the
	// broad phase must bound that box
	const float3 scales = {40.0f, 25.0f, 60.0f};
	const CV poly = MakeVolume(CV::COLVOL_TYPE_POLYGON, scales);
	const CV box  = MakeVolume(CV::COLVOL_TYPE_BOX, scales);

	const float expected = math::sqrt(20.0f * 20.0f + 12.5f * 12.5f + 30.0f * 30.0f);

	CHECK(poly.GetBoundingRadius() == Catch::Approx(expected));
	CHECK(poly.GetBoundingRadiusSq() == Catch::Approx(expected * expected));
	CHECK(poly.GetBoundingRadius() == Catch::Approx(box.GetBoundingRadius()));
}

TEST_CASE("CollisionVolume_PolygonBoundingRadiusDoesNotKeepPreviousShape")
{
	// a volume switched to polygon at runtime (Lua's Set*CollisionVolumeData
	// and SetPieceCollisionData call InitShape) must not keep the radius of
	// the shape it had before
	CV v = MakeVolume(CV::COLVOL_TYPE_SPHERE, float3{200.0f, 200.0f, 200.0f});
	REQUIRE(v.GetBoundingRadius() == Catch::Approx(100.0f));

	v.InitShape(float3{4.0f, 4.0f, 4.0f}, ZeroVector, CV::COLVOL_TYPE_POLYGON, CV::COLVOL_HITTEST_CONT, CV::COLVOL_AXIS_Z);

	CHECK(v.GetBoundingRadius() == Catch::Approx(math::sqrt(3.0f * 2.0f * 2.0f)));
}

TEST_CASE("CollisionVolume_HasCustomType")
{
	CHECK_FALSE(MakeVolume(CV::COLVOL_TYPE_SPHERE).HasCustomType());

	// equal scales would turn an ellipsoid into a sphere in FixTypeAndScale
	CHECK(MakeVolume(CV::COLVOL_TYPE_ELLIPSOID, float3{4.0f, 6.0f, 8.0f}).HasCustomType());
	CHECK(MakeVolume(CV::COLVOL_TYPE_CYLINDER).HasCustomType());
	CHECK(MakeVolume(CV::COLVOL_TYPE_BOX).HasCustomType());
	CHECK(MakeVolume(CV::COLVOL_TYPE_POLYGON).HasCustomType());
}
