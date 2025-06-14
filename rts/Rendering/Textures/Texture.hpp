#pragma once

#include <cstdint>
#include <array>
#include "System/type2.h"
#include "System/TemplateUtils.hpp"
#include "Rendering/GL/TexBind.h"
#include "Rendering/Textures/TextureCreationParams.hpp"

namespace GL {


	class Texture2D {
	public:
		Texture2D() {}
		Texture2D(int xsize_, int ysize_, uint32_t intFormat_, const TextureCreationParams& tcp_, bool wantCompress = true);
		Texture2D(const int2& size, uint32_t intFormat_, const TextureCreationParams& tcp_, bool wantCompress = true)
			: Texture2D(size.x, size.y, intFormat_, tcp_, wantCompress)
		{}
		~Texture2D();

		Texture2D(Texture2D&& other) noexcept { *this = std::move(other); }
		Texture2D(const Texture2D&) = delete;

		Texture2D& operator=(Texture2D&& other) noexcept;
		Texture2D& operator=(const Texture2D&) = delete;

		bool IsValid() const { return texID != 0; }
		const auto GetId() const { return texID; }
		const auto DisOwn() { ownTexID = false; return texID; }

		[[nodiscard]] GL::TexBind ScopedBind();
		[[nodiscard]] GL::TexBind ScopedBind(uint32_t relSlot);
		// reuse the existing scoped binding
		void ScopedBind(const GL::TexBind& existingScopedBinding);
		void Bind();
		void Bind(uint32_t relSlot);
		void Unbind();
		void Unbind(uint32_t relSlot);

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
		void ProduceMipmaps() const;
	private:
		int2 size;
		uint32_t texID = 0;
		uint32_t intFormat = 0;
		int32_t numLevels = -1;
		uint32_t lastBoundSlot = 0;
		bool ownTexID = true;
		static constexpr uint32_t texTarget = 0x0DE1; /*GL_TEXTURE_2D*/
	};
}