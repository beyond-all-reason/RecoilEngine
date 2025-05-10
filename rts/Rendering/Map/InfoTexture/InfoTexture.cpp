/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "InfoTexture.h"


CInfoTexture::CInfoTexture()
	: texture{}
	, texSize(0, 0)
{}

CInfoTexture::CInfoTexture(const std::string& _name, GL::Texture2D&& _texture, int2 _texSize)
	: texture(std::move(_texture))
	, name(_name)
	, texSize(_texSize)
{}