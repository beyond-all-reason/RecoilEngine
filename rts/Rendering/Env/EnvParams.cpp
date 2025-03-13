#include "EnvParams.hpp"

#include "Rendering/GlobalRendering.h"
#include "Map/MapInfo.h"
#include "System/EventHandler.h"

EnvParams envParams;

void EnvParams::Init()
{
	assert(mapInfo != nullptr);

	updatedFrame = globalRendering->drawFrame + 1;
	{
		const auto& waterMapInfo = mapInfo->water;

		water.repeatX = waterMapInfo.repeatX;
		water.repeatY = waterMapInfo.repeatY;
		water.absorb = waterMapInfo.absorb;
		water.baseColor = waterMapInfo.baseColor;
		water.minColor = waterMapInfo.minColor;
		water.surfaceColor = waterMapInfo.surfaceColor;
		water.surfaceAlpha = waterMapInfo.surfaceAlpha;
		water.planeColor = waterMapInfo.planeColor;
		water.diffuseColor = waterMapInfo.diffuseColor;
		water.specularColor = waterMapInfo.specularColor;
		water.ambientFactor = waterMapInfo.ambientFactor;
		water.diffuseFactor = waterMapInfo.diffuseFactor;
		water.specularFactor = waterMapInfo.specularFactor;
		water.specularPower = waterMapInfo.specularPower;
		water.fresnelMin = waterMapInfo.fresnelMin;
		water.fresnelMax = waterMapInfo.fresnelMax;
		water.fresnelPower = waterMapInfo.fresnelPower;
		water.reflDistortion = waterMapInfo.reflDistortion;
		water.blurBase = waterMapInfo.blurBase;
		water.blurExponent = waterMapInfo.blurExponent;
		water.perlinStartFreq = waterMapInfo.perlinStartFreq;
		water.perlinLacunarity = waterMapInfo.perlinLacunarity;
		water.perlinAmplitude = waterMapInfo.perlinAmplitude;
		water.windSpeed = waterMapInfo.windSpeed;
		water.waveOffsetFactor = waterMapInfo.waveOffsetFactor;
		water.waveLength = waterMapInfo.waveLength;
		water.waveFoamDistortion = waterMapInfo.waveFoamDistortion;
		water.waveFoamIntensity = waterMapInfo.waveFoamIntensity;
		water.causticsResolution = waterMapInfo.causticsResolution;
		water.causticsStrength = waterMapInfo.causticsStrength;
		water.shoreWaves = waterMapInfo.shoreWaves;
		water.forceRendering = waterMapInfo.forceRendering;
		water.hasWaterPlane = waterMapInfo.hasWaterPlane;
		water.numTiles = waterMapInfo.numTiles;
		water.texture = waterMapInfo.texture;
		water.foamTexture = waterMapInfo.foamTexture;
		water.normalTexture = waterMapInfo.normalTexture;
		water.causticTextures = waterMapInfo.causticTextures;
	}
	{
		const auto& mapLightInfo = mapInfo->light;

		sun.groundAmbientColor = mapLightInfo.groundAmbientColor;
		sun.groundDiffuseColor = mapLightInfo.groundDiffuseColor;
		sun.groundSpecularColor = mapLightInfo.groundSpecularColor;

		sun.modelAmbientColor = mapLightInfo.modelAmbientColor;
		sun.modelDiffuseColor = mapLightInfo.modelDiffuseColor;
		sun.modelSpecularColor = mapLightInfo.modelSpecularColor;

		sun.dir = mapLightInfo.sunDir;

		sun.specularExponent = mapLightInfo.specularExponent;
		sun.groundShadowDensity = mapLightInfo.groundShadowDensity;
		sun.modelShadowDensity = mapLightInfo.modelShadowDensity;
	}
	{
		const auto& mapAtmoInfo = mapInfo->atmosphere;

		atmo.skyColor = mapAtmoInfo.skyColor;
		atmo.sunColor = mapAtmoInfo.sunColor;
		atmo.cloudColor = mapAtmoInfo.cloudColor;
		atmo.fogColor = mapAtmoInfo.fogColor;
		atmo.fogStart = mapAtmoInfo.fogStart;
		atmo.fogEnd = mapAtmoInfo.fogEnd;
		atmo.cloudDensity = mapAtmoInfo.cloudDensity;
	}
}

bool EnvParams::NeedsUpdate() const
{
	return updatedFrame == globalRendering->drawFrame + 1;
}

void EnvParams::SetUpdated()
{
	updatedFrame = globalRendering->drawFrame;
}