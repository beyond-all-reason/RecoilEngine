#include "AABB.hpp"
#include "System/SpringMath.h"

void AABB::CalcCorners(std::array<float3, 8>& verts) const
{
	// bottom
	verts[0] = float3{ mins.x, mins.y, mins.z };
	verts[1] = float3{ mins.x, mins.y, maxs.z };
	verts[2] = float3{ maxs.x, mins.y, mins.z };
	verts[3] = float3{ maxs.x, mins.y, maxs.z };
	// top
	verts[4] = float3{ mins.x, maxs.y, mins.z };
	verts[5] = float3{ mins.x, maxs.y, maxs.z };
	verts[6] = float3{ maxs.x, maxs.y, mins.z };
	verts[7] = float3{ maxs.x, maxs.y, maxs.z };
}

float3 AABB::ClampInto(const float3& pnt) const
{
    return float3{
        std::clamp(pnt.x, mins.x, maxs.x),
        std::clamp(pnt.y, mins.y, maxs.y),
        std::clamp(pnt.z, mins.z, maxs.z)
    };
}

float3 AABB::GetVertexP(const float3& normal) const
{
    return mix(mins, maxs, float3{ static_cast<float>(normal.x >= 0), static_cast<float>(normal.y >= 0), static_cast<float>(normal.z >= 0) });
}

float3 AABB::GetVertexN(const float3& normal) const
{
    return mix(maxs, mins, float3{ static_cast<float>(normal.x >= 0), static_cast<float>(normal.y >= 0), static_cast<float>(normal.z >= 0) });
}
