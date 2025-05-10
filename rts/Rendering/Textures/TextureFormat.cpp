#include "TextureFormat.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"

uint32_t GL::GetDataFormatFromInternalFormat(uint32_t internalFormat) {
	uint32_t dataFormat;
	switch (internalFormat) {
		case GL_R8UI:
		case GL_R16UI:
		case GL_R32UI: {
			dataFormat = GL_RED_INTEGER;
		} break;
		case GL_RG8UI:
		case GL_RG16UI:
		case GL_RG32UI: {
			dataFormat = GL_RG_INTEGER;
		} break;
		case GL_RGB10_A2UI: {
			dataFormat = GL_RGB_INTEGER;
		} break;
		case GL_RGBA8UI:
		case GL_RGBA16UI:
		case GL_RGBA32UI: {
			dataFormat = GL_RGBA_INTEGER;
		} break;
		case GL_R8:
		case GL_R16:
		case GL_R16F:
		case GL_R32F: {
			dataFormat = GL_RED;
		} break;
		case GL_DEPTH_COMPONENT:
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32:
		case GL_DEPTH_COMPONENT32F: {
			dataFormat = GL_DEPTH_COMPONENT;
		} break;
		case GL_RG8:
		case GL_RG16:
		case GL_RG16F:
		case GL_RG32F: {
			dataFormat = GL_RG;
		} break;
		case GL_RGB10_A2:
		case GL_R11F_G11F_B10F: {
			dataFormat = GL_RGB;
		} break;
		case GL_RGBA8:
		case GL_RGBA16:
		case GL_RGBA16F:
		case GL_RGBA32F:
		default: {
			dataFormat = GL_RGBA;
		} break;
	}
	return dataFormat;
}

uint32_t GL::GetDataTypeFromInternalFormat(uint32_t internalFormat) {
	uint32_t dataType;
	switch (internalFormat) {
		case GL_RGBA16F:
		case GL_RG16F:
		case GL_R16F: {
			dataType = GL_HALF_FLOAT;
		} break;
		case GL_RGBA32F:
		case GL_RG32F:
		case GL_R32F:
		case GL_DEPTH_COMPONENT32F: {
			dataType = GL_FLOAT;
		} break;
		case GL_RGBA32UI:
		case GL_RG32UI:
		case GL_R32UI:
		case GL_DEPTH_COMPONENT32: {
			dataType = GL_UNSIGNED_INT;
		} break;
		case GL_DEPTH_COMPONENT24: {
			dataType = GL_UNSIGNED_INT;
		} break;
		case GL_RGBA16:
		case GL_RGBA16UI:
		case GL_RG16:
		case GL_RG16UI:
		case GL_R16:
		case GL_R16UI:
		case GL_DEPTH_COMPONENT16: {
			dataType = GL_UNSIGNED_SHORT;
		} break;
		case GL_R11F_G11F_B10F: {
			dataType = GL_UNSIGNED_INT_10F_11F_11F_REV;
		} break;
		case GL_RGB10_A2:
		case GL_RGB10_A2UI: {
			dataType = GL_UNSIGNED_INT_2_10_10_10_REV;
		} break;
		case GL_RGBA8:
		case GL_RGBA8UI:
		case GL_RG8:
		case GL_RG8UI:
		case GL_R8:
		case GL_R8UI:
		case GL_DEPTH_COMPONENT:
		default: {
			dataType = GL_UNSIGNED_BYTE;
		} break;
	}
	return dataType;
}

uint32_t GL::GetNumChannelsFromInternalFormat(uint32_t internalFormat)
{
	switch (internalFormat) {
	// 1 Channel Formats (Red/Depth/Stencil)
	case GL_RED:
	case GL_DEPTH_COMPONENT:
	case GL_STENCIL_INDEX:
	// Sized formats
	case GL_R8:
	case GL_R8_SNORM:
	case GL_R16:
	case GL_R16_SNORM:
	case GL_R16F:
	case GL_R32F:
	case GL_R8I:
	case GL_R8UI:
	case GL_R16I:
	case GL_R16UI:
	case GL_R32I:
	case GL_R32UI:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32F:
	case GL_STENCIL_INDEX8:
		return 1;

	// 2 Channel Formats (RG/Depth-Stencil) 
	case GL_RG:
	case GL_DEPTH_STENCIL:
	// Sized formats
	case GL_RG8:
	case GL_RG8_SNORM:
	case GL_RG16:
	case GL_RG16_SNORM:
	case GL_RG16F:
	case GL_RG32F:
	case GL_RG8I:
	case GL_RG8UI:
	case GL_RG16I:
	case GL_RG16UI:
	case GL_RG32I:
	case GL_RG32UI:
	case GL_DEPTH24_STENCIL8:
	case GL_DEPTH32F_STENCIL8:
		return 2;

	// 3 Channel Formats (RGB/BGR)
	case GL_RGB:
	case GL_BGR:
	// Sized formats
	case GL_R3_G3_B2:
	case GL_RGB4:
	case GL_RGB5:
	case GL_RGB8:
	case GL_RGB8_SNORM:
	case GL_RGB10:
	case GL_RGB12:
	case GL_RGB16_SNORM:
	case GL_RGB16F:
	case GL_RGB32F:
	case GL_RGB8I:
	case GL_RGB8UI:
	case GL_RGB16I:
	case GL_RGB16UI:
	case GL_RGB32I:
	case GL_RGB32UI:
	case GL_R11F_G11F_B10F:
	case GL_RGB9_E5:
	case GL_SRGB8:
		return 3;

	// 4 Channel Formats (RGBA/BGRA)
	case GL_RGBA:
	case GL_BGRA:
	// Sized formats
	case GL_RGBA2:
	case GL_RGBA4:
	case GL_RGB5_A1:
	case GL_RGBA8:
	case GL_RGBA8_SNORM:
	case GL_RGB10_A2:
	case GL_RGB10_A2UI:
	case GL_RGBA12:
	case GL_RGBA16:
	case GL_SRGB8_ALPHA8:
	case GL_RGBA16F:
	case GL_RGBA32F:
	case GL_RGBA8I:
	case GL_RGBA8UI:
	case GL_RGBA16I:
	case GL_RGBA16UI:
	case GL_RGBA32I:
	case GL_RGBA32UI:
		return 4;

	default: {
		assert(false);
		return 0; // Unknown format
	}
	}
}

uint32_t GL::GetExternalFormatFromInternalFormat(uint32_t internalFormat)
{
	const auto numChannels = GetNumChannelsFromInternalFormat(internalFormat);
	return GetExternalFormatFromNumChannels(numChannels);
}

uint32_t GL::GetExternalFormatFromNumChannels(uint32_t numChannels)
{
	static constexpr std::array extFormats = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA }; // GL_R is not accepted for [1]
	if (numChannels > 4)
		numChannels = 0; // invalid input

	return extFormats[numChannels];
}

uint32_t GL::GetDataTypeSize(uint32_t glDataType)
{
	switch (glDataType) {
	case GL_FLOAT:
		return sizeof(float);
	case GL_INT: [[fallthrough]];
	case GL_UNSIGNED_INT:
		return sizeof(uint32_t);
	case GL_SHORT: [[fallthrough]];
	case GL_UNSIGNED_SHORT:
		return sizeof(uint16_t);
	case GL_BYTE: [[fallthrough]];
	case GL_UNSIGNED_BYTE:
		return sizeof(uint8_t);
	default:
		assert(false);
		return 0;
	}
}

uint32_t GL::GetCompressedInternalFormat(uint32_t internalFormat)
{
	if (!globalRendering->compressTextures)
		return internalFormat;

	switch (internalFormat) {
	case 4:
	case GL_RGBA8:
	case GL_RGBA:
		internalFormat = GL_COMPRESSED_RGBA_ARB;
		break;

	case 3:
	case GL_RGB8:
	case GL_RGB:
		internalFormat = GL_COMPRESSED_RGB_ARB;
		break;

	case GL_LUMINANCE:
		internalFormat = GL_COMPRESSED_LUMINANCE_ARB;
		break;
	}

	return internalFormat;
}

uint32_t GL::GetBindingQueryFromTarget(uint32_t target) {
	switch (target) {
		case GL_TEXTURE_1D:                   return GL_TEXTURE_BINDING_1D;
		case GL_TEXTURE_2D:                   return GL_TEXTURE_BINDING_2D;
		case GL_TEXTURE_3D:                   return GL_TEXTURE_BINDING_3D;
		case GL_TEXTURE_1D_ARRAY:             return GL_TEXTURE_BINDING_1D_ARRAY;
		case GL_TEXTURE_2D_ARRAY:             return GL_TEXTURE_BINDING_2D_ARRAY;
		case GL_TEXTURE_RECTANGLE:            return GL_TEXTURE_BINDING_RECTANGLE;
		case GL_TEXTURE_CUBE_MAP:             return GL_TEXTURE_BINDING_CUBE_MAP;
		case GL_TEXTURE_BUFFER:               return GL_TEXTURE_BINDING_BUFFER;
		case GL_TEXTURE_2D_MULTISAMPLE:       return GL_TEXTURE_BINDING_2D_MULTISAMPLE;
		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: return GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY;
		default: break;
	}
	return 0;
}
