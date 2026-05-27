#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <optional>

#include "3DModelDefs.hpp"
#include "3DModelMisc.hpp"
#include "VertexData.hpp"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Matrix44f.h"
#include "System/Transform.hpp"
#include "System/AABB.hpp"

struct S3DModel;
struct S3DModelPiece {
	S3DModelPiece() = default;
	S3DModelPiece(const S3DModelPiece&) = delete;
	S3DModelPiece(S3DModelPiece&& p) noexcept = delete;

	S3DModelPiece& operator = (const S3DModelPiece& p) = delete;
	S3DModelPiece& operator = (S3DModelPiece&& p) noexcept = delete;

	void SetEmitters();

	const auto& GetEmitPos() const { return emitPos; }
	const auto& GetEmitDir() const { return emitDir; }

	void DrawElements(uint32_t prim = 0x0004/*GL_TRIANGLES*/) const;
	static void DrawShatterElements(uint32_t vboIndxStart, uint32_t vboIndxCount, uint32_t prim = 0x0004/*GL_TRIANGLES*/);
public:
	void DrawStaticLegacy(bool bind, bool bindPosMat) const;
	void DrawStaticLegacyRec() const;

	void CreateShatterPieces();
	void Shatter(float pieceChance, int modelType, int texType, int team, const float3 pos, const float3 speed, const CMatrix44f& m) const;

	void SetPieceTransform(const Transform& parentTra);
	void SetBakedTransform(const Transform& tra) {
		if (tra.IsIdentity())
			bakedTransform = std::nullopt;
		else
			bakedTransform = tra;
	}

	Transform ComposeTransform(const float3& t, const float3& r, float s) const;

	void SetCollisionVolume(const CollisionVolume& cv) { colvol = cv; }
	const CollisionVolume* GetCollisionVolume() const { return &colvol; }

	bool HasGeometryData() const { return relIndxCnt >= 3; }
	void SetParentModel(S3DModel* model_) { model = model_; }
	const S3DModel* GetParentModel() const { return model; }

	bool HasBackedTra() const { return bakedTransform.has_value(); }

	void SetGlobalOffset();
private:
	void CreateShatterPiecesVariation(int num);
	void DrawStaticLegacyRecImpl() const;
	CollisionVolume colvol;
public:
	std::string name;
	std::vector<S3DModelPiece*> children;
	std::array<S3DModelPiecePart, S3DModelPiecePart::SHATTER_VARIATIONS> shatterParts;

	S3DModelPiece* parent = nullptr;

	// bind-pose transform, including baked rots
	Transform bposeTransform;
	// inverse of bposeTransform, cached for delta transform calculations
	Transform bposeTransformInv;

	// baked local-space rotations
	std::optional<Transform> bakedTransform;

	float3 emitPos = ZeroVector;
	float3 emitDir = ZeroVector;

	float3 offset;      /// local (piece-space) offset wrt. parent piece
	float3 goffset;     /// global (model-space) offset wrt. root piece
	float scale{1.0f};  /// baked uniform scaling factor (assimp-only)

	AABB aabb;

	// Relative offset/count within model's VBO allocation
	// Absolute index offset = model.indxStart + piece.relIndxOff
	uint32_t relVertOff = ~0u;
	uint32_t relVertCnt = 0;
	uint32_t relIndxOff = ~0u;
	uint32_t relIndxCnt = 0;

	// Temporary vertex and index data, cleared after upload to GPU
	std::vector<SVertexData> tmpVerts;
	std::vector<uint32_t> tmpIndcs;
	std::vector<uint32_t> tmpShIndcs;

	S3DModel* model = nullptr;
};