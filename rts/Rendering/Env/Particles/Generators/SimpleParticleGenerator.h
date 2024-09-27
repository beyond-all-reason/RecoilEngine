#pragma once

#include "ParticleGenerator.h"

struct SimpleParticleData {
	CR_DECLARE_STRUCT(SimpleParticleData)
	float3 pos;
	float size;

	float3 speed;
	int32_t createFrame;

	float3 gravity;
	float airDrag;

	float lifeDecayRate;
	float sizeMod;
	float sizeGrowth;
	float unused1;

	float3 animParams;
	int32_t directional;

	float3 rotParams;
	int32_t drawOrder;

	uint32_t colMapOfft;
	uint32_t colMapSize;
	float unused2;
	float unused3;

	AtlasedTexture texCoord;

	int32_t GetMaxNumQuads() const { return 1 * (texCoord != AtlasedTexture::DefaultAtlasTexture); }
	void Invalidate() {
		texCoord = AtlasedTexture::DefaultAtlasTexture;
	}
};
static_assert(sizeof(SimpleParticleData) % 16 == 0);

class SimpleParticleGenerator : public ParticleGenerator<SimpleParticleData, SimpleParticleGenerator> {
	friend class ParticleGenerator<SimpleParticleData, SimpleParticleGenerator>;
public:
	SimpleParticleGenerator() {}
	~SimpleParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};
