/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>
#include <cstdint>

#include "3DModelPiece.hpp"
#include "IModelParser.h"

#include "System/Threading/SpringThreading.h"

namespace fastgltf {
	class Asset;
}

struct Transform;

struct GLTFPiece : public S3DModelPiece {
	GLTFPiece() = default;
	GLTFPiece(const GLTFPiece&) = delete;
	GLTFPiece(GLTFPiece&& p) noexcept = delete;

	GLTFPiece& operator = (const GLTFPiece& p) = delete;
	GLTFPiece& operator = (GLTFPiece&& p) noexcept = delete;

	static constexpr size_t INVALID_NODE_INDEX = size_t(-1);
	size_t nodeIndex = INVALID_NODE_INDEX;
};

class CGLTFParser: public TypedModelParser<GLTFPiece>
{
public:
	void Init() override {}
	void Kill() override;

	void Load(S3DModel& model, const std::string& name) override;
private:
	GLTFPiece* AllocPiece();
	GLTFPiece* AllocRootEmptyPiece(S3DModel* model, const Transform& parentTransform, const fastgltf::Asset& asset, size_t sceneIndex);
	GLTFPiece* LoadPiece(S3DModel* model, GLTFPiece* parentPiece, const fastgltf::Asset& asset, size_t nodeIndex);
};
