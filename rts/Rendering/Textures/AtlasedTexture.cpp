#include "AtlasedTexture.hpp"

CR_BIND(AtlasedTexture, )
CR_REG_METADATA(AtlasedTexture, (
	CR_MEMBER(x),
	CR_MEMBER(y),
	CR_MEMBER(z),
	CR_MEMBER(w)
))
const AtlasedTexture& AtlasedTexture::DefaultAtlasTexture = AtlasedTexture{};

CR_BIND(AtlasedTextureLayered, )
CR_REG_METADATA(AtlasedTextureLayered, (
	CR_MEMBER(pageNum)
))
const AtlasedTextureLayered& AtlasedTextureLayered::DefaultAtlasTextureLayered = AtlasedTextureLayered{};