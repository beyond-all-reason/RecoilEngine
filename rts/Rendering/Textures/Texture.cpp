#include "Texture.hpp"

#include <bit>
#include <tuple>
#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"

namespace GL {
	namespace Impl {
		std::pair<uint32_t, GL::TexBind> CreateTexture(const GL::TextureCreationParams& tcp, uint32_t texTarget, int32_t numLevels) {
			uint32_t texID = tcp.texID;

			if (texID == 0)
				glGenTextures(1, &texID);

			auto binding = GL::TexBind(texTarget, texID);

			const auto minFilter = tcp.GetMinFilter(numLevels);
			const auto magFilter = tcp.GetMagFilter();

			glTexParameteri(texTarget, GL_TEXTURE_MAG_FILTER, magFilter);
			glTexParameteri(texTarget, GL_TEXTURE_MIN_FILTER, minFilter);

			const auto texWrapMode = tcp.GetWrapMode();

			glTexParameteri(texTarget, GL_TEXTURE_WRAP_S, texWrapMode);
			glTexParameteri(texTarget, GL_TEXTURE_WRAP_T, texWrapMode);
			glTexParameteri(texTarget, GL_TEXTURE_WRAP_R, texWrapMode);

			if (tcp.clampBorder.has_value()) {
				glTexParameterfv(texTarget, GL_TEXTURE_BORDER_COLOR, &tcp.clampBorder.value().x);
			}

			if (tcp.lodBias != 0.0f)
				glTexParameterf(texTarget, GL_TEXTURE_LOD_BIAS, tcp.lodBias);

			if (tcp.aniso > 0.0f)
				glTexParameterf(texTarget, GL_TEXTURE_MAX_ANISOTROPY, tcp.aniso);

			return std::make_pair(texID, std::move(binding));
		}
	}

	Texture::~Texture() {
		if (!ownTexID)
			return;

		glDeleteTextures(1, &texID);
	}

	Texture2D::Texture2D()
		: xsize(0)
		, ysize(0)
	{
		texTarget = GL_TEXTURE_2D;
	}

	Texture2D::Texture2D(int xsize_, int ysize_, uint32_t intFormat_, const TextureCreationParams& tcp, bool wantCompress)
		: Texture2D()
	{
		xsize = xsize_;
		ysize = ysize_;
		intFormat = intFormat_;

		numLevels = tcp.reqNumLevels <= 0
			? std::bit_width(static_cast<uint32_t>(std::max({ xsize , ysize })))
			: tcp.reqNumLevels;

		auto&& [texID, binding] = Impl::CreateTexture(tcp, texTarget, numLevels);

		if (GLAD_GL_ARB_texture_storage && !wantCompress) {
			glTexStorage2D(texTarget, numLevels, intFormat, xsize, ysize);
		}
		else {
			const auto compressedIntFormat = GetCompressedInterFormat(intFormat);
			const auto extFormat = GetExternalFormatFromInternalFormat(intFormat);
			const auto dataType = GetDataTypeFromInternalFormat(intFormat);

			for (int level = 0; level < numLevels; ++level)
				glTexImage2D(texTarget, level, compressedIntFormat, std::max(xsize >> level, 1), std::max(ysize >> level, 1), 0, extFormat, dataType, nullptr);
		}
		glTexParameteri(texTarget, GL_TEXTURE_BASE_LEVEL,             0);
		glTexParameteri(texTarget, GL_TEXTURE_MAX_LEVEL , numLevels - 1);
	}

	void Texture2D::UploadSubImage(const void* data, int xOffset, int yOffset, int width, int height, int level) const
	{
		const auto extFormat = GetExternalFormatFromInternalFormat(intFormat);
		const auto dataType = GetDataTypeFromInternalFormat(intFormat);

		auto binding = GL::TexBind(texTarget, texID);
		glTexSubImage2D(texTarget, level, xOffset, yOffset, width, height, extFormat, dataType, data);
	}

	void Texture2D::ProduceMipmaps() const
	{
		if (globalRendering->amdHacks) {
			glEnable(texTarget);
			glGenerateMipmap(texTarget);
			glDisable(texTarget);
		}
		else {
			glGenerateMipmap(texTarget);
		}
	}
}