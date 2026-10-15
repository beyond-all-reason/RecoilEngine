/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <string>

#include "Rendering/GL/FBO.h"
#include "Rendering/GL/VAO.h"

namespace Shader {
	struct IProgramObject;
}

/**
 * Snapshot of the PBR environment lifecycle published to Lua through
 * Spring.GetPBREnvironmentInfo().
 *
 * `requested*` fields echo the persisted configuration strings verbatim;
 * the unprefixed fields are the values actually in effect. Whenever they
 * differ (unknown value, unimplemented mode) `fallback` explains why. Sizes
 * describe the published (active) set while `ready` is true, otherwise the
 * set being built. Revisions increase monotonically for the lifetime of the
 * handler, including across disable/re-enable and resize, so a consumer can
 * never mistake an older epoch for a newer one.
 */
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
	std::string requestedCaptureMode;
	std::string requestedAnchor;
	std::string requestedContent;
	std::string requestedFilter;
	std::string transfer;
	std::string fallback;
	std::string failure;
	// GPU time of the last completed epoch measured with timestamp queries,
	// or -1 while unknown/unsupported: sum of the six one-face captures, and
	// the decode + mip + irradiance + GGX filter pass
	float captureGpuMilliseconds = -1.0f;
	float filterGpuMilliseconds = -1.0f;
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
	// The PBR getters expose only the published (active) set; they return 0
	// while no complete epoch exists so that a consumer never samples a
	// partially captured or partially filtered cube.
	unsigned int GetPBREnvironmentTextureID() const { return pbrReady ? pbrSets[pbrActiveIndex].environmentTexID : 0; }
	unsigned int GetPBRIrradianceTextureID() const { return pbrReady ? pbrSets[pbrActiveIndex].irradianceTexID : 0; }
	unsigned int GetPBRSpecularTextureID() const { return pbrReady ? pbrSets[pbrActiveIndex].specularTexID : 0; }
	unsigned int GetPBREnvironmentTextureSize() const { return PublishedOrRequestedSet().environmentSize; }
	unsigned int GetPBRIrradianceTextureSize() const { return PublishedOrRequestedSet().irradianceSize; }
	unsigned int GetPBRSpecularTextureSize() const { return PublishedOrRequestedSet().specularSize; }
	PBREnvironmentInfo GetPBREnvironmentInfo() const;

private:
	/**
	 * One complete PBR environment epoch: the scene-linear source radiance,
	 * its diffuse irradiance, and its GGX-prefiltered radiance. Two sets are
	 * double-buffered; each records its own sizes so that a resize builds the
	 * new set while the previously published set stays bound and valid.
	 */
	struct PBREnvironmentSet {
		unsigned int environmentTexID = 0;
		unsigned int irradianceTexID = 0;
		unsigned int specularTexID = 0;
		unsigned int environmentSize = 0;
		unsigned int irradianceSize = 0;
		unsigned int specularSize = 0;
		unsigned int specularMipCount = 0;
	};

	const PBREnvironmentSet& PublishedOrRequestedSet() const { return pbrReady ? pbrSets[pbrActiveIndex] : pbrRequestedSet; }

	void CreateReflectionFace(unsigned int, bool);
	void CreateSpecularFacePart(unsigned int, unsigned int, const float3&, const float3&, const float3&, unsigned int, unsigned char*);
	void CreateSpecularFace(unsigned int, unsigned int, const float3&, const float3&, const float3&);
	void UpdateSpecularFace(unsigned int, unsigned int, const float3&, const float3&, const float3&, unsigned int, unsigned char*);
	bool InitPBREnvironment();
	void FreePBREnvironment();
	void FreePBRSet(PBREnvironmentSet& set);
	bool EnsurePBRCaptureResources();
	void UpdatePBREnvironment();
	void CreatePBRReflectionFace(unsigned int glFace);
	bool FilterPBREnvironment();
	bool AllocatePBRCube(unsigned int& textureID, unsigned int size, unsigned int mipCount);
	void UpdatePBRConfiguration();
	void RequestPBREpoch();
	void CollectPBRTimers();

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

	std::array<PBREnvironmentSet, 2> pbrSets;
	// sizes requested by the current configuration; the capture set is
	// (re)allocated to match them at the start of every epoch
	PBREnvironmentSet pbrRequestedSet;
	// display-referred legacy capture target; decoded into the capture set's
	// scene-linear environment once all six faces exist
	unsigned int pbrStagingTexID = 0;
	unsigned int pbrStagingSize = 0;
	unsigned int pbrActiveIndex = 0;
	unsigned int pbrCaptureIndex = 1;
	unsigned int pbrCaptureFace = 0;
	unsigned int pbrRequestedRevision = 0;
	unsigned int pbrActiveRevision = 0;
	int pbrRefreshRevision = -1;
	std::string pbrCaptureMode = "legacy-clone";
	std::string pbrAnchor = "map-center";
	std::string pbrContent = "sky-only";
	std::string pbrFilter = "raster";
	std::string pbrPublishedFilter = "raster";
	std::string pbrRequestedCaptureMode;
	std::string pbrRequestedAnchor;
	std::string pbrRequestedContent;
	std::string pbrRequestedFilter;
	std::string pbrFallback;
	std::string pbrFailure;
	// A failed BAR shader is retried only when its request/revision changes.
	std::string pbrFilterAttemptedRequest;
	int pbrFilterAttemptedRevision = -1;
	std::string pbrFilterLoadError;

	// GL_TIMESTAMP queries: [2k, 2k+1] bracket capture face k, [12, 13] the
	// filter pass. Results are read without stalling once all are available.
	static constexpr unsigned int PBR_TIMER_QUERY_COUNT = 14;
	std::array<unsigned int, PBR_TIMER_QUERY_COUNT> pbrTimerQueries = {};
	bool pbrTimersSupported = false;
	bool pbrTimersPending = false;
	float pbrCaptureGpuMilliseconds = -1.0f;
	float pbrFilterGpuMilliseconds = -1.0f;

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
