/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief extended bump-mapping water shader
 */

#include "BumpWater.h"

#include "ISky.h"
#include "SunLighting.h"
#include "WaterRendering.h"

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FastMath.h"
#include "System/SpringMath.h"
#include "System/EventHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/SpringFormat.h"
#include "System/StringUtil.h"

#include "System/Misc/TracyDefs.h"
#include <bit>

using std::string;
using std::vector;
using std::min;
using std::max;

CONFIG(int, BumpWaterTexSizeReflection).defaultValue(512).headlessValue(32).minimumValue(32).description("Sets the size of the framebuffer texture used to store the reflection in Bumpmapped water.");
CONFIG(int, BumpWaterReflection).defaultValue(1).headlessValue(0).minimumValue(0).maximumValue(2).description("Determines the amount of objects reflected in Bumpmapped water.\n0:=off, 1:=fast (skip terrain), 2:=full");
CONFIG(int, BumpWaterRefraction).defaultValue(1).headlessValue(0).minimumValue(0).maximumValue(1).description("Determines the method of refraction with Bumpmapped water.\n0:=off, 1:=screencopy, 2:=own rendering cycle (disabled)");
CONFIG(float, BumpWaterAnisotropy).defaultValue(0.0f).minimumValue(0.0f);
CONFIG(bool, BumpWaterUseDepthTexture).defaultValue(true).headlessValue(false);
CONFIG(int, BumpWaterDepthBits).defaultValue(24).minimumValue(16).maximumValue(32);
CONFIG(bool, BumpWaterBlurReflection).defaultValue(false);
CONFIG(bool, BumpWaterShoreWaves).defaultValue(true).headlessValue(false).safemodeValue(false).description("Enables rendering of shorewaves.");
CONFIG(bool, BumpWaterEndlessOcean).defaultValue(true).description("Sets whether Bumpmapped water will be drawn beyond the map edge.");
CONFIG(bool, BumpWaterDynamicWaves).defaultValue(true);
CONFIG(bool, BumpWaterUseUniforms).deprecated(true);
CONFIG(bool, BumpWaterOcclusionQuery).deprecated(true);

#define LOG_SECTION_BUMP_WATER "BumpWater"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_BUMP_WATER)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_BUMP_WATER

// coastmap updates: heightmap changes mark cells of COAST_CELL_SIZE^2 texels, and every
// COAST_UPDATE_INTERVAL sim frames the marked cells are recomputed on the GPU, padded by
// COAST_PADDING texels (how far the blur passes spread a change), at most COAST_MAX_PIXELS
// padded texels and COAST_MAX_RUN cells per side of one area per update
static constexpr int COAST_CELL_SIZE = 8;
static constexpr int COAST_PADDING = 15;
static constexpr int COAST_MAX_RUN = 32;
static constexpr int COAST_MAX_PIXELS = 512 * 512;
static constexpr int COAST_SCRATCH_SIZE = 1024;
static constexpr int COAST_UPDATE_INTERVAL = 10;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void GLSLDefineConst4f(string& str, const string& name, const float x, const float y, const float z, const float w)
{
	str += spring::format(string("#define ") + name + " vec4(%.12f,%.12f,%.12f,%.12f)\n", x, y, z, w);
}

static void GLSLDefineConstf4(string& str, const string& name, const float3& v, float alpha)
{
	str += spring::format(string("#define ") + name + " vec4(%.12f,%.12f,%.12f,%.12f)\n", v.x, v.y, v.z, alpha);
}

static void GLSLDefineConstf3(string& str, const string& name, const float3& v)
{
	str += spring::format(string("#define ") + name + " vec3(%.12f,%.12f,%.12f)\n", v.x, v.y, v.z);
}

static void GLSLDefineConstf2(string& str, const string& name, float x, float y)
{
	str += spring::format(string("#define ") + name + " vec2(%.12f,%.12f)\n", x, y);
}

static void GLSLDefineConstf1(string& str, const string& name, float x)
{
	str += spring::format(string("#define ") + name + " %.12f\n", x);
}


static GLuint LoadTexture(const string& filename, const float anisotropy = 0.0f, int* sizeX = nullptr, int* sizeY = nullptr)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CBitmap bm;

	if (!bm.Load(filename))
		throw content_error("[" LOG_SECTION_BUMP_WATER "] Could not load texture from file " + filename);

	const unsigned int texID = bm.CreateMipMapTexture(anisotropy);

	if (sizeY != nullptr) {
		*sizeX = bm.xsize;
		*sizeY = bm.ysize;
	}

	return texID;
}


static TypedRenderBuffer<VA_TYPE_0> GenWaterPlaneBuffer(bool radial)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto rb = TypedRenderBuffer<VA_TYPE_0>(9 * 9 * 6, 0, IStreamBufferConcept::Types::SB_BUFFERDATA);

	if (radial) {
		// FIXME: more or less copied from SMFGroundDrawer
		const float xsize = static_cast<float>((mapDims.mapx * SQUARE_SIZE) >> 2);
		const float ysize = static_cast<float>((mapDims.mapy * SQUARE_SIZE) >> 2);

		const float alphainc = math::TWOPI / 32.0f;
		const float size = std::min(xsize, ysize);

		float3 p; p.y = 0.0f;

		for (int n = 0; n < 4; ++n) {
			const float k = (n == 3) ? 0.5f : 1.0f;

			const float r1 = (n + 0) * (n + 0) * size;
			const float r2 = (n + k) * (n + k) * size;

			for (float alpha = 0.0f; (alpha - math::TWOPI) < alphainc; alpha += alphainc) {
				p.x = r1 * fastmath::sin(alpha) + 2 * xsize;
				p.z = r1 * fastmath::cos(alpha) + 2 * ysize;
				rb.AddVertex({ p });

				p.x = r2 * fastmath::sin(alpha) + 2 * xsize;
				p.z = r2 * fastmath::cos(alpha) + 2 * ysize;
				rb.AddVertex({ p });
			}
		}
	}
	else {
		const int mapX = mapDims.mapx * SQUARE_SIZE;
		const int mapZ = mapDims.mapy * SQUARE_SIZE;

		for (int z = 0; z < 9; z++) {
			for (int x = 0; x < 9; x++) {
				const float3 v0{ (x + 0) * (mapX / 9.0f), 0.0f, (z + 0) * (mapZ / 9.0f) };
				const float3 v1{ (x + 0) * (mapX / 9.0f), 0.0f, (z + 1) * (mapZ / 9.0f) };
				const float3 v2{ (x + 1) * (mapX / 9.0f), 0.0f, (z + 0) * (mapZ / 9.0f) };
				const float3 v3{ (x + 1) * (mapX / 9.0f), 0.0f, (z + 1) * (mapZ / 9.0f) };

				rb.AddVertex({ v0 });
				rb.AddVertex({ v1 });
				rb.AddVertex({ v2 });

				rb.AddVertex({ v1 });
				rb.AddVertex({ v3 });
				rb.AddVertex({ v2 });
			}
		}
	}

	rb.SetReadonly();
	return rb;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// (DE-)CONSTRUCTOR
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CBumpWater::CBumpWater()

	: CEventClient("[CBumpWater]", 271923, false)
	, target(GL_TEXTURE_2D)
	, screenTextureX(globalRendering->viewSizeX)
	, screenTextureY(globalRendering->viewSizeY)
	, refractTexture(0)
	, reflectTexture(0)
	, depthTexture(0)
	, waveRandTexture(0)
	, foamTexture(0)
	, normalTexture(0)
	, normalTexture2(0)
	, coastTexture(0)
	, coastScratchTexture(0)
{
	eventHandler.AddClient(this);
}

CBumpWater::~CBumpWater()
{
	RECOIL_DETAILED_TRACY_ZONE;
	FreeResources();
	eventHandler.RemoveClient(this);
}

void CBumpWater::InitResources(bool loadShader)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// LOAD USER CONFIGS
	reflTexSize  = std::bit_ceil <uint32_t> (configHandler->GetInt("BumpWaterTexSizeReflection"));
	reflection   = configHandler->GetInt("BumpWaterReflection");
	refraction   = configHandler->GetInt("BumpWaterRefraction");
	anisotropy   = configHandler->GetFloat("BumpWaterAnisotropy");
	depthCopy    = configHandler->GetBool("BumpWaterUseDepthTexture");
	depthBits    = configHandler->GetInt("BumpWaterDepthBits");
	depthBits    = std::min(depthBits, static_cast<char>(globalRendering->supportDepthBufferBitDepth));
	blurRefl     = configHandler->GetBool("BumpWaterBlurReflection");
	shoreWaves   = (configHandler->GetBool("BumpWaterShoreWaves")) && waterRendering->shoreWaves;
	endlessOcean = (configHandler->GetBool("BumpWaterEndlessOcean")) && waterRendering->hasWaterPlane
	               && ((readMap->HasVisibleWater()) || (waterRendering->forceRendering));
	dynWaves     = (configHandler->GetBool("BumpWaterDynamicWaves")) && (waterRendering->numTiles > 1);

	shoreWaves = shoreWaves && (FBO::IsSupported());
	dynWaves   = dynWaves && (FBO::IsSupported() && GLAD_GL_ARB_imaging);

	// LOAD TEXTURES
	foamTexture   = LoadTexture(waterRendering->foamTexture);
	normalTexture = LoadTexture(waterRendering->normalTexture, anisotropy, &normalTextureX, &normalTextureY);

	// caustic textures
	const vector<string>& causticNames = waterRendering->causticTextures;
	if (causticNames.empty()) {
		throw content_error("[" LOG_SECTION_BUMP_WATER "] no caustic textures");
	}
	for (int i = 0; i < (int)causticNames.size(); ++i) {
		caustTextures.push_back(LoadTexture(causticNames[i]));
	}

	// CHECK SHOREWAVES TEXTURE SIZE
	if (shoreWaves) {
		GLint maxw, maxh;
		glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA16F_ARB, 4096, 4096, 0, GL_RGBA, GL_FLOAT, NULL);
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &maxw);
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &maxh);
		if (mapDims.mapx>maxw || mapDims.mapy>maxh) {
			shoreWaves = false;
			LOG_L(L_WARNING, "Can not display shorewaves (map too large)!");
		}
	}


	// SHOREWAVES
	if (shoreWaves) {
		waveRandTexture = LoadTexture( "bitmaps/shorewaverand.png" );

		glGenTextures(1, &coastTexture);
		glBindTexture(GL_TEXTURE_2D, coastTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, mapDims.mapx, mapDims.mapy, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5, mapDims.mapx, mapDims.mapy, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		//glGenerateMipmapEXT(GL_TEXTURE_2D);


		{
			blurShader = shaderHandler->CreateProgramObject("[BumpWater]", "CoastBlurShader");
			blurShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/BumpWaterCoastBlurVS.glsl", "", GL_VERTEX_SHADER));
			blurShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/BumpWaterCoastBlurFS.glsl", "", GL_FRAGMENT_SHADER));
			blurShader->BindAttribLocations<VA_TYPE_T4>();
			blurShader->Link();

			if (!blurShader->IsValid()) {
				const char* fmt = "shorewaves-shader compilation error: %s";
				const char* log = (blurShader->GetLog()).c_str();

				LOG_L(L_ERROR, fmt, log);

				// string size is limited with content_error()
				throw content_error(string("[" LOG_SECTION_BUMP_WATER "] shorewaves-shader compilation error!"));
			}

			blurShader->Enable();
			blurShader->SetUniform("tex0", 0);
			blurShader->SetUniform("tex1", 1);
			blurShader->SetUniform("heightTex", 2);
			blurShader->SetUniform("args", 0, 0);
			blurShader->Disable();
			blurShader->Validate();

			if (!blurShader->IsValid()) {
				const char* fmt = "shorewaves-shader validation error: %s";
				const char* log = (blurShader->GetLog()).c_str();

				LOG_L(L_ERROR, fmt, log);
				throw content_error(string("[" LOG_SECTION_BUMP_WATER "] shorewaves-shader validation error!"));
			}
		}


		coastFBO.reloadOnAltTab = true;
		coastFBO.Bind();
		coastFBO.AttachTexture(coastTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0_EXT);
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

		if ((shoreWaves = coastFBO.CheckStatus("BUMPWATER(Coastmap)"))) {
			// initialize texture
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			coastCellsX = (mapDims.mapx + COAST_CELL_SIZE - 1) / COAST_CELL_SIZE;
			coastCellsY = (mapDims.mapy + COAST_CELL_SIZE - 1) / COAST_CELL_SIZE;
			coastDirtyCells.assign(coastCellsX * coastCellsY, 0);

			// fill with the current heightmap in one pass over the whole map; the map's
			// heightmap texture is not uploaded yet while loading, so use a copy here
			const float* heightMap = (!gs->PreSimFrame()) ? readMap->GetCornerHeightMapUnsynced() : readMap->GetCornerHeightMapSynced();

			GLuint heightTex = 0;
			glGenTextures(1, &heightTex);
			glBindTexture(GL_TEXTURE_2D, heightTex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, mapDims.mapxp1, mapDims.mapyp1, 0, GL_RED, GL_FLOAT, heightMap);
			glBindTexture(GL_TEXTURE_2D, 0);

			CreateCoastScratch(mapDims.mapx, mapDims.mapy);
			coastRects.assign(1, CoastRect{ 0, 0, mapDims.mapx, mapDims.mapy, 0, 0 });
			UpdateCoastmap(heightTex);
			glDeleteTextures(1, &heightTex);

			// later updates cover at most COAST_MAX_PIXELS texels
			CreateCoastScratch(std::min(mapDims.mapx, COAST_SCRATCH_SIZE), std::min(mapDims.mapy, COAST_SCRATCH_SIZE));

			eventHandler.InsertEvent(this, "UnsyncedHeightMapUpdate");
		}

		//coastFBO.Unbind(); // gets done below
	}


	// CREATE TEXTURES
	if (refraction > 0) {
		// CREATE REFRACTION TEXTURE
		glGenTextures(1, &refractTexture);
		glBindTexture(target, refractTexture);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(target, 0, GL_RGBA8, screenTextureX, screenTextureY, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	if (reflection > 0) {
		// CREATE REFLECTION TEXTURE
		glGenTextures(1, &reflectTexture);
		glBindTexture(GL_TEXTURE_2D, reflectTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	if (depthCopy) {
		// CREATE DEPTH TEXTURE
		glGenTextures(1, &depthTexture);
		glBindTexture(target, depthTexture);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		GLuint depthFormat = CGlobalRendering::DepthBitsToFormat(globalRendering->supportDepthBufferBitDepth);
		glTexImage2D(target, 0, depthFormat, screenTextureX, screenTextureY, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}

	if (dynWaves) {
		// SETUP DYNAMIC WAVES
		tileOffsets.resize(waterRendering->numTiles * waterRendering->numTiles);

		normalTexture2 = normalTexture;
		glBindTexture(GL_TEXTURE_2D, normalTexture2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glGenTextures(1, &normalTexture);
		glBindTexture(GL_TEXTURE_2D, normalTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		if (anisotropy > 0.0f) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, normalTextureX, normalTextureY, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glGenerateMipmapEXT(GL_TEXTURE_2D);
	}

	// CREATE FBOs
	if (FBO::IsSupported()) {
		GLuint depthRBOFormat = static_cast<GLuint>(CGlobalRendering::DepthBitsToFormat(depthBits));

		if (reflection>0) {
			reflectFBO.Bind();
			reflectFBO.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, depthRBOFormat, reflTexSize, reflTexSize);
			reflectFBO.AttachTexture(reflectTexture);
			if (!reflectFBO.CheckStatus("BUMPWATER(reflection)")) {
				reflection = 0;
			}
		}

		if (refraction > 0) {
			refractFBO.Bind();
			refractFBO.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, depthRBOFormat, screenTextureX, screenTextureY);
			refractFBO.AttachTexture(refractTexture,target);
			if (!refractFBO.CheckStatus("BUMPWATER(refraction)")) {
				refraction = 0;
			}
		}

		if (dynWaves) {
			dynWavesFBO.reloadOnAltTab = true;
			dynWavesFBO.Bind();
			dynWavesFBO.AttachTexture(normalTexture);
			if (dynWavesFBO.CheckStatus("BUMPWATER(DynWaves)")) {
				UpdateDynWaves(true); // initialize
			}
		}

		FBO::Unbind();
	}


	/*
	 * DEFINE SOME SHADER RUNTIME CONSTANTS
	 * (I do not use Uniforms for that, because the GLSL compiler can not
	 * optimize those!)
	 */
	string definitions;
	if (reflection>0) definitions += "#define opt_reflection\n";
	if (refraction>0) definitions += "#define opt_refraction\n";
	if (shoreWaves)   definitions += "#define opt_shorewaves\n";
	if (depthCopy)    definitions += "#define opt_depth\n";
	if (blurRefl)     definitions += "#define opt_blurreflection\n";
	if (endlessOcean) definitions += "#define opt_endlessocean\n";

	GLSLDefineConstf3(definitions, "MapMid",                    float3(mapDims.mapx * SQUARE_SIZE * 0.5f, 0.0f, mapDims.mapy * SQUARE_SIZE * 0.5f));
	GLSLDefineConstf2(definitions, "ScreenInverse",             1.0f / globalRendering->viewSizeX, 1.0f / globalRendering->viewSizeY);
	GLSLDefineConstf2(definitions, "ScreenTextureSizeInverse",  1.0f / screenTextureX, 1.0f / screenTextureY);
	GLSLDefineConstf2(definitions, "ViewPos",                   globalRendering->viewPosX, globalRendering->viewPosY);

	GLSLDefineConstf4(definitions, "SurfaceColor",   waterRendering->surfaceColor*0.4, waterRendering->surfaceAlpha );
	GLSLDefineConstf4(definitions, "PlaneColor",     waterRendering->planeColor*0.4, waterRendering->surfaceAlpha );
	GLSLDefineConstf3(definitions, "DiffuseColor",   waterRendering->diffuseColor);
	GLSLDefineConstf3(definitions, "SpecularColor",  waterRendering->specularColor);
	GLSLDefineConstf1(definitions, "SpecularPower",  waterRendering->specularPower);
	GLSLDefineConstf1(definitions, "SpecularFactor", waterRendering->specularFactor);
	GLSLDefineConstf1(definitions, "AmbientFactor",  waterRendering->ambientFactor);
	GLSLDefineConstf1(definitions, "DiffuseFactor",  waterRendering->diffuseFactor * 15.0f);
	GLSLDefineConstf3(definitions, "SunDir"       ,  ISky::GetSky()->GetLight()->GetLightDir()); // FIXME: not a constant
	GLSLDefineConstf1(definitions, "FresnelMin",     waterRendering->fresnelMin);
	GLSLDefineConstf1(definitions, "FresnelMax",     waterRendering->fresnelMax);
	GLSLDefineConstf1(definitions, "FresnelPower",   waterRendering->fresnelPower);
	GLSLDefineConstf1(definitions, "ReflDistortion", waterRendering->reflDistortion);
	GLSLDefineConstf2(definitions, "BlurBase",       0.0f, waterRendering->blurBase / globalRendering->viewSizeY);
	GLSLDefineConstf1(definitions, "BlurExponent",   waterRendering->blurExponent);
	GLSLDefineConstf1(definitions, "PerlinStartFreq",  waterRendering->perlinStartFreq);
	GLSLDefineConstf1(definitions, "PerlinLacunarity", waterRendering->perlinLacunarity);
	GLSLDefineConstf1(definitions, "PerlinAmp",        waterRendering->perlinAmplitude);
	GLSLDefineConstf1(definitions, "WaveOffsetFactor",   waterRendering->waveOffsetFactor);
	GLSLDefineConstf1(definitions, "WaveLength",         waterRendering->waveLength);
	GLSLDefineConstf1(definitions, "WaveFoamDistortion", waterRendering->waveFoamDistortion);
	GLSLDefineConstf1(definitions, "WaveFoamIntensity",  waterRendering->waveFoamIntensity);
	GLSLDefineConstf1(definitions, "CausticsResolution", waterRendering->causticsResolution);
	GLSLDefineConstf1(definitions, "CausticsStrength",   waterRendering->causticsStrength);
	GLSLDefineConstf1(definitions, "shadowDensity",  sunLighting->groundShadowDensity);

	{
		const int mapX = mapDims.mapx  * SQUARE_SIZE;
		const int mapZ = mapDims.mapy * SQUARE_SIZE;
		const float shadingX = (float)mapDims.mapx / mapDims.pwr2mapx;
		const float shadingZ = (float)mapDims.mapy / mapDims.pwr2mapy;

		const float scaleX = (mapX > mapZ) ? (mapDims.mapy >> 6) / 16.0f * (float)mapX / mapZ : (mapDims.mapx >> 6) / 16.0f;
		const float scaleZ = (mapX > mapZ) ? (mapDims.mapy >> 6) / 16.0f : (mapDims.mapx >> 6) / 16.0f * (float)mapZ / mapX;
		GLSLDefineConst4f(definitions, "TexGenPlane", 1.0f/mapX, 1.0f/mapZ, scaleX/mapX, scaleZ/mapZ);
		GLSLDefineConst4f(definitions, "ShadingPlane", shadingX/mapX, shadingZ/mapZ, shadingX, shadingZ);
	}

	// LOAD SHADERS
	{
		waterShader = shaderHandler->CreateProgramObject("[BumpWater]", "WaterShader");
		waterShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/BumpWaterVS.glsl", definitions, GL_VERTEX_SHADER));
		waterShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/BumpWaterFS.glsl", definitions, GL_FRAGMENT_SHADER));
		using VAT = std::decay_t<decltype(rb)>::VertType;
		waterShader->BindAttribLocations<VAT>();
		waterShader->Link();

		if (!waterShader->IsValid()) {
			const char* fmt = "water-shader compilation error: %s";
			const char* log = (waterShader->GetLog()).c_str();
			LOG_L(L_ERROR, fmt, log);
			throw content_error(string("[" LOG_SECTION_BUMP_WATER "] water-shader compilation error!"));
		}

		// BIND TEXTURE UNIFORMS
		// NOTE: ATI shader validation code is stricter wrt. state,
		// so postpone the call until all texture uniforms are set
		waterShader->Enable();

		waterShader->SetUniform("normalmap"     , 0 );
		waterShader->SetUniform("heightmap"     , 1 );
		waterShader->SetUniform("caustic"       , 2 );
		waterShader->SetUniform("foam"          , 3 );
		waterShader->SetUniform("reflection"    , 4 );
		waterShader->SetUniform("refraction"    , 5 );
		waterShader->SetUniform("coastmap"      , 6 );
		waterShader->SetUniform("depthmap"      , 7 );
		waterShader->SetUniform("waverand"      , 8 );
		waterShader->SetUniform("shadowmap"     , 9 );
		waterShader->SetUniform("infotex"       , 10);
		waterShader->SetUniform("shadowColorTex", 11);
		waterShader->SetUniform("windVector"    , 15.0f, 15.0f);

		waterShader->Disable();
		waterShader->Validate();

		if (!waterShader->IsValid()) {
			const char* fmt = "water-shader validation error: %s";
			const char* log = (waterShader->GetLog()).c_str();

			LOG_L(L_ERROR, fmt, log);
			throw content_error(string("[" LOG_SECTION_BUMP_WATER "] water-shader validation error!"));
		}
	}

	rb = GenWaterPlaneBuffer(endlessOcean);
}

void CBumpWater::FreeResources()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto DeleteTexture = [](GLuint& texID) { if (texID > 0) { glDeleteTextures(1, &texID); texID = 0; } };

	DeleteTexture(reflectTexture);
	DeleteTexture(refractTexture);
	DeleteTexture(depthTexture);
	DeleteTexture(foamTexture);
	DeleteTexture(normalTexture);
	DeleteTexture(normalTexture2);
	DeleteTexture(coastTexture);
	DeleteTexture(coastScratchTexture);
	DeleteTexture(waveRandTexture);
	for (auto& caustTexture : caustTextures) {
		DeleteTexture(caustTexture);
	}

	tileOffsets.clear();
	shaderHandler->ReleaseProgramObjects("[BumpWater]");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  UPDATE
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CBumpWater::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!waterRendering->forceRendering && !readMap->HasVisibleWater())
		return;

	if (dynWaves)
		UpdateDynWaves();

	// on the first call at or after every COAST_UPDATE_INTERVAL'th frame (more than one sim frame can pass between two calls)
	if (shoreWaves && numCoastDirtyCells > 0 && gs->frameNum >= nextCoastUpdateFrame) {
		nextCoastUpdateFrame = (gs->frameNum / COAST_UPDATE_INTERVAL + 1) * COAST_UPDATE_INTERVAL;

		if (CollectCoastRects())
			UpdateCoastmap(readMap->GetHeightMapTexture());
	}
}


void CBumpWater::UpdateWater(const CGame* game)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!waterRendering->forceRendering && !readMap->HasVisibleWater())
		return;

	glPushAttrib(GL_FOG_BIT);
	if (refraction > 1) DrawRefraction(game);
	if (reflection > 0) DrawReflection(game);
	if (reflection || refraction) {
		FBO::Unbind();
		globalRendering->LoadViewport();
	}
	glPopAttrib();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  SHOREWAVES/COASTMAP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CBumpWater::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!shoreWaves || !readMap->HasVisibleWater() || coastDirtyCells.empty())
		return;

	// corner rectangle (inclusive); coastmap texel (x, z) holds the height of corner (x, z)
	const int cx1 = std::clamp(rect.x1, 0, mapDims.mapx - 1) / COAST_CELL_SIZE;
	const int cx2 = std::clamp(rect.x2, 0, mapDims.mapx - 1) / COAST_CELL_SIZE;
	const int cy1 = std::clamp(rect.z1, 0, mapDims.mapy - 1) / COAST_CELL_SIZE;
	const int cy2 = std::clamp(rect.z2, 0, mapDims.mapy - 1) / COAST_CELL_SIZE;

	for (int cy = cy1; cy <= cy2; ++cy) {
		for (int cx = cx1; cx <= cx2; ++cx) {
			uint8_t& cell = coastDirtyCells[cy * coastCellsX + cx];
			numCoastDirtyCells += (cell == 0);
			cell = 1;
		}
	}
}


void CBumpWater::CreateCoastScratch(int sizeX, int sizeY)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (coastScratchTexture != 0 && sizeX == coastScratchSizeX && sizeY == coastScratchSizeY)
		return;

	if (coastScratchTexture == 0)
		glGenTextures(1, &coastScratchTexture);

	glBindTexture(GL_TEXTURE_2D, coastScratchTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, sizeX, sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

	coastScratchSizeX = sizeX;
	coastScratchSizeY = sizeY;

	coastScratchFBO.reloadOnAltTab = true;
	coastScratchFBO.Bind();
	coastScratchFBO.AttachTexture(coastScratchTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0_EXT);
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	coastScratchFBO.CheckStatus("BUMPWATER(CoastScratch)");
	coastScratchFBO.Unbind();
}


bool CBumpWater::CollectCoastRects()
{
	RECOIL_DETAILED_TRACY_ZONE;
	coastRects.clear();

	const auto IsDirty = [this](int cx, int cy) { return (coastDirtyCells[cy * coastCellsX + cx] != 0); };

	// areas are shelf-packed into the scratch texture
	int shelfX = 0;
	int shelfY = 0;
	int shelfH = 0;
	int numPixels = 0;

	// the scan continues where the previous update stopped, so a busy region cannot starve the rest
	for (int n = 0; n < coastCellsY && numCoastDirtyCells > 0; ++n) {
		const int cy = (coastScanRow + n) % coastCellsY;

		for (int cx = 0; cx < coastCellsX; ++cx) {
			if (!IsDirty(cx, cy))
				continue;

			// a run of dirty cells, extended downwards while every cell below it is dirty too
			int cx2 = cx + 1;
			int cy2 = cy + 1;

			while (cx2 < coastCellsX && (cx2 - cx) < COAST_MAX_RUN && IsDirty(cx2, cy))
				cx2++;

			while (cy2 < coastCellsY && (cy2 - cy) < COAST_MAX_RUN) {
				bool rowDirty = true;

				for (int x = cx; x < cx2 && rowDirty; ++x) {
					rowDirty = IsDirty(x, cy2);
				}
				if (!rowDirty)
					break;

				cy2++;
			}

			CoastRect r;
			r.x1 = std::max(cx  * COAST_CELL_SIZE - COAST_PADDING, 0);
			r.y1 = std::max(cy  * COAST_CELL_SIZE - COAST_PADDING, 0);
			r.x2 = std::min(cx2 * COAST_CELL_SIZE + COAST_PADDING, mapDims.mapx);
			r.y2 = std::min(cy2 * COAST_CELL_SIZE + COAST_PADDING, mapDims.mapy);

			const int w = r.x2 - r.x1;
			const int h = r.y2 - r.y1;

			if (shelfX + w > coastScratchSizeX) {
				shelfX = 0;
				shelfY += shelfH;
				shelfH = 0;
			}

			// out of budget or scratch space, the rest waits for the next update
			if (!coastRects.empty() && ((numPixels + w * h) > COAST_MAX_PIXELS || (shelfY + h) > coastScratchSizeY)) {
				coastScanRow = cy;
				return true;
			}

			r.sx = shelfX;
			r.sy = shelfY;
			shelfX += w;
			shelfH = std::max(shelfH, h);
			numPixels += w * h;

			for (int y = cy; y < cy2; ++y) {
				std::fill_n(coastDirtyCells.begin() + (y * coastCellsX + cx), cx2 - cx, uint8_t(0));
			}

			numCoastDirtyCells -= (cx2 - cx) * (cy2 - cy);
			coastRects.push_back(r);

			cx = cx2 - 1;
		}
	}

	return !coastRects.empty();
}


void CBumpWater::UpdateCoastmap(GLuint heightTex)
{
	RECOIL_DETAILED_TRACY_ZONE;
	SCOPED_GL_DEBUGGROUP("Update::Water::Coastmap");

	glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

	glDisable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, heightTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, coastScratchTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, coastTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);

	const float coastScaleX = 1.0f / mapDims.mapx;
	const float coastScaleY = 1.0f / mapDims.mapy;
	const float scratchScaleX = 1.0f / coastScratchSizeX;
	const float scratchScaleY = 1.0f / coastScratchSizeY;

	auto& rbt4 = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_T4>();

	// draws each area into the coastmap or into its scratch area; the texture coordinates
	// address the same texels in the other texture, and .pq spans the area from 0 to 1
	const auto DrawAreas = [&](bool toScratch) {
		for (const CoastRect& r : coastRects) {
			const float4 coastCoords = {
				r.x1 * coastScaleX,
				r.y1 * coastScaleY,
				r.x2 * coastScaleX,
				r.y2 * coastScaleY
			};
			const float4 scratchCoords = {
				r.sx * scratchScaleX,
				r.sy * scratchScaleY,
				(r.sx + r.x2 - r.x1) * scratchScaleX,
				(r.sy + r.y2 - r.y1) * scratchScaleY
			};
			const float4& v = toScratch? scratchCoords: coastCoords;
			const float4& t = toScratch? coastCoords: scratchCoords;

			rbt4.AddQuadTriangles(
				{ { v.x, v.y, 0.0f }, { t.x, t.y, 0.0f, 0.0f } },
				{ { v.x, v.w, 0.0f }, { t.x, t.w, 0.0f, 1.0f } },
				{ { v.z, v.w, 0.0f }, { t.z, t.w, 1.0f, 1.0f } },
				{ { v.z, v.y, 0.0f }, { t.z, t.y, 1.0f, 0.0f } }
			);
		}
		rbt4.DrawElements(GL_TRIANGLES);
	};

	blurShader->Enable();

	// initialize the areas from the heightmap, then blur them back and forth
	coastFBO.Bind();
	glViewport(0, 0, mapDims.mapx, mapDims.mapy);
	blurShader->SetUniform("args", 0, 0);
	DrawAreas(false);

	for (int i = 0; i < 5; ++i) {
		coastScratchFBO.Bind();
		glViewport(0, 0, coastScratchSizeX, coastScratchSizeY);
		blurShader->SetUniform("args", 1, i * 2 + 1);
		DrawAreas(true);

		coastFBO.Bind();
		glViewport(0, 0, mapDims.mapx, mapDims.mapy);
		blurShader->SetUniform("args", 0, i * 2 + 2);
		DrawAreas(false);
	}

	blurShader->Disable();
	coastFBO.Unbind();

	//glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glPopAttrib();

	// generate mipmaps (coastTexture is still bound to unit 0)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glGenerateMipmapEXT(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);

	coastRects.clear();
	globalRendering->LoadViewport();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  DYNAMIC WAVES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CBumpWater::UpdateDynWaves(const bool initialize)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!dynWaves || !dynWavesFBO.IsValid())
		return;

	const unsigned char tiles  = waterRendering->numTiles; // (numTiles <= 16)
	const unsigned char ntiles = tiles * tiles;

	const float tilesize = 1.0f / tiles;
	const int modFrameNum = (gs->frameNum + 1) % 60;

	if (modFrameNum == 0) {
		for (unsigned char i = 0; i < ntiles; ++i) {
			do {
				tileOffsets[i] = (unsigned char)(guRNG.NextFloat()*ntiles);
			} while (tileOffsets[i] == i);
		}
	}

	dynWavesFBO.Bind();
	glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, normalTexture2);
	glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
	glBlendColor(1.0f, 1.0f, 1.0f, (initialize) ? 1.0f : (modFrameNum + 1)/600.0f );

	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);

	glViewport(0, 0, normalTextureX, normalTextureY);
	glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, 1, 0, 1, -1, 1);
	glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glLoadIdentity();

	auto& rb2tc = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DTC>();

	for (unsigned char y = 0; y < tiles; ++y) {
		for (unsigned char x = 0; x < tiles; ++x) {
			uint8_t offset = tileOffsets[y * tiles + x];
			uint8_t tx = offset % tiles;
			uint8_t ty = (offset - tx)/tiles;
			rb2tc.AddQuadTriangles(
				{ (x + 0) * tilesize, (y + 0) * tilesize, (tx + 0) * tilesize, (ty + 0) * tilesize },
				{ (x + 0) * tilesize, (y + 1) * tilesize, (tx + 0) * tilesize, (ty + 1) * tilesize },
				{ (x + 1) * tilesize, (y + 1) * tilesize, (tx + 1) * tilesize, (ty + 1) * tilesize },
				{ (x + 1) * tilesize, (y + 0) * tilesize, (tx + 1) * tilesize, (ty + 0) * tilesize }
			);
		}
	}
	auto& rbSh = rb2tc.GetShader();
	rbSh.Enable();
	rb2tc.DrawElements(GL_TRIANGLES);
	rbSh.Disable();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glPopMatrix();
	glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	globalRendering->LoadViewport();

	glPopAttrib();
	dynWavesFBO.Unbind();

	glBindTexture(GL_TEXTURE_2D, normalTexture);
	glGenerateMipmapEXT(GL_TEXTURE_2D);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  DRAW FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CBumpWater::Draw()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!waterRendering->forceRendering && !readMap->HasVisibleWater())
		return;

	if (refraction == 1) {
		// _SCREENCOPY_ REFRACT TEXTURE
		glBindTexture(target, refractTexture);
		glCopyTexSubImage2D(target, 0, 0, 0, globalRendering->viewPosX, globalRendering->viewPosY, globalRendering->viewSizeX, globalRendering->viewSizeY);
	}

	if (depthCopy) {
		// _SCREENCOPY_ DEPTH TEXTURE
		glBindTexture(target, depthTexture);
		glCopyTexSubImage2D(target, 0, 0, 0, globalRendering->viewPosX, globalRendering->viewPosY, globalRendering->viewSizeX, globalRendering->viewSizeY);
	}

	glDisable(GL_ALPHA_TEST);

	if (refraction < 2)
		glDepthMask(GL_FALSE);

	if (refraction > 0)
		glDisable(GL_BLEND);

#if 1
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.0f, 2.0f);
#endif

	waterShader->SetFlag("opt_shadows", (shadowHandler.ShadowsLoaded()));
	waterShader->SetFlag("opt_infotex", infoTextureHandler->IsEnabled());

	waterShader->Enable();
	waterShader->SetUniform("eyePos", camera->GetPos().x, camera->GetPos().y, camera->GetPos().z);
	waterShader->SetUniform("frame", (gs->frameNum + globalRendering->timeOffset) / 15000.0f);

	if (shadowHandler.ShadowsLoaded()) {
		waterShader->SetUniformMatrix4x4("shadowMatrix", false, shadowHandler.GetShadowMatrixRaw());

		shadowHandler.SetupShadowTexSampler(GL_TEXTURE9);
		glActiveTexture(GL_TEXTURE11); glBindTexture(GL_TEXTURE_2D, shadowHandler.GetColorTextureID());
	}

	const int causticTexNum = (gs->frameNum % caustTextures.size());
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, readMap->GetShadingTexture());
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, caustTextures[causticTexNum]);
	glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, foamTexture);
	glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, reflectTexture);
	glActiveTexture(GL_TEXTURE5); glBindTexture(target,        refractTexture);
	glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, coastTexture);
	glActiveTexture(GL_TEXTURE7); glBindTexture(target,        depthTexture);
	glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, waveRandTexture);
	//glActiveTexture(GL_TEXTURE9); see above
	glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());
	//glActiveTexture(GL_TEXTURE11); see above
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, normalTexture);

	glPolygonMode(GL_FRONT_AND_BACK, wireFrameMode ? GL_LINE : GL_FILL);
	rb.DrawArrays(endlessOcean ? GL_TRIANGLE_STRIP : GL_TRIANGLES);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	waterShader->Disable();

#if 1
	glDisable(GL_POLYGON_OFFSET_FILL);
#endif

	if (shadowHandler.ShadowsLoaded()) {
		glActiveTexture(GL_TEXTURE9); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glActiveTexture(GL_TEXTURE0);
	}

	if (refraction < 2)
		glDepthMask(GL_TRUE);

	if (refraction > 0)
		glEnable(GL_BLEND);
}

void CBumpWater::DrawRefraction(const CGame* game)
{	
	ZoneScopedN("BumpWater::DrawRefraction");
	// _RENDER_ REFRACTION TEXTURE
	refractFBO.Bind();

	camera->Update();

	globalRendering->LoadViewport();
	const auto& sky = ISky::GetSky();
	glClearColor(sky->fogColor.x, sky->fogColor.y, sky->fogColor.z, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_FOG); // fog has overground settings, if at all we should add special underwater settings

	const double clipPlaneEqs[2 * 4] = {
		0.0, -1.0, 0.0, 5.0, // ground
		0.0, -1.0, 0.0, 0.0, // models
	};

	const float3 oldsun = sunLighting->modelDiffuseColor;
	const float3 oldambient = sunLighting->modelAmbientColor;

	sunLighting->modelDiffuseColor *= float3(0.5f, 0.7f, 0.9f);
	sunLighting->modelAmbientColor *= float3(0.6f, 0.8f, 1.0f);

	DrawRefractions(&clipPlaneEqs[0], true, true);

	glEnable(GL_FOG);

	sunLighting->modelDiffuseColor = oldsun;
	sunLighting->modelAmbientColor = oldambient;
}


void CBumpWater::DrawReflection(const CGame* game)
{
	ZoneScopedN("BumpWater::DrawReflection");
	reflectFBO.Bind();

	const auto& sky = ISky::GetSky();
	glClearColor(sky->fogColor.x, sky->fogColor.y, sky->fogColor.z, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const double clipPlaneEqs[2 * 4] = {
		0.0, 1.0, 0.0, 5.0, // ground; use d>0 to hide shoreline cracks
		0.0, 1.0, 0.0, 0.0, // models
	};

	CCamera* prvCam = CCameraHandler::GetSetActiveCamera(CCamera::CAMTYPE_UWREFL);
	CCamera* curCam = CCameraHandler::GetActiveCamera();

	{
		curCam->CopyStateReflect(prvCam);
		curCam->UpdateLoadViewport(0, 0, reflTexSize, reflTexSize);

		DrawReflections(&clipPlaneEqs[0], reflection > 1, true);
	}

	CCameraHandler::SetActiveCamera(prvCam->GetCamType());

	prvCam->Update();
	// done by caller
	// prvCam->LoadViewPort();
}
