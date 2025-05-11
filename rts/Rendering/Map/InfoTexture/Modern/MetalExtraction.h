/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _METAL_EXTRACTION_TEXTURE_H
#define _METAL_EXTRACTION_TEXTURE_H

#include "PboInfoTexture.h"
#include "Rendering/GL/FBO.h"


namespace Shader {
	struct IProgramObject;
}


class CMetalExtractionTexture : public CModernFBOInfoTexture
{
public:
	CMetalExtractionTexture();
	~CMetalExtractionTexture() override;
public:
	void Update() override;
	bool IsUpdateNeeded() override;

private:
	int updateN;
};

#endif // _METAL_EXTRACTION_TEXTURE_H
