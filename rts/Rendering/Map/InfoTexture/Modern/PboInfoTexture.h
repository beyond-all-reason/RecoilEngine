/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Rendering/Map/InfoTexture/InfoTexture.h"
#include "Rendering/GL/PBO.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/VAO.h"

class CModernInfoTexture : public CInfoTexture
{
public:
	CModernInfoTexture(const std::string& name);
	~CModernInfoTexture() override;
	CModernInfoTexture(const CModernInfoTexture&) = delete; // no-copy

public:
	virtual void Update() = 0;
	virtual bool IsUpdateNeeded() = 0;

protected:
	static constexpr const char* vertexCode = "GLSL/FullscreenTriangleVS.glsl";
	PBO infoTexPBO;
};

namespace Shader {
	struct IProgramObject;
}

class CModernFBOInfoTexture : public CModernInfoTexture
{
public:
	CModernFBOInfoTexture(const std::string& name);
	~CModernFBOInfoTexture() override;
	CModernFBOInfoTexture(const CModernFBOInfoTexture&) = delete; // no-copy
protected:
	bool CreateFBO(const char* fboName);
	void RunFullScreenPass();
protected:
	FBO fbo;
	VAO vao;
	Shader::IProgramObject* shader;
};