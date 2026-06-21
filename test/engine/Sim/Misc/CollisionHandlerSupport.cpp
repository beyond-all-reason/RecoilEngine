#include "Map/MapDimensions.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "System/float3.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Units/Unit.h"
#include "Game/Camera.h"

MapDimensions mapDims;
CGroundBlockingObjectMap groundBlockingObjectMap;

const CMatrix44f& LocalModelPiece::GetModelSpaceMatrix() const
{
	return modelSpaceMat;
}

CMatrix44f CUnit::GetTransformMatrix(bool synced, bool fullread) const
{
	float3 interPos = synced ? pos : drawPos;

	return (ComposeMatrix(interPos));
}
bool CCamera::Frustum::IntersectSphere(float3 p, float radius, uint8_t testMask) const
{
	for (size_t i = 0; i < FRUSTUM_PLANE_CNT; ++i) {
		if ((testMask & (1 << i)) == 0)
			continue;

		const auto& plane = planes[i];
		const float dist = plane.dot(p) + plane.w;
		if (dist < -radius)
			return false; // outside
		/*
		else if (dist < radius)
			return true;  // intersect
		*/
	}

	return true; // inside or intersect
}


