/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <array>

#include "Game/Camera.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Log/ILog.h"
#include "System/Matrix44f.h"
#include "System/TimeProfiler.h"
#include "System/float3.h"
#include "System/Misc/SpringTime.h"
#include "System/MathConstants.h"

#include <catch_amalgamated.hpp>

InitSpringTime ist;

namespace {

static CollisionVolume MakeBoxVolume(const float3& scales, const float3& offsets = ZeroVector)
{
	CollisionVolume v;
	v.InitBox(scales, offsets);
	return v;
}

static CollisionVolume MakeSphereVolume(const float radius, const float3& offsets = ZeroVector)
{
	CollisionVolume v;
	v.InitSphere(radius);
	v.SetOffsets(offsets);
	return v;
}

static CollisionVolume MakeEllipsoidVolume(const float3& scales, const float3& offsets = ZeroVector)
{
	CollisionVolume v;
	v.InitShape(scales, offsets, CollisionVolume::COLVOL_TYPE_ELLIPSOID, CollisionVolume::COLVOL_HITTEST_CONT,
		        CollisionVolume::COLVOL_AXIS_Z);
	return v;
}

static CollisionVolume MakeCylinderVolume(const float3& scales, const int axis, const float3& offsets = ZeroVector)
{
	CollisionVolume v;
	v.InitShape(scales, offsets, CollisionVolume::COLVOL_TYPE_CYLINDER, CollisionVolume::COLVOL_HITTEST_CONT, axis);
	return v;
}

static CMatrix44f MakeTransform(const float3& pos = ZeroVector, const float3& rotXYZ = ZeroVector)
{
	CMatrix44f m;

	if (rotXYZ.x != 0.0f)
		m.RotateX(rotXYZ.x);
	if (rotXYZ.y != 0.0f)
		m.RotateY(rotXYZ.y);
	if (rotXYZ.z != 0.0f)
		m.RotateZ(rotXYZ.z);

	m.SetPos(pos);
	return m;
}

static bool IntersectsVolume(const CollisionVolume& vol, const CMatrix44f& mat, const CollisionVolume& otherVol,
	                         const CMatrix44f& otherMat)
{
	return CCollisionHandler::IntersectVolume(vol, mat, otherVol, otherMat);
}

static CCamera::Frustum PyramidToFrustum(
	const float3& pyramidShape,
	const CMatrix44f& pyramidToWorld,
	const float3& pyramidOffsets,
	int primaryAxis,
	float nearFrac = 1e-4f   // small slice away from apex; exact apex is degenerate
)
{
	assert(pyramidToWorld.IsRotOrRotTranMatrix());

	CCamera::Frustum fr;

	const int pAx = primaryAxis;
	int sAx0 = CollisionVolume::COLVOL_AXIS_X;
	int sAx1 = CollisionVolume::COLVOL_AXIS_Y;

	switch (pAx) {
		case CollisionVolume::COLVOL_AXIS_X: {
			sAx0 = CollisionVolume::COLVOL_AXIS_Y;
			sAx1 = CollisionVolume::COLVOL_AXIS_Z;
		} break;
		case CollisionVolume::COLVOL_AXIS_Y: {
			sAx0 = CollisionVolume::COLVOL_AXIS_X;
			sAx1 = CollisionVolume::COLVOL_AXIS_Z;
		} break;
		case CollisionVolume::COLVOL_AXIS_Z:
		default: {
			sAx0 = CollisionVolume::COLVOL_AXIS_X;
			sAx1 = CollisionVolume::COLVOL_AXIS_Y;
		} break;
	}

	const float3 h = pyramidShape * 0.5f;
	const float3& o = pyramidOffsets;

	const float hp  = h[pAx];
	const float hs0 = h[sAx0];
	const float hs1 = h[sAx1];

	// A true pyramid has apex at primary=-hp and base at primary=+hp.
	// A frustum needs a finite near face, so slice a tiny face near the apex.
	const float tNear = std::clamp(nearFrac, 0.0f, 1.0f);
	const float tFar  = 1.0f;

	// For a cross-section parameter t in [0,1]:
	//   primary = -hp + 2*hp*t
	//   secondary half-extents = hs*t
	auto MakeLocalSectionPoint = [&](float t, float sign0, float sign1) {
		float3 p = o;
		p[pAx ] += (-hp + 2.0f * hp * t);
		p[sAx0] += (sign0 * hs0 * t);
		p[sAx1] += (sign1 * hs1 * t);
		return p;
	};

	// Local near slice
	const float3 nblL = MakeLocalSectionPoint(tNear, -1.0f, -1.0f);
	const float3 nbrL = MakeLocalSectionPoint(tNear,  1.0f, -1.0f);
	const float3 ntrL = MakeLocalSectionPoint(tNear,  1.0f,  1.0f);
	const float3 ntlL = MakeLocalSectionPoint(tNear, -1.0f,  1.0f);

	// Local base (= far slice)
	const float3 fblL = MakeLocalSectionPoint(tFar, -1.0f, -1.0f);
	const float3 fbrL = MakeLocalSectionPoint(tFar,  1.0f, -1.0f);
	const float3 ftrL = MakeLocalSectionPoint(tFar,  1.0f,  1.0f);
	const float3 ftlL = MakeLocalSectionPoint(tFar, -1.0f,  1.0f);

	// Transform to world
	fr.verts[CCamera::FRUSTUM_POINT_NBL] = pyramidToWorld.Mul(nblL);
	fr.verts[CCamera::FRUSTUM_POINT_NBR] = pyramidToWorld.Mul(nbrL);
	fr.verts[CCamera::FRUSTUM_POINT_NTR] = pyramidToWorld.Mul(ntrL);
	fr.verts[CCamera::FRUSTUM_POINT_NTL] = pyramidToWorld.Mul(ntlL);

	fr.verts[CCamera::FRUSTUM_POINT_FBL] = pyramidToWorld.Mul(fblL);
	fr.verts[CCamera::FRUSTUM_POINT_FBR] = pyramidToWorld.Mul(fbrL);
	fr.verts[CCamera::FRUSTUM_POINT_FTR] = pyramidToWorld.Mul(ftrL);
	fr.verts[CCamera::FRUSTUM_POINT_FTL] = pyramidToWorld.Mul(ftlL);

	const auto SetPlane = [&](uint32_t i, uint32_t v1i, uint32_t v2i, uint32_t v3i) {
		const float3& v1 = fr.verts[v1i];
		const float3& v2 = fr.verts[v2i];
		const float3& v3 = fr.verts[v3i];

		const float3 u = v1 - v2;
		const float3 v = v3 - v2;

		// Match your existing CCamera::Frustum::IntersectSphere convention:
		// inward-facing normals
		float3 n = -(v.cross(u));
		n.UnsafeANormalize();

		const float d = -n.dot(v2);
		fr.planes[i] = float4(n, d);
	};

	SetPlane(CCamera::FRUSTUM_PLANE_LFT, CCamera::FRUSTUM_POINT_NTL, CCamera::FRUSTUM_POINT_NBL, CCamera::FRUSTUM_POINT_FBL);
	SetPlane(CCamera::FRUSTUM_PLANE_RGT, CCamera::FRUSTUM_POINT_NBR, CCamera::FRUSTUM_POINT_NTR, CCamera::FRUSTUM_POINT_FBR);
	SetPlane(CCamera::FRUSTUM_PLANE_BOT, CCamera::FRUSTUM_POINT_NBL, CCamera::FRUSTUM_POINT_NBR, CCamera::FRUSTUM_POINT_FBR);
	SetPlane(CCamera::FRUSTUM_PLANE_TOP, CCamera::FRUSTUM_POINT_NTR, CCamera::FRUSTUM_POINT_NTL, CCamera::FRUSTUM_POINT_FTL);
	SetPlane(CCamera::FRUSTUM_PLANE_NEA, CCamera::FRUSTUM_POINT_NTL, CCamera::FRUSTUM_POINT_NTR, CCamera::FRUSTUM_POINT_NBR);
	SetPlane(CCamera::FRUSTUM_PLANE_FAR, CCamera::FRUSTUM_POINT_FTR, CCamera::FRUSTUM_POINT_FTL, CCamera::FRUSTUM_POINT_FBL);

	fr.edges[CCamera::FRUSTUM_EDGE_NTR_NTL] = (fr.verts[CCamera::FRUSTUM_POINT_NTR] - fr.verts[CCamera::FRUSTUM_POINT_NTL]).UnsafeANormalize();
	fr.edges[CCamera::FRUSTUM_EDGE_NTL_NBL] = (fr.verts[CCamera::FRUSTUM_POINT_NTL] - fr.verts[CCamera::FRUSTUM_POINT_NBL]).UnsafeANormalize();
	fr.edges[CCamera::FRUSTUM_EDGE_FTL_NTL] = (fr.verts[CCamera::FRUSTUM_POINT_FTL] - fr.verts[CCamera::FRUSTUM_POINT_NTL]).UnsafeANormalize();
	fr.edges[CCamera::FRUSTUM_EDGE_FTR_NTR] = (fr.verts[CCamera::FRUSTUM_POINT_FTR] - fr.verts[CCamera::FRUSTUM_POINT_NTR]).UnsafeANormalize();
	fr.edges[CCamera::FRUSTUM_EDGE_FBR_NBR] = (fr.verts[CCamera::FRUSTUM_POINT_FBR] - fr.verts[CCamera::FRUSTUM_POINT_NBR]).UnsafeANormalize();
	fr.edges[CCamera::FRUSTUM_EDGE_FBL_NBL] = (fr.verts[CCamera::FRUSTUM_POINT_FBL] - fr.verts[CCamera::FRUSTUM_POINT_NBL]).UnsafeANormalize();

	// For non-camera frusta this is mostly informational.
	// z/w store the local primary-axis slice positions relative to the pyramid center.
	fr.scales = float4(
		hs0 * tFar,
		hs1 * tFar,
		(-hp + 2.0f * hp * tNear),
		(+hp)
	);

	return fr;
}

} // namespace

// Generated with the following in CCollisionHandler::IntersectVolumeWithFrustum
// printf("f.edges = {");
// 	for (auto v : frustum.edges)
// 		printf("%s,", v.str().c_str());
// 	printf("};\n");
// 	printf("f.verts = {");
// 	for (auto v : frustum.verts)
// 		printf("%s,", v.str().c_str());
// 	printf("};\n");
// 	printf("f.planes = {");
// 	for (auto v : frustum.planes)
// 		printf("%s,", v.str().c_str());
// 	printf("};\n");
// 	printf("testVol.InitShape(%s,%s,%d,1,%d);\n",vol.GetScales().str().c_str(), vol.GetOffsets().str().c_str(), vol.GetVolumeType(),vol.GetPrimaryAxis());
// 	printf("testMat = {%s};\n", volumeToWorld.str_serialize().c_str());
TEST_CASE("CollisionHandler other frustum checks")
{
	CCamera::Frustum f;
	CollisionVolume testVol;
	CMatrix44f testMat;
	SECTION("should not intersect") {
		f.edges = {float3(1.000, 0.000, 0.025),float3(0.025, 0.081, -0.996),float3(0.035, -0.969, -0.246),float3(0.160, -0.957, -0.240),float3(0.159, -0.980, -0.118),float3(0.032, -0.992, -0.123),};
		f.verts = {float3(0.522, 837.284, 12307.813),float3(5.007, 837.284, 12307.926),float3(5.117, 837.644, 12303.524),float3(0.632, 837.644, 12303.412),float3(1050.357, -31674.688, 8287.396),float3(5282.000, -31674.688, 8393.214),float3(5385.849, -31335.734, 4240.269),float3(1154.206, -31335.734, 4134.452),};
		f.planes = {float4(0.999, 0.029, 0.027, -361.285),float4(-0.987, -0.156, -0.037, 595.606),float4(0.025, 0.123, -0.992, 12106.391),float4(-0.024, -0.247, 0.969, -11712.182),float4(0.002, -0.997, -0.081, 1835.318),float4(-0.002, 0.997, 0.081, 30898.180),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK_FALSE(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
		f.edges = {float3(1.000, 0.000, 0.025),float3(0.025, 0.081, -0.996),float3(-0.191, -0.948, -0.257),float3(0.121, -0.960, -0.252),float3(0.120, -0.984, -0.133),float3(-0.196, -0.971, -0.139),};
		f.verts = {float3(-7.567, 837.322, 12307.155),float3(3.600, 837.322, 12307.435),float3(3.707, 837.673, 12303.123),float3(-7.459, 837.673, 12302.844),float3(-6581.464, -31639.586, 7666.208),float3(3954.178, -31639.586, 7929.664),float3(4055.879, -31307.645, 3862.632),float3(-6479.763, -31307.645, 3599.176),};
		f.planes = {float4(0.980, -0.200, 0.008, 73.329),float4(-0.993, -0.116, -0.034, 522.868),float4(0.025, 0.137, -0.990, 12074.024),float4(-0.024, -0.257, 0.966, -11671.867),float4(0.002, -0.997, -0.081, 1834.836),float4(-0.002, 0.997, 0.081, 30898.059),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK_FALSE(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
		f.edges = {float3(1.000, 0.000, 0.025),float3(0.025, 0.081, -0.996),float3(-0.324, -0.944, -0.059),float3(-0.025, -0.998, -0.055),float3(-0.029, -0.994, 0.108),float3(-0.323, -0.942, 0.095),};
		f.verts = {float3(-12.656, 836.620, 12315.631),float3(-1.604, 836.620, 12315.907),float3(-1.461, 837.087, 12310.184),float3(-12.512, 837.087, 12309.907),float3(-11382.142, -32301.607, 15662.534),float3(-955.198, -32301.607, 15923.272),float3(-820.154, -31860.838, 10522.815),float3(-11247.097, -31860.838, 10262.077),};
		f.planes = {float4(0.946, -0.325, -0.003, 318.907),float4(-0.999, 0.026, -0.023, 257.816),float4(0.025, -0.109, -0.994, 12329.984),float4(-0.025, -0.054, 0.998, -12243.295),float4(0.002, -0.997, -0.081, 1835.242),float4(-0.002, 0.997, 0.081, 30898.098),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK_FALSE(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
		f.edges = {float3(0.836, 0.000, -0.548),float3(-0.329, 0.799, -0.503),float3(-0.837, -0.324, -0.441),float3(-0.526, -0.365, -0.769),float3(-0.472, -0.556, -0.685),float3(-0.796, -0.490, -0.355),};
		f.verts = {float3(42.884, 629.677, 12295.365),float3(54.852, 629.677, 12287.524),float3(52.816, 634.620, 12284.417),float3(40.848, 634.620, 12292.258),float3(-25014.186, -14804.015, 1105.640),float3(-13046.284, -14804.016, -6735.326),float3(-15081.872, -9860.531, -9842.305),float3(-27049.773, -9860.530, -2001.339),};
		f.planes = {float4(0.531, -0.283, -0.799, 9974.147),float4(-0.828, 0.012, 0.561, -6852.329),float4(-0.305, 0.831, -0.465, 5204.513),float4(0.200, -0.931, 0.305, -3166.272),float4(-0.438, -0.601, -0.669, 8619.063),float4(0.438, 0.601, 0.669, 19112.832),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK_FALSE(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
		f.edges = {float3(0.682, 0.000, -0.731),float3(-0.023, 0.999, -0.022),float3(-0.869, 0.281, -0.408),float3(-0.618, 0.292, -0.730),float3(-0.632, 0.206, -0.747),float3(-0.888, 0.198, -0.416),};
		f.verts = {float3(77.155, 583.387, 12355.345),float3(82.599, 583.387, 12349.512),float3(82.559, 585.122, 12349.474),float3(77.115, 585.122, 12355.307),float3(-17077.473, 4404.819, 4323.733),float3(-11633.604, 4404.818, -1509.402),float3(-11673.502, 6139.626, -1546.637),float3(-17117.371, 6139.627, 4286.498),};
		f.planes = {float4(0.422, -0.010, -0.906, 11173.166),float4(-0.764, -0.003, 0.645, -7900.939),float4(0.152, 0.978, 0.142, -2331.110),float4(-0.215, -0.956, -0.200, 3052.059),float4(-0.731, -0.032, -0.682, 8500.695),float4(0.731, 0.031, 0.682, 9391.692),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK_FALSE(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
		f.edges = {float3(-0.853, 0.000, -0.521),float3(0.000, 1.000, -0.001),float3(-0.209, 0.290, 0.934),float3(-0.750, 0.288, 0.595),float3(-0.782, -0.058, 0.621),float3(-0.218, -0.058, 0.974),};
		f.verts = {float3(-3.997, 580.482, 12305.900),float3(-14.486, 580.482, 12299.492),float3(-14.484, 587.122, 12299.488),float3(-3.995, 587.122, 12305.896),float3(-4007.037, -492.983, 30204.064),float3(-14495.960, -492.984, 23796.250),float3(-14493.824, 6147.524, 23792.752),float3(-4004.901, 6147.525, 30200.566),};
		f.planes = {float4(-0.976, 0.000, -0.218, 2681.797),float4(0.622, 0.000, 0.783, -9624.173),float4(-0.032, 0.998, 0.053, -1227.583),float4(-0.159, -0.952, 0.260, -2645.358),float4(-0.521, 0.001, 0.853, -10503.586),float4(0.521, -0.001, -0.853, 27863.449),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK_FALSE(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
		f.edges = {float3(-0.985, 0.000, 0.171),float3(0.054, 0.949, 0.309),float3(0.570, -0.384, 0.726),float3(0.358, -0.417, 0.835),float3(0.346, -0.509, 0.788),float3(0.556, -0.469, 0.686),};
		f.verts = {float3(14.452, 570.063, 12284.518),float3(8.299, 570.063, 12285.584),float3(8.432, 572.431, 12286.355),float3(14.586, 572.431, 12285.289),float3(14442.133, -11595.112, 30063.572),float3(8288.818, -11595.113, 31129.910),float3(8422.507, -9227.578, 31901.363),float3(14575.821, -9227.577, 30835.025),};
		f.planes = {float4(-0.813, -0.138, 0.565, -6853.808),float4(0.929, 0.067, -0.365, 4439.696),float4(0.089, 0.854, 0.512, -6782.632),float4(-0.073, -0.904, -0.421, 5685.377),float4(0.162, -0.314, 0.935, -11315.248),float4(-0.162, 0.314, -0.935, 34105.992),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK_FALSE(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
	}
	SECTION("should intersect") {
		f.edges = {float3(0.682, 0.000, -0.731),float3(-0.023, 1.000, -0.021),float3(-0.708, 0.285, -0.646),float3(-0.536, 0.278, -0.797),float3(-0.547, 0.193, -0.815),float3(-0.724, 0.198, -0.661),};
		f.verts = {float3(81.009, 583.199, 12351.224),float3(84.013, 583.199, 12348.005),float3(83.973, 584.933, 12347.968),float3(80.969, 584.933, 12351.187),float3(-13223.896, 4216.312, 203.296),float3(-10219.644, 4216.312, -3015.778),float3(-10259.535, 5950.870, -3053.008),float3(-13263.789, 5950.870, 166.066),};
		f.planes = {float4(0.674, -0.000, -0.739, 9067.249),float4(-0.831, -0.007, 0.556, -6788.835),float4(0.145, 0.980, 0.135, -2249.401),float4(-0.208, -0.959, -0.194, 2976.636),float4(-0.731, -0.031, -0.682, 8500.936),float4(0.731, 0.031, 0.682, 9391.729),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
		f.edges = {float3(0.682, 0.000, -0.731),float3(-0.023, 0.999, -0.024),float3(-0.714, 0.129, -0.689),float3(-0.466, 0.122, -0.876),float3(-0.467, 0.114, -0.877),float3(-0.714, 0.120, -0.689),};
		f.verts = {float3(81.380, 581.736, 12350.893),float3(85.392, 581.736, 12346.595),float3(85.388, 581.901, 12346.591),float3(81.377, 581.901, 12350.889),float3(-12852.104, 2753.490, -127.636),float3(-8840.790, 2753.490, -4425.781),float3(-8844.593, 2918.823, -4429.330),float3(-12855.906, 2918.823, -131.185),};
		f.planes = {float4(0.694, -0.001, -0.720, 8833.525),float4(-0.883, -0.009, 0.469, -5707.969),float4(0.088, 0.993, 0.082, -1595.795),float4(-0.094, -0.992, -0.088, 1671.206),float4(-0.731, -0.033, -0.682, 8501.226),float4(0.731, 0.031, 0.682, 9391.728),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		f.edges = {float3(0.682, 0.000, -0.731),float3(-0.023, 0.999, -0.022),float3(-0.812, 0.153, -0.563),float3(-0.798, 0.153, -0.583),float3(-0.801, 0.129, -0.585),float3(-0.815, 0.129, -0.565),};
		f.verts = {float3(79.387, 581.924, 12353.021),float3(79.693, 581.924, 12352.692),float3(79.683, 582.373, 12352.683),float3(79.377, 582.373, 12353.011),float3(-14845.652, 2942.108, 1999.766),float3(-14539.371, 2942.108, 1671.583),float3(-14549.681, 3390.393, 1661.962),float3(-14855.962, 3390.393, 1990.144),};
		f.planes = {float4(0.569, -0.005, -0.822, 10111.838),float4(-0.590, 0.004, 0.808, -9933.192),float4(0.095, 0.991, 0.089, -1681.872),float4(-0.113, -0.988, -0.105, 1884.351),float4(-0.731, -0.032, -0.682, 8501.328),float4(0.731, 0.031, 0.682, 9391.714),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
		f.edges = {float3(0.682, 0.000, -0.731),float3(-0.627, 0.514, -0.585),float3(-0.580, -0.631, -0.515),float3(-0.313, -0.596, -0.739),float3(-0.245, -0.681, -0.690),float3(-0.518, -0.723, -0.456),};
		f.verts = {float3(31.433, 647.251, 12305.595),float3(39.979, 647.251, 12296.437),float3(37.208, 649.519, 12293.851),float3(28.662, 649.519, 12303.009),float3(-17107.557, -23259.348, -2779.642),float3(-8561.848, -23259.348, -11938.163),float3(-11332.871, -20990.723, -14523.773),float3(-19878.580, -20990.723, -5365.252),};
		f.planes = {float4(0.674, -0.018, -0.738, 9073.854),float4(-0.770, -0.296, 0.565, -6728.932),float4(-0.529, 0.691, -0.493, 5642.091),float4(0.461, -0.776, 0.430, -4805.240),float4(-0.375, -0.858, -0.350, 4878.818),float4(0.376, 0.858, 0.350, 27355.217),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
		f.edges = {float3(0.682, 0.000, -0.731),float3(-0.627, 0.514, -0.585),float3(-0.581, -0.760, -0.292),float3(-0.565, -0.763, -0.313),float3(-0.540, -0.790, -0.289),float3(-0.557, -0.787, -0.267),};
		f.verts = {float3(30.215, 645.219, 12311.874),float3(30.838, 645.219, 12311.205),float3(29.918, 645.973, 12310.347),float3(29.294, 645.973, 12311.016),float3(-18325.928, -25290.641, 3500.269),float3(-17702.113, -25290.641, 2831.721),float3(-18622.484, -24537.137, 1972.935),float3(-19246.297, -24537.137, 2641.482),};
		f.planes = {float4(0.601, -0.159, -0.784, 9731.259),float4(-0.614, 0.136, 0.778, -9640.606),float4(-0.585, 0.600, -0.546, 6350.416),float4(0.565, -0.635, 0.527, -6093.689),float4(-0.376, -0.858, -0.350, 4878.344),float4(0.376, 0.858, 0.350, 27355.236),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
		f.edges = {float3(0.682, 0.000, -0.731),float3(-0.731, 0.031, -0.682),float3(-0.533, -0.831, 0.158),float3(0.055, -0.870, -0.491),float3(0.185, -0.903, -0.387),float3(-0.426, -0.861, 0.277),};
		f.verts = {float3(30.603, 652.356, 12254.607),float3(54.739, 652.356, 12228.741),float3(49.854, 652.566, 12224.184),float3(25.719, 652.566, 12250.050),float3(-16115.584, -31951.047, 22742.584),float3(6732.015, -31951.047, -1743.415),float3(2108.369, -31752.299, -6057.685),float3(-20739.230, -31752.299, 18428.314),};
		f.planes = {float4(0.581, -0.495, -0.646, 8217.051),float4(-0.631, -0.411, 0.658, -7739.013),float4(-0.724, 0.141, -0.675, 8206.695),float4(0.693, -0.321, 0.646, -7724.834),float4(-0.023, -1.000, -0.021, 915.368),float4(0.023, 1.000, 0.021, 31817.885),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
		f.edges = {float3(-0.990, 0.000, -0.138),float3(-0.041, 0.955, 0.292),float3(0.033, -0.359, 0.933),float3(-0.058, -0.363, 0.930),float3(-0.055, -0.426, 0.903),float3(0.035, -0.422, 0.906),};
		f.verts = {float3(1.861, 573.191, 12289.144),float3(-0.205, 573.191, 12288.856),float3(-0.269, 574.691, 12289.315),float3(1.797, 574.691, 12289.603),float3(809.515, -9126.199, 33112.176),float3(-1256.501, -9126.199, 32824.379),float3(-1320.377, -7626.235, 33282.926),float3(745.639, -7626.235, 33570.723),};
		f.planes = {float4(-0.999, -0.048, 0.017, -174.436),float4(0.997, 0.021, 0.071, -882.151),float4(-0.059, 0.904, 0.423, -5721.237),float4(0.050, -0.931, -0.361, 4967.497),float4(-0.132, -0.295, 0.946, -11460.339),float4(0.132, 0.295, -0.946, 33921.469),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
		f.edges = {float3(-0.985, 0.000, 0.171),float3(0.054, 0.949, 0.309),float3(0.354, -0.410, 0.841),float3(0.241, -0.417, 0.876),float3(0.228, -0.529, 0.817),float3(0.340, -0.520, 0.783),};
		f.verts = {float3(8.158, 569.765, 12285.511),float3(5.391, 569.765, 12285.990),float3(5.553, 572.629, 12286.924),float3(8.320, 572.629, 12286.444),float3(8147.957, -11893.595, 31054.150),float3(5381.024, -11893.595, 31533.650),float3(5542.797, -9028.725, 32467.160),float3(8309.729, -9028.725, 31987.660),};
		f.planes = {float4(-0.931, -0.065, 0.360, -4383.720),float4(0.967, 0.028, -0.252, 3078.543),float4(0.091, 0.847, 0.523, -6912.721),float4(-0.071, -0.908, -0.412, 5588.428),float4(0.162, -0.314, 0.935, -11315.248),float4(-0.162, 0.314, -0.935, 34105.922),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
	}
	SECTION("should not intersect High FOV") {
		f.edges = {float3(0.682, 0.000, -0.731),float3(-0.590, 0.590, -0.551),float3(-0.869, -0.041, 0.494),float3(-0.994, -0.094, 0.062),float3(-0.522, -0.637, 0.567),float3(-0.656, -0.264, 0.707),};
		f.verts = {float3(48.433, 611.083, 12214.961),float3(112.911, 611.083, 12145.859),float3(80.334, 643.667, 12115.462),float3(15.856, 643.667, 12184.563),float3(-95998.961, -37986.781, 115630.234),float3(-31521.281, -37986.785, 46528.809),float3(-64098.492, -5403.293, 16131.418),float3(-128576.172, -5403.289, 85232.844),};
		f.planes = {float4(-0.275, -0.788, -0.550, 7213.672),float4(-0.017, 0.673, 0.740, -9392.082),float4(-0.731, -0.008, -0.682, 8373.597),float4(0.099, -0.991, 0.093, -490.978),float4(-0.432, -0.807, -0.403, 5433.196),float4(0.432, 0.807, 0.403, 25529.211),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK_FALSE(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
		f.edges = {float3(-0.407, 0.000, -0.913),float3(-0.737, 0.590, 0.329),float3(-0.843, 0.465, 0.270),float3(-0.808, 0.279, -0.519),float3(-0.605, 0.038, -0.796),float3(-0.980, 0.156, 0.119),};
		f.verts = {float3(104.184, 571.486, 12251.956),float3(-5.523, 571.486, 12005.766),float3(-117.438, 661.039, 12055.637),float3(-7.731, 661.039, 12301.827),float3(-70625.453, 11838.199, 20862.594),float3(-180332.250, 11838.186, -225328.141),float3(-292247.000, 101391.445, -175457.000),float3(-182540.203, 101391.461, 70733.727),};
		f.planes = {float4(-0.037, 0.451, -0.892, 10673.827),float4(-0.493, -0.803, 0.336, -3579.562),float4(0.149, 0.987, -0.066, 234.522),float4(-0.427, -0.884, 0.190, -1759.416),float4(-0.539, -0.807, 0.240, -2425.197),float4(0.539, 0.807, -0.240, 33520.180),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK_FALSE(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
	}
	SECTION("should intersect High FOV") {
		f.edges = {float3(-0.407, 0.000, -0.913),float3(-0.737, 0.590, 0.329),float3(-0.516, 0.414, 0.750),float3(-0.866, 0.343, -0.363),float3(-0.705, 0.088, -0.703),float3(-0.168, 0.136, 0.976),};
		f.verts = {float3(150.660, 579.826, 12384.285),float3(18.175, 579.826, 12086.979),float3(-86.636, 663.695, 12133.686),float3(45.849, 663.695, 12430.991),float3(-24149.637, 20178.260, 153191.562),float3(-156634.062, 20178.244, -144113.969),float3(-261445.219, 104047.250, -97408.336),float3(-128960.781, 104047.266, 199897.188),};
		f.planes = {float4(-0.625, -0.781, 0.001, 535.509),float4(-0.472, -0.798, 0.374, -4043.666),float4(0.219, 0.971, -0.097, 610.495),float4(-0.429, -0.883, 0.191, -1772.757),float4(-0.539, -0.807, 0.240, -2425.199),float4(0.539, 0.807, -0.240, 33520.152),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
		f.edges = {float3(-0.407, 0.000, -0.913),float3(-0.737, 0.590, 0.329),float3(-0.963, 0.189, 0.191),float3(-0.759, 0.085, -0.646),float3(-0.658, -0.028, -0.752),float3(-0.993, -0.089, 0.081),};
		f.verts = {float3(125.646, 555.787, 12247.350),float3(70.797, 555.787, 12124.265),float3(47.411, 574.500, 12134.686),float3(102.260, 574.500, 12257.771),float3(-49163.855, -3860.420, 16256.131),float3(-104012.945, -3860.426, -106829.562),float3(-127398.156, 14852.221, -96408.719),float3(-72549.062, 14852.227, 26676.977),};
		f.planes = {float4(-0.109, 0.377, -0.920, 11071.541),float4(-0.446, -0.791, 0.419, -4614.567),float4(-0.086, 0.996, 0.038, -1012.289),float4(-0.177, -0.981, 0.079, -385.818),float4(-0.539, -0.807, 0.240, -2425.216),float4(0.539, 0.807, -0.240, 33520.074),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
		f.edges = {float3(-0.407, 0.000, -0.913),float3(-0.737, 0.590, 0.329),float3(-0.268, 0.278, 0.923),float3(-0.927, 0.374, -0.031),float3(-0.943, 0.168, -0.287),float3(-0.023, 0.096, 0.995),};
		f.verts = {float3(170.857, 577.485, 12421.743),float3(78.254, 577.485, 12213.936),float3(23.556, 621.254, 12238.310),float3(116.159, 621.254, 12446.117),float3(-3952.703, 17837.938, 190648.500),float3(-96555.344, 17837.928, -17159.141),float3(-151253.156, 61606.672, 7215.145),float3(-58650.523, 61606.680, 215022.781),};
		f.planes = {float4(-0.606, -0.793, 0.063, -217.976),float4(-0.315, -0.731, 0.606, -6953.736),float4(0.202, 0.975, -0.090, 517.748),float4(-0.373, -0.913, 0.166, -1460.794),float4(-0.539, -0.807, 0.240, -2425.206),float4(0.539, 0.807, -0.240, 33520.070),};
		testVol.InitShape(float3(22.000, 30.000, 22.000),float3(0.000, -1.000, 0.000),1,1,1);
		testMat = {0.99999f, -0.00000f, -0.00000f, 0.00000f, 0.00000f, 1.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.99999f, 0.00000f, 0.00000f, 588.55591f, 12287.00000f, 1.00000f};
		CHECK(CCollisionHandler::IntersectVolumeWithFrustum(f, testVol, testMat));
	}
}

// These were originally a pyramid type COLVOL, but I decided against it and just use them for frustum testing
// generated with the following and then put through llm
//  printf("SECTION(\"\") {");
// 	printf("CollisionVolume pyramid;");
// 	printf("pyramid.InitShape(");
// 	printf("%s,",selVol.GetScales().str().c_str());
// 	printf("%s,", selVol.GetOffsets().str().c_str());
// 	printf("%d,", selVol.GetVolumeType());
// 	printf("1,");
// 	printf("%d",selVol.GetPrimaryAxis());
// 	printf(");");
// 	printf("const CMatrix44f pyrMat = {%s};", selMat.str_serialize().c_str());
// 	printf("CollisionVolume testVol;");
// 	printf("testVol.InitShape(");
// 	printf("%s,",unitVol->GetScales().str().c_str());
// 	printf("%s,", unitVol->GetOffsets().str().c_str());
// 	printf("%d,", unitVol->GetVolumeType());
// 	printf("1,");
// 	printf("%d",unitVol->GetPrimaryAxis());
// 	printf(");");
// 	printf("const CMatrix44f testMat = {%s};", unitMat.str_serialize().c_str());
// 	if (result) {
// 	printf("CHECK(IntersectsPyramid(pyramid, pyrMat, testVol, testMat));");
// 	} else {
// 	printf("CHECK_FALSE(IntersectsPyramid(pyramid, pyrMat, testVol, testMat));");
// 	}
// 	printf("}\n");
TEST_CASE("CollisionHandler_PyramidVsCylinder + frustum tests")
{
	struct Case {
		const char* name;
		bool intersects;
		float3 pyramidShape;
		CMatrix44f pyramidMat;
	};

	const CollisionVolume testVol = [] {
		CollisionVolume v;
		v.InitShape(float3(28.000f, 52.000f, 28.000f), float3(0.000f, 3.000f, 0.000f), 1, 1, 1);
		return v;
	}();

	const CMatrix44f testMat = {
		0.99999f, -0.00000f, -0.00000f, 0.00000f,
		0.00000f,  1.00000f,  0.00000f, 0.00000f,
		0.00000f,  0.00000f,  0.99999f, 0.00000f,
		0.00000f, 599.55585f, 12284.00000f, 1.00000f
	};

	const std::vector<Case> cases = {{
		{
			"camera overhead, selection outside corner of cylinder",
			false,
			float3(4946.859f, 4953.576f, 33535.227f),
			{
				 0.992f,  0.125f,  0.023f, 0.000f,
				-0.000f, -0.182f,  0.983f, 0.000f,
				 0.128f, -0.975f, -0.180f, 0.000f,
			  2139.423f, -15565.261f, 9268.336f, 1.000f
			}
		},
		{
			"camera overhead, selection above top",
			false,
			float3(13539.558f, 3602.985f, 33260.023f),
			{
				 1.000f, -0.005f, -0.001f, 0.000f,
				-0.000f, -0.181f,  0.983f, 0.000f,
				-0.005f, -0.983f, -0.181f, 0.000f,
			   -89.609f, -15565.310f, 9272.988f, 1.000f
			}
		},
		{
			"camera overhead, selection outside left corner of cylinder",
			false,
			float3(8782.085f, 2429.013f, 33692.617f),
			{
				 0.980f, -0.196f, -0.027f, 0.000f,
				-0.000f, -0.135f,  0.991f, 0.000f,
				-0.198f, -0.971f, -0.132f, 0.000f,
			 -3336.510f, -15573.462f, 10064.639f, 1.000f
			}
		},
		{
			"cam overhead, selection on bottom",
			false,
			float3(14485.936f, 5780.520f, 33162.523f),
			{
				 1.000f, -0.031f,  0.004f, 0.000f,
				-0.000f,  0.140f,  0.990f, 0.000f,
				-0.032f, -0.990f,  0.140f, 0.000f,
			  -523.282f, -15620.354f, 14616.951f, 1.000f
			}
		},
		{
			"cam overhead, selection on top right corner touching",
			true,
			float3(4825.485f, 4466.979f, 33048.695f),
			{
				 0.99611f,  0.08762f,  0.00936f, 0.00000f,
				-0.00000f, -0.10627f,  0.99434f, 0.00000f,
				 0.08812f, -0.99047f, -0.10586f, 0.00000f,
			  1456.17041f, -15291.25195f, 10543.66895f, 1.00000f
			}
		},
		{
			"cam overhead, selection on top touching",
			true,
			float3(8733.372f, 4008.335f, 32931.742f),
			{
				 0.99990f, -0.01423f, -0.00156f, 0.00000f,
				-0.00000f, -0.10882f,  0.99406f, 0.00000f,
				-0.01431f, -0.99396f, -0.10881f, 0.00000f,
			   -235.67889f, -15290.82129f, 10501.34180f, 1.00000f
			}
		},
		{
			"cam overhead, selection complete cover",
			true,
			float3(19613.719f, 15580.658f, 32805.727f),
			{
				 0.999f, -0.048f, -0.000f, 0.000f,
				-0.000f, -0.006f,  1.000f, 0.000f,
				-0.048f, -0.999f, -0.006f, 0.000f,
			   -781.478f, -15599.557f, 12193.991f, 1.000f
			}
		},
		{
			"cam overhead, selection inside, not including centerpos",
			true,
			float3(358.245f, 905.194f, 32774.824f),
			{
				 0.99983f,  0.01869f,  0.00035f, 0.00000f,
				-0.00000f, -0.01848f,  0.99983f, 0.00000f,
				 0.01869f, -0.99965f, -0.01847f, 0.00000f,
			  306.34171f, -15445.67188f, 11985.80078f, 1.00000f
			}
		},
		{
			"cam 45, selection outside corner",
			false,
			float3(7407.396f, 6360.157f, 32262.871f),
			{
				 0.98874f,  0.09284f,  0.11737f, 0.00000f,
				-0.00000f, -0.78430f,  0.62038f, 0.00000f,
				 0.14965f, -0.61340f, -0.77547f, 0.00000f,
			  2414.12134f, -9039.30859f, 2.57227f, 1.00000f
			}
		},
		{
			"cam 45, selection above top",
			false,
			float3(12262.979f, 3520.008f, 31733.789f),
			{
				 0.99964f, -0.01719f, -0.02058f, 0.00000f,
				-0.00000f, -0.76743f,  0.64113f, 0.00000f,
				-0.02681f, -0.64090f, -0.76715f, 0.00000f,
			   -425.44968f, -9313.49219f, 339.63379f, 1.00000f
			}
		},
		{
			"cam 45, selection below bottom",
			false,
			float3(12146.539f, 4068.301f, 31314.023f),
			{
				 0.99920f, -0.03338f, -0.02212f, 0.00000f,
				-0.00000f, -0.55243f,  0.83356f, 0.00000f,
				-0.04004f, -0.83289f, -0.55198f, 0.00000f,
			   -626.96686f, -12184.95020f, 3869.54688f, 1.00000f
			}
		},
		{
			"cam 45, selection left side",
			true,
			float3(6369.395f, 72.348f, 31550.445f),
			{
				 0.99020f, -0.10092f, -0.09658f, 0.00000f,
				-0.00000f, -0.69139f,  0.72248f, 0.00000f,
				-0.13968f, -0.71540f, -0.68461f, 0.00000f,
			  -2203.52759f, -10429.92285f, 1712.07031f, 1.00000f
			}
		},
		{
			"cam 45, selection complete cover",
			true,
			float3(9741.429f, 10347.375f, 31165.438f),
			{
				 1.00000f,  0.00173f,  0.00152f, 0.00000f,
				-0.00000f, -0.66186f,  0.74963f, 0.00000f,
				 0.00230f, -0.74963f, -0.66186f, 0.00000f,
			    35.89650f, -10825.54004f, 2198.41211f, 1.00000f
			}
		},
		{
			"cam 45, selection inside, not including centerpos",
			true,
			float3(503.404f, 1666.904f, 31172.430f),
			{
				 0.99976f,  0.01637f,  0.01445f, 0.00000f,
				-0.00000f, -0.66164f,  0.74982f, 0.00000f,
				 0.02184f, -0.74964f, -0.66149f, 0.00000f,
			  340.38980f, -10828.39258f, 2201.91309f, 1.00000f
			}
		},
		{
			"cam 90, selection outside corner of cylinder",
			false,
			float3(4755.428f, 1256.933f, 18665.125f),
			{
				 0.97794f, -0.06061f,  0.19991f, 0.00000f,
				-0.00000f, -0.95699f, -0.29013f, 0.00000f,
				 0.20890f,  0.28373f, -0.93587f, 0.00000f,
			  1949.56860f, 3243.81738f, 3682.33105f, 1.00000f
			}
		},
		{
			"cam 90, selection through middle",
			true,
			float3(11874.237f, 341.919f, 17574.648f),
			{
				 0.99986f, -0.00180f,  0.01648f, 0.00000f,
				-0.00000f, -0.99408f, -0.10865f, 0.00000f,
				 0.01658f,  0.10864f, -0.99394f, 0.00000f,
				  145.72449f, 1550.55872f, 3682.33301f, 1.00000f
			}
		},
		{
			"cam 90, selection inside",
			true,
			float3(1046.949f, 224.509f, 18016.047f),
			{
				 0.99997f,  0.00205f, -0.00811f, 0.00000f,
				-0.00000f, -0.96963f, -0.24460f, 0.00000f,
				-0.00837f,  0.24459f, -0.96959f, 0.00000f,
			   -75.36008f, 2799.15015f, 3682.33008f, 1.00000f
			}
		},
		{
			"cam inside",
			true,
			float3(6911.847f, 7870.038f, 17812.121f),
			{
				-0.20841f,  0.01813f, -0.97787f, 0.00000f,
				-0.11242f, -0.99364f,  0.00554f, 0.00000f,
				-0.97156f,  0.11109f,  0.20912f, 0.00000f,
			  -8646.52832f, 1590.73901f, 14150.46387f, 1.00000f
			}
		},
		{
			"cam inside, pointing upwards",
			true,
			float3(2690.407f, 3344.530f, 17535.477f),
			{
				-0.25423f,  0.11189f, -0.96065f, 0.00000f,
				-0.96575f, -0.08267f,  0.24596f, 0.00000f,
				-0.05189f,  0.99028f,  0.12908f, 0.00000f,
			   -448.76093f, 9283.84082f, 13419.71289f, 1.00000f
			}
		},
		{
			"fps cam, 3 fov, inside volume",
			true,
			float3(270.500f, 186.473f, 17373.867f),
			{
				-0.24645f, -0.00125f, -0.96915f, 0.00000f,
				-0.95842f, -0.14811f,  0.24391f, 0.00000f,
				-0.14385f,  0.98897f,  0.03530f, 0.00000f,
			  -1243.35095f, 9192.48145f, 12594.67676f, 1.00000f
			}
		},
		{
			"fps cam, 3 fov, selection outside corner",
			false,
			float3(179.159f, 165.436f, 19869.779f),
			{
				-0.01150f,  0.00370f, -0.99993f, 0.00000f,
				 0.20433f, -0.97888f, -0.00597f, 0.00000f,
				-0.97883f, -0.20438f,  0.01050f, 0.00000f,
			  -8216.60156f, -1078.39661f, 12353.71582f, 1.00000f
			}
		},
		{
			"fps cam, 3 fov, selection inside cylinder",
			true,
			float3(34.699f, 78.686f, 19866.195f),
			{
				-0.01834f,  0.00247f, -0.99983f, 0.00000f,
				 0.22173f, -0.97509f, -0.00648f, 0.00000f,
				-0.97494f, -0.22181f,  0.01734f, 0.00000f,
			  -8176.11914f, -1251.13330f, 12421.60059f, 1.00000f
			}
		},
	}};

	for (const auto& tc : cases) {
		DYNAMIC_SECTION(tc.name) {
			const CCamera::Frustum frustum = PyramidToFrustum(tc.pyramidShape, tc.pyramidMat, ZeroVector, CollisionVolume::COLVOL_AXIS_Z);
			const bool frustumHit = CCollisionHandler::IntersectVolumeWithFrustum(frustum, testVol, testMat);
			CHECK(frustumHit == tc.intersects);
		}
	}
}
TEST_CASE("CollisionHandler_PyramidVsBox")
{
	struct Case {
		const char* name;
		bool intersects;
		float3 pyramidShape;
		CMatrix44f pyramidMat;
	};

	const CollisionVolume testVol = [] {
		CollisionVolume v;
		v.InitShape(float3(26.000f, 28.000f, 46.000f), float3(0.000f, 0.000f, 1.000f), 2, 1, 2);
		return v;
	}();

	const CMatrix44f testMat = {
		0.99865f, -0.05184f,  0.00100f, 0.00000f,
		0.05184f,  0.99790f, -0.03862f, 0.00000f,
		0.00100f,  0.03862f,  0.99924f, 0.00000f,
		0.29548f, 582.04340f, 12286.78027f, 1.00000f
	};

	const std::array<Case, 9> cases = {{
		{
			"cam overhead, outside corner",
			false,
			float3(2473.956f, 2022.063f, 32909.621f),
			{
				 0.99773f,  0.06712f,  0.00501f, 0.00000f,
				-0.00000f, -0.07441f,  0.99723f, 0.00000f,
				 0.06731f, -0.99497f, -0.07425f, 0.00000f,
			  1107.50830f, -15275.38672f, 11071.57617f, 1.00000f
			}
		},
		{
			"cam overhead, small corner overlap",
			true,
			float3(2682.963f, 2305.427f, 32937.918f),
			{
				 0.99787f,  0.06499f,  0.00577f, 0.00000f,
				-0.00000f, -0.08840f,  0.99609f, 0.00000f,
				 0.06524f, -0.99396f, -0.08821f, 0.00000f,
			  1074.51514f, -15272.93945f, 10840.51758f, 1.00000f
			}
		},
		{
			"cam overhead, top overlap",
			true,
			float3(6519.016f, 2575.506f, 32921.043f),
			{
				 0.99884f, -0.04789f, -0.00453f, 0.00000f,
				-0.00000f, -0.09409f,  0.99556f, 0.00000f,
				-0.04810f, -0.99441f, -0.09398f, 0.00000f,
			   -791.74121f, -15271.94141f, 10746.34082f, 1.00000f
			}
		},
		{
			"cam overhead, inside selection",
			true,
			float3(245.141f, 414.870f, 32774.449f),
			{
				 0.99995f, -0.01006f, -0.00028f, 0.00000f,
				-0.00000f, -0.02768f,  0.99962f, 0.00000f,
				-0.01007f, -0.99957f, -0.02768f, 0.00000f,
			   -164.93733f, -15283.52539f, 11839.68164f, 1.00000f
			}
		},
		{
			"cam rotated, small corner overlap",
			true,
			float3(3404.790f, 6794.942f, 26380.340f),
			{
				 0.97275f,  0.05260f, -0.22581f, 0.00000f,
				 0.12623f, -0.93707f,  0.32551f, 0.00000f,
				-0.19448f, -0.34515f, -0.91818f, 0.00000f,
			  -2517.89673f, -3916.15601f, 274.30566f, 1.00000f
			}
		},
		{
			"cam rotated, small side overlap",
			true,
			float3(4110.993f, 578.906f, 27373.383f),
			{
				 0.77230f, -0.15606f, -0.61579f, 0.00000f,
				 0.16912f, -0.88386f,  0.43611f, 0.00000f,
				-0.61233f, -0.44096f, -0.65621f, 0.00000f,
			  -8333.41992f, -5398.82764f, 3403.86523f, 1.00000f
			}
		},
		{
			"cam rotated, complete cover",
			true,
			float3(10601.073f, 9393.225f, 18002.875f),
			{
				 0.91500f,  0.00044f, -0.40346f, 0.00000f,
				-0.00355f, -0.99995f, -0.00915f, 0.00000f,
				-0.40345f,  0.00981f, -0.91495f, 0.00000f,
			  -3578.69434f, 669.62903f, 4163.81152f, 1.00000f
			}
		},
		{
			"fps cam inside volume",
			true,
			float3(1287.219f, 6537.732f, 17506.188f),
			{
				 0.65916f, -0.05785f,  0.74978f, 0.00000f,
				 0.37424f, -0.83957f, -0.39378f, 0.00000f,
				 0.65227f,  0.54016f, -0.53176f, 0.00000f,
			  5709.41406f, 5309.41113f, 7625.11621f, 1.00000f
			}
		},
		{
			"far away side overlap",
			true,
			float3(11147.938f, 4569.580f, 32794.816f),
			{
				 0.96058f, -0.01757f, -0.27746f, 0.00000f,
				 0.27572f, -0.06778f,  0.95885f, 0.00000f,
				-0.03565f, -0.99755f, -0.06026f, 0.00000f,
			   -558.47101f, -13007.40723f, 11276.16406f, 1.00000f
			}
		},
	}};

	for (const auto& tc : cases) {
		DYNAMIC_SECTION(tc.name) {
			const CCamera::Frustum frustum = PyramidToFrustum(tc.pyramidShape, tc.pyramidMat, ZeroVector, CollisionVolume::COLVOL_AXIS_Z);
			const bool frustumHit = CCollisionHandler::IntersectVolumeWithFrustum(frustum, testVol, testMat);
			CHECK(frustumHit == tc.intersects);
		}
	}
}


TEST_CASE("CollisionHandler_PyramidVsEllipse")
{
	struct Case {
		const char* name;
		bool intersects;
		float3 pyramidShape;
		CMatrix44f pyramidMat;
	};

	const CollisionVolume testVol = [] {
		CollisionVolume v;
		v.InitShape(float3(77.000f, 780.000f, 77.000f), float3(0.000f, 0.000f, 0.000f), 0, 1, 2);
		return v;
	}();

	const CMatrix44f testMat = {
		0.99999f, -0.00000f, -0.00000f, 0.00000f,
		0.00000f,  1.00000f,  0.00000f, 0.00000f,
		0.00000f,  0.00000f,  0.99999f, 0.00000f,
		0.00000f, 580.35535f, 12288.00000f, 1.00000f
	};

	const std::vector<Case> cases = {{
		{
			"cam overhead, outside corner",
			false,
			float3(3766.359f, 3012.594f, 32989.598f),
			{
				 0.99653f,  0.08286f,  0.00757f, 0.00000f,
				-0.00000f, -0.09096f,  0.99585f, 0.00000f,
				 0.08320f, -0.99240f, -0.09065f, 0.00000f,
			  1372.37646f, -14768.59570f, 10802.91895f, 1.00000f
			}
		},
		{
			"cam overhead, corner overlap",
			true,
			float3(2887.371f, 1946.611f, 32836.586f),
			{
				 0.99874f,  0.05017f,  0.00256f, 0.00000f,
				-0.00000f, -0.05096f,  0.99870f, 0.00000f,
				 0.05023f, -0.99744f, -0.05090f, 0.00000f,
			   824.72717f, -14775.38867f, 11462.48145f, 1.00000f
			}
		},
		{
			"cam 45, selection outside corner of ellipse",
			false,
			float3(8093.500f, 926.036f, 24689.268f),
			{
				 0.99585f,  0.00382f,  0.09095f, 0.00000f,
				-0.00000f, -0.99912f,  0.04197f, 0.00000f,
				 0.09103f, -0.04180f, -0.99497f, 0.00000f,
			  1180.86426f, 431.71051f, 514.58203f, 1.00000f
			}
		},
		{
			"cam 45, selection inside corner of ellipse",
			true,
			float3(7880.953f, 1007.565f, 24846.783f),
			{
				 0.99751f,  0.00117f,  0.07055f, 0.00000f,
				-0.00000f, -0.99986f,  0.01663f, 0.00000f,
				 0.07056f, -0.01658f, -0.99737f, 0.00000f,
			   933.78717f, 741.65442f, 406.41992f, 1.00000f
			}
		},
		{
			"cam 45, selection inside ellipse top",
			true,
			float3(1095.962f, 841.478f, 23664.766f),
			{
				 0.99506f, -0.02940f, -0.09480f, 0.00000f,
				-0.00000f, -0.95512f,  0.29623f, 0.00000f,
				-0.09926f, -0.29476f, -0.95040f, 0.00000f,
			  -1117.30334f, -2540.07324f, 1551.62305f, 1.00000f
			}
		},
		{
			"cam 45, large overlap",
			true,
			float3(14657.289f, 18940.803f, 23617.162f),
			{
				 0.99677f, -0.02453f, -0.07643f, 0.00000f,
				-0.00000f, -0.95217f,  0.30556f, 0.00000f,
				-0.08026f, -0.30457f, -0.94910f, 0.00000f,
			   -890.65515f, -2648.88330f, 1589.59277f, 1.00000f
			}
		},
		{
			"cam 45, small side intrusion",
			true,
			float3(5782.885f, 194.578f, 30748.346f),
			{
				 0.99165f, -0.08461f, -0.09735f, 0.00000f,
				-0.00000f, -0.75478f,  0.65597f, 0.00000f,
				-0.12898f, -0.65049f, -0.74848f, 0.00000f,
			  -1982.97302f, -8706.42383f, 1513.01562f, 1.00000f
			}
		},
	}};

	for (const auto& tc : cases) {
		DYNAMIC_SECTION(tc.name) {
			const CCamera::Frustum frustum = PyramidToFrustum(tc.pyramidShape, tc.pyramidMat, ZeroVector, CollisionVolume::COLVOL_AXIS_Z);
			const bool frustumHit = CCollisionHandler::IntersectVolumeWithFrustum(frustum, testVol, testMat);
			CHECK(frustumHit == tc.intersects);
		}
	}
}


TEST_CASE("CollisionHandler_PyramidVsSphere")
{
	struct Case {
		const char* name;
		bool intersects;
		float3 pyramidShape;
		CMatrix44f pyramidMat;
	};

	const CollisionVolume testVol = [] {
		CollisionVolume v;
		v.InitShape(float3(46.000f, 46.000f, 46.000f), float3(0.000f, 0.000f, 0.000f), 1, 1, 2);
		return v;
	}();

	const CMatrix44f testMat = {
		0.99999f, -0.00000f, -0.00000f, 0.00000f,
		0.00000f,  1.00000f,  0.00000f, 0.00000f,
		0.00000f,  0.00000f,  0.99999f, 0.00000f,
		1.00000f, 576.36536f, 12287.00000f, 1.00000f
	};

	const std::vector<Case> cases = {{
		{
			"touching top",
			true,
			float3(7308.042f, 3080.405f, 33003.332f),
			{
				 0.99940f, -0.03305f, -0.00988f, 0.00000f,
				 0.00495f, -0.14595f,  0.98928f, 0.00000f,
				-0.03414f, -0.98874f, -0.14570f, 0.00000f,
			  -549.24292f, -15410.39258f, 9894.20996f, 1.00000f
			}
		},
		{
			"touching corner",
			true,
			float3(4383.747f, 2346.379f, 32960.035f),
			{
				 0.99793f,  0.06418f,  0.00264f, 0.00000f,
				 0.00497f, -0.11800f,  0.99300f, 0.00000f,
				 0.06405f, -0.99094f, -0.11807f, 0.00000f,
			  1069.61316f, -15425.19531f, 10352.55859f, 1.00000f
			}
		},
		{
			"overlap",
			true,
			float3(14580.339f, 8530.826f, 32977.977f),
			{
				 0.99365f, -0.11228f, -0.00752f, 0.00000f,
				 0.00500f, -0.02271f,  0.99973f, 0.00000f,
				-0.11242f, -0.99342f, -0.02200f, 0.00000f,
			  -1839.57678f, -15474.97461f, 11935.65723f, 1.00000f
			}
		},
		{
			"inside selection",
			true,
			float3(905.302f, 849.550f, 32850.859f),
			{
				 0.99804f, -0.06184f, -0.00910f, 0.00000f,
				 0.00499f, -0.06636f,  0.99778f, 0.00000f,
				-0.06230f, -0.99588f, -0.06592f, 0.00000f,
			  -1009.23425f, -15452.25781f, 11215.72070f, 1.00000f
			}
		},
		{
			"inside volume",
			true,
			float3(7916.566f, 4656.177f, 32806.605f),
			{
				 0.99984f, -0.01661f, -0.00628f, 0.00000f,
				 0.00499f, -0.07691f,  0.99703f, 0.00000f,
				-0.01705f, -0.99690f, -0.07682f, 0.00000f,
			   -278.24249f, -15755.90820f, 11026.75781f, 1.00000f
			}
		},
	}};

	for (const auto& tc : cases) {
		DYNAMIC_SECTION(tc.name) {
			const CCamera::Frustum frustum = PyramidToFrustum(tc.pyramidShape, tc.pyramidMat, ZeroVector, CollisionVolume::COLVOL_AXIS_Z);
			const bool frustumHit = CCollisionHandler::IntersectVolumeWithFrustum(frustum, testVol, testMat);
			CHECK(frustumHit == tc.intersects);
		}
	}
}

TEST_CASE("CollisionHandler_IntersectBoxVolume_BoxVsBox")
{
	const CollisionVolume boxA = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f));
	const CollisionVolume boxB = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f));
	const CMatrix44f boxAMat = MakeTransform();

	SECTION("containment") {
		const CollisionVolume smallBox = MakeBoxVolume(float3(1.0f, 1.0f, 1.0f));
		CHECK(IntersectsVolume(boxA, boxAMat, smallBox, MakeTransform(float3(0.1f, 0.1f, 0.1f))));
	}

	SECTION("overlap") {
		const CMatrix44f boxBMat = MakeTransform(float3(1.5f, 0.0f, 0.0f));
		CHECK(IntersectsVolume(boxA, boxAMat, boxB, boxBMat));
	}

	SECTION("touching faces counts as intersecting") {
		const CMatrix44f boxBMat = MakeTransform(float3(2.0f, 0.0f, 0.0f));
		CHECK(IntersectsVolume(boxA, boxAMat, boxB, boxBMat));
	}

	SECTION("separated") {
		const CMatrix44f boxBMat = MakeTransform(float3(2.5f, 0.0f, 0.0f));
		CHECK_FALSE(IntersectsVolume(boxA, boxAMat, boxB, boxBMat));
	}

	SECTION("moving box transitions from separated to intersecting") {
		constexpr float tangentDistance = 2.0f;
		bool hasBeenSeparated = false;
		bool hasTransitionedToIntersection = false;

		for (int step = 0; step <= 8; ++step) {
			const float x = 3.0f - (0.25f * step);
			const bool hit = IntersectsVolume(boxA, boxAMat, boxB, MakeTransform(float3(x, 0.0f, 0.0f)));
			const bool expectedHit = (x <= tangentDistance);

			CHECK(hit == expectedHit);

			if (!hit)
				hasBeenSeparated = true;
			if (hit && hasBeenSeparated)
				hasTransitionedToIntersection = true;
		}

		CHECK(hasBeenSeparated);
		CHECK(hasTransitionedToIntersection);
	}

	SECTION("touching corners counts as intersecting") {
		const CMatrix44f boxBMat = MakeTransform(float3(2.0f, 2.0f, 2.0f));
		CHECK(IntersectsVolume(boxA, boxAMat, boxB, boxBMat));
	}

	SECTION("corner separation") {
		const CMatrix44f boxBMat = MakeTransform(float3(2.1f, 2.1f, 2.1f));
		CHECK_FALSE(IntersectsVolume(boxA, boxAMat, boxB, boxBMat));
	}
}

TEST_CASE("CollisionHandler_IntersectBoxVolume_BoxVsSphere")
{
	const CollisionVolume box    = MakeBoxVolume(float3(4.0f, 4.0f, 4.0f));
	const CollisionVolume sphere = MakeSphereVolume(1.0f);
	const CMatrix44f boxMat = MakeTransform();

	SECTION("containment") {
		const CMatrix44f sphereMat = MakeTransform(float3(0.5f, 0.5f, 0.5f));
		CHECK(IntersectsVolume(box, boxMat, sphere, sphereMat));
	}

	SECTION("overlap") {
		const CMatrix44f sphereMat = MakeTransform(float3(2.75f, 0.0f, 0.0f));
		CHECK(IntersectsVolume(box, boxMat, sphere, sphereMat));
	}

	SECTION("tangent sphere counts as intersecting") {
		const CMatrix44f sphereMat = MakeTransform(float3(3.0f, 0.0f, 0.0f));
		CHECK(IntersectsVolume(box, boxMat, sphere, sphereMat));
	}

	SECTION("separated") {
		const CMatrix44f sphereMat = MakeTransform(float3(3.1f, 0.0f, 0.0f));
		CHECK_FALSE(IntersectsVolume(box, boxMat, sphere, sphereMat));
	}

	SECTION("corner intersection") {
		// Closest box corner to sphere is (2, 2, 2).
		// Distance from sphere center (2.5, 2.5, 2.5) to corner is sqrt(3 * 0.5^2) ≈ 0.866. (0.866 < radius 1.0)
		const CMatrix44f sphereMat = MakeTransform(float3(2.5f, 2.5f, 2.5f));
		CHECK(IntersectsVolume(box, boxMat, sphere, sphereMat));
	}

	SECTION("corner separation (verifies not just testing AABBs)") {
		// Individual AABB axes would overlap (Box is [-2, 2], Sphere AABB is [1.8, 3.8])
		// But sphere surface doesn't reach the corner! Distance is sqrt(3 * 0.8^2) ≈ 1.38 (1.38 > radius 1.0)
		const CMatrix44f sphereMat = MakeTransform(float3(2.8f, 2.8f, 2.8f));
		CHECK_FALSE(IntersectsVolume(box, boxMat, sphere, sphereMat));
	}
}

TEST_CASE("CollisionHandler_IntersectVolume_SphereVsSphere")
{
	const CollisionVolume sphereA = MakeSphereVolume(3.0f);
	const CollisionVolume sphereB = MakeSphereVolume(4.0f);
	const CMatrix44f sphereAMat = MakeTransform();

	SECTION("containment") {
		CHECK(IntersectsVolume(sphereA, sphereAMat, sphereB, MakeTransform(float3(1.0f, 0.0f, 0.0f))));
	}

	SECTION("overlap") {
		CHECK(IntersectsVolume(sphereA, sphereAMat, sphereB, MakeTransform(float3(6.5f, 0.0f, 0.0f))));
	}

	SECTION("tangent counts as intersecting") {
		CHECK(IntersectsVolume(sphereA, sphereAMat, sphereB, MakeTransform(float3(7.0f, 0.0f, 0.0f))));
	}

	SECTION("separated") {
		CHECK_FALSE(IntersectsVolume(sphereA, sphereAMat, sphereB, MakeTransform(float3(7.1f, 0.0f, 0.0f))));
	}

	SECTION("moving sphere transitions from separated to intersecting") {
		constexpr float sumRadii = 7.0f;
		bool hasBeenSeparated = false;
		bool hasTransitionedToIntersection = false;

		for (int step = 0; step <= 8; ++step) {
			const float x = 8.0f - (0.25f * step);
			const bool hit = IntersectsVolume(sphereA, sphereAMat, sphereB, MakeTransform(float3(x, 0.0f, 0.0f)));
			const bool expectedHit = (x <= sumRadii);
			CHECK(hit == expectedHit);

			if (!hit)
				hasBeenSeparated = true;
			if (hit && hasBeenSeparated)
				hasTransitionedToIntersection = true;
		}

		CHECK(hasBeenSeparated);
		CHECK(hasTransitionedToIntersection);
	}
}

TEST_CASE("CollisionHandler_IntersectBoxVolume_BoxVsEllipsoid")
{
	const CollisionVolume box       = MakeBoxVolume(float3(4.0f, 4.0f, 4.0f));
	const CollisionVolume ellipsoid = MakeEllipsoidVolume(float3(4.0f, 2.0f, 2.0f));
	const CMatrix44f boxMat = MakeTransform();

	SECTION("overlap") {
		const CMatrix44f ellipsoidMat = MakeTransform(float3(3.25f, 0.0f, 0.0f));
		CHECK(IntersectsVolume(box, boxMat, ellipsoid, ellipsoidMat));
	}

	SECTION("touching counts as intersecting") {
		const CMatrix44f ellipsoidMat = MakeTransform(float3(4.0f, 0.0f, 0.0f));
		CHECK(IntersectsVolume(box, boxMat, ellipsoid, ellipsoidMat));
	}

	SECTION("separated") {
		const CMatrix44f ellipsoidMat = MakeTransform(float3(4.1f, 0.0f, 0.0f));
		CHECK_FALSE(IntersectsVolume(box, boxMat, ellipsoid, ellipsoidMat));
	}

	SECTION("rotated ellipsoid separates") {
		// Unrotated, it reaches into the box. Rotated by 90 on Y shrinks its X influence.
		const CMatrix44f ellipsoidMat = MakeTransform(float3(4.0f, 0.0f, 0.0f), float3(0.0f, math::HALFPI, 0.0f));
		CHECK_FALSE(IntersectsVolume(box, boxMat, ellipsoid, ellipsoidMat));
	}

	SECTION("corner intersection") {
		// Ellipsoid scales: X=6, Y=2, Z=2 (radii: X=3, Y=1, Z=1).
		// Center at (4.0, 2.5, 2.0). Box corner is at (2.0, 2.0, 2.0).
		// Relative: x=(-2), y=(-0.5). Ellipsoid Equation: (4/9) + (0.25/1) + 0 = ~0.694 < 1.0 (Intersect!)
		const CollisionVolume ell = MakeEllipsoidVolume(float3(6.0f, 2.0f, 2.0f));
		const CMatrix44f ellMat = MakeTransform(float3(4.0f, 2.5f, 2.0f));
		CHECK(IntersectsVolume(box, boxMat, ell, ellMat));
	}

	SECTION("corner separation") {
		// Center at (5.0, 3.0, 2.0). Box corner is at (2.0, 2.0, 2.0).
		// Relative: x=(-3), y=(-1). Ellipsoid Equation: (9/9) + (1/1) + 0 = 2.0 > 1.0 (Separates!)
		const CollisionVolume ell = MakeEllipsoidVolume(float3(6.0f, 2.0f, 2.0f));
		const CMatrix44f ellMat = MakeTransform(float3(5.0f, 3.0f, 2.0f));
		CHECK_FALSE(IntersectsVolume(box, boxMat, ell, ellMat));
	}
}

TEST_CASE("CollisionHandler_IntersectBoxVolume_BoxVsCylinder")
{
	const CollisionVolume box = MakeBoxVolume(float3(4.0f, 4.0f, 4.0f));
	const CMatrix44f boxMat = MakeTransform();

	SECTION("x-axis cylinder bounds (length)") {
		const CollisionVolume cylinder = MakeCylinderVolume(float3(6.0f, 2.0f, 2.0f), CollisionVolume::COLVOL_AXIS_X);

		CHECK(IntersectsVolume(box, boxMat, cylinder, MakeTransform(float3(4.5f, 0.0f, 0.0f))));
		CHECK(IntersectsVolume(box, boxMat, cylinder, MakeTransform(float3(5.0f, 0.0f, 0.0f))));
		CHECK_FALSE(IntersectsVolume(box, boxMat, cylinder, MakeTransform(float3(5.2f, 0.0f, 0.0f))));
	}

	SECTION("x-axis cylinder bounds (radial)") {
		const CollisionVolume cylinder = MakeCylinderVolume(float3(6.0f, 2.0f, 2.0f), CollisionVolume::COLVOL_AXIS_X);

		CHECK(IntersectsVolume(box, boxMat, cylinder, MakeTransform(float3(0.0f, 2.5f, 0.0f))));
		CHECK(IntersectsVolume(box, boxMat, cylinder, MakeTransform(float3(0.0f, 3.0f, 0.0f))));
		CHECK_FALSE(IntersectsVolume(box, boxMat, cylinder, MakeTransform(float3(0.0f, 3.1f, 0.0f))));
	}

	SECTION("y-axis cylinder") {
		const CollisionVolume cylinder = MakeCylinderVolume(float3(2.0f, 6.0f, 2.0f), CollisionVolume::COLVOL_AXIS_Y);

		CHECK(IntersectsVolume(box, boxMat, cylinder, MakeTransform(float3(2.75f, 0.0f, 0.0f))));
		CHECK(IntersectsVolume(box, boxMat, cylinder, MakeTransform(float3(3.0f, 0.0f, 0.0f))));
		CHECK_FALSE(IntersectsVolume(box, boxMat, cylinder, MakeTransform(float3(3.1f, 0.0f, 0.0f))));
	}

	SECTION("z-axis cylinder corner check") {
		const CollisionVolume cylinder = MakeCylinderVolume(float3(2.0f, 2.0f, 6.0f), CollisionVolume::COLVOL_AXIS_Z);

		// Distance from box corner (2, 2) in XY to cylinder core (2.5, 2.5) is ~0.707 < radius 1.0
		CHECK(IntersectsVolume(box, boxMat, cylinder, MakeTransform(float3(2.5f, 2.5f, 0.0f))));

		// Distance from box corner (2, 2) to cylinder core (2.8, 2.8) is ~1.13 > radius 1.0 (AABB separates)
		CHECK_FALSE(IntersectsVolume(box, boxMat, cylinder, MakeTransform(float3(2.8f, 2.8f, 0.0f))));
	}
}

TEST_CASE("CollisionHandler_IntersectVolume_CylinderVsCylinder")
{
	const CollisionVolume cylinderA = MakeCylinderVolume(float3(6.0f, 6.0f, 8.0f), CollisionVolume::COLVOL_AXIS_Z);
	const CollisionVolume cylinderB = MakeCylinderVolume(float3(6.0f, 6.0f, 8.0f), CollisionVolume::COLVOL_AXIS_Z);
	const CMatrix44f cylinderAMat = MakeTransform();

	SECTION("parallel overlap") {
		CHECK(IntersectsVolume(cylinderA, cylinderAMat, cylinderB, MakeTransform(float3(5.5f, 0.0f, 0.0f))));
	}

	SECTION("parallel tangent counts as intersecting") {
		CHECK(IntersectsVolume(cylinderA, cylinderAMat, cylinderB, MakeTransform(float3(6.0f, 0.0f, 0.0f))));
	}

	SECTION("parallel separation") {
		CHECK_FALSE(IntersectsVolume(cylinderA, cylinderAMat, cylinderB, MakeTransform(float3(6.1f, 0.0f, 0.0f))));
	}

	SECTION("moving cylinder transitions from separated to intersecting") {
		constexpr float tangentDistance = 6.0f;
		bool hasBeenSeparated = false;
		bool hasTransitionedToIntersection = false;

		for (int step = 0; step <= 8; ++step) {
			const float x = 7.0f - (0.25f * step);
			const bool hit = IntersectsVolume(cylinderA, cylinderAMat, cylinderB, MakeTransform(float3(x, 0.0f, 0.0f)));
			const bool expectedHit = (x <= tangentDistance);

			CHECK(hit == expectedHit);

			if (!hit)
				hasBeenSeparated = true;
			if (hit && hasBeenSeparated)
				hasTransitionedToIntersection = true;
		}

		CHECK(hasBeenSeparated);
		CHECK(hasTransitionedToIntersection);
	}

	SECTION("orthogonal crossing intersects") {
		const CollisionVolume cylinderX = MakeCylinderVolume(float3(8.0f, 6.0f, 6.0f), CollisionVolume::COLVOL_AXIS_X);
		CHECK(IntersectsVolume(cylinderA, cylinderAMat, cylinderX, MakeTransform()));
	}

	SECTION("orthogonal offset separates") {
		const CollisionVolume cylinderX = MakeCylinderVolume(float3(8.0f, 6.0f, 6.0f), CollisionVolume::COLVOL_AXIS_X);
		CHECK_FALSE(IntersectsVolume(cylinderA, cylinderAMat, cylinderX, MakeTransform(float3(0.0f, 6.5f, 0.0f))));
	}
}


TEST_CASE("CollisionHandler_IntersectBoxVolume_RespectsRotation")
{
	const CollisionVolume boxA = MakeBoxVolume(float3(4.0f, 2.0f, 2.0f));
	const CollisionVolume boxB = MakeBoxVolume(float3(1.0f, 1.0f, 1.0f));

	const CMatrix44f boxAMat = MakeTransform(ZeroVector, float3(0.0f, math::QUARTERPI, 0.0f));

	SECTION("rotated box intersects") {
		const CMatrix44f boxBMat = MakeTransform(float3(2.1f, 0.0f, 0.0f));
		CHECK(IntersectsVolume(boxA, boxAMat, boxB, boxBMat));
	}

	SECTION("rotated box separates") {
		const CMatrix44f boxBMat = MakeTransform(float3(2.5f, 0.0f, 0.0f));
		CHECK_FALSE(IntersectsVolume(boxA, boxAMat, boxB, boxBMat));
	}
}

TEST_CASE("CollisionHandler_IntersectBoxVolume_BothRotated_SAT_EdgeEdge")
{
	const CollisionVolume boxA = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f));
	const CollisionVolume boxB = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f));

	// Rotate A around X, turning its Y/Z faces diagonal.
	const CMatrix44f aMat = MakeTransform(ZeroVector, float3(math::QUARTERPI, 0.0f, 0.0f));

	SECTION("edge-edge crossing") {
		// Rotate B around Y, turning its X/Z faces diagonal.
		// Moving it closely on the Z axis forces a cross-product (Edge-vs-Edge) intersection.
		const CMatrix44f bMatIntersects = MakeTransform(float3(0.0f, 0.0f, 2.8f), float3(0.0f, math::QUARTERPI, 0.0f));
		CHECK(IntersectsVolume(boxA, aMat, boxB, bMatIntersects));

		const CMatrix44f bMatSeparates = MakeTransform(float3(0.0f, 0.0f, 2.9f), float3(0.0f, math::QUARTERPI, 0.0f));
		CHECK_FALSE(IntersectsVolume(boxA, aMat, boxB, bMatSeparates));
	}
}

TEST_CASE("CollisionHandler_IntersectBoxVolume_HandlesRotatedOtherVolume")
{
	const CollisionVolume box = MakeBoxVolume(float3(4.0f, 4.0f, 4.0f));
	const CollisionVolume cylinder = MakeCylinderVolume(float3(2.0f, 6.0f, 2.0f), CollisionVolume::COLVOL_AXIS_Y);
	const CMatrix44f boxMat = MakeTransform();

	SECTION("rotated cylinder still intersects") {
		const CMatrix44f cylinderMat = MakeTransform(float3(2.75f, 0.0f, 0.0f), float3(math::HALFPI, 0.0f, 0.0f));
		CHECK(IntersectsVolume(box, boxMat, cylinder, cylinderMat));
	}

	SECTION("rotated cylinder still separates when moved away") {
		const CMatrix44f cylinderMat = MakeTransform(float3(3.6f, 0.0f, 0.0f), float3(math::HALFPI, 0.0f, 0.0f));
		CHECK_FALSE(IntersectsVolume(box, boxMat, cylinder, cylinderMat));
	}
}

TEST_CASE("CollisionHandler_IntersectBoxVolume_RespectsOffsets")
{
	// Provide a volume whose local geometry is shifted forward by 2.0 in the X-axis
	const CollisionVolume boxWithOffset = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f), float3(2.0f, 0.0f, 0.0f));
	const CMatrix44f offsetBoxMat = MakeTransform();

	const CollisionVolume targetBox = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f));

	SECTION("positive offset") {
		// The target at X=3.0 (reaches back to X=2.0). The offsetBox spans X=[1.0, 3.0]. They intersect.
		const CMatrix44f targetMatIntersects = MakeTransform(float3(3.0f, 0.0f, 0.0f));
		CHECK(IntersectsVolume(boxWithOffset, offsetBoxMat, targetBox, targetMatIntersects));

		// Separated if target moved slightly to the left of the offset bounds
		const CMatrix44f targetMatSeparates = MakeTransform(float3(-0.1f, 0.0f, 0.0f));
		CHECK_FALSE(IntersectsVolume(boxWithOffset, offsetBoxMat, targetBox, targetMatSeparates));
	}

	SECTION("negative offsets") {
		const CollisionVolume negativeOffsetBox = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f), float3(-2.0f, -2.0f, -2.0f));

		// Negative offset box reaches up to (-1, -1, -1). Target at origin reaches down to (-1, -1, -1). They touch.
		CHECK(IntersectsVolume(negativeOffsetBox, offsetBoxMat, targetBox, MakeTransform(ZeroVector)));

		// Move target box slightly right/up/forward -> they separate.
		CHECK_FALSE(IntersectsVolume(negativeOffsetBox, offsetBoxMat, targetBox, MakeTransform(float3(0.1f, 0.1f, 0.1f))));
	}
}

TEST_CASE("CollisionHandler_IntersectBoxVolume_OffsetAndRotationCoupling")
{
	// Box with local offset of +10 on the X axis
	const CollisionVolume offsetBox = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f), float3(10.0f, 0.0f, 0.0f));

	// Rotated 90 degrees around the Y axis
	const CMatrix44f offsetBoxMat = MakeTransform(ZeroVector, float3(0.0f, math::HALFPI, 0.0f));

	const CollisionVolume targetBox = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f));

	SECTION("misses unrotated offset position") {
		// Because the box is rotated, it should no longer be at +10 X.
		const CMatrix44f targetMat = MakeTransform(float3(10.0f, 0.0f, 0.0f));
		CHECK_FALSE(IntersectsVolume(offsetBox, offsetBoxMat, targetBox, targetMat));
	}

	SECTION("hits rotated offset position") {
		// +10 X rotated 90 degrees around Y becomes either +10 Z or -10 Z depending on engine handedness.
		// We test both to ensure it correctly moved to the Z axis rather than staying on X.
		const CMatrix44f targetMatPosZ = MakeTransform(float3(0.0f, 0.0f, 10.0f));
		const CMatrix44f targetMatNegZ = MakeTransform(float3(0.0f, 0.0f, -10.0f));

		bool hitsRotatedPos = IntersectsVolume(offsetBox, offsetBoxMat, targetBox, targetMatPosZ) ||
		                      IntersectsVolume(offsetBox, offsetBoxMat, targetBox, targetMatNegZ);

		CHECK(hitsRotatedPos);
	}
}

TEST_CASE("CollisionHandler_IntersectBoxVolume_ThinGeometries")
{
	const CMatrix44f originMat = MakeTransform();

	SECTION("plane vs plane (2D boxes)") {
		const CollisionVolume planeA = MakeBoxVolume(float3(10.0f, 1.f, 10.0f));
		const CollisionVolume planeB = MakeBoxVolume(float3(10.0f, 1.f, 10.0f));

		CHECK(IntersectsVolume(planeA, originMat, planeB, originMat));

		// Y-scale is clamped to 1.0, so separation starts just beyond 1.0 on Y.
		const CMatrix44f sepMat = MakeTransform(float3(0.0f, 1.01f, 0.0f));
		CHECK_FALSE(IntersectsVolume(planeA, originMat, planeB, sepMat));
	}

	SECTION("line vs line (1D boxes)") {
		const CollisionVolume lineA = MakeBoxVolume(float3(10.0f, 1.f, 0.001f));
		// Perpendicular line
		const CollisionVolume lineB = MakeBoxVolume(float3(1.f, 10.0f, 1.f));

		CHECK(IntersectsVolume(lineA, originMat, lineB, originMat));

		// Z-scale is clamped to 1.0, so separation starts just beyond 1.0 on Z.
		const CMatrix44f sepMat = MakeTransform(float3(0.0f, 0.0f, 1.01f));
		CHECK_FALSE(IntersectsVolume(lineA, originMat, lineB, sepMat));
	}
}

TEST_CASE("CollisionHandler_IntersectBoxVolume_InvertedContainment")
{
	const CollisionVolume smallBox = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f));
	const CMatrix44f boxMat = MakeTransform();

	SECTION("massive sphere completely surrounds box") {
		const CollisionVolume massiveSphere = MakeSphereVolume(100.0f);
		const CMatrix44f sphereMat = MakeTransform(float3(5.0f, 5.0f, 5.0f)); // Offset center but still totally swallowing the box
		CHECK(IntersectsVolume(smallBox, boxMat, massiveSphere, sphereMat));
	}

	SECTION("massive cylinder completely surrounds box") {
		const CollisionVolume massiveCyl = MakeCylinderVolume(float3(100.0f, 100.0f, 100.0f), CollisionVolume::COLVOL_AXIS_Y);
		const CMatrix44f cylMat = MakeTransform(float3(-10.0f, 0.0f, 10.0f));
		CHECK(IntersectsVolume(smallBox, boxMat, massiveCyl, cylMat));
	}

	SECTION("massive ellipsoid completely surrounds box") {
		const CollisionVolume massiveEllipsoid = MakeEllipsoidVolume(float3(200.0f, 100.0f, 50.0f));
		const CMatrix44f ellMat = MakeTransform();
		CHECK(IntersectsVolume(smallBox, boxMat, massiveEllipsoid, ellMat));
	}
}

TEST_CASE("CollisionHandler_IntersectBoxVolume_CylinderEndCaps")
{
	// Box half-extents: (1, 1, 1)
	const CollisionVolume box = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f));
	const CMatrix44f boxMat = MakeTransform();

	// Z-axis cylinder. Radius=1.0 (X/Y=2.0), Length=6.0 (Z half-extent=3.0)
	const CollisionVolume cylinder = MakeCylinderVolume(float3(2.0f, 2.0f, 6.0f), CollisionVolume::COLVOL_AXIS_Z);

	SECTION("flat cap flush against box face") {
		// Box face is at Z=1.0. Cylinder cap is at Z=3.0 local.
		// If cylinder center is at Z=4.0, its cap reaches Z=1.0. (Touching)
		const CMatrix44f cylMatTouch = MakeTransform(float3(0.0f, 0.0f, 4.0f));
		CHECK(IntersectsVolume(box, boxMat, cylinder, cylMatTouch));

		// Move slightly away
		const CMatrix44f cylMatSep = MakeTransform(float3(0.0f, 0.0f, 4.01f));
		CHECK_FALSE(IntersectsVolume(box, boxMat, cylinder, cylMatSep));
	}

	SECTION("end cap rim touching box corner") {
		// Box corner at (1, 1, 1).
		// Cylinder length reaches Z=1.0 if center is at Z=4.0.
		// Cylinder rim radius is 1.0. If center X is 1.0, and Y is 2.0, rim hits (1, 1, 1).
		const CMatrix44f cylMatRimTouch = MakeTransform(float3(1.0f, 2.0f, 4.0f));
		CHECK(IntersectsVolume(box, boxMat, cylinder, cylMatRimTouch));

		// Move cylinder up slightly (Y=2.1), rim misses the box corner
		const CMatrix44f cylMatRimSep = MakeTransform(float3(1.0f, 2.1f, 4.0f));
		CHECK_FALSE(IntersectsVolume(box, boxMat, cylinder, cylMatRimSep));
	}
}

TEST_CASE("CollisionHandler_IntersectBoxVolume_ExtremeEllipsoids")
{
	const CollisionVolume box = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f)); // Box half extents 1, 1, 1
	const CMatrix44f boxMat = MakeTransform();

	// Input scale values must also be > 1.0f.
	// We use a major axis of 100.0f, making the clamped minimum exactly 2.0f (radius 1.0f).

	SECTION("pancake ellipsoid (highly squashed on Y)") {
		// Scales: X=100.0, Y=2.0, Z=100.0 -> Radii: X=50.0, Y=1.0, Z=50.0
		const CollisionVolume pancake = MakeEllipsoidVolume(float3(100.0f, 2.0f, 100.0f));

		// Penetrating the top face (Center < 2.0)
		CHECK(IntersectsVolume(box, boxMat, pancake, MakeTransform(float3(0.0f, 1.9f, 0.0f))));

		// Lifted just out of reach (Center > 2.0)
		CHECK_FALSE(IntersectsVolume(box, boxMat, pancake, MakeTransform(float3(0.0f, 2.1f, 0.0f))));
	}

	SECTION("cigar ellipsoid (highly elongated on X)") {
		// Scales: X=100.0, Y=2.0, Z=2.0 -> Radii: X=50.0, Y=1.0, Z=1.0
		const CollisionVolume cigar = MakeEllipsoidVolume(float3(100.0f, 2.0f, 2.0f));

		// Passing right next to the Z face, slightly intersecting (Center < 2.0)
		CHECK(IntersectsVolume(box, boxMat, cigar, MakeTransform(float3(0.0f, 0.0f, 1.9f))));

		// Moved slightly away on Z (Center > 2.0)
		CHECK_FALSE(IntersectsVolume(box, boxMat, cigar, MakeTransform(float3(0.0f, 0.0f, 2.1f))));
	}
}
TEST_CASE("CollisionHandler_IntersectBoxVolume_SphereVsRotatedBox")
{
	const CollisionVolume box = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f)); // Extents: [-1, 1]
	const CollisionVolume sphere = MakeSphereVolume(1.0f);               // Radius: 1.0
	const CMatrix44f sphereMat = MakeTransform(float3(2.2f, 0.0f, 0.0f));

	SECTION("unrotated box misses the sphere") {
		// Box face stops at X = 1.0. Sphere edge starts at X = 1.2. (Gap of 0.2)
		const CMatrix44f boxMatUnrotated = MakeTransform();
		CHECK_FALSE(IntersectsVolume(box, boxMatUnrotated, sphere, sphereMat));
	}

	SECTION("rotated box corner hits the sphere") {
		// Rotating the box 45 degrees around Z turns its flat X-face into a pointing diamond.
		// The corner now stretches to X = sqrt(1^2 + 1^2) = 1.414.
		// Sphere edge is at X = 1.2. Because 1.414 > 1.2, the sharp corner pierces the sphere.
		const CMatrix44f boxMatRotated = MakeTransform(ZeroVector, float3(0.0f, 0.0f, math::QUARTERPI));
		CHECK(IntersectsVolume(box, boxMatRotated, sphere, sphereMat));
	}
}
TEST_CASE("CollisionHandler_IntersectBoxVolume_BoxVsRotatedEllipsoid")
{
	const CollisionVolume box = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f)); // Extents: [-1, 1]
	const CMatrix44f boxMat = MakeTransform();

	// Scales: X=10.0, Y=2.0, Z=2.0 -> Radii: X=5.0, Y=1.0, Z=1.0
	// Satisfies the max scale 2% constraint (10.0 * 0.02 = 0.2 < 2.0)
	const CollisionVolume ellipsoid = MakeEllipsoidVolume(float3(10.0f, 2.0f, 2.0f));

	// Place ellipsoid off-center diagonally from the box corner
	const float3 pos(4.0f, 4.0f, 0.0f);

	SECTION("unrotated ellipsoid misses") {
		// Unrotated, the long X-axis stretches from X=-1 to X=9.
		// However, it remains at Y=4, which completely bypasses the Box's Y extents [-1, 1].
		const CMatrix44f ellMatUnrotated = MakeTransform(pos);
		CHECK_FALSE(IntersectsVolume(box, boxMat, ellipsoid, ellMatUnrotated));
	}

	SECTION("ellipsoid rotated perfectly toward box intersects") {
		// Spring's positive Z rotation turns +X toward -Y, so -45 degrees points the major axis
		// along the diagonal from (4, 4, 0) toward the box.
		// Distance from (4,4,0) to box corner (1,1,0) is sqrt(3^2 + 3^2) ≈ 4.24.
		// Since the rotated major radius (5.0) is greater than 4.24, it reaches the corner.
		const CMatrix44f ellMatAimed = MakeTransform(pos, float3(0.0f, 0.0f, -math::QUARTERPI));
		CHECK(IntersectsVolume(box, boxMat, ellipsoid, ellMatAimed));
	}

	SECTION("ellipsoid rotated perpendicular to box misses") {
		// +45 degrees leaves the major axis perpendicular to the diagonal back toward the box.
		// Only the minor radius (1.0) faces the box, which is much less than the 4.24 distance.
		const CMatrix44f ellMatAway = MakeTransform(pos, float3(0.0f, 0.0f, math::QUARTERPI));
		CHECK_FALSE(IntersectsVolume(box, boxMat, ellipsoid, ellMatAway));
	}
}
TEST_CASE("CollisionHandler_IntersectBoxVolume_SlantedCylinder")
{
	const CollisionVolume box = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f)); // Extents [-1, 1]
	const CMatrix44f boxMat = MakeTransform();

	// Y-axis cylinder. Radius=1.0, Half-length=5.0
	const CollisionVolume cylinder = MakeCylinderVolume(float3(2.0f, 10.0f, 2.0f), CollisionVolume::COLVOL_AXIS_Y);

	SECTION("unrotated misses overhead") {
		// Cylinder is suspended directly above the box. Lowest tip reaches Y = 4.0 - 5.0 = -1.0.
		// But wait, Z is at 4.0. The box only covers Z up to 1.0. Misses completely.
		const CMatrix44f cylMatUpright = MakeTransform(float3(0.0f, 4.0f, 4.0f));
		CHECK_FALSE(IntersectsVolume(box, boxMat, cylinder, cylMatUpright));
	}

	SECTION("slanted towards box hits") {
		// Spring's positive X rotation tilts +Y toward -Z, so -45 degrees slants the lower end-cap
		// downward and inward toward the box.
		// Tip travel is approx 5.0 * 0.707 = 3.53 units on Y and Z axes.
		// New tip position: Y = 4.0 - 3.53 = 0.47, Z = 4.0 - 3.53 = 0.47.
		// Since (0, 0.47, 0.47) is inside the box [-1, 1], it pierces the top face!
		const CMatrix44f cylMatSlanted = MakeTransform(float3(0.0f, 4.0f, 4.0f), float3(-math::QUARTERPI, 0.0f, 0.0f));
		CHECK(IntersectsVolume(box, boxMat, cylinder, cylMatSlanted));
	}

	SECTION("slanted too far out misses") {
		// Same 45 degree slant, but moved further out.
		// Tip reaches Y = 6.0 - 3.53 = 2.47. Z = 6.0 - 3.53 = 2.47.
		// Box max is 1.0, so it stops short.
		const CMatrix44f cylMatSlantedFar = MakeTransform(float3(0.0f, 6.0f, 6.0f), float3(-math::QUARTERPI, 0.0f, 0.0f));
		CHECK_FALSE(IntersectsVolume(box, boxMat, cylinder, cylMatSlantedFar));
	}
}
TEST_CASE("CollisionHandler_IntersectBoxVolume_MultiAxisObliqueBoxes")
{
	const CollisionVolume boxA = MakeBoxVolume(float3(4.0f, 4.0f, 4.0f)); // Extents: 2, 2, 2
	const CollisionVolume boxB = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f)); // Extents: 1, 1, 1

	const CMatrix44f matA = MakeTransform();

	// Rotate boxB by 45 degrees on ALL three axes.
	const float3 crazyRotation(math::QUARTERPI, math::QUARTERPI, math::QUARTERPI);

	SECTION("deep multi-axis overlap") {
		const CMatrix44f matB_Hits = MakeTransform(float3(2.5f, 2.5f, 2.5f), crazyRotation);
		CHECK(IntersectsVolume(boxA, matA, boxB, matB_Hits));
	}

	SECTION("glancing multi-axis corner touch") {
		// For this specific three-axis rotation, the limiting SAT axes are the A.y x B.z and
		// A.z x B.z cross-products, which separate at about 2.828 on each center coordinate.
		// Keep boxB just inside that bound so this remains a true near-tangent overlap.
		const CMatrix44f matB_Glance = MakeTransform(float3(2.82f, 2.82f, 2.82f), crazyRotation);
		CHECK(IntersectsVolume(boxA, matA, boxB, matB_Glance));
		const CMatrix44f matB_Far = MakeTransform(float3(2.89f, 2.89f, 2.89f), crazyRotation);
		CHECK_FALSE(IntersectsVolume(boxA, matA, boxB, matB_Far));
	}

	SECTION("multi-axis separation") {
		// Moved entirely out of the collision zone.
		const CMatrix44f matB_Misses = MakeTransform(float3(4.5f, 4.5f, 4.5f), crazyRotation);
		CHECK_FALSE(IntersectsVolume(boxA, matA, boxB, matB_Misses));
	}
}

TEST_CASE("CollisionHandler_IntersectBoxVolume_RotatedLineContactCountsAsIntersection")
{
	const CollisionVolume boxA = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f));
	const CollisionVolume boxB = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f));
	const CMatrix44f boxAMat = MakeTransform();
	const CMatrix44f boxBMat =
		MakeTransform(float3(2.41421356f, 0.0f, 0.0f), float3(0.0f, 0.0f, math::QUARTERPI));

	CHECK(IntersectsVolume(boxA, boxAMat, boxB, boxBMat));
}

// TEST_CASE("CollisionHandler_IntersectBoxVolume_AxisAligned_Performance")
// {
// 	const CollisionVolume box = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f));
// 	const CollisionVolume cylinder = MakeCylinderVolume(float3(2.0f, 2.0f, 6.0f), CollisionVolume::COLVOL_AXIS_Z);
// 	const CMatrix44f boxMat = MakeTransform();

// 	std::array<CMatrix44f, 64> hitMats;
// 	std::array<CMatrix44f, 64> missMats;

// 	for (std::size_t i = 0; i < hitMats.size(); ++i) {
// 		const float x = 0.75f + ((i % 8) * 0.05f);
// 		hitMats[i] = MakeTransform(float3(x, 2.0f, 4.0f));
// 		missMats[i] = MakeTransform(float3(x, 2.2f, 4.0f));
// 	}

// 	const std::int64_t iterations = 10000000;
// 	volatile std::int64_t sink = 0;

// 	LOG("CollisionHandler axis-aligned box-volume:");
// 	{
// 		ScopedOnceTimer timer(" axis-aligned box vs cylinder (intersecting)");
// 		for (std::int64_t j = iterations; j > 0; --j) {
// 			sink ^= static_cast<std::int64_t>(IntersectsVolume(box, boxMat, cylinder, hitMats[j % hitMats.size()])) * j;
// 		}
// 	}
// 	{
// 		ScopedOnceTimer timer(" axis-aligned box vs cylinder (separated)");
// 		for (std::int64_t j = iterations; j > 0; --j) {
// 			sink ^= static_cast<std::int64_t>(IntersectsVolume(box, boxMat, cylinder, missMats[j % missMats.size()])) * j;
// 		}
// 	}

// 	CHECK((sink | 1) != 0);
// }

// TEST_CASE("CollisionHandler_IntersectBoxVolume_Rotated_Performance")
// {
// 	const CollisionVolume box = MakeBoxVolume(float3(2.0f, 2.0f, 2.0f));
// 	const CollisionVolume cylinder = MakeCylinderVolume(float3(2.0f, 2.0f, 6.0f), CollisionVolume::COLVOL_AXIS_Z);
// 	const CMatrix44f boxMat = MakeTransform(ZeroVector, float3(0.0f, math::QUARTERPI, 0.0f));

// 	std::array<CMatrix44f, 64> hitMats;
// 	std::array<CMatrix44f, 64> missMats;

// 	for (std::size_t i = 0; i < hitMats.size(); ++i) {
// 		const float y = 0.75f + ((i % 8) * 0.05f);
// 		hitMats[i] = MakeTransform(float3(2.75f, y, 0.0f), float3(math::HALFPI, 0.0f, 0.0f));
// 		missMats[i] = MakeTransform(float3(3.6f, y, 0.0f), float3(math::HALFPI, 0.0f, 0.0f));
// 	}

// 	const std::int64_t iterations = 10000000;
// 	volatile std::int64_t sink = 0;

// 	LOG("CollisionHandler rotated box-volume:");
// 	{
// 		ScopedOnceTimer timer(" rotated box vs cylinder (intersecting)");
// 		for (std::int64_t j = iterations; j > 0; --j) {
// 			sink ^= static_cast<std::int64_t>(IntersectsVolume(box, boxMat, cylinder, hitMats[j % hitMats.size()])) * j;
// 		}
// 	}
// 	{
// 		ScopedOnceTimer timer(" rotated box vs cylinder (separated)");
// 		for (std::int64_t j = iterations; j > 0; --j) {
// 			sink ^= static_cast<std::int64_t>(IntersectsVolume(box, boxMat, cylinder, missMats[j % missMats.size()])) * j;
// 		}
// 	}

// 	CHECK((sink | 1) != 0);
// }
