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
#include "System/Misc/SpringTime.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// Build-time pruning switches for the three post-capture IBL filters. Define
// any switch to 0 (for example in compiler flags) to omit that implementation.
// All are 1 by default so existing builds retain the runtime A/B selectors.
// Keep at least one enabled; this does NOT disable legacy $reflection capture.
#ifndef PBR_ENABLE_ENGINE_RASTER_FILTER
#define PBR_ENABLE_ENGINE_RASTER_FILTER 1
#endif
#ifndef PBR_ENABLE_BAR_RASTER_FILTER
#define PBR_ENABLE_BAR_RASTER_FILTER 1
#endif
#ifndef PBR_ENABLE_BAR_COMPUTE_FILTER
#define PBR_ENABLE_BAR_COMPUTE_FILTER 1
#endif
#if (PBR_ENABLE_ENGINE_RASTER_FILTER != 0 && PBR_ENABLE_ENGINE_RASTER_FILTER != 1) || \
	(PBR_ENABLE_BAR_RASTER_FILTER != 0 && PBR_ENABLE_BAR_RASTER_FILTER != 1) || \
	(PBR_ENABLE_BAR_COMPUTE_FILTER != 0 && PBR_ENABLE_BAR_COMPUTE_FILTER != 1)
#error PBR filter switches must be 0 or 1
#endif
#if !PBR_ENABLE_ENGINE_RASTER_FILTER && !PBR_ENABLE_BAR_RASTER_FILTER && !PBR_ENABLE_BAR_COMPUTE_FILTER
#error Enable at least one PBR environment filter
#endif

CONFIG(int, CubeTexSizeSpecular).defaultValue(128).minimumValue(1).description("The square resolution of each face of the specular cubemap.");
CONFIG(int, CubeTexSizeReflection).defaultValue(128).minimumValue(1).description("The square resolution of each face of the environment reflection cubemap.");
CONFIG(bool, CubeTexGenerateMipMaps).defaultValue(false).description("Generate mipmaps for the reflection and specular cubemap textures, useful for efficient subsampling and blurring.");
// Which implementation owns the PBR environment. "bar" (default): BAR's LuaRules provider
// (gfx_pbr_ibl_provider.lua) builds it with gl.RenderEnvironmentFace and gl.PublishTexture,
// and this pipeline stays idle. "engine": this C++ pipeline below. Both read the same
// PBREnvironment* keys; exactly one of them runs.
CONFIG(std::string, PBRIBLProvider).defaultValue("bar").description("PBR environment provider: bar (BAR LuaRules provider) or engine (Recoil CubeMapHandler pipeline).");
// Lua reads this declared default too, so it must match BAR's provider
// (DFG_DEFAULT_SOURCE, "file": the precomputed table, loaded instead of generated).
CONFIG(std::string, PBRDFGSource).defaultValue("file").description("PBR split-sum DFG table source: bar-generated (BAR GLSL), engine-generated (this engine, $pbr_dfg) or file (BAR's validated data file). BAR validates and publishes the active one.");
CONFIG(std::string, PBREnvironmentCapture).defaultValue("legacy-clone").description("PBR environment capture mode: legacy-clone, hdr-modern, or compare.");
CONFIG(std::string, PBREnvironmentCaptureAnchor).defaultValue("map-center").description("PBR environment capture anchor: map-center or camera.");
CONFIG(std::string, PBREnvironmentCaptureContent).defaultValue("sky-only").description("PBR environment capture content: sky-only, sky-terrain, or legacy-scene.");
// Lua reads this declared default too, so it must match BAR's provider ("compute").
CONFIG(std::string, PBREnvironmentFilter).defaultValue("compute").description("PBR environment filter request: compute (default, needs OpenGL 4.3), raster, or bar-raster; unavailable build/GPU choices report a fallback.");
// Face-size limits of the three PBR cubes, shared by the CONFIG ranges below
// and the clamp in UpdatePBRConfiguration; BAR's provider uses the same values
// (CUBE_SIZES in gfx_pbr_ibl_provider.lua). The lower limits keep a usable mip
// chain and 4x4 irradiance texels; the upper limits bound the VRAM of two sets.
constexpr int PBR_ENVIRONMENT_SIZE_MIN = 16, PBR_ENVIRONMENT_SIZE_MAX = 2048;
constexpr int PBR_IRRADIANCE_SIZE_MIN = 4, PBR_IRRADIANCE_SIZE_MAX = 256;
constexpr int PBR_SPECULAR_SIZE_MIN = 16, PBR_SPECULAR_SIZE_MAX = 1024;
CONFIG(int, PBREnvironmentSize).defaultValue(256).minimumValue(PBR_ENVIRONMENT_SIZE_MIN).maximumValue(PBR_ENVIRONMENT_SIZE_MAX).description("PBR scene-linear HDR environment cubemap face size.");
CONFIG(int, PBRIrradianceSize).defaultValue(32).minimumValue(PBR_IRRADIANCE_SIZE_MIN).maximumValue(PBR_IRRADIANCE_SIZE_MAX).description("PBR diffuse irradiance cubemap face size.");
CONFIG(int, PBRSpecularSize).defaultValue(128).minimumValue(PBR_SPECULAR_SIZE_MIN).maximumValue(PBR_SPECULAR_SIZE_MAX).description("PBR GGX-prefiltered specular cubemap base face size.");
CONFIG(int, PBREnvironmentRevision).defaultValue(0).minimumValue(0).description("Increment to request a new atomic PBR environment capture/filter epoch.");

namespace {
	// Height of the map-centre capture point above the ground (or above water
	// level 0) in elmos; BAR's provider (CAPTURE_HEIGHT_ELMOS) and its self-test
	// use the same value, so both providers capture from the same point.
	constexpr float PBR_CAPTURE_HEIGHT_ELMOS = 128.0f;
	// Texture unit the filter programs sample their source cube from, and the
	// image unit the compute program writes its destination level to (the
	// shared program ABI: sourceEnvironment = unit 0, outputEnvironment =
	// binding 1).
	constexpr GLenum PBR_SOURCE_TEXTURE_UNIT = GL_TEXTURE0;
	constexpr GLuint PBR_DESTINATION_IMAGE_UNIT = 1;
	// Clip distances the raster filter reports and disables: the first six,
	// the legacy GL_MAX_CLIP_PLANES minimum that every driver offers and that
	// the engine's clipped passes stay within.
	constexpr int PBR_CHECKED_CLIP_DISTANCES = 6;

#if PBR_ENABLE_ENGINE_RASTER_FILTER
	// variant: PBREnvironmentFilter=raster (the engine's own embedded filter)
	constexpr const char* pbrFilterVertexShader = R"(
#version 150 compatibility

out vec2 texCoord;

void main()
{
	// Full-screen triangle with clip-space corners (-1,-1), (3,-1), (-1,3).
	// The visible [-1,1] square corresponds to position in [0,1], so the face
	// coordinate is position itself (scaling it by 0.5 would map every face to
	// one magnified quadrant of the intended directions).
	vec2 position = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
	texCoord = position;
	gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
}
)";

	// One program implements the three post-capture passes, selected by
	// filterType (see PBRFilterType):
	//   0  decode: display-referred legacy capture -> scene-linear radiance
	//      (sRGB EOTF applied exactly once, input clamped to its [0,1] domain)
	//   1  diffuse irradiance E(n) = integral(L(l) * max(n.l, 0) dl); stored
	//      as irradiance, NOT divided by pi (a Lambert consumer multiplies by
	//      albedo / pi)
	//   2  GGX prefilter with the split-sum N = V = R assumption; destination
	//      mip m of M stores perceptual roughness m / (M - 1)
	// Both integrals importance-sample their lobe and read the source mip
	// whose texel solid angle matches each sample's solid angle, so that a
	// fixed sample count does not alias the full-resolution source.
	constexpr const char* pbrFilterFragmentShader = R"(
#version 150 compatibility

const float PI = 3.14159265358979323846;
const uint DIFFUSE_SAMPLE_COUNT = 256u;
const uint SPECULAR_SAMPLE_COUNT = 256u;
// TangentBasis: above this |normal.y| the world up axis is too close to the
// normal for a stable cross product, so the x axis is used instead
const float TANGENT_UP_LIMIT = 0.999;
// guards against a division by a zero pdf or lobe weight
const float PDF_EPSILON = 1e-6;
const float WEIGHT_EPSILON = 1e-5;
// SourceLod: never read a mip coarser than an 8x8 face (log2(8) levels above
// the 1x1 mip)
const float COARSEST_LOD_OFFSET = 3.0;
// FilterSpecular: at or below this roughness mip 0 is a mirror copy; GGX alpha
// is kept above MIN_ALPHA so the distribution stays finite
const float MIRROR_ROUGHNESS = 0.0001;
const float MIN_ALPHA = 0.001;

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
	vec3 up = abs(normal.y) < TANGENT_UP_LIMIT ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
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

// Source mip whose texel solid angle matches the sample's solid angle
// 1 / (N * pdf). No extra +1 bias, and never coarser than an 8x8 face: low-pdf
// (grazing or wide-lobe) samples would otherwise read whole-face averages that
// mix in radiance from outside the lobe, which measured as a 3-5% darkening
// against the CPU reference in the in-game self-test.
float SourceLod(float pdf, uint sampleCount)
{
	float texelSolidAngle = 4.0 * PI / (6.0 * sourceResolution * sourceResolution);
	float sampleSolidAngle = 1.0 / (float(sampleCount) * max(pdf, PDF_EPSILON));
	float maxLod = max(0.0, log2(sourceResolution) - COARSEST_LOD_OFFSET);
	return clamp(0.5 * log2(sampleSolidAngle / texelSolidAngle), 0.0, maxLod);
}

vec3 SrgbToLinear(vec3 encoded)
{
	vec3 c = clamp(encoded, 0.0, 1.0);
	return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), c));
}

vec3 FilterDiffuse(vec3 normal)
{
	vec3 sum = vec3(0.0);
	mat3 basis = TangentBasis(normal);
	for (uint i = 0u; i < DIFFUSE_SAMPLE_COUNT; ++i) {
		vec2 xi = Hammersley(i, DIFFUSE_SAMPLE_COUNT);
		float phi = 2.0 * PI * xi.x;
		float radius = sqrt(xi.y);
		float cosTheta = sqrt(1.0 - xi.y);
		vec3 localDirection = vec3(radius * cos(phi), radius * sin(phi), cosTheta);
		// cosine-weighted pdf = cos(theta) / pi
		float lod = SourceLod(cosTheta / PI, DIFFUSE_SAMPLE_COUNT);
		sum += textureLod(sourceEnvironment, normalize(basis * localDirection), lod).rgb;
	}
	return PI * sum / float(DIFFUSE_SAMPLE_COUNT);
}

vec3 FilterSpecular(vec3 normal)
{
	if (roughness <= MIRROR_ROUGHNESS)
		return textureLod(sourceEnvironment, normal, 0.0).rgb;

	vec3 viewDirection = normal;
	vec3 sum = vec3(0.0);
	float weight = 0.0;
	float alpha = max(roughness * roughness, MIN_ALPHA);

	for (uint i = 0u; i < SPECULAR_SAMPLE_COUNT; ++i) {
		vec3 halfVector = ImportanceSampleGGX(Hammersley(i, SPECULAR_SAMPLE_COUNT), alpha, normal);
		vec3 lightDirection = normalize(2.0 * dot(viewDirection, halfVector) * halfVector - viewDirection);
		float nDotL = max(dot(normal, lightDirection), 0.0);
		if (nDotL <= 0.0)
			continue;

		float nDotH = max(dot(normal, halfVector), 0.0);
		float hDotV = max(dot(halfVector, viewDirection), 0.0);
		float denominator = nDotH * nDotH * (alpha * alpha - 1.0) + 1.0;
		float distribution = (alpha * alpha) / max(PI * denominator * denominator, PDF_EPSILON);
		float pdf = distribution * nDotH / max(4.0 * hDotV, PDF_EPSILON);
		sum += textureLod(sourceEnvironment, lightDirection, SourceLod(pdf, SPECULAR_SAMPLE_COUNT)).rgb * nDotL;
		weight += nDotL;
	}

	return sum / max(weight, WEIGHT_EPSILON);
}

void main()
{
	vec3 direction = FaceDirection(faceIndex, texCoord);
	vec3 color;
	if (filterType == 0)
		color = SrgbToLinear(textureLod(sourceEnvironment, direction, 0.0).rgb);
	else if (filterType == 1)
		color = FilterDiffuse(direction);
	else
		color = FilterSpecular(direction);
	fragColor = vec4(max(color, vec3(0.0)), 1.0);
}
)";
#endif

	// The engine owns capture, pass scheduling and atomic texture publication.
	// BAR may own only the filter program through the existing VFS Lua-shader
	// loader; this fixed path cannot redirect the engine to arbitrary files.
#if PBR_ENABLE_ENGINE_RASTER_FILTER || PBR_ENABLE_BAR_RASTER_FILTER
	Shader::IProgramObject* CreatePBRFilterProgram(bool barOwned)
	{
		Shader::IProgramObject* program = new Shader::GLSLProgramObject(barOwned ? "[PBR-BAR-EnvironmentFilter]" : "[PBR-EnvironmentFilter]");
		if (barOwned) {
#if PBR_ENABLE_BAR_RASTER_FILTER
			// variant: PBREnvironmentFilter=bar-raster (BAR's single-face file)
			if (!program->LoadFromLua("shaders/GLSL/pbrEnvironmentFilter.lua")) {
				delete program;
				return nullptr;
			}
#else
			delete program;
			return nullptr;
#endif
		} else {
#if PBR_ENABLE_ENGINE_RASTER_FILTER
			// variant: PBREnvironmentFilter=raster (the embedded engine filter)
			program->AttachShaderObject(new Shader::GLSLShaderObject(GL_VERTEX_SHADER, pbrFilterVertexShader));
			program->AttachShaderObject(new Shader::GLSLShaderObject(GL_FRAGMENT_SHADER, pbrFilterFragmentShader));
			program->Link();
			if (!program->IsValid()) {
				delete program;
				return nullptr;
			}
#else
			delete program;
			return nullptr;
#endif
		}

		// These uniforms are the engine/BAR ABI. Reject a shader that compiles
		// but cannot run all three passes, before it can replace a valid one.
		for (const char* name: {"sourceEnvironment", "faceIndex", "filterType", "roughness", "sourceResolution"}) {
			if (glGetUniformLocation(program->GetObjID(), name) < 0) {
				LOG_L(L_ERROR, "[CubeMapHandler] PBR filter shader is missing required uniform %s", name);
				delete program;
				return nullptr;
			}
		}
		program->Enable();
		program->SetUniform("sourceEnvironment", 0);
		program->Disable();
		if (!program->Validate()) {
			delete program;
			return nullptr;
		}
		return program;
	}
#endif

#if PBR_ENABLE_BAR_COMPUTE_FILTER
	// variant: PBREnvironmentFilter=compute (BAR's compute file, dispatched here)
	Shader::IProgramObject* CreatePBRComputeProgram()
	{
		if (!GLAD_GL_VERSION_4_3)
			return nullptr;

		Shader::IProgramObject* program = new Shader::GLSLProgramObject("[PBR-BAR-EnvironmentCompute]");
		if (!program->LoadFromLua("shaders/GLSL/pbrEnvironmentCompute.lua")) {
			delete program;
			return nullptr;
		}

		for (const char* name: {"sourceEnvironment", "filterType", "roughness", "sourceResolution"}) {
			if (glGetUniformLocation(program->GetObjID(), name) < 0) {
				LOG_L(L_ERROR, "[CubeMapHandler] PBR compute shader is missing required uniform %s", name);
				delete program;
				return nullptr;
			}
		}
		program->Enable();
		program->SetUniform("sourceEnvironment", 0);
		program->Disable();
		if (!program->Validate()) {
			delete program;
			return nullptr;
		}
		return program;
	}
#endif

	// This factory is the sole filter-program ownership boundary. BAR owns the
	// external GLSL files; Recoil validates their ABI and publishes only a
	// complete result. Disabled backends are absent from this binary.
	Shader::IProgramObject* CreatePBRProgram(const std::string& filter)
	{
#if PBR_ENABLE_ENGINE_RASTER_FILTER
		if (filter == "raster") return CreatePBRFilterProgram(false);
#endif
#if PBR_ENABLE_BAR_RASTER_FILTER
		if (filter == "bar-raster") return CreatePBRFilterProgram(true);
#endif
#if PBR_ENABLE_BAR_COMPUTE_FILTER
		if (filter == "compute") return CreatePBRComputeProgram();
#endif
		return nullptr;
	}

	// Runtime fallback is deterministic even in a one-backend build. Compute
	// alone cannot run below GL 4.3; the caller reports that case as unavailable
	// instead of pretending that an omitted raster backend exists.
	const char* AvailablePBRFilter()
	{
#if PBR_ENABLE_ENGINE_RASTER_FILTER
		return "raster";
#elif PBR_ENABLE_BAR_RASTER_FILTER
		return "bar-raster";
#else
		return GLAD_GL_VERSION_4_3 ? "compute" : nullptr;
#endif
	}

	bool SupportsPBRFilter(const std::string& filter)
	{
#if PBR_ENABLE_ENGINE_RASTER_FILTER
		if (filter == "raster") return true;
#endif
#if PBR_ENABLE_BAR_RASTER_FILTER
		if (filter == "bar-raster") return true;
#endif
#if PBR_ENABLE_BAR_COMPUTE_FILTER
		if (filter == "compute") return GLAD_GL_VERSION_4_3;
#endif
		return false;
	}

	enum PBRFilterType {
		PBR_FILTER_DECODE = 0,
		PBR_FILTER_DIFFUSE = 1,
		PBR_FILTER_SPECULAR = 2,
	};

	// Accepted values of the persisted PBREnvironment* selectors. Anything else
	// is reported as requested-but-unavailable and replaced by the documented
	// safe value; the persisted request itself is never rewritten.
	bool IsKnownCaptureMode(const std::string& v) { return v == "legacy-clone" || v == "hdr-modern" || v == "compare"; }
	bool IsKnownAnchor(const std::string& v) { return v == "map-center" || v == "camera"; }
	bool IsKnownContent(const std::string& v) { return v == "sky-only" || v == "sky-terrain" || v == "legacy-scene"; }

	void AppendFallback(std::string& text, const std::string& reason)
	{
		if (!text.empty())
			text += "; ";
		text += reason;
	}

	unsigned int MipCount(unsigned int size)
	{
		unsigned int count = 1;
		while (size > 1) {
			size >>= 1;
			++count;
		}
		return count;
	}

	// glGetError returns one recorded error per call. The bound ends a drain on a
	// driver that keeps reporting errors (for example after context loss).
	constexpr unsigned int PBR_MAX_DRAINED_GL_ERRORS = 32;

	// Clears errors left by earlier passes, so that the next glGetError reports
	// only the PBR step's own, and logs what it cleared in one line.
	void DrainEarlierGLErrors(const char* step)
	{
		std::string codes;
		unsigned int count = 0;
		for (GLenum error; count < PBR_MAX_DRAINED_GL_ERRORS && (error = glGetError()) != GL_NO_ERROR; ++count) {
			char code[16];
			std::snprintf(code, sizeof(code), "%s0x%x", codes.empty() ? "" : ", ", static_cast<unsigned int>(error));
			codes += code;
		}

		if (count == 0)
			return;

		LOG_L(L_WARNING, "[CubeMapHandler] %s: cleared %u%s GL error(s) left by earlier passes: %s",
			step, count, (count == PBR_MAX_DRAINED_GL_ERRORS ? " (limit reached)" : ""), codes.c_str());
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
		// Legacy $reflection/$map_reflection is explicitly 8-bit UNORM.
		// Changing this allocation alone would not make the captured legacy
		// sky/terrain shaders HDR or change existing consumers' expectations.
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
		// Optional $sky_reflection for SSMF uses the same legacy RGBA8 layout.
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

		// resolves the persisted PBREnvironment* controls and, when enabled,
		// builds the GPU programs and schedules the first capture epoch
		UpdatePBRConfiguration();
		return true;
	}

	Free();
	return false;
}

void CubeMapHandler::Free() {
	RECOIL_DETAILED_TRACY_ZONE;
	FreePBREnvironment();
	// the next Init (a new game in this process) must initialize again; left
	// enabled, UpdatePBRConfiguration would take the missing program for a
	// persistent failure and stay idle until a control changed
	pbrEnabled = false;
	if (pbrDFGTexID != 0) {
		glDeleteTextures(1, &pbrDFGTexID);
		pbrDFGTexID = 0;
	}

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

// variant: PBRIBLProvider=engine (everything from here to the end of
// FilterPBREnvironmentCompute; the legacy reflection and specular cubes below
// and above it serve every provider)
PBREnvironmentInfo CubeMapHandler::GetPBREnvironmentInfo() const
{
	const PBREnvironmentSet& set = PublishedOrRequestedSet();

	PBREnvironmentInfo info;
	info.requestedRevision = pbrRequestedRevision;
	info.activeRevision = pbrActiveRevision;
	info.environmentSize = set.environmentSize;
	info.irradianceSize = set.irradianceSize;
	info.specularSize = set.specularSize;
	info.specularMipCount = set.specularMipCount;
	info.enabled = pbrEnabled;
	info.ready = pbrReady;
	info.pending = pbrPending;
	// One set at a time: with a published set, anchor, content and filter
	// describe it, like the sizes above; the request being built is in the
	// requested* fields. Before the first publication everything describes the
	// request. captureMode is the switch itself, not a property of a set.
	info.captureMode = pbrCaptureMode;
	info.anchor = pbrReady ? pbrPublishedAnchor : pbrAnchor;
	info.content = pbrReady ? pbrPublishedContent : pbrContent;
	info.filter = pbrReady ? pbrPublishedFilter : pbrFilter;
	info.requestedCaptureMode = pbrRequestedCaptureMode;
	info.requestedAnchor = pbrRequestedAnchor;
	info.requestedContent = pbrRequestedContent;
	info.requestedFilter = pbrRequestedFilter;
#if PBR_ENABLE_ENGINE_RASTER_FILTER
	info.compiledFilters += "raster,";
	info.availableFilters += "raster,";
#endif
#if PBR_ENABLE_BAR_RASTER_FILTER
	info.compiledFilters += "bar-raster,";
	info.availableFilters += "bar-raster,";
#endif
#if PBR_ENABLE_BAR_COMPUTE_FILTER
	info.compiledFilters += "compute,";
	if (GLAD_GL_VERSION_4_3)
		info.availableFilters += "compute,";
#endif
	if (!info.compiledFilters.empty()) info.compiledFilters.pop_back();
	if (!info.availableFilters.empty()) info.availableFilters.pop_back();
	// The capture draws BAR's legacy, display-referred sky/terrain shaders;
	// FilterPBREnvironment decodes them with the sRGB EOTF exactly once.
	info.transfer = "srgb-decoded-legacy";
	info.fallback = pbrFallback;
	info.failure = pbrFailure;
	info.captureGpuMilliseconds = pbrCaptureGpuMilliseconds;
	info.filterGpuMilliseconds = pbrFilterGpuMilliseconds;
	return info;
}

void CubeMapHandler::CollectPBRTimers()
{
	if (!pbrTimersPending)
		return;

	for (const unsigned int query: pbrTimerQueries) {
		GLint available = 0;
		glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &available);
		if (!available)
			return;
	}

	std::array<GLuint64, PBR_TIMER_QUERY_COUNT> stamps = {};
	for (unsigned int i = 0; i < PBR_TIMER_QUERY_COUNT; ++i)
		glGetQueryObjectui64v(pbrTimerQueries[i], GL_QUERY_RESULT, &stamps[i]);

	GLuint64 captureNanoseconds = 0;
	for (unsigned int face = 0; face < PBR_FACE_COUNT; ++face)
		captureNanoseconds += stamps[face * 2 + 1] - stamps[face * 2];

	// GL timestamps are nanoseconds; 1e-6 converts to milliseconds
	pbrCaptureGpuMilliseconds = static_cast<float>(captureNanoseconds) * 1e-6f;
	pbrFilterGpuMilliseconds = static_cast<float>(stamps[PBR_FILTER_TIMER_END] - stamps[PBR_FILTER_TIMER_BEGIN]) * 1e-6f;
	pbrTimersPending = false;
}

bool CubeMapHandler::AllocatePBRCube(unsigned int& textureID, unsigned int size, unsigned int mipCount)
{
	// Dedicated PBR cubes (including the display-referred staging target) are
	// float RGBA16F; they are not aliases for the legacy RGBA8 cube names.
	DrainEarlierGLErrors("AllocatePBRCube");
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
		for (unsigned int face = 0; face < PBR_FACE_COUNT; ++face) {
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

void CubeMapHandler::FreePBRSet(PBREnvironmentSet& set)
{
	for (unsigned int* textureID: {&set.environmentTexID, &set.irradianceTexID, &set.specularTexID}) {
		if (*textureID != 0)
			glDeleteTextures(1, textureID);
	}
	set = PBREnvironmentSet();
}

// Creates the size-independent GPU objects (FBOs and the filter program).
// Texture sets are sized per epoch by EnsurePBRCaptureResources().
bool CubeMapHandler::InitPBREnvironment()
{
	FreePBREnvironment();

	pbrCaptureFBO.Init(false);
#if PBR_ENABLE_ENGINE_RASTER_FILTER || PBR_ENABLE_BAR_RASTER_FILTER
	pbrFilterFBO.Init(false);
#endif
	if (!pbrCaptureFBO.IsValid()
#if PBR_ENABLE_ENGINE_RASTER_FILTER || PBR_ENABLE_BAR_RASTER_FILTER
		|| !pbrFilterFBO.IsValid()
#endif
	) {
		pbrFailure = "PBR capture or filter FBO creation failed";
		FreePBREnvironment();
		return false;
	}

	// Start with the first compiled, GPU-supported backend. With all switches
	// enabled this is the independent engine raster oracle. In a pruned build
	// it is the sole (or first remaining) backend, never an omitted program.
	const char* initialFilter = AvailablePBRFilter();
	pbrFilterShader = initialFilter != nullptr ? CreatePBRProgram(initialFilter) : nullptr;
	if (pbrFilterShader == nullptr) {
		pbrFailure = "PBR initial filter shader unavailable or invalid";
		FreePBREnvironment();
		return false;
	}
	pbrFilter = initialFilter;

	// timestamp queries are optional diagnostics; their absence never blocks
	// the environment
	pbrTimersSupported = (GLAD_GL_ARB_timer_query || GLAD_GL_VERSION_3_3);
	if (pbrTimersSupported)
		glGenQueries(PBR_TIMER_QUERY_COUNT, pbrTimerQueries.data());
	pbrTimersPending = false;

	// Seamless cube-map filtering is enabled globally by CGlobalRendering
	// when supported; the filter relies on it at face edges.
	pbrActiveIndex = 0;
	pbrCaptureIndex = 1;
	pbrCaptureFace = 0;
	pbrReady = false;
	pbrPending = false;
	pbrFailure.clear();
	return true;
}

void CubeMapHandler::FreePBREnvironment()
{
	FreePBRSet(pbrSets[0]);
	FreePBRSet(pbrSets[1]);
	if (pbrStagingTexID != 0)
		glDeleteTextures(1, &pbrStagingTexID);
	pbrStagingTexID = 0;
	pbrStagingSize = 0;

	if (pbrTimerQueries[0] != 0)
		glDeleteQueries(PBR_TIMER_QUERY_COUNT, pbrTimerQueries.data());
	pbrTimerQueries = {};
	pbrTimersPending = false;

	pbrCaptureFBO.Kill();
	pbrFilterFBO.Kill();
	// The empty vertex array of the raster filter (its bufferless triangle):
	// deleted here while the GL context exists; left to its destructor it would
	// run at process exit, after the context is gone. VAO::Bind generates a new
	// one on the next filter pass.
	pbrFilterVAO.Delete();
	delete pbrFilterShader;
	pbrFilterShader = nullptr;
	pbrReady = false;
	pbrPending = false;
	pbrCaptureFace = 0;
}

// (Re)allocates the capture set and the staging cube when the requested sizes
// differ from what they hold. Only the capture set is touched: the published
// set stays bound and valid until the new epoch is swapped in, which makes a
// live resize transactional.
bool CubeMapHandler::EnsurePBRCaptureResources()
{
	PBREnvironmentSet& set = pbrSets[pbrCaptureIndex];
	const PBREnvironmentSet& wanted = pbrRequestedSet;

	const bool setMatches =
		set.environmentTexID != 0 &&
		set.environmentSize == wanted.environmentSize &&
		set.irradianceSize == wanted.irradianceSize &&
		set.specularSize == wanted.specularSize;

	if (!setMatches) {
		FreePBRSet(set);
		const bool allocated =
			AllocatePBRCube(set.environmentTexID, wanted.environmentSize, MipCount(wanted.environmentSize)) &&
			AllocatePBRCube(set.irradianceTexID, wanted.irradianceSize, 1) &&
			AllocatePBRCube(set.specularTexID, wanted.specularSize, wanted.specularMipCount);
		if (!allocated) {
			FreePBRSet(set);
			pbrFailure = "RGBA16F cubemap allocation failed";
			return false;
		}
		set.environmentSize = wanted.environmentSize;
		set.irradianceSize = wanted.irradianceSize;
		set.specularSize = wanted.specularSize;
		set.specularMipCount = wanted.specularMipCount;
	}

	if (pbrStagingTexID == 0 || pbrStagingSize != wanted.environmentSize) {
		if (pbrStagingTexID != 0)
			glDeleteTextures(1, &pbrStagingTexID);
		pbrStagingTexID = 0;
		pbrStagingSize = 0;
		if (!AllocatePBRCube(pbrStagingTexID, wanted.environmentSize, 1)) {
			pbrFailure = "RGBA16F capture staging allocation failed";
			return false;
		}
		pbrStagingSize = wanted.environmentSize;

		// the capture depth buffer must match the staging face resolution
		pbrCaptureFBO.Bind();
		pbrCaptureFBO.Detach(GL_DEPTH_ATTACHMENT_EXT);
		pbrCaptureFBO.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, GL_DEPTH_COMPONENT, pbrStagingSize, pbrStagingSize);
		FBO::Unbind();
	}

	return true;
}

void CubeMapHandler::RequestPBREpoch()
{
	// Revisions are never reset (not on disable/re-enable, not on resize):
	// a waiting consumer compares requested and active revisions, so a reset
	// would let an older epoch look current.
	++pbrRequestedRevision;
	pbrCaptureFace = 0;
	pbrPending = true;
	pbrFailure.clear();
}

// Builds the split-sum DFG table on the CPU in double precision, the C++ variant of
// PBRDFGSource. Convention BAR_PBR_V1_GLTF_GGX_SMITHCORR_SCHLICK_LAMBERT_FMIX_DFG_SCALE_BIAS,
// identical to BAR's GLSL generator and modelmaterials_gl4/validation/generate_pbr_dfg.py:
// texel centres x = NdotV, y = perceptual roughness r (row 0 first), alpha = r^2; GGX
// importance sampling with Hammersley points (i / N, radical inverse of i); height-
// correlated Smith visibility; Schlick Fc = (1 - VdotH)^5; R = A (scale on F0), G = B (bias).
// variant: PBRDFGSource=engine-generated
void CubeMapHandler::GeneratePBRDFG()
{
	const spring_time start = spring_gettime();
	constexpr unsigned int size = PBR_DFG_SIZE;
	constexpr unsigned int samples = PBR_DFG_SAMPLES;
	std::vector<float> texels(size * size * 2);

	const auto radicalInverse = [](uint32_t bits) {
		bits = (bits << 16u) | (bits >> 16u);
		bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
		bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
		bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
		bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		return static_cast<double>(bits) * 2.3283064365386963e-10;
	};

	for (unsigned int j = 0; j < size; ++j) {
		const double roughness = (j + 0.5) / size;
		const double alpha = roughness * roughness;
		const double a2 = alpha * alpha;
		for (unsigned int i = 0; i < size; ++i) {
			const double nDotV = (i + 0.5) / size;
			const double vx = std::sqrt(1.0 - nDotV * nDotV), vz = nDotV;
			double a = 0.0, b = 0.0;
			for (unsigned int s = 0; s < samples; ++s) {
				const double phi = 2.0 * 3.14159265358979323846 * (static_cast<double>(s) / samples);
				const double xiY = radicalInverse(s);
				const double cosH = std::sqrt((1.0 - xiY) / (1.0 + (a2 - 1.0) * xiY));
				const double sinH = std::sqrt(1.0 - cosH * cosH);
				const double hx = sinH * std::cos(phi), hz = cosH;
				const double vDotH = vx * hx + vz * hz;
				const double nDotL = 2.0 * vDotH * hz - vz;
				if (nDotL <= 0.0)
					continue;
				// literals are single precision in this build (-fsingle-precision-constant), hence the casts
				const double vh = std::min(std::max(vDotH, static_cast<double>(0)), static_cast<double>(1));
				const double ggxV = nDotL * std::sqrt(nDotV * nDotV * (1.0 - a2) + a2);
				const double ggxL = nDotV * std::sqrt(nDotL * nDotL * (1.0 - a2) + a2);
				const double gv = (0.5 / (ggxV + ggxL)) * 4.0 * vh * nDotL / hz;
				const double fc = std::pow(1.0 - vh, 5.0);
				a += (1.0 - fc) * gv;
				b += fc * gv;
			}
			texels[(j * size + i) * 2 + 0] = static_cast<float>(a / samples);
			texels[(j * size + i) * 2 + 1] = static_cast<float>(b / samples);
		}
	}

	glGenTextures(1, &pbrDFGTexID);
	glBindTexture(GL_TEXTURE_2D, pbrDFGTexID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, size, size, 0, GL_RG, GL_FLOAT, texels.data());
	glBindTexture(GL_TEXTURE_2D, 0);
	LOG("[CubeMapHandler] engine-generated PBR DFG table %ux%u, %u samples, built on the CPU in %.1f ms",
		size, size, samples, (spring_gettime() - start).toMilliSecsf());
}

// PBRDFGSource is independent of PBRIBLProvider: the engine builds its table only
// while engine-generated is requested and releases it otherwise.
void CubeMapHandler::UpdatePBRDFG()
{
	const bool wanted = (configHandler->GetString("PBRDFGSource") == "engine-generated");
	if (wanted && pbrDFGTexID == 0) {
		GeneratePBRDFG();
	} else if (!wanted && pbrDFGTexID != 0) {
		glDeleteTextures(1, &pbrDFGTexID);
		pbrDFGTexID = 0;
	}
}

void CubeMapHandler::UpdatePBRConfiguration()
{
	UpdatePBRDFG();
	pbrRequestedCaptureMode = configHandler->GetString("PBREnvironmentCapture");
	pbrRequestedAnchor = configHandler->GetString("PBREnvironmentCaptureAnchor");
	pbrRequestedContent = configHandler->GetString("PBREnvironmentCaptureContent");
	const std::string previousRequestedFilter = pbrRequestedFilter;
	pbrRequestedFilter = configHandler->GetString("PBREnvironmentFilter");
	const int refreshRevision = configHandler->GetInt("PBREnvironmentRevision");

	// Resolve every request to an implemented value. The persisted request is
	// left untouched; `fallback` records each substitution for Lua and the log.
	std::string fallback;
	std::string captureMode = pbrRequestedCaptureMode;
	std::string anchor = pbrRequestedAnchor;
	std::string content = pbrRequestedContent;
	std::string filter = pbrRequestedFilter;

	if (!IsKnownCaptureMode(captureMode)) {
		AppendFallback(fallback, "PBREnvironmentCapture=" + captureMode + " is unknown; legacy-clone is active");
		captureMode = "legacy-clone";
	}
	if (!IsKnownAnchor(anchor)) {
		AppendFallback(fallback, "PBREnvironmentCaptureAnchor=" + anchor + " is unknown; map-center is active");
		anchor = "map-center";
	}
	if (content == "compare") {
		AppendFallback(fallback, "PBREnvironmentCaptureContent=compare is not implemented; sky-only is active");
		content = "sky-only";
	} else if (!IsKnownContent(content)) {
		AppendFallback(fallback, "PBREnvironmentCaptureContent=" + content + " is unknown; sky-only is active");
		content = "sky-only";
	}
	const char* availableFilter = AvailablePBRFilter();
	if (availableFilter == nullptr) {
		AppendFallback(fallback, "no compiled PBR filter is supported by this GPU; PBR capture is disabled");
		filter = "unavailable";
	} else if (!SupportsPBRFilter(filter)) {
		const std::string reason = (filter == "compute" && !GLAD_GL_VERSION_4_3)
			? " requires OpenGL 4.3" : " is unknown or omitted from this build";
		AppendFallback(fallback, "PBREnvironmentFilter=" + filter + reason + "; " + availableFilter + " is active");
		filter = availableFilter;
	}

	// the CONFIG ranges document these limits; clamp here as well so that a
	// hand-edited value can never request an invalid allocation
	PBREnvironmentSet requestedSet;
	requestedSet.environmentSize = std::clamp(configHandler->GetInt("PBREnvironmentSize"), PBR_ENVIRONMENT_SIZE_MIN, PBR_ENVIRONMENT_SIZE_MAX);
	requestedSet.irradianceSize = std::clamp(configHandler->GetInt("PBRIrradianceSize"), PBR_IRRADIANCE_SIZE_MIN, PBR_IRRADIANCE_SIZE_MAX);
	requestedSet.specularSize = std::clamp(configHandler->GetInt("PBRSpecularSize"), PBR_SPECULAR_SIZE_MIN, PBR_SPECULAR_SIZE_MAX);
	requestedSet.specularMipCount = MipCount(requestedSet.specularSize);

	const bool sizesChanged =
		requestedSet.environmentSize != pbrRequestedSet.environmentSize ||
		requestedSet.irradianceSize != pbrRequestedSet.irradianceSize ||
		requestedSet.specularSize != pbrRequestedSet.specularSize;
	const bool wasEnabled = pbrEnabled;
	const bool shaderWasMissing = (pbrFilterShader == nullptr);
	const std::string previousFilter = pbrFilter;
	const bool otherControlsChanged =
		anchor != pbrAnchor || content != pbrContent ||
		refreshRevision != pbrRefreshRevision || sizesChanged;
	pbrCaptureMode = captureMode;
	pbrAnchor = anchor;
	pbrContent = content;
	pbrRequestedSet = requestedSet;
	pbrRefreshRevision = refreshRevision;

	// Another provider owns the environment: stay idle and release the engine's PBR resources.
	const bool engineProvider = (configHandler->GetString("PBRIBLProvider") == "engine");

	if (!engineProvider || (captureMode != "hdr-modern" && captureMode != "compare") || availableFilter == nullptr) {
		if (pbrEnabled)
			FreePBREnvironment();
		pbrEnabled = false;
		pbrFilter = filter;
		pbrFallback = fallback;
		return;
	}

	pbrEnabled = true;
	// A persistent program/FBO failure is retried only after the user changes
	// a control (including a PBREnvironmentRevision bump), never every frame.
	if (shaderWasMissing && wasEnabled && !otherControlsChanged && pbrRequestedFilter == previousRequestedFilter)
		return;
	if (shaderWasMissing && !InitPBREnvironment()) {
		LOG_L(L_WARNING, "[CubeMapHandler] PBR environment initialization failed: %s", pbrFailure.c_str());
		return;
	}
	if (shaderWasMissing) {
		// Init already loaded the first available program. Only compile another
		// one below if the requested backend differs from that initial choice.
		pbrFilterAttemptedRequest = pbrRequestedFilter;
		pbrFilterAttemptedRevision = refreshRevision;
		pbrFilterLoadError.clear();
	}

	// A new program is compiled before the old one is retired. A missing or
	// invalid BAR shader cannot discard published textures or poison the
	// engine-owned raster backend. A revision bump retries/reloads BAR edits.
	const bool newFilterRequest =
		shaderWasMissing ||
		pbrFilterAttemptedRequest != pbrRequestedFilter ||
		pbrFilterAttemptedRevision != refreshRevision;
	if (filter == "raster" && pbrFilter == "raster" && newFilterRequest) {
		pbrFilterAttemptedRequest = pbrRequestedFilter;
		pbrFilterAttemptedRevision = refreshRevision;
		pbrFilterLoadError.clear();
	}
	const bool needProgram = filter != pbrFilter || ((filter == "bar-raster" || filter == "compute") && newFilterRequest);
	if (needProgram && (newFilterRequest || pbrFilterLoadError.empty())) {
		Shader::IProgramObject* replacement = CreatePBRProgram(filter);
		pbrFilterAttemptedRequest = pbrRequestedFilter;
		pbrFilterAttemptedRevision = refreshRevision;
		if (replacement != nullptr) {
			delete pbrFilterShader;
			pbrFilterShader = replacement;
			pbrFilter = filter;
			pbrFilterLoadError.clear();
		} else {
			pbrFilterLoadError = "PBREnvironmentFilter=" + pbrRequestedFilter +
				" could not load or validate its shader; " + pbrFilter + " remains active";
		}
	}
	if (!pbrFilterLoadError.empty() && filter != pbrFilter)
		AppendFallback(fallback, pbrFilterLoadError);
	else if (!pbrFilterLoadError.empty() && (filter == "bar-raster" || filter == "compute"))
		AppendFallback(fallback, pbrFilterLoadError);
	if (fallback != pbrFallback) {
		if (!fallback.empty())
			LOG_L(L_WARNING, "[CubeMapHandler] PBR environment fallback: %s", fallback.c_str());
		pbrFallback = fallback;
	}

	const bool controlsChanged = otherControlsChanged || previousFilter != pbrFilter;
	if (wasEnabled && !shaderWasMissing && !controlsChanged)
		return;
	RequestPBREpoch();
}

void CubeMapHandler::UpdatePBREnvironment()
{
	CollectPBRTimers();

	if (!pbrPending || pbrFilterShader == nullptr)
		return;

	if (pbrCaptureFace == 0) {
		// results of an unread earlier epoch are abandoned, never mixed
		pbrTimersPending = false;
		if (!EnsurePBRCaptureResources()) {
			pbrPending = false;
			LOG_L(L_WARNING, "[CubeMapHandler] PBR environment epoch %u rejected: %s", pbrRequestedRevision, pbrFailure.c_str());
			return;
		}
	}

	const bool timed = pbrTimersSupported && pbrTimerQueries[0] != 0;
	if (timed)
		glQueryCounter(pbrTimerQueries[pbrCaptureFace * 2], GL_TIMESTAMP);
	pbrCaptureFBO.Bind();
	CreatePBRReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X + pbrCaptureFace);
	if (timed)
		glQueryCounter(pbrTimerQueries[pbrCaptureFace * 2 + 1], GL_TIMESTAMP);
	if (!pbrFailure.empty())
		return;

	if (++pbrCaptureFace < PBR_FACE_COUNT)
		return;

	if (timed)
		glQueryCounter(pbrTimerQueries[PBR_FILTER_TIMER_BEGIN], GL_TIMESTAMP);
	const bool filtered = FilterPBREnvironment();
	if (timed) {
		glQueryCounter(pbrTimerQueries[PBR_FILTER_TIMER_END], GL_TIMESTAMP);
		pbrTimersPending = filtered;
	}

	if (!filtered) {
		pbrPending = false;
		LOG_L(L_WARNING, "[CubeMapHandler] PBR environment epoch %u rejected: %s", pbrRequestedRevision, pbrFailure.c_str());
		return;
	}

	// atomic publication: all three resources of the epoch switch together
	pbrActiveIndex = pbrCaptureIndex;
	pbrCaptureIndex = 1 - pbrActiveIndex;
	pbrActiveRevision = pbrRequestedRevision;
	pbrPublishedAnchor = pbrAnchor;
	pbrPublishedContent = pbrContent;
	pbrPublishedFilter = pbrFilter;
	pbrCaptureFace = 0;
	pbrReady = true;
	pbrPending = false;
}

void CubeMapHandler::CreatePBRReflectionFace(unsigned int glFace)
{
	pbrCaptureFBO.AttachTexture(pbrStagingTexID, glFace);
	if (!pbrCaptureFBO.CheckStatus("PBR-ENVIRONMENT-CAPTURE")) {
		pbrFailure = "PBR environment capture FBO is incomplete";
		pbrPending = false;
		return;
	}

	glPushAttrib(GL_FOG_BIT | GL_DEPTH_BUFFER_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT | GL_SCISSOR_BIT);
	// an inherited scissor rectangle would leave stale texels in the face
	glDisable(GL_SCISSOR_TEST);
	const auto& sky = ISky::GetSky();
	glViewport(0, 0, pbrStagingSize, pbrStagingSize);
	glClearColor(sky->fogColor.x, sky->fogColor.y, sky->fogColor.z, 1.0f);
	// glClear honours the depth write mask: with depth writes left off by an
	// earlier pass the depth buffer would keep the previous face, and terrain
	// would test against it. Enable writes for the clear (the GL_DEPTH_BUFFER_BIT
	// push restores the caller's mask); gl.RenderEnvironmentFace does the same.
	glDepthMask(GL_TRUE);
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
		const float mapCenterY = std::max(0.0f, CGround::GetHeightReal(mapCenterX, mapCenterZ, false)) + PBR_CAPTURE_HEIGHT_ELMOS;
		environmentCamera->SetPos(float3(mapCenterX, mapCenterY, mapCenterZ));
	}

	environmentCamera->UpdateLoadViewport(0, 0, pbrStagingSize, pbrStagingSize);
	environmentCamera->UpdateViewRange();
	environmentCamera->UpdateMatrices(globalRendering->viewSizeX, globalRendering->viewSizeY, environmentCamera->GetAspectRatio());
	environmentCamera->UpdateFrustum();
	environmentCamera->LoadMatrices();

	game->SetDrawMode(CGame::gameReflectionDraw);
	if (!globalRendering->drawDebugCubeMap) {
		sky->Draw();
		// NOTE: the ROAM ground mesh reuses the player camera's tessellation
		// for every non-shadow pass, so patches the player camera never
		// tessellated draw nothing here; sky-terrain content is therefore
		// incomplete and depends on the player camera (see status docs).
		if (!skyOnly)
			readMap->GetGroundDrawer()->Draw(DrawPass::TerrainReflection);
	} else {
		debugCubeMapTexture.Draw(glFace);
	}
	game->SetDrawMode(CGame::gameNormalDraw);
	CCameraHandler::SetActiveCamera(previousCamera->GetCamType());
	glPopAttrib();
}

// Reports (once per distinct combination) GL state inherited from earlier
// passes that could drop or clip the full-screen filter triangles. The filter
// overrides all of it; the log makes a leaking pass visible.
static void ReportInheritedFilterState()
{
	static std::string lastReport;
	std::string report;

	const auto Check = [&](GLenum cap, const char* name) {
		if (glIsEnabled(cap))
			report += std::string(" ") + name;
	};
	Check(GL_SCISSOR_TEST, "scissor");
	Check(GL_CULL_FACE, "cull-face");
	Check(GL_STENCIL_TEST, "stencil");
	Check(GL_ALPHA_TEST, "alpha-test");
	Check(GL_BLEND, "blend");
	Check(GL_DEPTH_TEST, "depth-test");
	Check(GL_RASTERIZER_DISCARD, "rasterizer-discard");
	for (int i = 0; i < PBR_CHECKED_CLIP_DISTANCES; ++i) {
		if (glIsEnabled(GL_CLIP_DISTANCE0 + i))
			report += " clip-distance" + std::to_string(i);
	}

	GLint frontFace = GL_CCW;
	glGetIntegerv(GL_FRONT_FACE, &frontFace);
	if (frontFace != GL_CCW)
		report += " front-face=cw";

	GLboolean colorMask[4] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
	glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
	if (!colorMask[0] || !colorMask[1] || !colorMask[2] || !colorMask[3])
		report += " color-mask";

	if (report != lastReport && !report.empty())
		LOG_L(L_WARNING, "[CubeMapHandler] PBR filter overrides inherited GL state:%s", report.c_str());
	lastReport = report;
}

// variant: PBREnvironmentFilter=raster and =bar-raster (compute is dispatched to
// FilterPBREnvironmentCompute at the top)
bool CubeMapHandler::FilterPBREnvironment()
{
#if PBR_ENABLE_BAR_COMPUTE_FILTER
	if (pbrFilter == "compute")
		return FilterPBREnvironmentCompute();
#endif

#if PBR_ENABLE_ENGINE_RASTER_FILTER || PBR_ENABLE_BAR_RASTER_FILTER

	PBREnvironmentSet& set = pbrSets[pbrCaptureIndex];

	pbrFilterFBO.Bind();
	ReportInheritedFilterState();

	glPushAttrib(GL_ENABLE_BIT | GL_VIEWPORT_BIT | GL_COLOR_BUFFER_BIT | GL_SCISSOR_BIT | GL_POLYGON_BIT | GL_STENCIL_BUFFER_BIT);
	const GLboolean rasterizerDiscard = glIsEnabled(GL_RASTERIZER_DISCARD);
	glDisable(GL_RASTERIZER_DISCARD);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_ALPHA_TEST);
	for (int i = 0; i < PBR_CHECKED_CLIP_DISTANCES; ++i)
		glDisable(GL_CLIP_DISTANCE0 + i);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	pbrFilterShader->Enable();
	pbrFilterVAO.Bind();

	// One pass into one destination level: the six faces of that level, each
	// attached as a single 2D face (the embedded program has no geometry stage
	// to route a draw to a layer). The framebuffer's completeness depends only
	// on the attachment's format and size, which are the same for every face
	// of one level, so it is checked once, after the first face is attached
	// (it used to be checked before every draw: 60 checks per epoch at the
	// default sizes, walkthrough finding 3). The viewport and the pass uniforms
	// are also set once per level; only faceIndex changes per draw.
	auto renderLevel = [&](unsigned int source, unsigned int sourceSize, unsigned int destination, unsigned int mip, unsigned int size, int filterType, float roughness) {
		glViewport(0, 0, size, size);
		// bound before the level's draws so that no intervening call can leave
		// a different cube map on unit 0
		glActiveTexture(PBR_SOURCE_TEXTURE_UNIT);
		glBindTexture(GL_TEXTURE_CUBE_MAP, source);
		pbrFilterShader->SetUniform("sourceResolution", static_cast<float>(sourceSize));
		pbrFilterShader->SetUniform("filterType", filterType);
		pbrFilterShader->SetUniform("roughness", roughness);
		for (unsigned int face = 0; face < PBR_FACE_COUNT; ++face) {
			pbrFilterFBO.AttachTexture(destination, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, GL_COLOR_ATTACHMENT0, mip);
			if (face == 0 && !pbrFilterFBO.CheckStatus("PBR-ENVIRONMENT-FILTER"))
				return false;
			pbrFilterShader->SetUniform("faceIndex", static_cast<int>(face));
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}
		return true;
	};

	bool valid = true;

	// 1. display-referred staging capture -> scene-linear environment mip 0
	valid &= renderLevel(pbrStagingTexID, pbrStagingSize, set.environmentTexID, 0, set.environmentSize, PBR_FILTER_DECODE, 0.0f);

	// 2. box mips of the linear source: an integration aid for the filters
	//    below (solid-angle source-LOD selection), never published as GGX
	if (valid) {
		pbrFilterFBO.Detach(GL_COLOR_ATTACHMENT0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, set.environmentTexID);
		glGenerateMipmapEXT(GL_TEXTURE_CUBE_MAP);
	}

	// 3. diffuse irradiance, 4. GGX prefiltered radiance per roughness mip
	valid = valid && renderLevel(set.environmentTexID, set.environmentSize, set.irradianceTexID, 0, set.irradianceSize, PBR_FILTER_DIFFUSE, 1.0f);

	for (unsigned int mip = 0; mip < set.specularMipCount && valid; ++mip) {
		const unsigned int mipSize = std::max(1u, set.specularSize >> mip);
		const float roughness = (set.specularMipCount > 1) ? static_cast<float>(mip) / static_cast<float>(set.specularMipCount - 1) : 0.0f;
		valid = renderLevel(set.environmentTexID, set.environmentSize, set.specularTexID, mip, mipSize, PBR_FILTER_SPECULAR, roughness);
	}

	pbrFilterFBO.Detach(GL_COLOR_ATTACHMENT0);
	pbrFilterVAO.Unbind();
	pbrFilterShader->Disable();
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	if (rasterizerDiscard)
		glEnable(GL_RASTERIZER_DISCARD);
	glPopAttrib();
	FBO::Unbind();
	globalRendering->LoadViewport();

	if (!valid)
		pbrFailure = "PBR environment raster filter FBO is incomplete";
	return valid;
#else
	return false; // Compute-only build: there is no raster pass to dispatch.
#endif
}

#if PBR_ENABLE_BAR_COMPUTE_FILTER
// variant: PBRIBLProvider=engine, PBREnvironmentFilter=compute
bool CubeMapHandler::FilterPBREnvironmentCompute()
{
	PBREnvironmentSet& set = pbrSets[pbrCaptureIndex];
	// The capture FBO is no longer needed; image stores target the inactive set.
	FBO::Unbind();
	DrainEarlierGLErrors("FilterPBREnvironmentCompute");
	// BAR owns the shader's local_size declaration. Query the linked program
	// instead of fixing 8x8x1 in Recoil, so BAR can tune group dimensions while
	// the engine still bounds dispatch and owns publication.
	GLint groupSize[3] = {};
	glGetProgramiv(pbrFilterShader->GetObjID(), GL_COMPUTE_WORK_GROUP_SIZE, groupSize);
	if (groupSize[0] <= 0 || groupSize[1] <= 0 || groupSize[2] <= 0 || glGetError() != GL_NO_ERROR) {
		pbrFailure = "PBR compute shader has no valid work-group size";
		return false;
	}
	pbrFilterShader->Enable();
	glActiveTexture(PBR_SOURCE_TEXTURE_UNIT);

	// Memory barriers only where the data flow needs them (variant:
	// PBREnvironmentFilter=compute). The decode writes environment level 0
	// with image stores, which glGenerateMipmap then reads; the irradiance and
	// specular passes sample the finished environment; the published cubes are
	// sampled after the filter. The irradiance and specular dispatches write
	// different textures and read only the environment, so no barrier between
	// them: a full GL_ALL_BARRIER_BITS after each of the eleven dispatches made
	// the GPU drain between them and cost about 0.3 ms per update at
	// 256/32/128 (RTX 4070, measured with GPU timer queries).
	constexpr GLbitfield DECODE_TO_MIPS_BARRIERS = GL_TEXTURE_UPDATE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT;
	constexpr GLbitfield MIPS_TO_FILTER_BARRIERS = GL_TEXTURE_FETCH_BARRIER_BIT;
	constexpr GLbitfield FILTER_TO_CONSUMER_BARRIERS = GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;

	auto dispatch = [&](unsigned int source, unsigned int sourceSize, unsigned int destination,
	                    unsigned int mip, unsigned int size, int filterType, float roughness) {
		glBindTexture(GL_TEXTURE_CUBE_MAP, source);
		glBindImageTexture(PBR_DESTINATION_IMAGE_UNIT, destination, mip, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		pbrFilterShader->SetUniform("sourceResolution", static_cast<float>(sourceSize));
		pbrFilterShader->SetUniform("filterType", filterType);
		pbrFilterShader->SetUniform("roughness", roughness);
		glDispatchCompute((size + groupSize[0] - 1) / groupSize[0],
		                  (size + groupSize[1] - 1) / groupSize[1],
		                  (PBR_FACE_COUNT + groupSize[2] - 1) / groupSize[2]);
		return glGetError() == GL_NO_ERROR;
	};

	bool valid = dispatch(pbrStagingTexID, pbrStagingSize, set.environmentTexID,
	                      0, set.environmentSize, PBR_FILTER_DECODE, 0.0f);
	if (valid) {
		glMemoryBarrier(DECODE_TO_MIPS_BARRIERS);
		glBindTexture(GL_TEXTURE_CUBE_MAP, set.environmentTexID);
		glGenerateMipmapEXT(GL_TEXTURE_CUBE_MAP);
		glMemoryBarrier(MIPS_TO_FILTER_BARRIERS);
		valid = (glGetError() == GL_NO_ERROR);
	}
	if (valid)
		valid = dispatch(set.environmentTexID, set.environmentSize, set.irradianceTexID,
		                 0, set.irradianceSize, PBR_FILTER_DIFFUSE, 1.0f);
	for (unsigned int mip = 0; mip < set.specularMipCount && valid; ++mip) {
		const unsigned int mipSize = std::max(1u, set.specularSize >> mip);
		const float roughness = (set.specularMipCount > 1) ? static_cast<float>(mip) / static_cast<float>(set.specularMipCount - 1) : 0.0f;
		valid = dispatch(set.environmentTexID, set.environmentSize, set.specularTexID,
		                 mip, mipSize, PBR_FILTER_SPECULAR, roughness);
	}
	glMemoryBarrier(FILTER_TO_CONSUMER_BARRIERS);

	glBindImageTexture(PBR_DESTINATION_IMAGE_UNIT, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	pbrFilterShader->Disable();
	globalRendering->LoadViewport();
	if (!valid)
		pbrFailure = "PBR environment compute filtering failed";
	return valid;
}
#endif


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

	// Legacy $specular is generated into unsigned-byte CPU rows and uploaded
	// as RGBA8 with no mip chain. It is not the GGX-filtered $pbr_specular cube.
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

