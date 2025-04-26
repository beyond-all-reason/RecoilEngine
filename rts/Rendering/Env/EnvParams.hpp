#pragma once

#include <cstdint>
#include "System/float4.h"

class EnvParams {
public:
	void Init();
	struct Water {
		float  repeatX;           ///< (calculated default is in IWater)
		float  repeatY;           ///< (calculated default is in IWater)
		float3 absorb;
		float3 baseColor;
		float3 minColor;
		float3 surfaceColor;
		float  surfaceAlpha;
		float4 planeColor;
		float3 diffuseColor;
		float3 specularColor;
		float  ambientFactor;
		float  diffuseFactor;
		float  specularFactor;
		float  specularPower;
		float  fresnelMin;
		float  fresnelMax;
		float  fresnelPower;
		float  reflDistortion;
		float  blurBase;
		float  blurExponent;
		float  perlinStartFreq;
		float  perlinLacunarity;
		float  perlinAmplitude;
		float  windSpeed;
		float  waveOffsetFactor;
		float  waveLength;
		float  waveFoamDistortion;
		float  waveFoamIntensity;
		float  causticsResolution;
		float  causticsStrength;
		bool   shoreWaves;
		bool   forceRendering;    ///< if false the renderers will render it only if currentMinMapHeight<0
		bool   hasWaterPlane;     ///< true if "MAP\WATER\WaterPlaneColor" is set
		uint8_t numTiles;
		std::string texture;
		std::string foamTexture;
		std::string normalTexture;
		std::vector<std::string> causticTextures;
	} water;
	struct Sun {
		float4 groundAmbientColor; // RGB
		float4 groundDiffuseColor; // RGB
		float4 groundSpecularColor; // RGB

		float4 modelAmbientColor;
		float4 modelDiffuseColor;
		float4 modelSpecularColor; // RGB

		float3 dir; // XYZ

		float specularExponent;
		float groundShadowDensity;
		float modelShadowDensity;
	} sun;
	struct Atmo {
		void IncreaseCloudDensity() { cloudDensity *= 1.05f; }
		void DecreaseCloudDensity() { cloudDensity *= 0.95f; }

		float3 skyColor;
		float3 sunColor;
		float3 cloudColor;
		float4 fogColor;

		float fogStart;
		float fogEnd;
		float cloudDensity;
	} atmo;

	bool NeedsUpdate() const;
	void SetUpdated();
private:
	uint32_t updatedFrame;
};

extern EnvParams envParams;