#pragma once

#include <cstdint>
#include "Rendering/GL/VAO.h"
#include "System/type2.h"

namespace Shader {
	struct IProgramObject;
}

class CCamera;

class DebugCubeMapTexture {
public:
	DebugCubeMapTexture();
	~DebugCubeMapTexture();

	uint32_t GetId() const { return texId; }
	int2 GetDimensions() const { return dims; }

	// true: the labelled cube bitmaps/testsky.dds, false: six plain colours.
	// Replaces the texture (its id changes), so the sky box that borrows it must be rebuilt
	// with ISky::SetSky() afterwards. Returns whether the file is in use (false if it could not be loaded).
	bool SetUseFile(bool useFile);
	bool UsesFile() const { return usesFile; }

	void Draw(uint32_t face = 0) const;

	static DebugCubeMapTexture& GetInstance();
private:
	void CreateTexture(bool wantFile);

	uint32_t texId;
	bool usesFile;
	int2 dims;
	VAO vao; //even though VAO has no attached VBOs, it's still needed to perform rendering
	Shader::IProgramObject* shader;
};

#define debugCubeMapTexture (DebugCubeMapTexture::GetInstance())