/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "WaterRendering.h"

#include "Rendering/GlobalRendering.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "RefractWater.h"
#include "Map/ReadMap.h"

#include "System/Misc/TracyDefs.h"

void CRefractWater::InitResources(bool loadShader)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CAdvWater::InitResources(false);
	LoadGfx();
}

void CRefractWater::FreeResources()
{
	RECOIL_DETAILED_TRACY_ZONE;
	shaderHandler->ReleaseProgramObjects("[RefractWater]");
	waterShader = nullptr;

	if (subSurfaceTex) {
		glDeleteTextures(1, &subSurfaceTex);
		subSurfaceTex = 0;
	}
}

void CRefractWater::LoadGfx()
{
	RECOIL_DETAILED_TRACY_ZONE;
	glGenTextures(1, &subSurfaceTex);
	glBindTexture(GL_TEXTURE_2D, subSurfaceTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, globalRendering->viewSizeX, globalRendering->viewSizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

	waterShader = shaderHandler->CreateProgramObject("[RefractWater]", "RefractWater");
	waterShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/AdvWaterVS.glsl", "", GL_VERTEX_SHADER));
	waterShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/RefractWaterFS.glsl", "", GL_FRAGMENT_SHADER));
	waterShader->Link();
	waterShader->Enable();
	waterShader->SetUniform("reflectTex", 0);
	waterShader->SetUniform("bumpTex", 1);
	waterShader->SetUniform("subSurfTex", 2);
	waterShader->SetUniform("waterDepthTex", 3);
	waterShader->SetUniform("waterDepthScale",
		1.0f / (mapDims.mapxp1 * SQUARE_SIZE),
		1.0f / (mapDims.mapyp1 * SQUARE_SIZE));
	waterShader->Disable();
}

void CRefractWater::Draw()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!waterRendering->forceRendering && !readMap->HasVisibleWater())
		return;

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, subSurfaceTex);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, globalRendering->viewPosX, globalRendering->viewPosY, globalRendering->viewSizeX, globalRendering->viewSizeY);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, readMap->GetShadingTexture());

	glActiveTexture(GL_TEXTURE0);

	waterShader->Enable();
	waterShader->SetUniform("env2", 10.0f, 10.0f, 0.0f, 0.0f);
	waterShader->SetUniform("env3",
		1.0f / globalRendering->viewSizeX,
		1.0f / globalRendering->viewSizeY,
		0.0f, 0.0f);
	waterShader->Disable();

	CAdvWater::Draw(false);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
}
