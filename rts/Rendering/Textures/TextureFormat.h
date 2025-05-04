/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include "Rendering/GL/myGL.h"

namespace GL
{
	uint32_t GetInternalFormatDataFormat(uint32_t internalFormat);
	uint32_t GetInternalFormatDataType(uint32_t internalFormat);
	uint32_t GetDataTypeSize(uint32_t glDataType);
	uint32_t GetBindingQueryFromTarget(GLenum target);
}