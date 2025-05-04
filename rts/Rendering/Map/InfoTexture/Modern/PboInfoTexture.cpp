/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PboInfoTexture.h"


CModernInfoTexture::CModernInfoTexture(const std::string& _name)
{
	name        = _name;
	texChannels = 0;
	texture     = 0;
}


CModernInfoTexture::~CModernInfoTexture()
{
	glDeleteTextures(1, &texture);
}

CModernFBOInfoTexture::CModernFBOInfoTexture(const std::string& name)
	: CModernInfoTexture(name)
	, shader(nullptr)
{
}

CModernFBOInfoTexture::~CModernFBOInfoTexture()
{
}
