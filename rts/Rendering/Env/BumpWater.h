/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BUMP_WATER_H
#define BUMP_WATER_H

#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderBuffers.h"
#include "IWater.h"

#include "System/EventClient.h"
#include "System/Rectangle.h"

#include <cstdint>
#include <vector>


namespace Shader {
	struct IProgramObject;
}

class CBumpWater : public IWater, public CEventClient
{
public:
	//! CEventClient interface
	bool WantsEvent(const std::string& eventName) override {
		return shoreWaves && (eventName == "UnsyncedHeightMapUpdate");
	}
	bool GetFullRead() const override { return true; }
	int GetReadAllyTeam() const override { return AllAccessTeam; }

public:
	CBumpWater();
	~CBumpWater() override;
	void InitResources(bool loadShader) override;
	void FreeResources() override;

	void Update() override;
	void UpdateWater(const CGame* game) override;
	void DrawReflection(const CGame* game);
	void DrawRefraction(const CGame* game);
	void Draw() override;
	WATER_RENDERER GetID() const override { return WATER_RENDERER_BUMPMAPPED; }

	bool CanDrawReflectionPass() const override { return true; }
	bool CanDrawRefractionPass() const override { return true; }
private:
	//! coastmap (needed for shorewaves)
	struct CoastRect {
		int x1, y1; ///< update area in coastmap texels (one per heightmap corner), padded by the blur reach
		int x2, y2; ///< exclusive
		int sx, sy; ///< position of the area in the scratch texture
	};

	std::vector<CoastRect> coastRects;
	std::vector<uint8_t> coastDirtyCells; ///< heightmap changes not yet in the coastmap, one flag per cell of COAST_CELL_SIZE^2 texels

	int coastCellsX = 0;
	int coastCellsY = 0;
	int numCoastDirtyCells = 0;
	int coastScanRow = 0;
	int nextCoastUpdateFrame = 0;
	int coastScratchSizeX = 0;
	int coastScratchSizeY = 0;

	void CreateCoastScratch(int sizeX, int sizeY);
	bool CollectCoastRects();
	void UpdateCoastmap(GLuint heightTex);
	void UpdateDynWaves(const bool initialize = false);

	void UnsyncedHeightMapUpdate(const SRectangle& rect) override;

private:
	//! user options
	char  reflection;   ///< 0:=off, 1:=don't render the terrain, 2:=render everything+terrain
	char  refraction;   ///< 0:=off, 1:=screencopy, 2:=own rendering cycle
	int   reflTexSize;
	bool  depthCopy;    ///< uses a screen depth copy, which allows a nicer interpolation between deep sea and shallow water
	float anisotropy;
	char  depthBits;    ///< depthBits for reflection/refraction RBO
	bool  blurRefl;
	bool  shoreWaves;
	bool  endlessOcean; ///< render the water around the whole map
	bool  dynWaves;     ///< only usable if bumpmap/normal texture is a TileSet

	std::vector<uint8_t> tileOffsets; ///< used to randomize the wave/bumpmap/normal texture
	int  normalTextureX; ///< needed for dynamic waves
	int  normalTextureY;

	GLuint target; ///< for screen copies (color/depth), can be GL_TEXTURE_RECTANGLE (nvidia) or GL_TEXTURE_2D (others)
	int  screenTextureX;
	int  screenTextureY;

	FBO reflectFBO;
	FBO refractFBO;
	FBO coastFBO;
	FBO coastScratchFBO;
	FBO dynWavesFBO;

	TypedRenderBuffer<VA_TYPE_0> rb;

	GLuint refractTexture;
	GLuint reflectTexture;
	GLuint depthTexture;   ///< screen depth copy
	GLuint waveRandTexture;
	GLuint foamTexture;
	GLuint normalTexture;  ///< final used
	GLuint normalTexture2; ///< updates normalTexture with dynamic waves turned on
	GLuint coastTexture;
	GLuint coastScratchTexture; ///< ping-pong target of the coastmap blur passes
	std::vector<GLuint> caustTextures;

	Shader::IProgramObject* waterShader;
	Shader::IProgramObject* blurShader;
};

#endif // BUMP_WATER_H

