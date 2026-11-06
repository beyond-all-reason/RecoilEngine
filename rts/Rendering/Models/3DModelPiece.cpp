#include "3DModelPiece.hpp"

#include <cmath>
#include <algorithm>
#include "System/UnorderedMap.hpp"

#include "3DModelVAO.hpp"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Game/GlobalUnsynced.h"
#include "System/Misc/TracyDefs.h"

/** ****************************************************************************************************
 * S3DModelPiece
 */

void S3DModelPiece::DrawStaticLegacy(bool bind, bool bindPosMat) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!HasGeometryData())
		return;

	if (bind) S3DModelHelpers::BindLegacyAttrVBOs();

	if (bindPosMat) {
		glPushMatrix();
		glMultMatrixf(bposeTransform.ToMatrix());
		DrawElements();
		glPopMatrix();
	}
	else {
		DrawElements();
	}

	if (bind) S3DModelHelpers::UnbindLegacyAttrVBOs();
}

void S3DModelPiece::DrawStaticLegacyRecImpl(const float3& rootT) const
{
	// Build a per-piece transform that correctly positions it relative to the root piece:
	//   rotation + scale  come from this piece's own accumulated bposeTransform
	//   translation       is the offset from the root piece in model space (bpose.t - rootT),
	//                     so the chunk flies as a connected rigid body centered at drawPos.
	const CMatrix44f relMat = Transform(bposeTransform.r, bposeTransform.t - rootT, bposeTransform.s).ToMatrix();
	glPushMatrix();
	glMultMatrixf(relMat);
	DrawElements();
	glPopMatrix();

	for (const S3DModelPiece* childPiece : children) {
		childPiece->DrawStaticLegacyRecImpl(rootT);
	}
}

// only used by projectiles with the PF_Recursive flag
void S3DModelPiece::DrawStaticLegacyRec() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	S3DModelHelpers::BindLegacyAttrVBOs();
	DrawStaticLegacyRecImpl(bposeTransform.t);
	S3DModelHelpers::UnbindLegacyAttrVBOs();
}


float3 S3DModelPiece::GetEmitPos() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	switch (vertices.size()) {
		case 0:
		case 1: { return ZeroVector; } break;
		default: { return GetVertexPos(0); } break;
	}
}

float3 S3DModelPiece::GetEmitDir() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	switch (vertices.size()) {
		case 0: { return FwdVector; } break;
		case 1: { return GetVertexPos(0); } break;
		default: { return (GetVertexPos(1) - GetVertexPos(0)); } break;
	}
}


void S3DModelPiece::CreateShatterPieces()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!HasGeometryData())
		return;

	shatterIndices.reserve(S3DModelPiecePart::SHATTER_VARIATIONS * indices.size());

	for (int i = 0; i < S3DModelPiecePart::SHATTER_VARIATIONS; ++i) {
		CreateShatterPiecesVariation(i);
	}
}


void S3DModelPiece::CreateShatterPiecesVariation(int num)
{
	RECOIL_DETAILED_TRACY_ZONE;
	using ShatterPartDataPair = std::pair<S3DModelPiecePart::RenderData, std::vector<uint32_t>>;
	using ShatterPartsBuffer  = std::array<ShatterPartDataPair, S3DModelPiecePart::SHATTER_MAX_PARTS>;

	ShatterPartsBuffer shatterPartsBuf;

	for (auto& [rd, idcs] : shatterPartsBuf) {
		rd.dir = (guRNG.NextVector()).ANormalize();
	}

	// helper
	const auto GetPolygonDir = [&](size_t idx)
	{
		float3 midPos;
		midPos += GetVertexPos(indices[idx + 0]);
		midPos += GetVertexPos(indices[idx + 1]);
		midPos += GetVertexPos(indices[idx + 2]);
		midPos /= 3.0f;
		return midPos.ANormalize();
	};

	// add vertices to splitter parts
	for (size_t i = 0; i < indices.size(); i += 3) {
		const float3& dir = GetPolygonDir(i);

		// find the closest shatter part (the one that points into same dir)
		float md = -2.0f;

		ShatterPartDataPair* mcp = nullptr;
		const S3DModelPiecePart::RenderData* rd = nullptr;

		for (ShatterPartDataPair& cp: shatterPartsBuf) {
			rd = &cp.first;

			if (rd->dir.dot(dir) < md)
				continue;

			md = rd->dir.dot(dir);
			mcp = &cp;
		}

		assert(mcp);

		//  + vertIndex will be added in void S3DModelVAO::ProcessIndicies(S3DModel* model)
		(mcp->second).push_back(indices[i + 0]);
		(mcp->second).push_back(indices[i + 1]);
		(mcp->second).push_back(indices[i + 2]);
	}

	{
		const size_t mapSize = indices.size();

		uint32_t indxPos = 0;

		for (auto& [rd, idcs] : shatterPartsBuf) {
			rd.indexCount = static_cast<uint32_t>(idcs.size());
			rd.indexStart = static_cast<uint32_t>(num * mapSize) + indxPos;

			if (rd.indexCount > 0) {
				shatterIndices.insert(shatterIndices.end(), idcs.begin(), idcs.end());
				indxPos += rd.indexCount;
			}
		}
	}

	{
		// delete empty splitter parts
		size_t backIdx = shatterPartsBuf.size() - 1;

		for (size_t j = 0; j < shatterPartsBuf.size() && j < backIdx; ) {
			const auto& [rd, idcs] = shatterPartsBuf[j];

			if (rd.indexCount == 0) {
				std::swap(shatterPartsBuf[j], shatterPartsBuf[backIdx--]);
				continue;
			}

			j++;
		}

		shatterParts[num].renderData.clear();
		shatterParts[num].renderData.reserve(backIdx + 1);

		// finish: copy buffer to actual memory
		for (size_t n = 0; n <= backIdx; n++) {
			shatterParts[num].renderData.push_back(shatterPartsBuf[n].first);
		}
	}
}


void S3DModelPiece::Shatter(float pieceChance, int modelType, int texType, int team, const float3 pos, const float3 speed, const CMatrix44f& m) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float2  pieceParams = {float3::max(float3::fabs(maxs), float3::fabs(mins)).Length(), pieceChance};
	const   int2 renderParams = {texType, team};

	projectileHandler.AddFlyingPiece(modelType, this, m, pos, speed, pieceParams, renderParams);
}

void S3DModelPiece::SetPieceTransform(const Transform& parentTra)
{
	bposeTransform = parentTra * ComposeTransform(offset, ZeroVector, scale);

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


void S3DModelPiece::BuildCollisionVerts()
{
	RECOIL_DETAILED_TRACY_ZONE;

	collisionVerts.clear();
	closedCollisionMesh = false;

	if (!HasGeometryData())
		return;

	collisionVerts.reserve(vertices.size());

	for (const SVertexData& v : vertices)
		collisionVerts.push_back(v.pos);

	// Is the surface closed? Edges cannot be matched by vertex index: model
	// formats split vertices along texture and normal seams, so a plain cube
	// arrives as six unconnected quads sharing no indices at all. Match by
	// position instead, snapping to a grid first so that coordinates which
	// only differ in the last bits still meet.
	constexpr float WELD_GRID = 1000.0f;   // 1/1000 elmo

	spring::unordered_map<int64_t, uint32_t> weld;
	std::vector<uint32_t> canonical(collisionVerts.size());

	// Exact key, not a hash: two distinct positions must never share one, or
	// unrelated vertices get welded together and the surface is misjudged.
	// 21 bits per axis span +/- 1048 elmo at this grid, well beyond any piece.
	const auto QuantKey = [](const float3& p) -> int64_t {
		constexpr int64_t LIM = (1LL << 20) - 1;

		const int64_t x = std::clamp<int64_t>(std::llround(p.x * WELD_GRID), -LIM, LIM);
		const int64_t y = std::clamp<int64_t>(std::llround(p.y * WELD_GRID), -LIM, LIM);
		const int64_t z = std::clamp<int64_t>(std::llround(p.z * WELD_GRID), -LIM, LIM);

		return ((x & 0x1FFFFF) << 42) | ((y & 0x1FFFFF) << 21) | (z & 0x1FFFFF);
	};

	for (size_t i = 0; i < collisionVerts.size(); i++) {
		const int64_t key = QuantKey(collisionVerts[i]);
		const auto it = weld.find(key);

		if (it != weld.end()) {
			canonical[i] = it->second;
			continue;
		}

		weld[key] = static_cast<uint32_t>(i);
		canonical[i] = static_cast<uint32_t>(i);
	}

	spring::unordered_map<uint64_t, uint32_t> edgeUse;

	for (size_t i = 0; i + 2 < indices.size(); i += 3) {
		const uint32_t ia = indices[i + 0];
		const uint32_t ib = indices[i + 1];
		const uint32_t ic = indices[i + 2];

		if (ia >= canonical.size() || ib >= canonical.size() || ic >= canonical.size())
			return;   // malformed: leave the surface counted as open

		const uint32_t ca = canonical[ia];
		const uint32_t cb = canonical[ib];
		const uint32_t cc = canonical[ic];

		const uint32_t tri[3][2] = {{ca, cb}, {cb, cc}, {cc, ca}};

		for (const auto& e : tri) {
			if (e[0] == e[1])
				return;   // degenerate: not a surface

			const uint64_t lo = std::min(e[0], e[1]);
			const uint64_t hi = std::max(e[0], e[1]);

			edgeUse[(lo << 32) | hi]++;
		}
	}

	if (edgeUse.empty())
		return;

	for (const auto& [edge, uses] : edgeUse) {
		if (uses != 2)
			return;
	}

	closedCollisionMesh = true;
}

void S3DModelPiece::PostProcessGeometry(uint32_t pieceIndex)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!HasGeometryData())
		return;


	for (auto& v : vertices) {
		if (v.boneIDsLow == SVertexData::DEFAULT_BONEIDS_LOW && v.boneIDsHigh == SVertexData::DEFAULT_BONEIDS_HIGH) {
			v.boneIDsLow [0] = static_cast<uint8_t>((pieceIndex     ) & 0xFF);
			v.boneIDsHigh[0] = static_cast<uint8_t>((pieceIndex >> 8) & 0xFF);
		}
	}
}

void S3DModelPiece::DrawElements(GLuint prim) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (indxCount == 0)
		return;
	assert(indxCount != ~0u);

	S3DModelVAO::GetInstance().DrawElements(prim, indxStart, indxCount);
}

void S3DModelPiece::DrawShatterElements(uint32_t vboIndxStart, uint32_t vboIndxCount, GLuint prim)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (vboIndxCount == 0)
		return;

	S3DModelVAO::GetInstance().DrawElements(prim, vboIndxStart, vboIndxCount);
}

void S3DModelPiece::ReleaseShatterIndices()
{
	RECOIL_DETAILED_TRACY_ZONE;
	shatterIndices.clear();
}