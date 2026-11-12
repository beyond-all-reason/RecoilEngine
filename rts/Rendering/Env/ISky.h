/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_SKY_H
#define I_SKY_H

#include "SkyLight.h"
#include <memory>
#include <string>

struct MapTextureData;
class ISky
{
protected:
	ISky();

public:
	virtual ~ISky();

	virtual void Update() = 0;
	virtual void UpdateSunDir() = 0;
	virtual void UpdateSkyTexture() = 0;

	virtual void Draw() = 0;

	virtual void SetLuaTexture(const MapTextureData& td) {}

	virtual bool IsValid() const = 0;
	// true when Draw() honours drawWithoutSunLobe (the procedural sky draws its
	// sun as a lobe it can leave out; a painted skybox has the sun in its texels)
	virtual bool SupportsSunFreeDraw() const { return false; }

	virtual std::string GetName() const = 0;

	void IncreaseCloudDensity() { cloudDensity *= 1.05f; }
	void DecreaseCloudDensity() { cloudDensity *= 0.95f; }
	float GetCloudDensity() const { return cloudDensity; }

	ISkyLight* GetLight() const { return skyLight; }

	bool SunVisible(const float3 pos) const;

	bool& WireFrameModeRef() { return wireFrameMode; }

	/**
	 * Sets up OpenGL to draw fog or not, according to the value of
	 * globalRendering->drawFog.
	 */
	void SetupFog();

	bool IsUpdated() {
		return std::exchange(updated, false);
	}
	void SetUpdated() { updated = true; }
public:
	static void SetSky();
	static void SetSkyLuaTexture(const MapTextureData& td);
	static auto& GetSky() { return sky; }
	static void KillSky() { sky = nullptr; }
public:
	void SetSkyAxisAngle(const float4& skyAxisAngleRaw);
	const float4& GetSkyAxisAngle() const { return skyAxisAngle; }
public:
	float3 skyColor;
	float3 sunColor;
	float3 cloudColor;
	float4 fogColor;

	float fogStart;
	float fogEnd;
	float cloudDensity;

	// Set only around one Draw() by gl.RenderEnvironmentFace(..., sunFree = true):
	// an environment for image-based lighting must not contain the sun, which
	// the renderer already adds as the direct light.
	bool drawWithoutSunLobe = false;
protected:
	float4 skyAxisAngle;
protected:
	static std::unique_ptr<ISky> sky;

	ISkyLight* skyLight;

	bool wireFrameMode;
private:
	bool updated;
};

#endif // I_SKY_H
