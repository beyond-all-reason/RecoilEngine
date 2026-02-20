/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Texture.hpp"
#include "System/type2.h"
#include <string>

class CInfoTexture
{
public:
	CInfoTexture();
	CInfoTexture(const std::string& name, GL::Texture2D&& texture, int2 texSize);
	virtual ~CInfoTexture() {}

public:
	virtual GLuint GetTexture() { return texture.GetId(); }
	int2 GetTexSize()     const { return texSize; }
	const std::string& GetName() const { return name; }
protected:
	friend class IInfoTextureHandler;

	GL::Texture2D texture;
	std::string name;
	int2 texSize;
};

class CDummyInfoTexture: public CInfoTexture {
public:
	CDummyInfoTexture() : CInfoTexture("dummy", {}, int2(0, 0)) {}
};