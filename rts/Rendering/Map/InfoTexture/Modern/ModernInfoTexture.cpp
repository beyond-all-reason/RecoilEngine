/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ModernInfoTexture.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/GlobalRendering.h"


CModernInfoTexture::CModernInfoTexture(const std::string& _name)
	: CInfoTexture(_name, {}, int2(0, 0))
{}

bool CModernInfoTexture::CreateFBO(const char* fboName)
{
	if (!FBO::IsSupported())
		return false;

	fbo.Bind();
	fbo.AttachTexture(texture.GetId());
	bool status = fbo.CheckStatus(fboName);
	FBO::Unbind();

	return status;
}

void CModernInfoTexture::RunFullScreenPass()
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