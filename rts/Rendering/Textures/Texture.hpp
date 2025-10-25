#pragma once

#include <cstdint>
#include <array>
#include "System/type2.h"
#include "System/TemplateUtils.hpp"
#include "Rendering/GL/TexBind.h"
#include "Rendering/Textures/TextureCreationParams.hpp"

namespace GL {


	class TextureBase {
	public:
		TextureBase(uint32_t texTarget_)
			: texTarget(texTarget_)
		{}
		virtual ~TextureBase();

		bool IsValid() const { return texID != 0; }
		auto GetId() const { return texID; }
		auto DisOwn() { ownTexID = false; return texID; }

		[[nodiscard]] GL::TexBind ScopedBind();
		[[nodiscard]] GL::TexBind ScopedBind(uint32_t relSlot);
		// reuse the existing scoped binding
		void ScopedBind(const GL::TexBind& existingScopedBinding);
		void Bind();
		void Bind(uint32_t relSlot);
		void Unbind();
		void Unbind(uint32_t relSlot);
	public:
		TextureBase(TextureBase&& other) noexcept{ *this = std::move(other); }
		TextureBase(const TextureBase&) = delete;

		TextureBase& operator=(TextureBase&& other) noexcept;
		TextureBase& operator=(const TextureBase&) = delete;
	public:
		virtual void ProduceMipmaps() const = 0;
	protected:
		uint32_t texID = 0;
		uint32_t intFormat = 0;
		int32_t numLevels = -1;
		uint32_t lastBoundSlot = 0;
		bool ownTexID = true;
		uint32_t texTarget;
	};

	class Texture2D : public TextureBase {
	public:
		Texture2D()
			: TextureBase(0x0DE1/*GL_TEXTURE_2D*/)
		{}
		Texture2D(int xsize_, int ysize_, uint32_t intFormat_, const TextureCreationParams& tcp_, bool wantCompress = true);
		Texture2D(const int2& size, uint32_t intFormat_, const TextureCreationParams& tcp_, bool wantCompress = true)
			: Texture2D(size.x, size.y, intFormat_, tcp_, wantCompress)
		{}

		Texture2D(Texture2D&& other) noexcept
			: TextureBase(other.texTarget)
		{
			*this = std::move(other);
		}
		Texture2D(const Texture2D&) = delete;

		Texture2D& operator=(Texture2D&& other) noexcept;
		Texture2D& operator=(const Texture2D&) = delete;

		template <Concepts::HasSizeAndData C>
		void UploadImage(const C& c) const {
		#ifdef DEBUG
			const auto dataType = GetDataTypeFromInternalFormat(intFormat);
			const auto dataSize = GetDataTypeSize(dataType);
			const auto numChannels = GetNumChannelsFromInternalFormat(intFormat);
			assert(c.size() * sizeof(typename C::value_type) >= size.x * size.y * numChannels * dataSize);
		#endif // DEBUG
			UploadImage(c.data());
		}

		template <Concepts::HasSizeAndData C>
		void UploadSubImage(const C& c, int xOffset, int yOffset, int width, int height) const {
		#ifdef DEBUG
			assert(xOffset + width <= size.x && yOffset + height <= size.y);
			const auto dataType = GetDataTypeFromInternalFormat(intFormat);
			const auto dataSize = GetDataTypeSize(dataType);
			const auto numChannels = GetNumChannelsFromInternalFormat(intFormat);
			assert(c.size() * sizeof(typename C::value_type) >= width * height * numChannels * dataSize);
		#endif // DEBUG
			UploadSubImage(c.data(), xOffset, yOffset, width, height, 0);
		}

		void UploadImage(const void* data, int level = 0) const {
			UploadSubImage(data, 0, 0, size.x, size.y, level);
		}
		void UploadSubImage(const void* data, int xOffset, int yOffset, int width, int height, int level = 0) const;
		void ProduceMipmaps() const override;
	private:
		int2 size;
	};

	class Texture2DArray : public TextureBase {
	public:
		Texture2DArray()
			: TextureBase(0x8C1A/*GL_TEXTURE_2D_ARRAY*/)
			, numPages(0)
		{}
		Texture2DArray(int xsize_, int ysize_, int numPages_, uint32_t intFormat_, const TextureCreationParams& tcp_, bool wantCompress = true);
		Texture2DArray(const int2& size, int numPages_, uint32_t intFormat_, const TextureCreationParams& tcp_, bool wantCompress = true)
			: Texture2DArray(size.x, size.y, numPages_, intFormat_, tcp_, wantCompress)
		{}

		Texture2DArray(Texture2DArray&& other) noexcept
			: TextureBase(other.texTarget)
		{
			*this = std::move(other);
		}
		Texture2DArray(const Texture2DArray&) = delete;

		Texture2DArray& operator=(Texture2DArray&& other) noexcept;
		Texture2DArray& operator=(const Texture2DArray&) = delete;

		template <Concepts::HasSizeAndData C>
		void UploadImage(const C& c, int layer) const {
		#ifdef DEBUG
			const auto dataType = GetDataTypeFromInternalFormat(intFormat);
			const auto dataSize = GetDataTypeSize(dataType);
			const auto numChannels = GetNumChannelsFromInternalFormat(intFormat);
			assert(c.size() * sizeof(typename C::value_type) >= size.x * size.y * numChannels * dataSize);
		#endif // DEBUG
			UploadImage(c.data());
		}

		template <Concepts::HasSizeAndData C>
		void UploadSubImage(const C& c, int layer, int xOffset, int yOffset, int width, int height) const {
		#ifdef DEBUG
			assert(xOffset + width <= size.x && yOffset + height <= size.y);
			const auto dataType = GetDataTypeFromInternalFormat(intFormat);
			const auto dataSize = GetDataTypeSize(dataType);
			const auto numChannels = GetNumChannelsFromInternalFormat(intFormat);
			assert(c.size() * sizeof(typename C::value_type) >= width * height * numChannels * dataSize);
		#endif // DEBUG
			UploadSubImage(c.data(), layer, xOffset, yOffset, width, height, 0);
		}

		void UploadImage(const void* data, int layer, int level = 0) const {
			UploadSubImage(data, layer, 0, 0, size.x, size.y, level);
		}
		void UploadSubImage(const void* data, int layer, int xOffset, int yOffset, int width, int height, int level = 0) const;
		void ProduceMipmaps() const override;
	private:
		int2 size;
		int numPages;
	};
}