/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AirLos.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Sim/Misc/LosHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"

#include "System/Misc/TracyDefs.h"


CAirLosTexture::CAirLosTexture()
: CModernInfoTexture("airlos")
{
	texSize = losHandler->airLos.size;

	GL::TextureCreationParams tcp{
		.reqNumLevels = -1,
		.linearMipMapFilter = false,
		.linearTextureFilter = true,
		.wrapMirror = false
	};

	texture = GL::Texture2D(texSize, GL_R8, tcp, false);

	infoTexPBO.Bind();
	infoTexPBO.New(texSize.x * texSize.y * 2, GL_STREAM_DRAW);
	infoTexPBO.Unbind();

	CreateFBO("CAirLosTexture");

	const std::string fragmentCode = R"(
		#version 130

		uniform sampler2D tex0;

		in vec2 uv;
		out float fragData;

		void main() {
			float f = texture(tex0, uv).r;

			fragData = float(f > 0.0);
		}
	)";

	shader = shaderHandler->CreateProgramObject("[CAirLosTexture]", "CAirLosTexture");
	shader->AttachShaderObject(shaderHandler->CreateShaderObject(vertexCode,   "", GL_VERTEX_SHADER));
	shader->AttachShaderObject(shaderHandler->CreateShaderObject(fragmentCode, "", GL_FRAGMENT_SHADER));
	shader->Link();
	if (!shader->IsValid()) {
		const char* fmt = "%s-shader compilation error: %s";
		LOG_L(L_ERROR, fmt, shader->GetName().c_str(), shader->GetLog().c_str());
	} else {
		shader->Enable();
		shader->SetUniform("tex0", 0);
		shader->Disable();
		shader->Validate();
		if (!shader->IsValid()) {
			const char* fmt = "%s-shader validation error: %s";
			LOG_L(L_ERROR, fmt, shader->GetName().c_str(), shader->GetLog().c_str());
		}
	}

	if (fbo.IsValid() && shader->IsValid()) {
		GL::TextureCreationParams tcp{
			.reqNumLevels = 1,
			.linearMipMapFilter = false,
			.linearTextureFilter = false,
			.wrapMirror = false
		};

		uploadTex = GL::Texture2D(texSize, GL_R16, tcp, false);
	} else {
		throw opengl_error("");
	}
}


CAirLosTexture::~CAirLosTexture()
{
	RECOIL_DETAILED_TRACY_ZONE;
	shaderHandler->ReleaseProgramObject("[CAirLosTexture]", "CAirLosTexture");
}


void CAirLosTexture::UpdateCPU()
{
	RECOIL_DETAILED_TRACY_ZONE;
	infoTexPBO.Bind();
	auto infoTexMem = reinterpret_cast<unsigned char*>(infoTexPBO.MapBuffer());

	if (!losHandler->GetGlobalLOS(gu->myAllyTeam)) {
		const unsigned short* myAirLos = &losHandler->airLos.losMaps[gu->myAllyTeam].front();
		for (int y = 0; y < texSize.y; ++y) {
			for (int x = 0; x < texSize.x; ++x) {
				infoTexMem[y * texSize.x + x] = (myAirLos[y * texSize.x + x] != 0) ? 255 : 0;
			}
		}
	} else {
		memset(infoTexMem, 255, texSize.x * texSize.y);
	}

	infoTexPBO.UnmapBuffer();
	auto binding = texture.ScopedBind();
	texture.UploadImage(infoTexPBO.GetPtr());
	texture.ProduceMipmaps();
	infoTexPBO.Invalidate();
	infoTexPBO.Unbind();
}


void CAirLosTexture::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!fbo.IsValid() || !shader->IsValid() || !uploadTex.IsValid())
		return UpdateCPU();

	if (losHandler->GetGlobalLOS(gu->myAllyTeam)) {
		fbo.Bind();
		glViewport(0, 0, texSize.x, texSize.y);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		globalRendering->LoadViewport();
		FBO::Unbind();

		auto binding = texture.ScopedBind();
		texture.ProduceMipmaps();
		return;
	}

	infoTexPBO.Bind();
	auto infoTexMem = reinterpret_cast<unsigned char*>(infoTexPBO.MapBuffer());
	const unsigned short* myAirLos = &losHandler->airLos.losMaps[gu->myAllyTeam].front();
	memcpy(infoTexMem, myAirLos, texSize.x * texSize.y * 1 * sizeof(short));
	infoTexPBO.UnmapBuffer();

	{
		auto binding = uploadTex.ScopedBind();
		uploadTex.UploadImage(infoTexPBO.GetPtr());
	}
	infoTexPBO.Invalidate();
	infoTexPBO.Unbind();

	// do post-processing on the gpu (los-checking & scaling)
	glDisable(GL_BLEND);
	RunFullScreenPass();

	// generate mipmaps
	auto binding = texture.ScopedBind();
	texture.ProduceMipmaps();
}
