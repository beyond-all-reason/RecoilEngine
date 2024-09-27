#pragma once

#include "ParticleGenerator.h"
// needs Update()
struct LaserParticleData {
	CR_DECLARE_STRUCT(LaserParticleData)

	float3 pos;
	float curLength;

	float3 dir;
	float maxLength;

	float thickness;
	float coreThickness;
	SColor color1;
	SColor color2;

	float lodDistance;
	int32_t drawOrder;
	int32_t checkCol;
	float stayTime;

	float4 speed;

	AtlasedTexture texCoord1;
	AtlasedTexture texCoord2;

	int32_t GetMaxNumQuads() const {
		return
			2 * (texCoord1 != AtlasedTexture::DefaultAtlasTexture) +
			4 * (texCoord2 != AtlasedTexture::DefaultAtlasTexture);
	}
	void Invalidate() {
		texCoord1 = AtlasedTexture::DefaultAtlasTexture;
		texCoord2 = AtlasedTexture::DefaultAtlasTexture;
	}
};

static_assert(sizeof(LaserParticleData) % 16 == 0);

class LaserParticleGenerator : public ParticleGenerator<LaserParticleData, LaserParticleGenerator> {
	friend class ParticleGenerator<LaserParticleData, LaserParticleGenerator>;
public:
	LaserParticleGenerator() {}
	~LaserParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};