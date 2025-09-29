#include "VertexData.hpp"

static_assert(sizeof(SVertexData) == (3 + 3 + 3 + 3 + 4 + 2 + 1) * 4);

void SVertexData::TransformBy(const Transform& transform)
{
	pos      = (transform * float4{ pos     , 1.0f }).xyz;
	normal   = (transform * float4{ normal  , 0.0f }).xyz;
	sTangent = (transform * float4{ sTangent, 0.0f }).xyz;
	tTangent = (transform * float4{ tTangent, 0.0f }).xyz;
}