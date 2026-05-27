#include "3DModelPiece.hpp"

#include <ranges>

#include "3DModel.hpp"
#include "3DModelVAO.hpp"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Game/GlobalUnsynced.h"
#include "System/Misc/TracyDefs.h"
#include "System/Threading/ThreadPool.h"

/** ****************************************************************************************************
 * S3DModelPiece
 */

void S3DModelPiece::DrawStaticLegacy(bool bind, bool bindPosMat) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!HasGeometryData())
		return;

	if (bind) S3DModelHelpers::BindLegacyAttrVBOs();

	// Note: bindPosMat is now ignored because vertices are already in model space
	// The bposeTransform was already applied during TransferPiecesToSkinnedMesh()
	// For static rendering, we don't need any additional transform
	DrawElements();

	if (bind) S3DModelHelpers::UnbindLegacyAttrVBOs();
}

void S3DModelPiece::DrawStaticLegacyRecImpl() const
{
	// Vertices are already in model space
	// so no per-piece transform is needed — just draw and recurse into children.
	DrawElements();

	for (const S3DModelPiece* childPiece : children) {
		childPiece->DrawStaticLegacyRecImpl();
	}
}

// only used by projectiles with the PF_Recursive flag
void S3DModelPiece::DrawStaticLegacyRec() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	S3DModelHelpers::BindLegacyAttrVBOs();
	DrawStaticLegacyRecImpl();
	S3DModelHelpers::UnbindLegacyAttrVBOs();
}

void S3DModelPiece::CreateShatterPieces()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (tmpIndcs.size() < 3 || tmpIndcs.size() % 3 != 0)
		return;

	tmpShIndcs.reserve(S3DModelPiecePart::SHATTER_VARIATIONS * tmpIndcs.size());
	for (int i = 0; i < S3DModelPiecePart::SHATTER_VARIATIONS; ++i) {
		CreateShatterPiecesVariation(i);
	}
}

void S3DModelPiece::SetGlobalOffset()
{
	// Calculate goffset (global offset from root piece)
	// Note: goffset only captures translation, not rotation/scale
	// Model bounds are calculated separately using bposeTransform in S3DModel::CalcModelBounds()
	const CMatrix44f scaleRotMat = ComposeTransform(ZeroVector, ZeroVector, scale).ToMatrix();
	goffset = scaleRotMat.Mul(offset) + (parent != nullptr ? parent->goffset : ZeroVector);
}

void S3DModelPiece::CreateShatterPiecesVariation(int num)
{
	RECOIL_DETAILED_TRACY_ZONE;
	using ShatterPartDataPair = std::pair<S3DModelPiecePart::RenderData, std::vector<uint32_t>>;
	using ShatterPartsBuffer  = std::array<ShatterPartDataPair, S3DModelPiecePart::SHATTER_MAX_PARTS>;

	ShatterPartsBuffer shatterPartsBuf;

	auto& gutRNG = guThreadRNGs[ThreadPool::GetThreadNum()];
	// Assign a random "fly direction" to each shatter part
	for (auto& [rd, idcs] : shatterPartsBuf) {
		rd.dir = (gutRNG.NextVector()).ANormalize();
	}

	const auto GetPolygonDir = [&](uint32_t i0, uint32_t i1, uint32_t i2) -> float3 {
		const auto& v0 = tmpVerts[i0];
		const auto& v1 = tmpVerts[i1];
		const auto& v2 = tmpVerts[i2];

		float3 N;
		N += v0.normal;
		N += v1.normal;
		N += v2.normal;

		if likely(const auto SqL = N.SqLength(); SqL >= float3::apx_eps())
			return N *= math::isqrt(SqL);

		const auto p10 = v1.pos - v0.pos;
		const auto p20 = v2.pos - v0.pos;

		// not normalized so bigger triangles have more influence on shatter part assignment
		return p10.cross(p20);
	};

	namespace rv = std::ranges::views;

	// We iterate over every triangle (3 consecutive indices = 1 triangle).
	// For each triangle, we find the shatter part whose random direction is
	// most aligned with the triangle's facing direction (highest dot product).
	// This ensures triangles on the "left side" of a model go to a left-flying
	// fragment, right-side triangles to a right-flying fragment, etc.
	for (auto&& chunk : tmpIndcs | rv::chunk(3)) {
		// actually transform it into the model space, so different pieces experience consistent shattering
		const auto dir = bposeTransform * GetPolygonDir(chunk[0], chunk[1], chunk[2]);

		const auto best = std::ranges::max_element(
			shatterPartsBuf,
			std::less{},
			[&dir](const ShatterPartDataPair& cp) {
				return cp.first.dir.dot(dir);
			}
		);

		assert(best != shatterPartsBuf.end());
		std::ranges::copy(chunk, std::back_inserter(best->second));
	}

	// Now that triangles are distributed, we calculate where each part's indices
	// live in tmpShIndcs
	// The offset `num * mapSize` ensures different variations don't overlap.
	{
		const size_t mapSize = tmpIndcs.size();

		uint32_t indxPos = 0;

		for (auto& [rd, idcs] : shatterPartsBuf) {
			rd.indexCount = static_cast<uint32_t>(idcs.size());
			rd.indexStart = static_cast<uint32_t>(num * mapSize) + indxPos;
			tmpShIndcs.insert(tmpShIndcs.end(), idcs.begin(), idcs.end());
			indxPos += rd.indexCount;
		}
	}

	// partition() moves all non-empty parts to the front and returns a subrange
	const auto emptyBegin = std::ranges::partition(
		shatterPartsBuf,
		[](const ShatterPartDataPair& p) { return p.first.indexCount > 0; }
	);
	const auto validRange = std::ranges::subrange(shatterPartsBuf.begin(), emptyBegin.begin());

	// Copy the render metadata (direction, index range) of all non-empty parts
	// into shatterParts[num], which is what the renderer will actually use.
	shatterParts[num].renderData.clear();
	shatterParts[num].renderData.reserve(validRange.size());

	for (const auto& [rd, idcs] : validRange) {
		shatterParts[num].renderData.push_back(rd);
	}
}


void S3DModelPiece::Shatter(float pieceChance, int modelType, int texType, int team, const float3 pos, const float3 speed, const CMatrix44f& m) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float2  pieceParams = { aabb.CalcRadius(), pieceChance };
	const   int2 renderParams = {texType, team};

	projectileHandler.AddFlyingPiece(modelType, this, m, pos, speed, pieceParams, renderParams);
}

void S3DModelPiece::SetPieceTransform(const Transform& parentTra)
{
	bposeTransform = parentTra * ComposeTransform(offset, ZeroVector, scale);
	bposeTransformInv = bposeTransform.InvertAffine();

	for (S3DModelPiece* c : children) {
		c->SetPieceTransform(bposeTransform);
	}
}

Transform S3DModelPiece::ComposeTransform(const float3& t, const float3& r, float s) const
{
	// NOTE:
	//   ORDER MATTERS (T(baked + script) * R(baked) * R(script) * S(baked))
	//   translating + rotating + scaling is faster than matrix-multiplying
	//   m is identity so m.SetPos(t)==m.Translate(t) but with fewer instrs
	Transform tra;
	tra.t = t;

	if (bakedTransform.has_value())
		tra *= bakedTransform.value();

	tra *= Transform(CQuaternion::FromEulerYPRNeg(-r), ZeroVector, s);
	return tra;
}

void S3DModelPiece::SetEmitters()
{
	const auto vertCount = tmpVerts.size();
	if (vertCount == 0) {
		emitPos = ZeroVector;
		emitDir = FwdVector;
	}
	else if (vertCount == 1) {
		emitPos = ZeroVector;
		emitDir = tmpVerts[0].pos;
	}
	else {
		emitPos = tmpVerts[0].pos;
		emitDir = tmpVerts[1].pos - tmpVerts[0].pos;
	}
	emitDir.SafeANormalize();
}

void S3DModelPiece::DrawElements(GLuint prim) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (relIndxCnt == 0)
		return;
	assert(relIndxCnt != ~0u);
	assert(model != nullptr);

	// Use model's global VBO offset + piece's relative offset
	S3DModelVAO::GetInstance().DrawElements(prim, model->indxStart + relIndxOff, relIndxCnt);
}

void S3DModelPiece::DrawShatterElements(uint32_t vboIndxStart, uint32_t vboIndxCount, GLuint prim)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (vboIndxCount == 0)
		return;

	S3DModelVAO::GetInstance().DrawElements(prim, vboIndxStart, vboIndxCount);
}