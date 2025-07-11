 /* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _HEIGHT_TEXTURE_H
#define _HEIGHT_TEXTURE_H

#include "ModernInfoTexture.h"
#include "Rendering/GL/FBO.h"
#include "System/EventHandler.h"


namespace Shader {
	struct IProgramObject;
}



class CHeightTexture : public CModernInfoTexture, public CEventClient
{
public:
	CHeightTexture();
	~CHeightTexture() override;

public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) override {
		return (eventName == "UnsyncedHeightMapUpdate");
	}
	bool GetFullRead() const override { return false; }
	int  GetReadAllyTeam() const override { return NoAccessTeam; }

	void UnsyncedHeightMapUpdate(const SRectangle& rect) override;

public:
	// IInfoTexture interface
	void Update() override;
	bool IsUpdateNeeded() override;
private:
	bool needUpdate;
	GL::Texture2D paletteTex;
};

#endif // _HEIGHT_TEXTURE_H
