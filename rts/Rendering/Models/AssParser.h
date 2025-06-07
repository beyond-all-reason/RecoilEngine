/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ASS_PARSER_H
#define ASS_PARSER_H

#include <vector>

#include "3DModel.h"
#include "IModelParser.h"
#include "System/float3.h"
#include "System/type2.h"
#include "System/UnorderedMap.hpp"


struct aiNode;
struct aiScene;
class LuaTable;
struct SPseudoAssPiece;

struct SAssPiece: public S3DModelPiece
{
	SAssPiece() = default;
	SAssPiece(const SAssPiece&) = delete;
	SAssPiece(SAssPiece&& p) { *this = std::move(p); }

	SAssPiece& operator = (const SAssPiece& p) = delete;
	SAssPiece& operator = (SAssPiece&& p) {
		#if 0
		// piece is never actually moved, just need the operator for pool
		vertices = std::move(p.vertices);
		indices = std::move(p.indices);
		#endif
		return *this;
	}

	void Clear() override {
		S3DModelPiece::Clear();

		vertices.clear();
		indices.clear();
	}
};


class CAssParser: public IModelParser
{
public:
	using ModelPieceMap = spring::unordered_map<std::string, S3DModelPiece*>;
	using ParentNameMap = spring::unordered_map<std::string, std::string>;

	void Init() override;
	void Kill() override;

	void Load(S3DModel& model, const std::string& name) override;
private:
	static void PreProcessFileBuffer(std::vector<unsigned char>& fileBuffer);

	static void SetPieceName(
		SAssPiece* piece,
		const S3DModel* model,
		const aiNode* pieceNode,
		ModelPieceMap& pieceMap
	);
	static void SetPieceParentName(
		SAssPiece* piece,
		const S3DModel* model,
		const aiNode* pieceNode,
		const LuaTable& pieceTable,
		ParentNameMap& parentMap
	);
	static void LoadPieceTransformations(
		SAssPiece* piece,
		const S3DModel* model,
		const aiNode* pieceNode,
		const LuaTable& pieceTable
	);
	static void LoadPieceTransformations(
		SPseudoAssPiece* piece,
		const S3DModel* model,
		const aiNode* pieceNode,
		const LuaTable& pieceTable
	);
	static void LoadPieceGeometry(
		SAssPiece* piece,
		const S3DModel* model,
		const aiNode* pieceNode,
		const aiScene* scene
	);

	SAssPiece* AllocPiece();
	SAssPiece* LoadPiece(
		S3DModel* model,
		const aiNode* pieceNode,
		const aiScene* scene,
		const LuaTable& modelTable,
		const std::vector<std::string>& skipList,
		ModelPieceMap& pieceMap,
		ParentNameMap& parentMap
	);

	static void BuildPieceHierarchy(S3DModel* model, ModelPieceMap& pieceMap, const ParentNameMap& parentMap);
	static void FindTextures(
		S3DModel* model,
		const aiScene* scene,
		const LuaTable& pieceTable,
		const std::string& modelPath,
		const std::string& modelName
	);

private:
	unsigned int maxIndices = 0;
	unsigned int maxVertices = 0;
	unsigned int numPoolPieces = 0;

	std::vector<SAssPiece> piecePool;
	spring::mutex poolMutex;
};

#endif /* ASS_PARSER_H */
