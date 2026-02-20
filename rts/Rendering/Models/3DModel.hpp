/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>
#include <array>
#include <cstdint>

#include "3DModelDefs.hpp"
#include "ModelsMemStorage.h"
#include "System/float3.h"

struct S3DModelPiece;

/**
 * S3DModel
 * A 3D model definition. Holds geometry (vertices/normals) and texture data as well as the piece tree.
 * The S3DModel is static and shouldn't change once created, instead a LocalModel is used by each agent.
 */
struct S3DModel
{
	enum LoadStatus {
		NOTLOADED,
		LOADING,
		LOADED
	};
	S3DModel()
		: id(-1)
		, numPieces(0)
		, textureType(-1)

		, indxStart(~0u)
		, indxCount(0u)

		, type(MODELTYPE_CNT)

		, radius(0.0f)
		, height(0.0f)

		, mins(DEF_MIN_SIZE)
		, maxs(DEF_MAX_SIZE)
		, relMidPos(ZeroVector)

		, loadStatus(NOTLOADED)
		, uploaded(false)

		, traAlloc(ScopedTransformMemAlloc())
	{}

	S3DModel(const S3DModel& m) = delete;
	S3DModel(S3DModel&& m) noexcept { *this = std::move(m); }

	S3DModel& operator= (const S3DModel& m) = delete;
	S3DModel& operator= (S3DModel&& m) noexcept;

	      S3DModelPiece* FindPiece(const std::string& name);
	const S3DModelPiece* FindPiece(const std::string& name) const;
	size_t FindPieceOffset(const std::string& name) const;

	S3DModelPiece* GetPiece(size_t i) const { assert(i < pieceObjects.size()); return pieceObjects[i]; }
	S3DModelPiece* GetRootPiece() const { return (GetPiece(GetRootPieceIndex())); }
	size_t GetRootPieceIndex() const { return 0; }

	void AddPiece(S3DModelPiece* p) { pieceObjects.push_back(p); }
	void DrawStatic() const;
	void SetPieceMatrices();

	void FlattenPieceTree(S3DModelPiece* root);

	void UpdatePiecesMinMaxExtents();

	// default values set by parsers; radius is also cached in WorldObject::drawRadius (used by projectiles)
	float CalcDrawRadius() const { return ((maxs - mins).Length() * 0.5f); }
	float CalcDrawHeight() const { return (maxs.y - mins.y); }
	float GetDrawRadius() const { return radius; }
	float GetDrawHeight() const { return height; }

	float3 CalcDrawMidPos() const { return ((maxs + mins) * 0.5f); }
	float3 GetDrawMidPos() const { return relMidPos; }

	const ScopedTransformMemAlloc& GetTransformAlloc() const { return traAlloc; }
public:
	static constexpr size_t NUM_MODEL_TEXTURES = 2;

	std::string name;
	std::array<std::string, NUM_MODEL_TEXTURES> texs;

	// flattened tree; pieceObjects[0] is the root
	std::vector<S3DModelPiece*> pieceObjects;

	int id;                     /// unsynced ID, starting with 1
	int numPieces;
	int textureType;            /// FIXME: MAKE S3O ONLY (0 = 3DO, otherwise S3O or ASSIMP)

	uint32_t indxStart; //global VBO offset, size data
	uint32_t indxCount;

	ModelType type;

	float radius;
	float height;

	float3 mins;
	float3 maxs;
	float3 relMidPos;

	LoadStatus loadStatus;
	bool uploaded;
private:
	ScopedTransformMemAlloc traAlloc;
};