#pragma once

#include "ParticleGenerator.h"

//no regular Update() needed
struct EmgParticleData {
	CR_DECLARE_STRUCT(EmgParticleData)
	float3 pos;
	float size;

	float3 speed;
	int32_t createFrame;

	float3 animParams;
	SColor color;

	float3 rotParams;
	int32_t drawOrder;

	AtlasedTexture texCoord;

	int32_t GetMaxNumQuads() const { return 1 * (texCoord != AtlasedTexture::DefaultAtlasTexture); }
	void Invalidate() {
		texCoord = AtlasedTexture::DefaultAtlasTexture;
	}
};
static_assert(sizeof(EmgParticleData) % 16 == 0);

class EmgParticleGenerator : public ParticleGenerator<EmgParticleData, EmgParticleGenerator> {
	friend class ParticleGenerator<EmgParticleData, EmgParticleGenerator>;
public:
	EmgParticleGenerator() {}
	~EmgParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};
