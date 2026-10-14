/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <string>

#include "Rendering/GL/FBO.h"
#include "Rendering/GL/VAO.h"

namespace Shader {
	struct IProgramObject;
}

struct PBREnvironmentInfo {
	unsigned int requestedRevision = 0;
	unsigned int activeRevision = 0;
	unsigned int environmentSize = 0;
	unsigned int irradianceSize = 0;
	unsigned int specularSize = 0;
	unsigned int specularMipCount = 0;
	bool enabled = false;
	bool ready = false;
	bool pending = false;
	std::string captureMode = "legacy-clone";
	std::string anchor = "map-center";
	std::string content = "sky-only";
	std::string filter = "raster";
	std::string failure;
};

class CubeMapHandler {
public:
	CubeMapHandler(): reflectionCubeFBO(true), pbrCaptureFBO(true), pbrFilterFBO(true) {}

	bool Init();
	void Free();

	void UpdateReflectionTexture();
	void UpdateSpecularTexture();

	unsigned int GetEnvReflectionTextureID() const { return envReflectionTexID; }
	unsigned int GetSkyReflectionTextureID() const { return skyReflectionTexID; }
	unsigned int GetSpecularTextureID() const { return specularTexID; }
	unsigned int GetReflectionTextureSize() const { return reflTexSize; }
	unsigned int GetSpecularTextureSize() const { return specTexSize; }
	unsigned int GetPBREnvironmentTextureID() const { return pbrReady ? pbrEnvironmentTexIDs[pbrActiveIndex] : 0; }
	unsigned int GetPBRIrradianceTextureID() const { return pbrReady ? pbrIrradianceTexIDs[pbrActiveIndex] : 0; }
	unsigned int GetPBRSpecularTextureID() const { return pbrReady ? pbrSpecularTexIDs[pbrActiveIndex] : 0; }
	unsigned int GetPBREnvironmentTextureSize() const { return pbrEnvironmentSize; }
	unsigned int GetPBRIrradianceTextureSize() const { return pbrIrradianceSize; }
	unsigned int GetPBRSpecularTextureSize() const { return pbrSpecularSize; }
	PBREnvironmentInfo GetPBREnvironmentInfo() const;

private:
	void CreateReflectionFace(unsigned int, bool);
	void CreateSpecularFacePart(unsigned int, unsigned int, const float3&, const float3&, const float3&, unsigned int, unsigned char*);
	void CreateSpecularFace(unsigned int, unsigned int, const float3&, const float3&, const float3&);
	void UpdateSpecularFace(unsigned int, unsigned int, const float3&, const float3&, const float3&, unsigned int, unsigned char*);
	bool InitPBREnvironment();
	void FreePBREnvironment();
	void UpdatePBREnvironment();
	void CreatePBRReflectionFace(unsigned int glFace);
	bool FilterPBREnvironment();
	bool AllocatePBRCube(unsigned int& textureID, unsigned int size, unsigned int mipCount);
	void UpdatePBRConfiguration();

	unsigned int envReflectionTexID; // sky and map
	unsigned int skyReflectionTexID; // sky only
	unsigned int specularTexID;

	unsigned int reflTexSize;
	unsigned int specTexSize;

	unsigned int currReflectionFace;
	unsigned int specularTexIter;

	bool mapSkyReflections;
	bool generateMipMaps;
	bool pbrEnabled = false;
	bool pbrReady = false;
	bool pbrPending = false;

	std::array<unsigned int, 2> pbrEnvironmentTexIDs = {0, 0};
	std::array<unsigned int, 2> pbrIrradianceTexIDs = {0, 0};
	std::array<unsigned int, 2> pbrSpecularTexIDs = {0, 0};
	unsigned int pbrEnvironmentSize = 0;
	unsigned int pbrIrradianceSize = 0;
	unsigned int pbrSpecularSize = 0;
	unsigned int pbrSpecularMipCount = 0;
	unsigned int pbrActiveIndex = 0;
	unsigned int pbrCaptureIndex = 1;
	unsigned int pbrCaptureFace = 0;
	unsigned int pbrRequestedRevision = 0;
	unsigned int pbrActiveRevision = 0;
	int pbrRefreshRevision = 0;
	std::string pbrCaptureMode = "legacy-clone";
	std::string pbrAnchor = "map-center";
	std::string pbrContent = "sky-only";
	std::string pbrFilter = "raster";
	std::string pbrFailure;

	std::vector<unsigned char> specTexPartBuf;
	std::vector<unsigned char> specTexFaceBuf;

	FBO reflectionCubeFBO;
	FBO pbrCaptureFBO;
	FBO pbrFilterFBO;
	VAO pbrFilterVAO;
	Shader::IProgramObject* pbrFilterShader = nullptr;

	/*
	GL_TEXTURE_CUBE_MAP_POSITIVE_X
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	*/

	const float3 faceDirs[6][3] = {
		{ RgtVector,  FwdVector,   UpVector}, // fwd = +x, right = +z, up = +y
		{-RgtVector, -FwdVector,   UpVector}, // fwd = -x
		{  UpVector, -RgtVector, -FwdVector}, // fwd = +y
		{ -UpVector, -RgtVector,  FwdVector}, // fwd = -y
		{ FwdVector, -RgtVector,   UpVector}, // fwd = +z
		{-FwdVector,  RgtVector,   UpVector}, // fwd = -z
	};
};

extern CubeMapHandler cubeMapHandler;
