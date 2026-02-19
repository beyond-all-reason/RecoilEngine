/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Height.h"
#include "Map/HeightLinePalette.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/GL/SubState.h"
#include "System/Color.h"
#include "System/Exceptions.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

#include "System/Misc/TracyDefs.h"


// currently defined in HeightLinePalette.cpp
//CONFIG(bool, ColorElev).defaultValue(true).description("If heightmap (default hotkey [F1]) should be colored or not.");



CHeightTexture::CHeightTexture()
: CModernInfoTexture("height")
, CEventClient("[CHeightTexture]", 271990, false)
, needUpdate(true)
{
	eventHandler.AddClient(this);

	texSize = int2(mapDims.mapxp1, mapDims.mapyp1);

	{
		GL::TextureCreationParams tcp{
			.reqNumLevels = 1,
			.wrapMirror = false,
			.minFilter = GL_NEAREST,
			.magFilter = GL_LINEAR
		};

		texture = GL::Texture2D(texSize, GL_RGBA8, tcp, false);
	}
	{
		GL::TextureCreationParams tcp{
			.reqNumLevels = 1,
			.minFilter = GL_NEAREST,
			.magFilter = GL_LINEAR,
			.wrapModes = std::array<int32_t, 3>{ GL_REPEAT, GL_CLAMP_TO_EDGE, GL_REPEAT } // R wrap unused for 2D textures
		};
		paletteTex = GL::Texture2D(256, 2, GL_RGBA8, tcp, false);
		auto binding = paletteTex.ScopedBind();
		paletteTex.UploadSubImage(CHeightLinePalette::paletteColored      , 0, 0, 256, 1);
		paletteTex.UploadSubImage(CHeightLinePalette::paletteBlackAndWhite, 0, 1, 256, 1);
	}

	CreateFBO("CHeightTexture");

	const std::string fragmentCode = R"(
		#version 130

		uniform sampler2D texHeight;
		uniform sampler2D texPalette;
		uniform float paletteOffset;

		in vec2 uv;
		out vec4 fragData;

		void main() {
			float h = texture(texHeight, uv).r;
			vec2 tc = vec2(h * (8. / 256.), paletteOffset);
			fragData = texture(texPalette, tc);
		}
	)";

	shader = shaderHandler->CreateProgramObject("[CHeightTexture]", "CHeightTexture");
	shader->AttachShaderObject(shaderHandler->CreateShaderObject(vertexCode,   "", GL_VERTEX_SHADER));
	shader->AttachShaderObject(shaderHandler->CreateShaderObject(fragmentCode, "", GL_FRAGMENT_SHADER));
	shader->Link();
	if (!shader->IsValid()) {
		const char* fmt = "%s-shader compilation error: %s";
		LOG_L(L_ERROR, fmt, shader->GetName().c_str(), shader->GetLog().c_str());
	} else {
		shader->Enable();
		shader->SetUniform("texHeight", 0);
		shader->SetUniform("texPalette", 1);
		shader->SetUniform("paletteOffset", configHandler->GetBool("ColorElev") ? 0.0f : 1.0f);
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

CHeightTexture::~CHeightTexture()
{
	RECOIL_DETAILED_TRACY_ZONE;
	shaderHandler->ReleaseProgramObject("[CHeightTexture]", "CHeightTexture");
}


void CHeightTexture::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	needUpdate = false;

	const auto hmTexID = readMap->GetHeightMapTexture();

	using namespace GL::State;
	auto state = GL::SubState(
		Blending(GL_FALSE)
	);
	auto binding = paletteTex.ScopedBind(1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hmTexID);

	RunFullScreenPass();
}


void CHeightTexture::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	RECOIL_DETAILED_TRACY_ZONE;
	needUpdate = true;
}


bool CHeightTexture::IsUpdateNeeded()
{
	RECOIL_DETAILED_TRACY_ZONE;
	return needUpdate;
}
