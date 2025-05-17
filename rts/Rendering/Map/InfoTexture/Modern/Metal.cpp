/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Metal.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"

#include "System/Misc/TracyDefs.h"



CMetalTexture::CMetalTexture()
: CModernInfoTexture("metal")
, CEventClient("[CMetalTexture]", 271990, false)
, metalMapChanged(true)
{
	eventHandler.AddClient(this);
	texSize = int2(mapDims.hmapx, mapDims.hmapy);

	GL::TextureCreationParams tcp{
		.reqNumLevels = 1,
		.linearMipMapFilter = false,
		.linearTextureFilter = true,
		.wrapMirror = false
	};

	texture = GL::Texture2D(texSize, GL_R8, tcp, false);
}

void CMetalTexture::Update()
{
	assert(metalMap.GetSizeX() == texSize.x && metalMap.GetSizeZ() == texSize.y);

	auto binding = texture.ScopedBind();
	texture.UploadImage(metalMap.GetDistributionMap());

	metalMapChanged = false;
}


void CMetalTexture::MetalMapChanged(const int x, const int z)
{
	metalMapChanged = true;
}


bool CMetalTexture::IsUpdateNeeded()
{
	return metalMapChanged;
}
