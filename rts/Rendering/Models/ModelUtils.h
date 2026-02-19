#pragma once

#include <vector>
#include <optional>
#include <string>

#include "VertexData.hpp"

class LuaTable;
struct S3DModel;
struct S3DModelPiece;

namespace Skinning {
	struct SkinnedMesh {
		std::vector<SVertexData> verts;
		std::vector<uint32_t> indcs;
	};

	uint16_t GetBoneID(const SVertexData& vert, size_t wi);

	void ReparentMeshesTrianglesToBones(S3DModel* model, const std::vector<SkinnedMesh>& meshes);
	void ReparentCompleteMeshesToBones (S3DModel* model, const std::vector<SkinnedMesh>& meshes);
};

namespace ModelUtils {
	struct ModelParams {
		std::array<std::optional<std::string>, 2> texs;
		std::optional<float3> relMidPos;
		std::optional<float3> mins;
		std::optional<float3> maxs;
		std::optional<float> radius;
		std::optional<float> height;
		std::optional<bool> flipTextures;
		std::optional<bool> invertTeamColor;
		std::optional<bool> s3oCompat;
	};

	// Iterate over the model and calculate its overall dimensions
	void CalculateModelDimensions(S3DModel* model, S3DModelPiece* piece);

	// Calculate model radius from the min/max extents
	void CalculateModelProperties(S3DModel* model, const LuaTable& modelTable);

	// Get ModelParams from the modelTable
	void GetModelParams(const LuaTable& modelTable, ModelParams& modelParams);

	// Apply model properties from modelParams
	void ApplyModelProperties(S3DModel* model, const ModelParams& modelParams);

	// Calculate missing normals
	void CalculateNormals(std::vector<SVertexData>& verts, const std::vector<uint32_t>& indcs);

	// Calculate missing tangents
	void CalculateTangents(std::vector<SVertexData>& verts, const std::vector<uint32_t>& indcs);

	static constexpr uint32_t INVALID_INDEX = uint32_t(-1);
}

namespace ModelLog {
	// Verbose logging of model properties
	void LogModelProperties(const S3DModel& model);
}