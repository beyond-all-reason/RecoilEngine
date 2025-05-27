/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Radar.h"
#include "InfoTextureHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/GL/SubState.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"

#include "System/Misc/TracyDefs.h"



CRadarTexture::CRadarTexture()
: CModernInfoTexture("radar")
{
	texSize = losHandler->radar.size;

	GL::TextureCreationParams tcp{
		.reqNumLevels = -1,
		.wrapMirror = false,
		.minFilter = GL_NEAREST,
		.magFilter = GL_LINEAR
	};

	texture = GL::Texture2D(texSize, GL_RG8, tcp, false);

	CreateFBO("CRadarTexture");

	const std::string fragmentCode = R"(
		#version 130

		uniform sampler2D texLoS;
		uniform sampler2D texRadar;
		uniform sampler2D texJammer;

		in vec2 uv;
		out vec2 fragData;

		void main() {
			float los = texture(texLoS, uv).r;

			float fr = texture(texRadar,  uv).r;
			float fj = texture(texJammer, uv).r;

			fragData = vec2(
				float(fr > 0.0),
				los * float(fj > 0.0)
			);
		}
	)";

	shader = shaderHandler->CreateProgramObject("[CRadarTexture]", "CRadarTexture");
	shader->AttachShaderObject(shaderHandler->CreateShaderObject(vertexCode,   "", GL_VERTEX_SHADER));
	shader->AttachShaderObject(shaderHandler->CreateShaderObject(fragmentCode, "", GL_FRAGMENT_SHADER));
	shader->Link();
	if (!shader->IsValid()) {
		const char* fmt = "%s-shader compilation error: %s";
		LOG_L(L_ERROR, fmt, shader->GetName().c_str(), shader->GetLog().c_str());
	} else {
		shader->Enable();
		shader->SetUniform("texRadar",  1);
		shader->SetUniform("texJammer", 0);
		shader->SetUniform("texLoS",    2);
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
			.wrapMirror = false,
			.minFilter = GL_NEAREST,
			.magFilter = GL_NEAREST
		};

		uploadTexRadar  = GL::Texture2D(texSize, GL_R16, tcp, false);
		uploadTexJammer = GL::Texture2D(texSize, GL_R16, tcp, false);
	}

	if (!fbo.IsValid() || !shader->IsValid()) {
		throw opengl_error("");
	}
}


CRadarTexture::~CRadarTexture()
{
	RECOIL_DETAILED_TRACY_ZONE;
	shaderHandler->ReleaseProgramObject("[CRadarTexture]", "CRadarTexture");
}


void CRadarTexture::UpdateCPU()
{
	RECOIL_DETAILED_TRACY_ZONE;
	static std::vector<uint8_t> infoTexMem;
	infoTexMem.resize(texSize.x * texSize.y * 2);

	if (!losHandler->GetGlobalLOS(gu->myAllyTeam)) {
		const int jammerAllyTeam = modInfo.separateJammers ? gu->myAllyTeam : 0;

		const auto& myLos = losHandler->los.losMaps[gu->myAllyTeam].GetLosMap();

		const auto& myRadar  = losHandler->radar.losMaps[gu->myAllyTeam].GetLosMap();
		const auto& myJammer = losHandler->jammer.losMaps[jammerAllyTeam].GetLosMap();
		for (int y = 0; y < texSize.y; ++y) {
			for (int x = 0; x < texSize.x; ++x) {
				const int idx = y * texSize.x + x;
				infoTexMem[idx * 2 + 0] = ( myRadar[idx] != 0) ? 0xFF : 0x00;
				infoTexMem[idx * 2 + 1] = (myJammer[idx] != 0 && myLos[idx] != 0) ? 0xFF : 0x00;
			}
		}
	} else {
		for (int y = 0; y < texSize.y; ++y) {
			for (int x = 0; x < texSize.x; ++x) {
				const int idx = y * texSize.x + x;
				infoTexMem[idx * 2 + 0] = 0xFF;
				infoTexMem[idx * 2 + 1] = 0x00;
			}
		}
	}

	auto binding = texture.ScopedBind();
	texture.UploadImage(infoTexMem.data());
}


void CRadarTexture::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!fbo.IsValid() || !shader->IsValid() || !uploadTexRadar.IsValid() || uploadTexJammer.IsValid())
		return UpdateCPU();

	if (losHandler->GetGlobalLOS(gu->myAllyTeam)) {
		fbo.Bind();
		glViewport(0, 0, texSize.x, texSize.y);
		glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		globalRendering->LoadViewport();
		FBO::Unbind();

		auto binding = texture.ScopedBind();
		texture.ProduceMipmaps();
		return;
	}

	const int jammerAllyTeam = modInfo.separateJammers ? gu->myAllyTeam : 0;

	const auto& myRadar  = losHandler->radar.losMaps[gu->myAllyTeam].GetLosMap();
	const auto& myJammer = losHandler->jammer.losMaps[jammerAllyTeam].GetLosMap();

	auto binding1 = uploadTexRadar.ScopedBind(1);
	uploadTexRadar.UploadImage(myRadar.data());

	auto binding0 = uploadTexJammer.ScopedBind(0);
	uploadTexJammer.UploadImage(myJammer.data());

	// do post-processing on the gpu (los-checking & scaling)
	using namespace GL::State;
	auto state = GL::SubState(
		Blending(GL_FALSE)
	);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetInfoTexture("los")->GetTexture());
	RunFullScreenPass();

	// cleanup
	glBindTexture(GL_TEXTURE_2D, 0);
	binding1 = {};
	binding0 = {};

	// generate mipmaps
	glActiveTexture(GL_TEXTURE0);
	auto binding = texture.ScopedBind();
	texture.ProduceMipmaps();
}
