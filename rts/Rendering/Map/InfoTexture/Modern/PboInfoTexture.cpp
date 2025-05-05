/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PboInfoTexture.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/GlobalRendering.h"


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

bool CModernFBOInfoTexture::CreateFBO(const char* fboName)
{
	if (!FBO::IsSupported())
		return false;

	fbo.Bind();
	fbo.AttachTexture(texture);
	bool status = fbo.CheckStatus(fboName);
	FBO::Unbind();

	return status;
}

void CModernFBOInfoTexture::RunFullScreenPass()
{
	fbo.Bind();
	glViewport(0, 0, texSize.x, texSize.y);
	shader->Enable();
	vao.Bind();
	glDrawArrays(GL_TRIANGLES, 0, 3); // full screen triangle
	vao.Unbind();
	shader->Disable();
	FBO::Unbind();
	globalRendering->LoadViewport();
}
