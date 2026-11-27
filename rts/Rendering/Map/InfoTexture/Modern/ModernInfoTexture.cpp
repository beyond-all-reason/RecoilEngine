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

void CModernInfoTexture::UploadThroughPBO(const GL::Texture2D& tex, PBO& pbo, const void* data, size_t size) const
{
	// a re-specified unpack buffer lets the driver upload asynchronously instead of
	// waiting for the GPU to release the texture, which was used by the previous frame
	pbo.Bind();
	pbo.New(size, GL_STREAM_DRAW, data);
	tex.UploadImage(nullptr);
	pbo.Unbind();
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