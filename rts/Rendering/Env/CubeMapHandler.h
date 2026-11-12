/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CUBEMAP_HANDLER_HDR
#define CUBEMAP_HANDLER_HDR

#include "Rendering/GL/FBO.h"

class CubeMapHandler {
public:
	CubeMapHandler(): reflectionCubeFBO(true) {}

	bool Init();
	void Free();

	void UpdateReflectionTexture();
	void UpdateSpecularTexture();

	unsigned int GetEnvReflectionTextureID() const { return envReflectionTexID; }
	unsigned int GetSkyReflectionTextureID() const { return skyReflectionTexID; }
	unsigned int GetSpecularTextureID() const { return specularTexID; }
	unsigned int GetReflectionTextureSize() const { return reflTexSize; }
	unsigned int GetSpecularTextureSize() const { return specTexSize; }

	/**
	 * Camera axes of the six cube faces, in OpenGL face order (POSITIVE_X,
	 * NEGATIVE_X, POSITIVE_Y, NEGATIVE_Y, POSITIVE_Z, NEGATIVE_Z): forward,
	 * right and up per face. The reflection captures here negate right and up
	 * (reflections are drawn mirrored). Public and static so that
	 * gl.RenderEnvironmentFace (LuaOpenGL.cpp) uses this one table: a Lua
	 * cube's faces then line up with the legacy cubes' by construction.
	 */
	static constexpr float3 faceDirs[6][3] = {
		{ RgtVector,  FwdVector,   UpVector}, // fwd = +x, right = +z, up = +y
		{-RgtVector, -FwdVector,   UpVector}, // fwd = -x
		{  UpVector, -RgtVector, -FwdVector}, // fwd = +y
		{ -UpVector, -RgtVector,  FwdVector}, // fwd = -y
		{ FwdVector, -RgtVector,   UpVector}, // fwd = +z
		{-FwdVector,  RgtVector,   UpVector}, // fwd = -z
	};

private:
	void CreateReflectionFace(unsigned int, bool);
	void CreateSpecularFacePart(unsigned int, unsigned int, const float3&, const float3&, const float3&, unsigned int, unsigned char*);
	void CreateSpecularFace(unsigned int, unsigned int, const float3&, const float3&, const float3&);
	void UpdateSpecularFace(unsigned int, unsigned int, const float3&, const float3&, const float3&, unsigned int, unsigned char*);

	unsigned int envReflectionTexID; // sky and map
	unsigned int skyReflectionTexID; // sky only
	unsigned int specularTexID;

	unsigned int reflTexSize;
	unsigned int specTexSize;

	unsigned int currReflectionFace;
	unsigned int specularTexIter;

	bool mapSkyReflections;
	bool generateMipMaps;

	std::vector<unsigned char> specTexPartBuf;
	std::vector<unsigned char> specTexFaceBuf;

	FBO reflectionCubeFBO;
};

extern CubeMapHandler cubeMapHandler;

#endif
