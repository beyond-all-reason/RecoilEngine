/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>

namespace GL
{
	uint32_t GetDataFormatFromInternalFormat(uint32_t internalFormat);
	uint32_t GetDataTypeFromInternalFormat(uint32_t internalFormat);
	uint32_t GetNumChannelsFromInternalFormat(uint32_t internalFormat);
	uint32_t GetExternalFormatFromInternalFormat(uint32_t internalFormat);
	uint32_t GetExternalFormatFromNumChannels(uint32_t numChannels);
	uint32_t GetDataTypeSize(uint32_t glDataType);
	uint32_t GetBindingQueryFromTarget(uint32_t target);
	uint32_t GetCompressedInternalFormat(uint32_t internalFormat);
}