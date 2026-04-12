/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "Quaternion.h"
#include "System/SpringMath.h"

//contains some code from
// https://github.com/ilmola/gml/blob/master/include/gml/quaternion.hpp
// https://github.com/ilmola/gml/blob/master/include/gml/mat.hpp
// https://github.com/g-truc/glm/blob/master/glm/ext/quaternion_common.inl
// Also nice source https://www.shadertoy.com/view/fdtfWM

CR_BIND(CQuaternion, )
CR_REG_METADATA(CQuaternion, (
	CR_MEMBER(x),
	CR_MEMBER(y),
	CR_MEMBER(z),
	CR_MEMBER(r)
))

/// <summary>
/// Quaternion from Euler PYR/XYZ angles
/// </summary>
CQuaternion CQuaternion::FromEulerPYR(const float3& angles)
{
	// CMatrix44f::RotateEulerXYZ defines it as (R=R(Z)*R(Y)*R(X))
	// so R(r)*R(y)*R(p)

	const float sp = math::sin(angles[CMatrix44f::ANGLE_P] * 0.5f);
	const float cp = math::cos(angles[CMatrix44f::ANGLE_P] * 0.5f);
	const float sy = math::sin(angles[CMatrix44f::ANGLE_Y] * 0.5f);
	const float cy = math::cos(angles[CMatrix44f::ANGLE_Y] * 0.5f);
	const float sr = math::sin(angles[CMatrix44f::ANGLE_R] * 0.5f);
	const float cr = math::cos(angles[CMatrix44f::ANGLE_R] * 0.5f);

	CQuaternion pyrQ{
		cr * cy * sp + cp * sr * sy,
		cp * cr * sy - cy * sp * sr,
		cp * cy * sr + cr * sp * sy,
		cp * cr * cy - sp * sr * sy
	};

	return AssertNormalized(pyrQ);
}

/// <summary>
/// Quaternion from Euler YPR/YXZ angles
/// </summary>
CQuaternion CQuaternion::FromEulerYPR(const float3& angles)
{
	// CMatrix44f::RotateEulerYXZ defines it as (R=R(Z)*R(X)*R(Y))
	// so R(r)*R(p)*R(y)

	const float sp = math::sin(angles[CMatrix44f::ANGLE_P] * 0.5f);
	const float cp = math::cos(angles[CMatrix44f::ANGLE_P] * 0.5f);
	const float sy = math::sin(angles[CMatrix44f::ANGLE_Y] * 0.5f);
	const float cy = math::cos(angles[CMatrix44f::ANGLE_Y] * 0.5f);
	const float sr = math::sin(angles[CMatrix44f::ANGLE_R] * 0.5f);
	const float cr = math::cos(angles[CMatrix44f::ANGLE_R] * 0.5f);

	CQuaternion yprQ{
		cr * cy * sp + cp * sr * sy,
		cp * cr * sy - cy * sp * sr,
		cp * cy * sr - cr * sp * sy,
		cp * cr * cy + sp * sr * sy
	};

	return AssertNormalized(yprQ);
}

/// <summary>
/// Return YPR Euler angles, so that
/// CMatrix44f::RotateEulerYXZ(-ang) == CQuaternion::ToRotMatrix()
/// Note that for m = CMatrix44f::RotateEulerYXZ(-in), out = m.CQuaternion::ToEulerYPR()
/// `in` may not be equal to `out`, but they still produce the same matrix
/// </summary>
float3 CQuaternion::ToEulerYPR() const
{
	float r11 =  2.0f * (x * z + r * y);
	float r12 =  r * r - x * x - y * y + z * z;
	float r21 = -2.0f * (y * z - r * x);
	float r31 =  2.0f * (x * y + r * z);
	float r32 =  r * r - x * x + y * y - z * z;

	return {
		math::asin(std::clamp(r21, -1.0f, 1.0f)), // CMatrix44f::ANGLE_P
		math::atan2(r11, r12),                    // CMatrix44f::ANGLE_Y
		math::atan2(r31, r32)                     // CMatrix44f::ANGLE_R
	};
}

/// <summary>
/// Return PYR Euler angles, so that
/// CMatrix44f::RotateEulerXYZ(-ang) == CQuaternion::ToRotMatrix()
/// Note that for m = CMatrix44f::RotateEulerXYZ(-in), out = m.CQuaternion::ToEulerPYR()
/// `in` may not be equal to `out`, but they still produce the same matrix
/// </summary>
float3 CQuaternion::ToEulerPYR() const
{
	float r11 = -2.0f * (y * z - r * x);
	float r12 =  r * r - x * x - y * y + z * z;
	float r21 =  2.0f * (x * z + r * y);
	float r31 = -2.0f * (x * y - r * z);
	float r32 =  r * r + x * x - y * y - z * z;

	return {
		math::atan2(r11, r12),                    // CMatrix44f::ANGLE_P
		math::asin(std::clamp(r21, -1.0f, 1.0f)), // CMatrix44f::ANGLE_Y
		math::atan2(r31, r32)                     // CMatrix44f::ANGLE_R
	};
}

/// <summary>
/// Quaternion from rotation angle and axis
/// </summary>
CQuaternion CQuaternion::MakeFrom(float angle, const float3& axis)
{
	assert(axis.Normalized());

	const float a = 0.5f * angle;
	return CQuaternion(axis * math::sin(a), math::cos(a)); //Normalized if axis.Normalized()
}

/// <summary>
/// Quaternion to rotate from v1 to v2
/// Expects v1 and v2 to be already normalized
/// </summary>
CQuaternion CQuaternion::MakeFrom(const float3& v1, const float3& v2)
{
	assert(v1.Normalized());
	assert(v2.Normalized());

	// https://raw.org/proof/quaternion-from-two-vectors/

	const auto dp = v1.dot(v2);
	// v1 and v2 are antiparallel
	if unlikely(epscmp(dp, -1.0f, float3::cmp_eps())) {
		// any perpendicular vector to v1/v2 will suffice
		float3 npVec = v1.PickNonParallel();
		const auto cp = v1.cross(npVec).Normalize();
		return CQuaternion(cp, 0.0f);
	}
	else {
		const auto cp = v1.cross(v2);
		return CQuaternion(cp, 1.0f + dp).ANormalize();
	}
}

/// <summary>
/// Quaternion to rotate from the default FwdDir(0,0,1) to newFwdDir
/// Expects newFwdDir to be already normalized
/// </summary>
CQuaternion CQuaternion::MakeFrom(const float3& newFwdDir)
{
	assert(newFwdDir.Normalized());

	// same as CQuaternion::MakeFrom(const float3& v1, const float3& v2) for v1 = (0,0,1)
	return CQuaternion{ -newFwdDir.y, newFwdDir.x, 0.0f, 1.0f + newFwdDir.z } * (math::HALFSQRT2 * math::isqrt(1.0f + newFwdDir.z));
}

/// <summary>
///  Quaternion from a rotation matrix
///  Should only be called on R or T * R matrices
/// </summary>
CQuaternion CQuaternion::MakeFrom(const CMatrix44f& mat)
{
    assert(mat.IsRotOrRotTranMatrix());
    return MakeFromAxes(mat.col[0], mat.col[1], mat.col[2]);
}

/// <summary>
/// Quaternion from three orthonormal axes expressed in world space.
/// Axes form the columns of the equivalent rotation matrix:
///   xAxis = local +X (right),  yAxis = local +Y (up),  zAxis = local +Z (forward)
/// Implements Shepperd's method — branches on the largest diagonal
/// to avoid numerical instability near zero.
/// Input axes must be orthonormal; output is a unit quaternion.
/// </summary>
CQuaternion CQuaternion::MakeFromAxes(
    const float3& xAxis,
    const float3& yAxis,
    const float3& zAxis)
{
    const float trace = xAxis.x + yAxis.y + zAxis.z;

    if (trace > 0.0f) {
        const float s    = 0.5f * InvSqrt(trace + 1.0f);

        return AssertNormalized(CQuaternion(
            s * (yAxis.z - zAxis.y),
            s * (zAxis.x - xAxis.z),
            s * (xAxis.y - yAxis.x),
            0.25f / s
        ));
    }
    else if (xAxis.x > yAxis.y && xAxis.x > zAxis.z) {
        const float s    = 2.0f * math::sqrt(1.0f + xAxis.x - yAxis.y - zAxis.z);
        const float invs = 1.0f / s;

        return AssertNormalized(CQuaternion(
            0.25f * s,
            (yAxis.x + xAxis.y) * invs,
            (zAxis.x + xAxis.z) * invs,
            (yAxis.z - zAxis.y) * invs
        ));
    }
    else if (yAxis.y > zAxis.z) {
        const float s    = 2.0f * math::sqrt(1.0f + yAxis.y - xAxis.x - zAxis.z);
        const float invs = 1.0f / s;

        return AssertNormalized(CQuaternion(
            (yAxis.x + xAxis.y) * invs,
            0.25f * s,
            (zAxis.y + yAxis.z) * invs,
            (zAxis.x - xAxis.z) * invs
        ));
    }
    else {
        const float s    = 2.0f * math::sqrt(1.0f + zAxis.z - xAxis.x - yAxis.y);
        const float invs = 1.0f / s;

        return AssertNormalized(CQuaternion(
            (zAxis.x + xAxis.z) * invs,
            (zAxis.y + yAxis.z) * invs,
            0.25f * s,
            (xAxis.y - yAxis.x) * invs
        ));
    }
}

/// <summary>
/// Orientation quaternion for a bone segment with fixed roll.
///   v10 = normalize(v - v_parent)             local +Z, bone axis
///   v21 = normalize(v_grandparent - v_parent) roll reference, local +Y plane
/// Degenerate (v10 ∥ v21): fallback matches MakeFrom(v1, v2) antiparallel case.
/// </summary>
CQuaternion CQuaternion::MakeFromChain(
    const float3& v10,   // local +Z
    const float3& v21)   // roll reference
{
    assert(v10.Normalized());
    assert(v21.Normalized());

    // Gram-Schmidt: project v21 onto plane ⊥ to v10 → local +Y
    const float proj = v10.dot(v21);
    float3 yAxis;
	if unlikely(epscmp(std::abs(proj), 1.0f, float3::cmp_eps())) {
		const float3 npVec = v10.PickNonParallel();
        yAxis = v10.cross(npVec).Normalize();
    } else {
        yAxis = (v21 - v10 * proj).Normalize();
    }

    // Y × Z = X
    const float3 xAxis = yAxis.cross(v10);

    return MakeFromAxes(xAxis, yAxis, v10);
}

std::pair<CQuaternion, CQuaternion> CQuaternion::SwingTwist(const CQuaternion& Q, const float3& axis)
{
	assert(axis.Normalized());

    const float3 v = float3(Q.x, Q.y, Q.z);
    const float  dp = v.dot(axis);

	if unlikely(CQuaternion(axis * dp, Q.r).SqNorm() < float3::apx_eps() * float3::apx_eps()) {
		// singularity: 180 degree rotation perpendicular to axis → full twist
		const CQuaternion twist = CQuaternion(axis, 0.0f); // 180 degree around axis
		const CQuaternion swing = Q * twist.InverseNormalized();
		return std::make_pair(swing, twist);
	}

    // Twist: rotation around `axis` — project vector part onto axis, keep Q.w
    const CQuaternion twist = CQuaternion(axis * dp, Q.r).ANormalize();

    // Swing: the remainder — rotation perpendicular to `axis`
    const CQuaternion swing = Q * twist.InverseNormalized();

    return std::make_pair(swing, twist);
}

const CQuaternion& CQuaternion::AssertNormalized(const CQuaternion& q)
{
	assert(q.Normalized());
	return q;
}

bool CQuaternion::Normalized() const
{
	return math::fabs(1.0f - (r * r + x * x + y * y + z * z)) <= float3::apx_eps();
}

CQuaternion& CQuaternion::Normalize()
{
	const float sqn = SqNorm();
	if unlikely(sqn < float3::nrm_eps())
		return *this;

	*this /= math::sqrt(sqn);

	return *this;
}

CQuaternion& CQuaternion::ANormalize()
{
	const float sqn = SqNorm();
	if unlikely(sqn < float3::nrm_eps())
		return *this;

	*this *= math::isqrt(sqn);

	return *this;
}

/// <summary>
/// Find axis and angle equivalent rotation from a quaternion
/// </summary>
float4 CQuaternion::ToAxisAndAngle() const
{
	assert(Normalized());
	return float4(
		float3(x, y, z) * InvSqrt(std::max(0.0f, 1.0f - r * r)),
		2.0f * math::acos(std::clamp(r, -1.0f, 1.0f))
	);
}

/// <summary>
/// Converts a quaternion to rotational matrix
/// </summary>
CMatrix44f CQuaternion::ToRotMatrix() const
{
	const float qxx = x * x;
	const float qyy = y * y;
	const float qzz = z * z;
	const float qxz = x * z;
	const float qxy = x * y;
	const float qyz = y * z;
	const float qrx = r * x;
	const float qry = r * y;
	const float qrz = r * z;

	return CMatrix44f(
		1.0f - 2.0f * (qyy + qzz), 2.0f * (qxy + qrz)       , 2.0f * (qxz - qry)       , 0.0f,
		2.0f * (qxy - qrz)       , 1.0f - 2.0f * (qxx + qzz), 2.0f * (qyz + qrx)       , 0.0f,
		2.0f * (qxz + qry)       , 2.0f * (qyz - qrx)       , 1.0f - 2.0f * (qxx + qyy), 0.0f,
		0.0f                     , 0.0f                     , 0.0f                     , 1.0f
	);
}

float3 CQuaternion::Rotate(const float3& v) const
{
	assert(Normalized());
#if 0
	const auto vRotQ = (*this) * CQuaternion(v, 0.0f) * this->Inverse();
	return float3{ vRotQ.x, vRotQ.y, vRotQ.z };
#else
	const float3 u = float3{ x, y, z };
	return 2.0f * u.dot(v) * u
		+ (r * r - u.dot(u)) * v
		+ 2.0f * r * u.cross(v);
#endif
}

float4 CQuaternion::Rotate(const float4& v) const
{
	return float4{ Rotate(float3(v.xyz)), v.w };
}

bool CQuaternion::equals(const CQuaternion& rhs) const
{
	return
		(
			epscmp(x,  rhs.x, float3::cmp_eps()) &&
			epscmp(y,  rhs.y, float3::cmp_eps()) &&
			epscmp(z,  rhs.z, float3::cmp_eps()) &&
			epscmp(r,  rhs.r, float3::cmp_eps())
		) || (
			epscmp(x, -rhs.x, float3::cmp_eps()) &&
			epscmp(y, -rhs.y, float3::cmp_eps()) &&
			epscmp(z, -rhs.z, float3::cmp_eps()) &&
			epscmp(r, -rhs.r, float3::cmp_eps())
		);
}

CQuaternion CQuaternion::Inverse() const
{
	CQuaternion inv = *this;
	inv.InverseInPlace();
	return inv;
}

CQuaternion& CQuaternion::InverseInPlace()
{
	const float sqn = SqNorm();
	if unlikely(sqn < float3::nrm_eps())
		return *this;

	*this = Conjugate() / sqn; // aparently not math::sqrt(sqn)
	return *this;
}

CQuaternion CQuaternion::InverseNormalized() const
{
	CQuaternion inv = *this;
	inv.InverseInPlaceNormalized();
	return inv;
}

CQuaternion& CQuaternion::InverseInPlaceNormalized()
{
	assert(Normalized());
	return Conjugate();
}

CQuaternion CQuaternion::operator*(const CQuaternion& rhs) const
{
	// *this or rhs can be a vertex from CQuaternion::Rotate(), can't assume either of them is normalized

	std::array<float, 3> crossProduct = {
		(y * rhs.z) - (z * rhs.y),
		(z * rhs.x) - (x * rhs.z),
		(x * rhs.y) - (y * rhs.x)
	};

	return CQuaternion(
		r * rhs.x + rhs.r * x + crossProduct[0],
		r * rhs.y + rhs.r * y + crossProduct[1],
		r * rhs.z + rhs.r * z + crossProduct[2],
		r * rhs.r - (x * rhs.x + y * rhs.y + z * rhs.z)
	);
}

CQuaternion& CQuaternion::operator*=(float f)
{
	x *= f;
	y *= f;
	z *= f;
	r *= f;

	return *this;
}

CQuaternion& CQuaternion::operator/=(float f)
{
	if unlikely(epscmp(f, 0.0f, float3::cmp_eps()))
		return *this;

	f = 1.0f / f;

	x *= f;
	y *= f;
	z *= f;
	r *= f;

	return *this;
}

void CQuaternion::AssertNaNs() const
{
	assert(!math::isnan(x) && !math::isinf(x));
	assert(!math::isnan(y) && !math::isinf(y));
	assert(!math::isnan(z) && !math::isinf(z));
	assert(!math::isnan(r) && !math::isinf(r));
}

float CQuaternion::InvSqrt(float f)
{
#if 0
	return math::isqrt(f);
#else
	return 1.0f / math::sqrt(f);
#endif
}

CQuaternion CQuaternion::Lerp(const CQuaternion& q1, const CQuaternion& q2, const float a) {
	assert(q1.Normalized());
	assert(q2.Normalized());
	return (q1 * (1.0f - a) + (q2 * a)).ANormalize();
}

CQuaternion CQuaternion::SLerp(const CQuaternion& qa, const CQuaternion& qb_, const float t) {
	assert( qa.Normalized());
	assert(qb_.Normalized());

	// Calculate angle between them.
	float cosHalfTheta = qa.x * qb_.x + qa.y * qb_.y + qa.z * qb_.z + qa.r * qb_.r;

	// Unfortunately every rotation can be represented by two quaternions: (++++) or (----)
	// avoid taking the longer way: choose one representation
	const float s = Sign(cosHalfTheta);
	CQuaternion qb = qb_ * s;
	cosHalfTheta *= s;

	// if qa = qb or qa = -qb then theta = 0 and we can return qa
	if (math::fabs(cosHalfTheta) >= 1.0f) // greater-sign necessary for numerical stability
		return qa;

	// Calculate temporary values.
	float halfTheta = math::acos(cosHalfTheta);
	float sinHalfTheta = math::sqrt(1.0f - cosHalfTheta * cosHalfTheta); // NOTE: we checked above that |cosHalfTheta| < 1

	// if theta = pi then result is not fully defined
	// we could rotate around any axis normal to qa or qb
	if unlikely(sinHalfTheta < 1e-3f)
		return Lerp(qa, qb, 0.5f);

	// both should be divided by sinHalfTheta, but makes no sense to do it due to follow up normalization
	const float ratioA = math::sin((1.0f - t) * halfTheta);
	const float ratioB = math::sin((       t) * halfTheta);

	return (qa * ratioA + qb * ratioB).ANormalize();
}
