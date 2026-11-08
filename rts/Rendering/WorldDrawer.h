/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

class CWorldDrawer
{
public:
	void InitPre() const;
	void InitPost() const;
	void Kill();

	void Update(bool newSimFrame);
	void Draw() const;

	// the part of Update that depends on the active player camera
	// (visibility, draw flags, terrain LOD); re-run it for a view
	// that uses a different camera
	void UpdateViewDependent() const;

	void GenerateIBLTextures() const;
	void ResetMVPMatrices() const;

private:
	void DrawOpaqueObjects() const;
	void DrawAlphaObjects() const;
	void DrawMiscObjects() const;
	void DrawBelowWaterOverlay() const;

private:
	unsigned int numUpdates = 0;
};
