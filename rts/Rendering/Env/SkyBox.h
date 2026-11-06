/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <string>

#include "ISky.h"
#include "Rendering/GL/VAO.h"
#include "Map/MapTexture.h"

namespace Shader {
	struct IProgramObject;
}

class CSkyBox : public ISky
{
public:
	// borrows textureID (the debug cube map, a Lua texture): its owner deletes it
	explicit CSkyBox(uint32_t textureID, uint32_t xsize, uint32_t ysize): ownsTexture(false) { Init(textureID, xsize, ysize, false); }
	explicit CSkyBox(const std::string& texture);
	~CSkyBox() override;
	void Init(uint32_t textureID, uint32_t xsize, uint32_t ysize, bool convertToCM);

	void Update() override {}
	void UpdateSunDir() override {}
	void UpdateSkyTexture() override {}

	void Draw() override;

	bool IsValid() const override { return valid; }

	std::string GetName() const override { return "SkyBox"; }

	void SetLuaTexture(const MapTextureData& td) override
	{
		skyTex.SetLuaTexture(td);
	}

private:
	VAO skyVAO; //even though VAO has no attached VBOs, it's still needed to perform rendering
	Shader::IProgramObject* shader;
	MapTexture skyTex;
	bool valid = false;
	bool ownsTexture = true; // false: skyTex holds a borrowed id that must not be deleted
};
