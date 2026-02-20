#include "AtlasedTexture.hpp"

CR_BIND(AtlasedTexture, )
CR_REG_METADATA(AtlasedTexture, (
	CR_MEMBER(x),
	CR_MEMBER(y),
	CR_MEMBER(z),
	CR_MEMBER(w),
	CR_MEMBER(pageNum)
))

const AtlasedTexture& AtlasedTexture::DefaultAtlasTexture = AtlasedTexture{};