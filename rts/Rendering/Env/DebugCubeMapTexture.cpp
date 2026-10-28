#include "DebugCubeMapTexture.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "System/Log/ILog.h"
#include "Game/Camera.h"

DebugCubeMapTexture::DebugCubeMapTexture()
	: texId(0)
	, usesFile(false) // plain colours until /DebugCubeMap file asks for the labelled testsky.dds
	, vao()
{
#ifndef HEADLESS
	CreateTexture(usesFile);

	shader = shaderHandler->CreateProgramObject("[DebugCubeMap]", "DebugCubeMap");
	shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/CubeMapVS.glsl", "", GL_VERTEX_SHADER));
	shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/CubeMapFS.glsl", "", GL_FRAGMENT_SHADER));
	shader->Link();
	shader->Enable();
	shader->SetUniform("uvFlip", 1.0f, -1.0f, 1.0f);
	shader->SetUniform("skybox", 0);
	shader->Disable();
	shader->Validate();
#endif
}

void DebugCubeMapTexture::CreateTexture(bool wantFile)
{
#ifndef HEADLESS
	// the old texture is replaced; a sky box borrowing it must be rebuilt afterwards (ISky::SetSky)
	if (texId != 0) {
		glDeleteTextures(1, &texId);
		texId = 0;
	}

	static constexpr const char* texture = "bitmaps/testsky.dds";

	CBitmap btex;
	usesFile = wantFile && btex.Load(texture) && btex.textype == GL_TEXTURE_CUBE_MAP;

	if (!usesFile) {
		if (wantFile)
			LOG_L(L_WARNING, "[DebugCubeMapTexture] could not load debug skybox texture from file %s, using fallback colors", texture);

		// match testsky.dds colors
		static constexpr const SColor debugFaceColors[] = {
			{1.0f, 1.0f, 0.0f, 1.0f}, // yellow GL_TEXTURE_CUBE_MAP_POSITIVE_X, Right
			{0.0f, 1.0f, 0.0f, 1.0f}, // green 	GL_TEXTURE_CUBE_MAP_NEGATIVE_X, Left
			{1.0f, 1.0f, 1.0f, 1.0f}, // white 	GL_TEXTURE_CUBE_MAP_POSITIVE_Y, Top
			{0.0f, 0.0f, 0.0f, 1.0f}, // black	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, Bottom
			{0.0f, 1.0f, 1.0f, 1.0f}, // cyan	GL_TEXTURE_CUBE_MAP_POSITIVE_Z, Front
			{1.0f, 0.0f, 0.0f, 1.0f}, // red  	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, Back
		};

		static constexpr GLsizei FALLBACK_DIM = 16;
		std::vector<SColor> debugColorVec;
		debugColorVec.resize(FALLBACK_DIM * FALLBACK_DIM);

		dims = { FALLBACK_DIM, FALLBACK_DIM };

		// the file branch gets its texture from CBitmap::CreateTexture, so a name is reserved only here
		glGenTextures(1, &texId);
		glEnable(GL_TEXTURE_CUBE_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texId);
		for (GLenum glFace = GL_TEXTURE_CUBE_MAP_POSITIVE_X; glFace <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; ++glFace) {
			std::fill(debugColorVec.begin(), debugColorVec.end(), debugFaceColors[glFace - GL_TEXTURE_CUBE_MAP_POSITIVE_X]);
			glTexImage2D(glFace, 0, GL_RGBA8, FALLBACK_DIM, FALLBACK_DIM, 0, GL_RGBA, GL_UNSIGNED_BYTE, debugColorVec.data());
		}
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		glDisable(GL_TEXTURE_CUBE_MAP);
	} else {
		dims = { btex.xsize, btex.ysize };
		texId = btex.CreateTexture();
	}

	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texId);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glDisable(GL_TEXTURE_CUBE_MAP);
#endif
}

bool DebugCubeMapTexture::SetUseFile(bool useFile)
{
	CreateTexture(useFile);
	return usesFile;
}

DebugCubeMapTexture::~DebugCubeMapTexture()
{
#ifndef HEADLESS
	glDeleteTextures(1, &texId);
	shaderHandler->ReleaseProgramObject("[DebugCubeMap]", "DebugCubeMap");
#endif
}

void DebugCubeMapTexture::Draw(uint32_t face) const
{
#ifndef HEADLESS
	assert(face == 0 || (face >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z));

	//all faces (default) are expected
	GLint baseVertex = 0;
	GLsizei vertCount = 36;

	//one face is expected
	if (face > 0) {
		baseVertex = (face - GL_TEXTURE_CUBE_MAP_POSITIVE_X) * 6;
		vertCount = 6;
	}

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);

	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texId);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	CMatrix44f view = camera->GetViewMatrix();
	view.SetPos(float3());
	glLoadMatrixf(view);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(camera->GetProjectionMatrix());

	vao.Bind();
	assert(shader->IsValid());
	shader->Enable();

	glDrawArrays(GL_TRIANGLES, baseVertex, vertCount);

	shader->Disable();
	vao.Unbind();

	// glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glDisable(GL_TEXTURE_CUBE_MAP);
#endif
}

DebugCubeMapTexture& DebugCubeMapTexture::GetInstance()
{
	static DebugCubeMapTexture instance;
	return instance;
}
