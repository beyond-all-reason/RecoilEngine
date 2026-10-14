/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/Game.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Map/MapInfo.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Env/DebugCubeMapTexture.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "Sim/Misc/GlobalConstants.h"

#include "System/Misc/TracyDefs.h"

CONFIG(int, CubeTexSizeSpecular).defaultValue(128).minimumValue(1).description("The square resolution of each face of the specular cubemap.");
CONFIG(int, CubeTexSizeReflection).defaultValue(128).minimumValue(1).description("The square resolution of each face of the environment reflection cubemap.");
CONFIG(bool, CubeTexGenerateMipMaps).defaultValue(false).description("Generate mipmaps for the reflection and specular cubemap textures, useful for efficient subsampling and blurring.");
CONFIG(std::string, PBREnvironmentCapture).defaultValue("legacy-clone").description("PBR environment capture mode: legacy-clone, hdr-modern, or compare.");
CONFIG(std::string, PBREnvironmentCaptureAnchor).defaultValue("map-center").description("PBR environment capture anchor: map-center or camera.");
CONFIG(std::string, PBREnvironmentCaptureContent).defaultValue("sky-only").description("PBR environment capture content: sky-only, sky-terrain, or legacy-scene.");
CONFIG(std::string, PBREnvironmentFilter).defaultValue("raster").description("PBR environment filtering backend. Raster is currently implemented; compute falls back explicitly.");
CONFIG(int, PBREnvironmentSize).defaultValue(256).minimumValue(16).maximumValue(2048).description("PBR scene-linear HDR environment cubemap face size.");
CONFIG(int, PBRIrradianceSize).defaultValue(32).minimumValue(4).maximumValue(256).description("PBR diffuse irradiance cubemap face size.");
CONFIG(int, PBRSpecularSize).defaultValue(128).minimumValue(16).maximumValue(1024).description("PBR GGX-prefiltered specular cubemap base face size.");
CONFIG(int, PBREnvironmentRevision).defaultValue(0).minimumValue(0).description("Increment to request a new atomic PBR environment capture/filter epoch.");

namespace {
	constexpr const char* pbrFilterVertexShader = R"(
#version 150 compatibility

out vec2 texCoord;

void main()
{
	vec2 position = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
	texCoord = position * 0.5;
	gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
}
)";

	constexpr const char* pbrFilterFragmentShader = R"(
#version 150 compatibility

const float PI = 3.14159265358979323846;
const uint DIFFUSE_SAMPLE_COUNT = 128u;
const uint SPECULAR_SAMPLE_COUNT = 256u;

uniform samplerCube sourceEnvironment;
uniform int faceIndex;
uniform int filterType;
uniform float roughness;
uniform float sourceResolution;

in vec2 texCoord;
out vec4 fragColor;

float RadicalInverseVdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint index, uint count)
{
	return vec2(float(index) / float(count), RadicalInverseVdC(index));
}

vec3 FaceDirection(int face, vec2 uv)
{
	vec2 p = uv * 2.0 - 1.0;
	if (face == 0) return normalize(vec3( 1.0, -p.y, -p.x));
	if (face == 1) return normalize(vec3(-1.0, -p.y,  p.x));
	if (face == 2) return normalize(vec3( p.x,  1.0,  p.y));
	if (face == 3) return normalize(vec3( p.x, -1.0, -p.y));
	if (face == 4) return normalize(vec3( p.x, -p.y,  1.0));
	return normalize(vec3(-p.x, -p.y, -1.0));
}

mat3 TangentBasis(vec3 normal)
{
	vec3 up = abs(normal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, normal));
	vec3 bitangent = cross(normal, tangent);
	return mat3(tangent, bitangent, normal);
}

vec3 ImportanceSampleGGX(vec2 xi, float alpha, vec3 normal)
{
	float phi = 2.0 * PI * xi.x;
	float alpha2 = alpha * alpha;
	float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (alpha2 - 1.0) * xi.y));
	float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
	vec3 halfVector = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
	return normalize(TangentBasis(normal) * halfVector);
}

vec3 FilterDiffuse(vec3 normal)
{
	vec3 sum = vec3(0.0);
	mat3 basis = TangentBasis(normal);
	for (uint i = 0u; i < DIFFUSE_SAMPLE_COUNT; ++i) {
		vec2 xi = Hammersley(i, DIFFUSE_SAMPLE_COUNT);
		float phi = 2.0 * PI * xi.x;
		float radius = sqrt(xi.y);
		vec3 localDirection = vec3(radius * cos(phi), radius * sin(phi), sqrt(1.0 - xi.y));
		sum += textureLod(sourceEnvironment, normalize(basis * localDirection), 0.0).rgb;
	}
	return PI * sum / float(DIFFUSE_SAMPLE_COUNT);
}

vec3 FilterSpecular(vec3 normal)
{
	if (roughness <= 0.0001)
		return textureLod(sourceEnvironment, normal, 0.0).rgb;

	vec3 viewDirection = normal;
	vec3 sum = vec3(0.0);
	float weight = 0.0;
	float alpha = max(roughness * roughness, 0.001);
	float texelSolidAngle = 4.0 * PI / (6.0 * sourceResolution * sourceResolution);

	for (uint i = 0u; i < SPECULAR_SAMPLE_COUNT; ++i) {
		vec3 halfVector = ImportanceSampleGGX(Hammersley(i, SPECULAR_SAMPLE_COUNT), alpha, normal);
		vec3 lightDirection = normalize(2.0 * dot(viewDirection, halfVector) * halfVector - viewDirection);
		float nDotL = max(dot(normal, lightDirection), 0.0);
		if (nDotL <= 0.0)
			continue;

		float nDotH = max(dot(normal, halfVector), 0.0);
		float hDotV = max(dot(halfVector, viewDirection), 0.0);
		float denominator = nDotH * nDotH * (alpha * alpha - 1.0) + 1.0;
		float distribution = (alpha * alpha) / max(PI * denominator * denominator, 1e-6);
		float pdf = max(distribution * nDotH / max(4.0 * hDotV, 1e-6), 1e-6);
		float sampleSolidAngle = 1.0 / (float(SPECULAR_SAMPLE_COUNT) * pdf);
		float sourceMip = max(0.0, 0.5 * log2(sampleSolidAngle / texelSolidAngle));
		sum += textureLod(sourceEnvironment, lightDirection, sourceMip).rgb * nDotL;
		weight += nDotL;
	}

	return sum / max(weight, 1e-5);
}

void main()
{
	vec3 direction = FaceDirection(faceIndex, texCoord);
	vec3 color = filterType == 1 ? FilterDiffuse(direction) : FilterSpecular(direction);
	fragColor = vec4(max(color, vec3(0.0)), 1.0);
}
)";

	unsigned int MipCount(unsigned int size)
	{
		unsigned int count = 1;
		while (size > 1) {
			size >>= 1;
			++count;
		}
		return count;
	}
}

CubeMapHandler cubeMapHandler;


bool CubeMapHandler::Init() {
	RECOIL_DETAILED_TRACY_ZONE;
	envReflectionTexID = 0;
	skyReflectionTexID = 0;
	specularTexID = 0;

	specTexSize = configHandler->GetInt("CubeTexSizeSpecular");
	reflTexSize = configHandler->GetInt("CubeTexSizeReflection");

	specTexPartBuf.clear();
	specTexPartBuf.resize(specTexSize * 4, 0);
	specTexFaceBuf.clear();
	specTexFaceBuf.resize(specTexSize * specTexSize * 4, 0);

	currReflectionFace = 0;
	specularTexIter = 0;

	mapSkyReflections = (!mapInfo->smf.skyReflectModTexName.empty());
	generateMipMaps = configHandler->GetBool("CubeTexGenerateMipMaps");

	{
		glGenTextures(1, &specularTexID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, specularTexID);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X, specTexSize, float3( 1,  1,  1), float3( 0, 0, -2), float3(0, -2,  0));
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, specTexSize, float3(-1,  1, -1), float3( 0, 0,  2), float3(0, -2,  0));
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, specTexSize, float3(-1,  1, -1), float3( 2, 0,  0), float3(0,  0,  2));
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, specTexSize, float3(-1, -1,  1), float3( 2, 0,  0), float3(0,  0, -2));
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, specTexSize, float3(-1,  1,  1), float3( 2, 0,  0), float3(0, -2,  0));
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, specTexSize, float3( 1,  1, -1), float3(-2, 0,  0), float3(0, -2,  0));
	}

	{
		glGenTextures(1, &envReflectionTexID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envReflectionTexID);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, generateMipMaps? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER,                                           GL_LINEAR); // magnification doesn't use mips
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	}

	if (generateMipMaps)
		glGenerateMipmapEXT(GL_TEXTURE_CUBE_MAP);

	if (mapSkyReflections) {
		glGenTextures(1, &skyReflectionTexID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyReflectionTexID);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	// reflectionCubeFBO is no-op constructed, has to be initialized manually
	reflectionCubeFBO.Init(false);

	if (reflectionCubeFBO.IsValid()) {
		reflectionCubeFBO.Bind();
		reflectionCubeFBO.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, GL_DEPTH_COMPONENT, reflTexSize, reflTexSize);
		reflectionCubeFBO.Unbind();

		pbrCaptureMode = configHandler->GetString("PBREnvironmentCapture");
		pbrEnabled = (pbrCaptureMode == "hdr-modern" || pbrCaptureMode == "compare");
		if (pbrEnabled && !InitPBREnvironment()) {
			LOG_L(L_WARNING, "[CubeMapHandler] PBR environment initialization failed: %s", pbrFailure.c_str());
		}
		return true;
	}

	Free();
	return false;
}

void CubeMapHandler::Free() {
	RECOIL_DETAILED_TRACY_ZONE;
	FreePBREnvironment();

	if (specularTexID != 0) {
		glDeleteTextures(1, &specularTexID);
		specularTexID = 0;
	}
	if (envReflectionTexID != 0) {
		glDeleteTextures(1, &envReflectionTexID);
		envReflectionTexID = 0;
	}
	if (skyReflectionTexID != 0) {
		glDeleteTextures(1, &skyReflectionTexID);
		skyReflectionTexID = 0;
	}

	reflectionCubeFBO.Kill();
}



void CubeMapHandler::UpdateReflectionTexture()
{
	RECOIL_DETAILED_TRACY_ZONE;
	UpdatePBRConfiguration();

	// NOTE:
	//   we unbind later in WorldDrawer::GenerateIBLTextures() to save render
	//   context switches (which are one of the slowest OpenGL operations!)
	//   together with VP restoration
	reflectionCubeFBO.Bind();

	switch (currReflectionFace) {
		case 0: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X, false); } break;
		case 1: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, false); } break;
		case 2: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, false); } break;
		case 3: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, false); } break;
		case 4: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, false); } break;
		case 5: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, false); } break;
		default: {} break;
	}

	if (mapSkyReflections) {
		// draw only the sky (into its own cubemap) for SSMF
		// by reusing data from previous frame we could also
		// make terrain reflect itself, not just the sky
		switch (currReflectionFace) {
			case  6: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X, true); } break;
			case  7: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, true); } break;
			case  8: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, true); } break;
			case  9: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, true); } break;
			case 10: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, true); } break;
			case 11: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, true); } break;
			default: {} break;
		}

		currReflectionFace +=  1;
		currReflectionFace %= 12;
	} else {
		// touch the FBO at least once per frame
		currReflectionFace += 1;
		currReflectionFace %= 6;
	}

	if (generateMipMaps && currReflectionFace == 0) {
		glBindTexture(GL_TEXTURE_CUBE_MAP, envReflectionTexID);
		glGenerateMipmapEXT(GL_TEXTURE_CUBE_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	if (pbrEnabled)
		UpdatePBREnvironment();

	// WorldDrawer deliberately unbinds this FBO after this call. Preserve that
	// legacy contract even though the PBR path uses independent FBOs internally.
	reflectionCubeFBO.Bind();
}

void CubeMapHandler::CreateReflectionFace(unsigned int glFace, bool skyOnly)
{
	RECOIL_DETAILED_TRACY_ZONE;
	reflectionCubeFBO.AttachTexture((skyOnly? skyReflectionTexID: envReflectionTexID), glFace);

	glPushAttrib(GL_FOG_BIT | GL_DEPTH_BUFFER_BIT);
	const auto& sky = ISky::GetSky();
	glClearColor(sky->fogColor.x, sky->fogColor.y, sky->fogColor.z, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	if (!skyOnly) {
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
	} else {
		// do not need depth-testing for the sky alone
		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);
	}

	{
		CCamera* prvCam = CCameraHandler::GetSetActiveCamera(CCamera::CAMTYPE_ENVMAP);
		CCamera* curCam = CCameraHandler::GetActiveCamera();

		const float3* fd = faceDirs[glFace - GL_TEXTURE_CUBE_MAP_POSITIVE_X];

		// env-reflections are only correct when drawn from an inverted
		// perspective (meaning right becomes left and up becomes down)
		curCam->forward  = fd[0];
		curCam->right    = fd[1] * -1.0f;
		curCam->up       = fd[2] * -1.0f;

		// set vertical *and* horizontal FOV to 90 degrees
		curCam->SetVFOV(90.0f);
		curCam->SetAspectRatio(1.0f);
		curCam->SetPos(prvCam->GetPos());

		curCam->UpdateLoadViewport(0, 0, reflTexSize, reflTexSize);
		curCam->UpdateViewRange();
		curCam->UpdateMatrices(globalRendering->viewSizeX, globalRendering->viewSizeY, curCam->GetAspectRatio());
		curCam->UpdateFrustum();
		curCam->LoadMatrices();

		// generate the face
		game->SetDrawMode(CGame::gameReflectionDraw);

		if (!globalRendering->drawDebugCubeMap) {
			sky->Draw();
			if (!skyOnly)
				readMap->GetGroundDrawer()->Draw(DrawPass::TerrainReflection);
		}
		else {
			debugCubeMapTexture.Draw(glFace);
		}

		game->SetDrawMode(CGame::gameNormalDraw);


		CCameraHandler::SetActiveCamera(prvCam->GetCamType());
	}

	glPopAttrib();
}

PBREnvironmentInfo CubeMapHandler::GetPBREnvironmentInfo() const
{
	PBREnvironmentInfo info;
	info.requestedRevision = pbrRequestedRevision;
	info.activeRevision = pbrActiveRevision;
	info.environmentSize = pbrEnvironmentSize;
	info.irradianceSize = pbrIrradianceSize;
	info.specularSize = pbrSpecularSize;
	info.specularMipCount = pbrSpecularMipCount;
	info.enabled = pbrEnabled;
	info.ready = pbrReady;
	info.pending = pbrPending;
	info.captureMode = pbrCaptureMode;
	info.anchor = pbrAnchor;
	info.content = pbrContent;
	info.filter = pbrFilter;
	info.failure = pbrFailure;
	return info;
}

bool CubeMapHandler::AllocatePBRCube(unsigned int& textureID, unsigned int size, unsigned int mipCount)
{
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, mipCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, mipCount - 1);

	for (unsigned int mip = 0; mip < mipCount; ++mip) {
		const unsigned int mipSize = std::max(1u, size >> mip);
		for (unsigned int face = 0; face < 6; ++face) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, mip, GL_RGBA16F, mipSize, mipSize, 0, GL_RGBA, GL_FLOAT, nullptr);
		}
	}

	const GLenum error = glGetError();
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	if (error == GL_NO_ERROR)
		return true;

	glDeleteTextures(1, &textureID);
	textureID = 0;
	return false;
}

bool CubeMapHandler::InitPBREnvironment()
{
	FreePBREnvironment();

	pbrEnvironmentSize = configHandler->GetInt("PBREnvironmentSize");
	pbrIrradianceSize = configHandler->GetInt("PBRIrradianceSize");
	pbrSpecularSize = configHandler->GetInt("PBRSpecularSize");
	pbrSpecularMipCount = MipCount(pbrSpecularSize);
	pbrAnchor = configHandler->GetString("PBREnvironmentCaptureAnchor");
	pbrContent = configHandler->GetString("PBREnvironmentCaptureContent");
	pbrFilter = configHandler->GetString("PBREnvironmentFilter");
	pbrRefreshRevision = configHandler->GetInt("PBREnvironmentRevision");

	if (pbrAnchor != "camera")
		pbrAnchor = "map-center";
	if (pbrContent != "sky-terrain" && pbrContent != "legacy-scene")
		pbrContent = "sky-only";
	if (pbrFilter != "raster") {
		LOG_L(L_WARNING, "[CubeMapHandler] PBREnvironmentFilter=%s is not implemented; using the explicit raster fallback", pbrFilter.c_str());
		pbrFilter = "raster";
	}

	const unsigned int environmentMipCount = MipCount(pbrEnvironmentSize);
	for (unsigned int index = 0; index < 2; ++index) {
		if (!AllocatePBRCube(pbrEnvironmentTexIDs[index], pbrEnvironmentSize, environmentMipCount) ||
			!AllocatePBRCube(pbrIrradianceTexIDs[index], pbrIrradianceSize, 1) ||
			!AllocatePBRCube(pbrSpecularTexIDs[index], pbrSpecularSize, pbrSpecularMipCount)) {
			pbrFailure = "RGBA16F cubemap allocation failed";
			FreePBREnvironment();
			return false;
		}
	}

	pbrCaptureFBO.Init(false);
	pbrFilterFBO.Init(false);
	if (!pbrCaptureFBO.IsValid() || !pbrFilterFBO.IsValid()) {
		pbrFailure = "PBR capture or filter FBO creation failed";
		FreePBREnvironment();
		return false;
	}

	pbrCaptureFBO.Bind();
	pbrCaptureFBO.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, GL_DEPTH_COMPONENT, pbrEnvironmentSize, pbrEnvironmentSize);
	FBO::Unbind();

	pbrFilterShader = new Shader::GLSLProgramObject("[PBR-EnvironmentFilter]");
	pbrFilterShader->AttachShaderObject(new Shader::GLSLShaderObject(GL_VERTEX_SHADER, pbrFilterVertexShader));
	pbrFilterShader->AttachShaderObject(new Shader::GLSLShaderObject(GL_FRAGMENT_SHADER, pbrFilterFragmentShader));
	pbrFilterShader->Link();
	if (!pbrFilterShader->IsValid()) {
		pbrFailure = "PBR raster filter shader compilation or validation failed";
		FreePBREnvironment();
		return false;
	}

	pbrFilterShader->Enable();
	pbrFilterShader->SetUniform("sourceEnvironment", 0);
	pbrFilterShader->Disable();
	if (!pbrFilterShader->Validate()) {
		pbrFailure = "PBR raster filter shader validation failed";
		FreePBREnvironment();
		return false;
	}

	if (GLAD_GL_ARB_seamless_cube_map || GLAD_GL_VERSION_3_2)
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	pbrActiveIndex = 0;
	pbrCaptureIndex = 1;
	pbrCaptureFace = 0;
	pbrRequestedRevision = 1;
	pbrActiveRevision = 0;
	pbrReady = false;
	pbrPending = true;
	pbrFailure.clear();
	return true;
}

void CubeMapHandler::FreePBREnvironment()
{
	for (unsigned int& textureID: pbrEnvironmentTexIDs) {
		if (textureID != 0)
			glDeleteTextures(1, &textureID);
		textureID = 0;
	}
	for (unsigned int& textureID: pbrIrradianceTexIDs) {
		if (textureID != 0)
			glDeleteTextures(1, &textureID);
		textureID = 0;
	}
	for (unsigned int& textureID: pbrSpecularTexIDs) {
		if (textureID != 0)
			glDeleteTextures(1, &textureID);
		textureID = 0;
	}

	pbrCaptureFBO.Kill();
	pbrFilterFBO.Kill();
	delete pbrFilterShader;
	pbrFilterShader = nullptr;
	pbrReady = false;
	pbrPending = false;
}

void CubeMapHandler::UpdatePBRConfiguration()
{
	const std::string captureMode = configHandler->GetString("PBREnvironmentCapture");
	const bool shouldEnable = (captureMode == "hdr-modern" || captureMode == "compare");
	const unsigned int environmentSize = configHandler->GetInt("PBREnvironmentSize");
	const unsigned int irradianceSize = configHandler->GetInt("PBRIrradianceSize");
	const unsigned int specularSize = configHandler->GetInt("PBRSpecularSize");
	const int newRefreshRevision = configHandler->GetInt("PBREnvironmentRevision");

	pbrCaptureMode = captureMode;
	if (!shouldEnable) {
		if (pbrEnabled)
			FreePBREnvironment();
		pbrEnabled = false;
		return;
	}

	const bool wasEnabled = pbrEnabled;
	pbrEnabled = true;
	const bool sizeChanged = (environmentSize != pbrEnvironmentSize || irradianceSize != pbrIrradianceSize || specularSize != pbrSpecularSize);
	if (pbrFilterShader == nullptr || sizeChanged) {
		// A persistent capability or shader failure must not allocate and compile
		// again every frame. A mode re-enable, size change, or explicit revision
		// bump is the retry contract exposed to the viewer and config.
		if (!pbrFailure.empty() && wasEnabled && !sizeChanged && newRefreshRevision == pbrRefreshRevision)
			return;

		if (!InitPBREnvironment())
			LOG_L(L_WARNING, "[CubeMapHandler] PBR environment reinitialization failed: %s", pbrFailure.c_str());
		return;
	}

	std::string anchor = configHandler->GetString("PBREnvironmentCaptureAnchor");
	std::string content = configHandler->GetString("PBREnvironmentCaptureContent");
	std::string filter = configHandler->GetString("PBREnvironmentFilter");
	if (anchor != "camera") anchor = "map-center";
	if (content != "sky-terrain" && content != "legacy-scene") content = "sky-only";
	if (filter != "raster") filter = "raster";

	if (anchor == pbrAnchor && content == pbrContent && filter == pbrFilter && newRefreshRevision == pbrRefreshRevision)
		return;

	pbrAnchor = anchor;
	pbrContent = content;
	pbrFilter = filter;
	pbrRefreshRevision = newRefreshRevision;
	++pbrRequestedRevision;
	pbrCaptureFace = 0;
	pbrPending = true;
	pbrFailure.clear();
}

void CubeMapHandler::UpdatePBREnvironment()
{
	if (!pbrPending || pbrFilterShader == nullptr)
		return;

	pbrCaptureFBO.Bind();
	CreatePBRReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X + pbrCaptureFace);
	if (!pbrFailure.empty())
		return;

	++pbrCaptureFace;
	if (pbrCaptureFace < 6)
		return;

	glBindTexture(GL_TEXTURE_CUBE_MAP, pbrEnvironmentTexIDs[pbrCaptureIndex]);
	glGenerateMipmapEXT(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	if (!FilterPBREnvironment()) {
		pbrPending = false;
		return;
	}

	pbrActiveIndex = pbrCaptureIndex;
	pbrCaptureIndex = 1 - pbrActiveIndex;
	pbrActiveRevision = pbrRequestedRevision;
	pbrCaptureFace = 0;
	pbrReady = true;
	pbrPending = false;
}

void CubeMapHandler::CreatePBRReflectionFace(unsigned int glFace)
{
	pbrCaptureFBO.AttachTexture(pbrEnvironmentTexIDs[pbrCaptureIndex], glFace);
	if (!pbrCaptureFBO.CheckStatus("PBR-ENVIRONMENT-CAPTURE")) {
		pbrFailure = "PBR environment capture FBO is incomplete";
		pbrPending = false;
		return;
	}

	glPushAttrib(GL_FOG_BIT | GL_DEPTH_BUFFER_BIT | GL_VIEWPORT_BIT);
	const auto& sky = ISky::GetSky();
	glViewport(0, 0, pbrEnvironmentSize, pbrEnvironmentSize);
	glClearColor(sky->fogColor.x, sky->fogColor.y, sky->fogColor.z, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	const bool skyOnly = (pbrContent == "sky-only");
	glDepthMask(skyOnly ? GL_FALSE : GL_TRUE);
	if (skyOnly)
		glDisable(GL_DEPTH_TEST);
	else
		glEnable(GL_DEPTH_TEST);

	CCamera* previousCamera = CCameraHandler::GetSetActiveCamera(CCamera::CAMTYPE_ENVMAP);
	CCamera* environmentCamera = CCameraHandler::GetActiveCamera();
	const float3* faceDirection = faceDirs[glFace - GL_TEXTURE_CUBE_MAP_POSITIVE_X];
	environmentCamera->forward = faceDirection[0];
	environmentCamera->right = faceDirection[1] * -1.0f;
	environmentCamera->up = faceDirection[2] * -1.0f;
	environmentCamera->SetVFOV(90.0f);
	environmentCamera->SetAspectRatio(1.0f);

	if (pbrAnchor == "camera") {
		environmentCamera->SetPos(previousCamera->GetPos());
	} else {
		const float mapCenterX = mapDims.mapx * SQUARE_SIZE * 0.5f;
		const float mapCenterZ = mapDims.mapy * SQUARE_SIZE * 0.5f;
		const float mapCenterY = std::max(0.0f, CGround::GetHeightReal(mapCenterX, mapCenterZ, false)) + 128.0f;
		environmentCamera->SetPos(float3(mapCenterX, mapCenterY, mapCenterZ));
	}

	environmentCamera->UpdateLoadViewport(0, 0, pbrEnvironmentSize, pbrEnvironmentSize);
	environmentCamera->UpdateViewRange();
	environmentCamera->UpdateMatrices(globalRendering->viewSizeX, globalRendering->viewSizeY, environmentCamera->GetAspectRatio());
	environmentCamera->UpdateFrustum();
	environmentCamera->LoadMatrices();

	game->SetDrawMode(CGame::gameReflectionDraw);
	if (!globalRendering->drawDebugCubeMap) {
		sky->Draw();
		if (!skyOnly)
			readMap->GetGroundDrawer()->Draw(DrawPass::TerrainReflection);
	} else {
		debugCubeMapTexture.Draw(glFace);
	}
	game->SetDrawMode(CGame::gameNormalDraw);
	CCameraHandler::SetActiveCamera(previousCamera->GetCamType());
	glPopAttrib();
}

bool CubeMapHandler::FilterPBREnvironment()
{
	pbrFilterFBO.Bind();
	glPushAttrib(GL_ENABLE_BIT | GL_VIEWPORT_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, pbrEnvironmentTexIDs[pbrCaptureIndex]);

	pbrFilterShader->Enable();
	pbrFilterShader->SetUniform("sourceResolution", static_cast<float>(pbrEnvironmentSize));
	pbrFilterVAO.Bind();

	auto renderFaceMip = [&](unsigned int destination, unsigned int face, unsigned int mip, unsigned int size, int filterType, float roughness) {
		pbrFilterFBO.AttachTexture(destination, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, GL_COLOR_ATTACHMENT0, mip);
		if (!pbrFilterFBO.CheckStatus("PBR-ENVIRONMENT-FILTER"))
			return false;
		glViewport(0, 0, size, size);
		pbrFilterShader->SetUniform("faceIndex", static_cast<int>(face));
		pbrFilterShader->SetUniform("filterType", filterType);
		pbrFilterShader->SetUniform("roughness", roughness);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		return true;
	};

	bool valid = true;
	for (unsigned int face = 0; face < 6 && valid; ++face)
		valid &= renderFaceMip(pbrIrradianceTexIDs[pbrCaptureIndex], face, 0, pbrIrradianceSize, 1, 1.0f);

	for (unsigned int mip = 0; mip < pbrSpecularMipCount && valid; ++mip) {
		const unsigned int mipSize = std::max(1u, pbrSpecularSize >> mip);
		const float roughness = (pbrSpecularMipCount > 1) ? static_cast<float>(mip) / static_cast<float>(pbrSpecularMipCount - 1) : 0.0f;
		for (unsigned int face = 0; face < 6 && valid; ++face)
			valid &= renderFaceMip(pbrSpecularTexIDs[pbrCaptureIndex], face, mip, mipSize, 2, roughness);
	}

	pbrFilterVAO.Unbind();
	pbrFilterShader->Disable();
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glPopAttrib();
	FBO::Unbind();
	globalRendering->LoadViewport();

	if (!valid)
		pbrFailure = "PBR environment raster filter FBO is incomplete";
	return valid;
}


void CubeMapHandler::UpdateSpecularTexture()
{
	RECOIL_DETAILED_TRACY_ZONE;

	glBindTexture(GL_TEXTURE_CUBE_MAP, specularTexID);

	int specularTexRow = specularTexIter / 3; //FIXME WTF

	switch (specularTexIter % 3) {
		case 0: {
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X, specTexSize, float3( 1,  1,  1), float3( 0, 0, -2), float3(0, -2,  0), specularTexRow, &specTexPartBuf[0]);
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, specTexSize, float3(-1,  1, -1), float3( 0, 0,  2), float3(0, -2,  0), specularTexRow, &specTexPartBuf[0]);
		} break;
		case 1: {
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, specTexSize, float3(-1,  1, -1), float3( 2, 0,  0), float3(0,  0,  2), specularTexRow, &specTexPartBuf[0]);
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, specTexSize, float3(-1, -1,  1), float3( 2, 0,  0), float3(0,  0, -2), specularTexRow, &specTexPartBuf[0]);
		} break;
		case 2: {
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, specTexSize, float3(-1,  1,  1), float3( 2, 0,  0), float3(0, -2,  0), specularTexRow, &specTexPartBuf[0]);
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, specTexSize, float3( 1,  1, -1), float3(-2, 0,  0), float3(0, -2,  0), specularTexRow, &specTexPartBuf[0]);
		} break;
	}

	// update one face of one row per frame
	specularTexIter += 1;
	specularTexIter %= (specTexSize * 3);
}

void CubeMapHandler::CreateSpecularFacePart(
	unsigned int texType,
	unsigned int size,
	const float3& cdir,
	const float3& xdif,
	const float3& ydif,
	unsigned int y,
	unsigned char* buf
) {
	RECOIL_DETAILED_TRACY_ZONE;
	const auto& sky = ISky::GetSky();
	// TODO move to a shader
	for (int x = 0; x < size; ++x) {
		const float3 dir = (cdir + (xdif * (x + 0.5f)) / size + (ydif * (y + 0.5f)) / size).Normalize();
		const float dot  = std::max(0.0f, dir.dot(sky->GetLight()->GetLightDir()));
		const float spec = std::min(1.0f, std::pow(dot, sunLighting->specularExponent) + std::pow(dot, 3.0f) * 0.25f);

		buf[x * 4 + 0] = (sunLighting->modelSpecularColor.x * spec * 255);
		buf[x * 4 + 1] = (sunLighting->modelSpecularColor.y * spec * 255);
		buf[x * 4 + 2] = (sunLighting->modelSpecularColor.z * spec * 255);
		buf[x * 4 + 3] = 255;
	}
}

void CubeMapHandler::CreateSpecularFace(
	unsigned int texType,
	unsigned int size,
	const float3& cdir,
	const float3& xdif,
	const float3& ydif
) {
	RECOIL_DETAILED_TRACY_ZONE;
	for (int y = 0; y < size; ++y) {
		CreateSpecularFacePart(texType, size, cdir, xdif, ydif, y, &specTexFaceBuf[y * size * 4]);
	}

	//! note: no mipmaps, cubemap linear filtering is broken
	glTexImage2D(texType, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, &specTexFaceBuf[0]);
}

void CubeMapHandler::UpdateSpecularFace(
	unsigned int texType,
	unsigned int size,
	const float3& cdir,
	const float3& xdif,
	const float3& ydif,
	unsigned int y,
	unsigned char* buf
) {
	RECOIL_DETAILED_TRACY_ZONE;
	CreateSpecularFacePart(texType, size, cdir, xdif, ydif, y, buf);

	glTexSubImage2D(texType, 0, 0, y, size, 1, GL_RGBA, GL_UNSIGNED_BYTE, buf);
}

