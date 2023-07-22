#include "TextureFormat.h"

const spring::unordered_map<GLenum, GLenum> GL::Impl::TargetToBindingQuery {
	{ GL_TEXTURE_1D                  , GL_TEXTURE_BINDING_1D                   },
	{ GL_TEXTURE_2D                  , GL_TEXTURE_BINDING_2D                   },
	{ GL_TEXTURE_3D                  , GL_TEXTURE_BINDING_3D                   },
	{ GL_TEXTURE_1D_ARRAY            , GL_TEXTURE_BINDING_1D_ARRAY             },
	{ GL_TEXTURE_2D_ARRAY            , GL_TEXTURE_BINDING_2D_ARRAY             },
	{ GL_TEXTURE_RECTANGLE           , GL_TEXTURE_BINDING_RECTANGLE            },
	{ GL_TEXTURE_CUBE_MAP            , GL_TEXTURE_BINDING_CUBE_MAP             },
	{ GL_TEXTURE_BUFFER              , GL_TEXTURE_BINDING_BUFFER               },
	{ GL_TEXTURE_2D_MULTISAMPLE      , GL_TEXTURE_BINDING_2D_MULTISAMPLE       },
	{ GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY },
};

GLenum GL::GetInternalFormatDataFormat(GLenum internalFormat) {
	GLenum dataFormat;
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

GLenum GL::GetInternalFormatDataType(GLenum internalFormat) {
	GLenum dataType;
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
			dataType = GL_UNSIGNED_INT_24_8;
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
