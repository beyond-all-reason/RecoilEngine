#include "VertexData.hpp"

static_assert(sizeof(SVertexData) == (3 + 3 + 4 + 4 + 2 + 1) * 4);

void SVertexData::TransformBy(const Transform& transform)
{
	pos      = (transform * float4{ pos     , 1.0f }).xyz;
	normal   = (transform * float4{ normal  , 0.0f }).xyz;
	const float handednessSign = tangent.w;
	tangent  = (transform * float4{ tangent , 0.0f }).xyz;
	tangent.w = handednessSign;
}

SVertexData SVertexData::TransformBy(const Transform& transform) const
{
	SVertexData copy = *this;
	copy.TransformBy(transform); // calls the non-const overload
	return copy;
}