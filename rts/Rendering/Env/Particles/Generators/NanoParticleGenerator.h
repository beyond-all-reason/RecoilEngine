#pragma once

#include "ParticleGenerator.h"

struct NanoParticleData {
	CR_DECLARE_STRUCT(NanoParticleData)

	float3 pos;
	int32_t createFrame;

	float3 speed;
	SColor color;

	float3 animParams;
	float partSize;

	float3 rotParams;
	int32_t drawOrder;

	AtlasedTexture texCoord;

	int32_t GetMaxNumQuads() const {  return 1 * (texCoord != AtlasedTexture::DefaultAtlasTexture);	}
	void Invalidate() {
		texCoord = AtlasedTexture::DefaultAtlasTexture;
	}
};
static_assert(sizeof(NanoParticleData) % 16 == 0);

class NanoParticleGenerator : public ParticleGenerator<NanoParticleData, NanoParticleGenerator> {
	friend class ParticleGenerator<NanoParticleData, NanoParticleGenerator>;
public:
	NanoParticleGenerator() {}
	~NanoParticleGenerator() {}
protected:
	bool GenerateCPUImpl() { return false; }
};
