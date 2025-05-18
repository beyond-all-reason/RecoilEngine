/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MetalExtraction.h"
#include "InfoTextureHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/GL/SubState.h"
#include "Sim/Misc/LosHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"

#include "System/Misc/TracyDefs.h"



CMetalExtractionTexture::CMetalExtractionTexture()
: CModernInfoTexture("metalextraction")
, updateN(0)
{
	texSize = int2(mapDims.hmapx, mapDims.hmapy);

	GL::TextureCreationParams tcp{
		.reqNumLevels = 1,
		.wrapMirror = false,
		.minFilter = GL_NEAREST,
		.magFilter = GL_LINEAR
	};

	//Info: 32F isn't really needed for the final result, but it allows us
	//  to upload the CPU array directly to the GPU w/o any (slow) cpu-side
	//  transformation. The transformation (0..1 range rescaling) happens
	//  then on the gpu instead.
	texture = GL::Texture2D(texSize, GL_R32F, tcp, false);

	CreateFBO("CLosTexture");

	const std::string fragmentCode = R"(
		#version 130

		uniform sampler2D tex0;

		in vec2 uv;
		out vec4 fragData;

		void main() {
			fragData = texture(tex0, uv) * 800.0;
		}
	)";

	shader = shaderHandler->CreateProgramObject("[CMetalExtractionTexture]", "CMetalExtractionTexture");
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

	if (!fbo.IsValid() || !shader->IsValid()) {
		throw opengl_error("");
	}
}

CMetalExtractionTexture::~CMetalExtractionTexture()
{
	RECOIL_DETAILED_TRACY_ZONE;
	shaderHandler->ReleaseProgramObject("[CMetalExtractionTexture]", "CMetalExtractionTexture");
}


bool CMetalExtractionTexture::IsUpdateNeeded()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// update only once per second
	return (updateN++ % GAME_SPEED == 0);
}


void CMetalExtractionTexture::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// los-checking is done in FBO: when FBO isn't working don't expose hidden data!
	if (!fbo.IsValid() || !shader->IsValid())
		return;

	CInfoTexture* infoTex = infoTextureHandler->GetInfoTexture("los");

	assert(metalMap.GetSizeX() == texSize.x && metalMap.GetSizeZ() == texSize.y);

	// upload raw data to gpu
	{
		auto binding = texture.ScopedBind();
		texture.UploadImage(metalMap.GetExtractionMap());
	}

	using namespace GL::State;
	auto state = GL::SubState(
		Blending(GL_TRUE),
		BlendFunc(GL_ZERO, GL_SRC_COLOR)
	);

	// do post-processing on the gpu (los-checking & scaling)
	glBindTexture(GL_TEXTURE_2D, infoTex->GetTexture());
	RunFullScreenPass();
	glBindTexture(GL_TEXTURE_2D, 0);
}
