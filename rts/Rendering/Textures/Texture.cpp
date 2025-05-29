#include "Texture.hpp"

#include <bit>
#include <tuple>
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/SubState.h"
#include "Rendering/GlobalRendering.h"

namespace GL {
	namespace Impl {
		std::pair<uint32_t, GL::TexBind> InitTexture(const GL::TextureCreationParams& tcp, uint32_t texTarget, int32_t numLevels) {
			uint32_t texID = tcp.texID;

			if (texID == 0)
				glGenTextures(1, &texID);

			auto binding = GL::TexBind(texTarget, texID);

			const auto minFilter = tcp.GetMinFilter(numLevels);
			const auto magFilter = tcp.GetMagFilter();

			glTexParameteri(texTarget, GL_TEXTURE_MAG_FILTER, magFilter);
			glTexParameteri(texTarget, GL_TEXTURE_MIN_FILTER, minFilter);

			if (tcp.wrapModes.has_value()) {
				static constexpr std::array<uint32_t, 3> texWrapModes{
					GL_TEXTURE_WRAP_S,
					GL_TEXTURE_WRAP_T,
					GL_TEXTURE_WRAP_R
				};

				const auto& wrapModes = tcp.wrapModes.value();
				for (size_t i = 0; auto wrapMode : wrapModes) {
					glTexParameteri(texTarget, texWrapModes[i++], wrapMode);
				}
			}
			else {
				const auto texWrapMode = tcp.GetWrapMode();

				glTexParameteri(texTarget, GL_TEXTURE_WRAP_S, texWrapMode);
				glTexParameteri(texTarget, GL_TEXTURE_WRAP_T, texWrapMode);
				glTexParameteri(texTarget, GL_TEXTURE_WRAP_R, texWrapMode);
			}

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

	// not great as the texture is created in a derived class, but passable
	TextureBase::~TextureBase()
	{
		if (!ownTexID || !texID)
			return;

		glDeleteTextures(1, &texID);
	}

	GL::TexBind TextureBase::ScopedBind()
	{
		auto scopedBinding = GL::TexBind(texTarget, texID);
		lastBoundSlot = scopedBinding.GetLastActiveTextureSlot();
		return scopedBinding; // NRTO should optimize it
	}
	GL::TexBind TextureBase::ScopedBind(uint32_t relSlot)
	{
		lastBoundSlot = GL_TEXTURE0 + relSlot;
		return GL::TexBind(relSlot, texTarget, texID);
	}

	void TextureBase::ScopedBind(const GL::TexBind& existingScopedBinding)
	{
		glActiveTexture(existingScopedBinding.GetLastActiveTextureSlot());
		glBindTexture(texTarget, texID);
	}

	void TextureBase::Bind()
	{
		lastBoundSlot = GL::FetchActiveTextureSlot();
		glBindTexture(texTarget, texID);
	}

	void TextureBase::Bind(uint32_t relSlot)
	{
		lastBoundSlot = GL_TEXTURE0 + relSlot;
		glActiveTexture(GL_TEXTURE0 + relSlot);
		glBindTexture(texTarget, texID);
	}

	void TextureBase::Unbind()
	{
		glBindTexture(texTarget, 0);
		lastBoundSlot = 0;
	}

	void TextureBase::Unbind(uint32_t relSlot)
	{
		glActiveTexture(GL_TEXTURE0 + relSlot);
		glBindTexture(texTarget, 0);
		lastBoundSlot = 0;
	}

	TextureBase& TextureBase::operator=(TextureBase&& other) noexcept {
		std::swap(texID, other.texID);
		std::swap(intFormat, other.intFormat);
		std::swap(numLevels, other.numLevels);
		std::swap(lastBoundSlot, other.lastBoundSlot);
		std::swap(ownTexID, other.ownTexID);

		return *this;
	}

	Texture2D::Texture2D(int xsize_, int ysize_, uint32_t intFormat_, const TextureCreationParams& tcp, bool wantCompress)
		: Texture2D()
	{
		size = int2(xsize_, ysize_);
		intFormat = intFormat_;

		numLevels = tcp.reqNumLevels <= 0
			? std::bit_width(static_cast<uint32_t>(std::max({ size.x , size.y })))
			: tcp.reqNumLevels;

		lastBoundSlot = GL::FetchActiveTextureSlot();
		auto&& [genTexID, binding] = Impl::InitTexture(tcp, texTarget, numLevels);
		texID = genTexID;

		if (GLAD_GL_ARB_texture_storage && !wantCompress) {
			glTexStorage2D(texTarget, numLevels, intFormat, size.x, size.y);
		}
		else {
			const auto compressedIntFormat = GetCompressedInternalFormat(intFormat);
			const auto extFormat = GetExternalFormatFromInternalFormat(intFormat);
			const auto dataType = GetDataTypeFromInternalFormat(intFormat);

			for (int level = 0; level < numLevels; ++level)
				glTexImage2D(texTarget, level, compressedIntFormat, std::max(size.x >> level, 1), std::max(size.y >> level, 1), 0, extFormat, dataType, nullptr);
		}
		glTexParameteri(texTarget, GL_TEXTURE_BASE_LEVEL,             0);
		glTexParameteri(texTarget, GL_TEXTURE_MAX_LEVEL , numLevels - 1);
	}

	Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
	{
		TextureBase::operator=(static_cast<TextureBase&&>(other));
		std::swap(size, other.size);

		return *this;
	}

	void Texture2D::UploadSubImage(const void* data, int xOffset, int yOffset, int width, int height, int level) const
	{
		assert(lastBoundSlot >= GL_TEXTURE0);
		const auto extFormat = GetExternalFormatFromInternalFormat(intFormat);
		const auto dataType = GetDataTypeFromInternalFormat(intFormat);
		const auto dataSize = GetDataTypeSize(dataType);
		using namespace GL::State;
		auto state = GL::SubState(
			PixelStoreUnpackAlignment(dataSize)
		);
		glTexSubImage2D(texTarget, level, xOffset, yOffset, width, height, extFormat, dataType, data);
	}

	void Texture2D::ProduceMipmaps() const
	{
		assert(lastBoundSlot >= GL_TEXTURE0);
		if (globalRendering->amdHacks) {
			glEnable(texTarget);
			glGenerateMipmap(texTarget);
			glDisable(texTarget);
		}
		else {
			glGenerateMipmap(texTarget);
		}
	}


	Texture2DArray::Texture2DArray(int xsize_, int ysize_, int numPages_, uint32_t intFormat_, const TextureCreationParams& tcp, bool wantCompress)
		: Texture2DArray()
	{
		size = int2(xsize_, ysize_);
		numPages = numPages_;
		intFormat = intFormat_;

		numLevels = tcp.reqNumLevels <= 0
			? std::bit_width(static_cast<uint32_t>(std::max({ size.x , size.y })))
			: tcp.reqNumLevels;

		lastBoundSlot = GL::FetchActiveTextureSlot();
		auto&& [genTexID, binding] = Impl::InitTexture(tcp, texTarget, numLevels);
		texID = genTexID;

		if (GLAD_GL_ARB_texture_storage && !wantCompress) {
			glTexStorage3D(texTarget, numLevels, intFormat, size.x, size.y, numPages);
		}
		else {
			const auto compressedIntFormat = GetCompressedInternalFormat(intFormat);
			const auto extFormat = GetExternalFormatFromInternalFormat(intFormat);
			const auto dataType = GetDataTypeFromInternalFormat(intFormat);

			for (int level = 0; level < numLevels; ++level)
				glTexImage3D(texTarget, level, compressedIntFormat, std::max(size.x >> level, 1), std::max(size.y >> level, 1), numPages, 0, extFormat, dataType, nullptr);
		}
		glTexParameteri(texTarget, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(texTarget, GL_TEXTURE_MAX_LEVEL, numLevels - 1);
	}

	Texture2DArray& Texture2DArray::operator=(Texture2DArray&& other) noexcept
	{
		TextureBase::operator=(static_cast<TextureBase&&>(other));
		std::swap(size, other.size);
		std::swap(numLevels, other.numLevels);

		return *this;
	}

	void Texture2DArray::UploadSubImage(const void* data, int layer, int xOffset, int yOffset, int width, int height, int level) const
	{
		assert(lastBoundSlot >= GL_TEXTURE0);
		const auto extFormat = GetExternalFormatFromInternalFormat(intFormat);
		const auto dataType = GetDataTypeFromInternalFormat(intFormat);
		const auto dataSize = GetDataTypeSize(dataType);
		using namespace GL::State;
		auto state = GL::SubState(
			PixelStoreUnpackAlignment(dataSize)
		);
		glTexSubImage3D(texTarget, level, xOffset, yOffset, layer, width, height, 1, extFormat, dataType, data);
	}

	void Texture2DArray::ProduceMipmaps() const
	{
		assert(lastBoundSlot >= GL_TEXTURE0);
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