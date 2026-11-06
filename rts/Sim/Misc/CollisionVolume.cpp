/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CollisionVolume.h"
#include "Rendering/Models/3DModelPiece.hpp"
#include "Rendering/Models/LocalModelPiece.hpp"
#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"

#include "System/Misc/TracyDefs.h"

CR_BIND(CollisionVolume, )
CR_REG_METADATA(CollisionVolume, (
	CR_MEMBER(fullAxisScales),
	CR_IGNORED(halfAxisScales),
	CR_IGNORED(halfAxisScalesSqr),
	CR_IGNORED(halfAxisScalesInv),
	CR_MEMBER(axisOffsets),

	CR_IGNORED(volumeBoundingRadius),
	CR_IGNORED(volumeBoundingRadiusSq),

	CR_MEMBER(volumeType),
	CR_MEMBER(volumeAxes),

	CR_MEMBER(ignoreHits),
	CR_MEMBER(useContHitTest),
	CR_MEMBER(defaultToFootPrint),
	CR_MEMBER(defaultToPieceTree),

	CR_POSTLOAD(PostLoad)
))

CollisionVolume& CollisionVolume::operator = (const CollisionVolume& v) {
	fullAxisScales         = v.fullAxisScales;
	halfAxisScales         = v.halfAxisScales;
	halfAxisScalesSqr      = v.halfAxisScalesSqr;
	halfAxisScalesInv      = v.halfAxisScalesInv;
	axisOffsets            = v.axisOffsets;

	volumeBoundingRadius   = v.volumeBoundingRadius;
	volumeBoundingRadiusSq = v.volumeBoundingRadiusSq;

	volumeType             = v.volumeType;
	volumeAxes[0]          = v.volumeAxes[0];
	volumeAxes[1]          = v.volumeAxes[1];
	volumeAxes[2]          = v.volumeAxes[2];

	ignoreHits             = v.ignoreHits;
	useContHitTest         = v.useContHitTest;
	defaultToFootPrint     = v.defaultToFootPrint;
	defaultToPieceTree     = v.defaultToPieceTree;

	return *this;
}

CollisionVolume::CollisionVolume(
	const char cvTypeChar,
	const char cvAxisChar,
	const float3& cvScales,
	const float3& cvOffsets
) {
	RECOIL_DETAILED_TRACY_ZONE;
	// default-initialize
	*this = CollisionVolume();

	int cvType = COLVOL_TYPE_SPHERE;
	int cvAxis = COLVOL_AXIS_Z;

	switch (cvTypeChar) {
		case 'E': case 'e': { cvType = COLVOL_TYPE_ELLIPSOID; } break; // "[E|e]ll..."
		case 'C': case 'c': { cvType = COLVOL_TYPE_CYLINDER;  } break; // "[C|c]yl..."
		case 'B': case 'b': { cvType = COLVOL_TYPE_BOX;       } break; // "[B|b]ox"
		case 'S': case 's': { cvType = COLVOL_TYPE_SPHERE;    } break; // "[S|s]ph..."
		default           : {                                 } break;
	}

	if (cvType == COLVOL_TYPE_CYLINDER) {
		switch (cvAxisChar) {
			case 'X': case 'x': { cvAxis = COLVOL_AXIS_X; } break;
			case 'Y': case 'y': { cvAxis = COLVOL_AXIS_Y; } break;
			case 'Z': case 'z': { cvAxis = COLVOL_AXIS_Z; } break;
			default           : {                         } break; // just use the z-axis
		}
	}

	InitShape(cvScales, cvOffsets, cvType, COLVOL_HITTEST_CONT, cvAxis);
}


void CollisionVolume::PostLoad()
{
	RECOIL_DETAILED_TRACY_ZONE;
	SetAxisScales(fullAxisScales);
	SetBoundingRadius();
}


void CollisionVolume::InitShape(
	const float3& scales,
	const float3& offsets,
	const int vType,
	const int tType,
	const int pAxis
) {
	RECOIL_DETAILED_TRACY_ZONE;
	axisOffsets = offsets;

	// make sure none of the scales are ever negative or zero
	//
	// if the clamped vector is <1, 1, 1> (ie. all scales were <= 1.0f)
	// then we assume a "default volume" is wanted and the unit/feature
	// instances will be assigned spheres (of size model->radius)
	//
	float3 clampedScales = float3::max(scales, OnesVector);

	// assign these here, since we can be
	// called from outside the constructor
	volumeType    = std::max(vType, 0) % (COLVOL_TYPE_POLYGON + 1);
	volumeAxes[0] = std::max(pAxis, 0) % (COLVOL_AXIS_Z + 1);

	// POLYGON takes its shape AND its placement straight from the piece
	// geometry, so any caller-supplied offset would double-shift it away
	// from the mesh it is supposed to trace. Zero it so that the matrix
	// built in IntersectPiecesHelper (which translates by GetOffsets())
	// leaves the ray exactly in vertex space.
	if (volumeType == COLVOL_TYPE_POLYGON)
		axisOffsets = ZeroVector;

	///< [0] is primary axis, [1] and [2] are secondary (all COLVOL_AXIS_*)
	switch (volumeAxes[0]) {
		case COLVOL_AXIS_X: {
			volumeAxes[1] = COLVOL_AXIS_Y;
			volumeAxes[2] = COLVOL_AXIS_Z;
		} break;
		case COLVOL_AXIS_Y: {
			volumeAxes[1] = COLVOL_AXIS_X;
			volumeAxes[2] = COLVOL_AXIS_Z;
		} break;
		case COLVOL_AXIS_Z: {
			volumeAxes[1] = COLVOL_AXIS_X;
			volumeAxes[2] = COLVOL_AXIS_Y;
		} break;
	}

	FixTypeAndScale(clampedScales);
	SetAxisScales(clampedScales);
	SetBoundingRadius();
	SetUseContHitTest(tType == COLVOL_HITTEST_CONT);
}


void CollisionVolume::SetBoundingRadius() {
	RECOIL_DETAILED_TRACY_ZONE;
	// set the radius of the minimum bounding sphere
	// that encompasses this custom collision volume
	// (for early-out testing)
	// NOTE:
	//   this must be called manually after either
	//   a call to SetAxisScales or to RescaleAxes
	switch (volumeType) {
		case COLVOL_TYPE_BOX: {
			// would be an over-estimation for cylinders
			volumeBoundingRadiusSq = halfAxisScalesSqr.x + halfAxisScalesSqr.y + halfAxisScalesSqr.z;
			volumeBoundingRadius = math::sqrt(volumeBoundingRadiusSq);
		} break;
		case COLVOL_TYPE_CYLINDER: {
			const float prhs = halfAxisScales[volumeAxes[0]];   // primary axis half-scale
			const float sahs = halfAxisScales[volumeAxes[1]];   // 1st secondary axis half-scale
			const float sbhs = halfAxisScales[volumeAxes[2]];   // 2nd secondary axis half-scale
			const float mshs = std::max(sahs, sbhs);            // max. secondary axis half-scale

			volumeBoundingRadiusSq = prhs * prhs + mshs * mshs;
			volumeBoundingRadius = math::sqrt(volumeBoundingRadiusSq);
		} break;
		case COLVOL_TYPE_SPHERE: {
			volumeBoundingRadius = halfAxisScales.x;
			volumeBoundingRadiusSq = volumeBoundingRadius * volumeBoundingRadius;
		} break;
		case COLVOL_TYPE_ELLIPSOID: {
			volumeBoundingRadius = std::max(halfAxisScales.x, std::max(halfAxisScales.y, halfAxisScales.z));
			volumeBoundingRadiusSq = volumeBoundingRadius * volumeBoundingRadius;
		} break;
		case COLVOL_TYPE_POLYGON: {
			// not used: the broad-phase radius for this type is derived
			// from the piece's own mins/maxs at test time (see Intersect)
		} break;
	}
}

void CollisionVolume::SetAxisScales(const float3& scales) {
	RECOIL_DETAILED_TRACY_ZONE;
	fullAxisScales = scales;
	halfAxisScales = fullAxisScales * 0.5f;

	halfAxisScalesSqr = halfAxisScales * halfAxisScales;
	halfAxisScalesInv = OnesVector / halfAxisScales;
}

void CollisionVolume::RescaleAxes(const float3& scales) {
	RECOIL_DETAILED_TRACY_ZONE;
	fullAxisScales *= scales;
	halfAxisScales *= scales;

	// h*h --> h*h*s*s; 1/h --> 1/h/s = 1/(h*s)
	halfAxisScalesSqr *= (scales * scales);
	halfAxisScalesInv /= scales;
}

void CollisionVolume::FixTypeAndScale(float3& scales) {
	RECOIL_DETAILED_TRACY_ZONE;
	// NOTE:
	//   prevent Lua (which calls InitShape directly) from
	//   creating non-uniform spheres to emulate ellipsoids
	switch (volumeType) {
		case COLVOL_TYPE_SPHERE: {
			scales = OnesVector * std::max(scales.x, std::max(scales.y, scales.z));
			return;
		} break;

		case COLVOL_TYPE_ELLIPSOID: {
			if (scales.x == scales.y && scales.y == scales.z) {
				volumeType = COLVOL_TYPE_SPHERE;
			} else {
				// disallow insane ellipsoids
				scales = float3::max(scales, OnesVector * std::fmax(scales.x, std::max(scales.y, scales.z)) * 0.02f);
			}

			return;
		} break;

		case COLVOL_TYPE_CYLINDER: {
			scales[volumeAxes[1]] = std::max(scales[volumeAxes[1]], scales[volumeAxes[2]]);
			scales[volumeAxes[2]] =          scales[volumeAxes[1]];
		} break;

		default: {
		} break;
	}
}



float3 CollisionVolume::GetWorldSpacePos(const CSolidObject* o, const float3& extOffsets) const {
	RECOIL_DETAILED_TRACY_ZONE;
	// collision-volumes are always centered on midPos
	return (o->midPos + o->GetObjectSpaceVec(axisOffsets + extOffsets));
}



float CollisionVolume::GetPointSurfaceDistance(const CUnit* u, const LocalModelPiece* lmp, const float3& pos) const {
	RECOIL_DETAILED_TRACY_ZONE;
	return (GetPointSurfaceDistance(u, lmp, u->GetTransformMatrix(true), pos));
}

float CollisionVolume::GetPointSurfaceDistance(const CFeature* f, const LocalModelPiece* lmp, const float3& pos) const {
	RECOIL_DETAILED_TRACY_ZONE;
	return (GetPointSurfaceDistance(f, lmp, f->GetTransformMatrixRef(true), pos));
}

float CollisionVolume::GetPointSurfaceDistance(
	const CSolidObject* obj,
	const LocalModelPiece* lmp,
	const CMatrix44f& mat,
	const float3& pos
) const {
	RECOIL_DETAILED_TRACY_ZONE;
	CMatrix44f vm = mat;

	if (lmp != nullptr && (obj->collisionVolume).DefaultToPieceTree()) {
		// NOTE: if we get here, <this> is the piece-volume
		assert(this == lmp->GetCollisionVolume());

		// transform into piece-space relative to pos
		vm <<= lmp->GetModelSpaceMatrix();
	} else {
		// SObj::GetTransformMatrix does not include this
		// (its translation component is pos, not midPos)
		vm.Translate(obj->relMidPos);
	}

	vm.Translate(GetOffsets());
	vm.InvertAffineInPlace();

	return (GetPointSurfaceDistance(vm, pos, lmp));
}



float CollisionVolume::GetPointSurfaceDistance(const CMatrix44f& mv, const float3& p, const LocalModelPiece* lmp) const {
	RECOIL_DETAILED_TRACY_ZONE;
	// transform <p> from world- to volume-space
	float3 pv = mv.Mul(p);

	float d = 0.0f;

	switch (volumeType) {
		case COLVOL_TYPE_BOX: {
			// always clamp <pv> to box (!) surface
			// (so minimum distance is always zero)
			pv.x = ((int(pv.x >= 0.0f) * 2) - 1) * std::max(math::fabs(pv.x), halfAxisScales.x);
			pv.y = ((int(pv.y >= 0.0f) * 2) - 1) * std::max(math::fabs(pv.y), halfAxisScales.y);
			pv.z = ((int(pv.z >= 0.0f) * 2) - 1) * std::max(math::fabs(pv.z), halfAxisScales.z);

			// calculate the closest surface point
			float3 pt;
			pt.x = std::clamp(pv.x, -halfAxisScales.x, halfAxisScales.x);
			pt.y = std::clamp(pv.y, -halfAxisScales.y, halfAxisScales.y);
			pt.z = std::clamp(pv.z, -halfAxisScales.z, halfAxisScales.z);

			// float l = std::min(pv.x - pt.x, std::min(pv.y - pt.y, pv.z - pt.z));
			d = pv.distance(pt);
		} break;

		case COLVOL_TYPE_SPHERE: {
			float l = pv.Length();
			d = std::max(l - volumeBoundingRadius, 0.0f);
			//float3 pt = (pv / std::max(0.01f, l)) * d;
		} break;

		case COLVOL_TYPE_CYLINDER: {
			// code below is only valid for non-ellipsoidal cylinders
			assert(halfAxisScales   [volumeAxes[1]] == halfAxisScales   [volumeAxes[2]]);
			assert(halfAxisScalesSqr[volumeAxes[1]] == halfAxisScalesSqr[volumeAxes[2]]);

			switch (volumeAxes[0]) {
				case COLVOL_AXIS_X: { d = GetCylinderDistance(pv, 0, 1, 2); } break;
				case COLVOL_AXIS_Y: { d = GetCylinderDistance(pv, 1, 0, 2); } break;
				case COLVOL_AXIS_Z: { d = GetCylinderDistance(pv, 2, 0, 1); } break;
			}
		} break;

		case COLVOL_TYPE_ELLIPSOID: {
			d = GetEllipsoidDistance(pv);
		} break;

		case COLVOL_TYPE_POLYGON: {
			// distance to the closest point on the closest triangle; the
			// same triangles IntersectPolygon() traces, in the same space,
			// so the two never disagree about where the volume is
			d = GetPolygonDistance(pv, lmp);
		} break;

		default: {
			assert(false);
		} break;
	}

	return d;
}



// closest point on a triangle to p (standard barycentric region test)
static float3 ClosestPointOnTriangle(const float3& p, const float3& a, const float3& b, const float3& c)
{
	const float3 ab = b - a;
	const float3 ac = c - a;
	const float3 ap = p - a;

	const float d1 = ab.dot(ap);
	const float d2 = ac.dot(ap);
	if (d1 <= 0.0f && d2 <= 0.0f) return a;

	const float3 bp = p - b;
	const float d3 = ab.dot(bp);
	const float d4 = ac.dot(bp);
	if (d3 >= 0.0f && d4 <= d3) return b;

	const float vc = d1 * d4 - d3 * d2;
	if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
		return a + ab * (d1 / (d1 - d3));

	const float3 cp = p - c;
	const float d5 = ab.dot(cp);
	const float d6 = ac.dot(cp);
	if (d6 >= 0.0f && d5 <= d6) return c;

	const float vb = d5 * d2 - d1 * d6;
	if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
		return a + ac * (d2 / (d2 - d6));

	const float va = d3 * d6 - d5 * d4;
	if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
		return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));

	const float denom = 1.0f / (va + vb + vc);
	return a + ab * (vb * denom) + ac * (vc * denom);
}

// Crossing parity along an arbitrary ray: odd means the point is enclosed.
// The direction is deliberately skew, so that it is very unlikely to graze an
// edge or a vertex, where a crossing could be counted twice or not at all.
static bool PointInsidePolygonMesh(const float3& p, const std::vector<float3>& verts, const std::vector<uint32_t>& indcs)
{
	constexpr float EPS = 1e-6f;
	const float3 dir = float3(0.5773503f, 0.6981317f, 0.4240113f).SafeNormalize();

	int crossings = 0;

	for (size_t i = 0; i + 2 < indcs.size(); i += 3) {
		const uint32_t ia = indcs[i + 0];
		const uint32_t ib = indcs[i + 1];
		const uint32_t ic = indcs[i + 2];

		if (ia >= verts.size() || ib >= verts.size() || ic >= verts.size())
			continue;

		const float3& v0 = verts[ia];
		const float3 e1 = verts[ib] - v0;
		const float3 e2 = verts[ic] - v0;
		const float3 pv = dir.cross(e2);
		const float det = e1.dot(pv);

		if (math::fabs(det) <= EPS)
			continue;

		const float invDet = 1.0f / det;
		const float3 tv = p - v0;

		const float u = tv.dot(pv) * invDet;

		if (u < 0.0f || u > 1.0f)
			continue;

		const float3 qv = tv.cross(e1);
		const float w = dir.dot(qv) * invDet;

		if (w < 0.0f || (u + w) > 1.0f)
			continue;

		if ((e2.dot(qv) * invDet) > EPS)
			crossings++;
	}

	return ((crossings & 1) == 1);
}

float CollisionVolume::GetPolygonDistance(const float3& pv, const LocalModelPiece* lmp) const
{
	RECOIL_DETAILED_TRACY_ZONE;

	// same triangles IntersectPolygon() traces, reached the same way, so
	// the two can never disagree about where the volume actually is
	const S3DModelPiece* piece = (lmp != nullptr) ? lmp->original : nullptr;

	if (piece == nullptr || !piece->HasGeometryData())
		return 0.0f;

	const auto& verts = piece->GetCollisionVertsVec();
	const auto& indcs = piece->GetIndicesVec();

	if (verts.empty())
		return 0.0f;

	float minDistSq = std::numeric_limits<float>::max();

	for (size_t i = 0; i + 2 < indcs.size(); i += 3) {
		const uint32_t ia = indcs[i + 0];
		const uint32_t ib = indcs[i + 1];
		const uint32_t ic = indcs[i + 2];

		if (ia >= verts.size() || ib >= verts.size() || ic >= verts.size())
			continue;

		const float3 q = ClosestPointOnTriangle(pv, verts[ia], verts[ib], verts[ic]);
		minDistSq = std::min(minDistSq, (q - pv).SqLength());
	}

	if (minDistSq == std::numeric_limits<float>::max())
		return 0.0f;

	// Every other volume type reports 0 for a point inside itself, and the
	// callers rely on it: explosion damage falls off with this distance and
	// stops applying entirely past the blast radius. Without this a charge
	// going off inside a hull would be treated as sitting a hull-width away.
	//
	// Only asked of a closed surface, since "inside" means nothing for an
	// open one, and answering it anyway would put distant points on the far
	// side of a single plate "inside" it.
	if (piece->HasClosedCollisionMesh() && PointInsidePolygonMesh(pv, verts, indcs))
		return 0.0f;

	return math::sqrt(minDistSq);
}

float CollisionVolume::GetCylinderDistance(const float3& pv, size_t axisA, size_t axisB, size_t axisC) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float pSq = (pv[axisB] * pv[axisB]) + (pv[axisC] * pv[axisC]);
	const float rSq = (halfAxisScalesSqr[axisB] + halfAxisScalesSqr[axisC]) * 0.5f;

	float d = 0.0f;

	if (pv[axisA] >= -halfAxisScales[axisA] && pv[axisA] <= halfAxisScales[axisA]) {
		/* case 1: point is between end-cap bounds along primary axis */
		d = std::max(math::sqrt(pSq) - math::sqrt(rSq), 0.0f);
	} else {
		if (pSq <= rSq) {
			/* case 2: point is outside end-cap bounds but inside inf-tube */
			d = std::max(math::fabs(pv[axisA]) - halfAxisScales[axisA], 0.0f);
		} else {
			/* case 3: compute orthogonal distance to end-cap edge (rim) */
			const float l = Square(math::fabs(pv[axisA]) - halfAxisScales[axisA]);
			d = Square(std::max(math::sqrt(pSq) - math::sqrt(rSq), 0.0f));
			d = math::sqrt(l + d);
		}
	}

	return d;
}

#define MAX_ITERATIONS 10
#define THRESHOLD 0.001

//Newton's method according to http://wwwf.imperial.ac.uk/~rn/distance2ellipse.pdf
float CollisionVolume::GetEllipsoidDistance(const float3& pv) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3& abc1 = halfAxisScales;    // {a, b, c}
	const float3& abc2 = halfAxisScalesSqr; // {a2, b2, c2}

	assert(abc1.x > 0.0f && abc1.y > 0.0f && abc1.z > 0.0f);
	assert(abc2.x > 0.0f && abc2.y > 0.0f && abc2.z > 0.0f);

	const float3 xyz1     = float3::fabs(pv);     // {x, y, z}
	const float3 xyz1abc1 = (xyz1       ) * abc1; // {xa, yb, zc}
	const float3 xyz2abc2 = (xyz1 * xyz1) / abc2; // {x2_a2, y2_b2, z2_c2}, same as xyzSq * Square(halfAxisScalesInv)

	//bail if inside the ellipsoid
	if (xyz2abc2.dot(OnesVector) <= 1.0f)
		return 0.0f;

	//Initial guess
	float theta = math::atan2(abc1.x * xyz1.y, abc1.y * xyz1.x);
	float phi = math::atan2(xyz1.z, abc1.z * math::sqrt(xyz2abc2.x + xyz2abc2.y));

	float currDist = 0.0f;
	float lastDist = 0.0f;

	//Iterations
	for (int i = 0; i < MAX_ITERATIONS; i++) {
		const float sint = math::sin(theta);
		const float cost = math::cos(theta);
		const float sinp = math::sin(phi);
		const float cosp = math::cos(phi);

		{
			const float3 angs = {cosp * cost, cosp * sint, sinp};
			const float3 fxyz = (abc1 * angs) - xyz1; // {fx, fy, fz}

			lastDist = currDist;
			currDist = fxyz.Length();

			if (math::fabsf(currDist - lastDist) < THRESHOLD * currDist)
				break;
		}

		const float sin2t = sint * sint;
		const float xacost_ybsint = xyz1abc1.x * cost + xyz1abc1.y * sint;
		const float xasint_ybcost = xyz1abc1.x * sint - xyz1abc1.y * cost;
		const float a2b2costsint = (abc2.x - abc2.y) * cost * sint;
		const float a2cos2t_b2sin2t_c2 = abc2.x * cost * cost + abc2.y * sin2t - abc2.z;

		const float d1 = a2b2costsint * cosp - xasint_ybcost;
		const float d2 = a2cos2t_b2sin2t_c2 * sinp * cosp - sinp * xacost_ybsint + xyz1abc1.z * cosp;

		//Derivative matrix
		const float a11 = (abc2.x - abc2.y) * (1 - 2 * sin2t) * cosp - xacost_ybsint;
		const float a12 = -a2b2costsint * sinp;
		const float a21 = 2 * a12 * cosp + sinp * xasint_ybcost;
		const float a22 = a2cos2t_b2sin2t_c2 * (1 - 2 * sinp * sinp) - cosp * xacost_ybsint - xyz1abc1.z;

		const float invDet = 1.0f / (a11 * a22 - a21 * a12);

		theta += (a12 * d2 - a22 * d1) * invDet;
		theta = std::clamp(theta, 0.0f, math::HALFPI);
		phi += (a21 * d1 - a11 * d2) * invDet;
		phi = std::clamp(phi, 0.0f, math::HALFPI);
	}

	return currDist;
}

