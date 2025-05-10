#pragma once

#include <cstdint>
#include <array>
#include "System/type2.h"
#include "Rendering/GL/TexBind.h"
#include "Rendering/Textures/TextureCreationParams.hpp"

namespace GL {
	template <typename T>
	concept ContainerLike = requires(const T & t) {
		// Validate .size() returns std::size_t
		{ t.size() } -> std::same_as<std::size_t>;
		// Validate .data() returns a pointer to element type
		{ t.data() } -> std::convertible_to<const typename T::value_type*>;
	};

	class Texture2D {
	public:
		Texture2D() {}
		Texture2D(int xsize_, int ysize_, uint32_t intFormat_, const TextureCreationParams& tcp_, bool wantCompress = true);
		~Texture2D();

		Texture2D(Texture2D&& other) noexcept { *this = std::move(other); }
		Texture2D(const Texture2D&) = delete;

		Texture2D& operator=(Texture2D&& other) noexcept;
		Texture2D& operator=(const Texture2D&) = delete;

		const auto GetId() const { return texID; }
		const auto DisOwn() { ownTexID = false; return texID; }

		[[nodiscard]] GL::TexBind ScopedBind();
		[[nodiscard]] GL::TexBind ScopedBind(uint32_t relSlot);
		void Bind();
		void Bind(uint32_t relSlot);
		void Unbind();
		void Unbind(uint32_t relSlot);

		template <ContainerLike C>
		void UploadImage(const C& c) const {
		#ifdef DEBUG
			const auto dataType = GetDataTypeFromInternalFormat(intFormat);
			const auto dataSize = GetDataTypeSize(dataType);
			const auto numChannels = GetNumChannelsFromInternalFormat(intFormat);
			assert(c.size() >= size.x * size.y * numChannels * dataSize);
		#endif // DEBUG
			UploadImage(c.data());
		}
		void UploadImage(const void* data, int level = 0) const {
			UploadSubImage(data, 0, 0, size.x, size.y, level);
		}
		void UploadSubImage(const void* data, int xOffset, int yOffset, int width, int height, int level = 0) const;
		void ProduceMipmaps() const;
	public:
		int2 size;
		uint32_t texID = 0;
		uint32_t intFormat = 0;
		int32_t numLevels = -1;
		uint32_t lastBoundSlot = 0;
		bool ownTexID = true;
		static constexpr uint32_t texTarget = 0x0DE1; /*GL_TEXTURE_2D*/
	};
}