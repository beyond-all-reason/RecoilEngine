#pragma once

#include <cstdint>
#include <array>
#include <optional>
#include "Rendering/GL/TexBind.h"
#include "System/float4.h"

namespace GL {
	struct TextureCreationParams {
		float aniso = 0.0f;
		float lodBias = 0.0f;
		uint32_t texID = 0;
		int32_t reqNumLevels = 0;
		bool linearMipMapFilter = true;
		bool linearTextureFilter = true;
		bool wrapMirror = true;
		bool repeatMirror = false;
		std::optional<float4> clampBorder = std::nullopt;
		uint32_t GetMinFilter(int32_t numLevels) const;
		uint32_t GetMagFilter() const;
		uint32_t GetWrapMode() const;
	};

	class Texture {
	public:
		virtual ~Texture();
	protected:
		uint32_t texID = 0;
		uint32_t texTarget = 0;
		uint32_t intFormat = 0;
		int32_t numLevels = -1;
		bool ownTexID = true;
		virtual void UploadImageAndCreateMipmaps(const void* data) const = 0;
	};

	class Texture2D : public Texture {
	public:
		Texture2D();
		Texture2D(int xsize_, int ysize_, uint32_t intFormat_, const TextureCreationParams& tcp_, bool wantCompress = true);

		template <typename ContainerLike>
		void UploadImageAndCreateMipmaps(const ContainerLike& containerLike) {
		#ifdef DEBUG
			const auto dataType = GetDataTypeFromInternalFormat(intFormat);
			const auto dataSize = GetDataTypeSize(dataType);
			const auto numChannels = GetNumChannelsFromInternalFormat(intFormat);
			assert(containerLike.size() >= xsize * ysize * numChannels * dataSize);
		#endif // DEBUG
			UploadImageAndCreateMipmaps(containerLike.data());
		}
		void UploadImageAndCreateMipmaps(const void* data) const override;
	protected:
		int xsize;
		int ysize;
	};
}