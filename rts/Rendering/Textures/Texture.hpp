#pragma once

#include <cstdint>
#include <array>
#include "Rendering/GL/TexBind.h"
#include "Rendering/Textures/TextureCreationParams.hpp"

namespace GL {
	class Texture {
	public:
		virtual ~Texture();
		const auto GetId() const { return texID; }
		const auto DisOwn() { ownTexID = false; return texID; }
	protected:
		uint32_t texID = 0;
		uint32_t texTarget = 0;
		uint32_t intFormat = 0;
		int32_t numLevels = -1;
		bool ownTexID = true;
		virtual void UploadImage(const void* data, int level = 0) const = 0;
		virtual void ProduceMipmaps() const = 0;
	};

	class Texture2D : public Texture {
	public:
		Texture2D();
		Texture2D(int xsize_, int ysize_, uint32_t intFormat_, const TextureCreationParams& tcp_, bool wantCompress = true);

		template <typename ContainerLike>
		void UploadImage(const ContainerLike& containerLike) const {
		#ifdef DEBUG
			const auto dataType = GetDataTypeFromInternalFormat(intFormat);
			const auto dataSize = GetDataTypeSize(dataType);
			const auto numChannels = GetNumChannelsFromInternalFormat(intFormat);
			assert(containerLike.size() >= xsize * ysize * numChannels * dataSize);
		#endif // DEBUG
			UploadImage(containerLike.data());
		}
		void UploadImage(const void* data, int level = 0) const override {
			UploadSubImage(data, 0, 0, xsize, ysize, level);
		}
		void UploadSubImage(const void* data, int xOffset, int yOffset, int width, int height, int level = 0) const;
		void ProduceMipmaps() const override;
	protected:
		int xsize;
		int ysize;
	};
}