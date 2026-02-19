#include "3DModelPiece.hpp"

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

// only used by projectiles with the PF_Recursive flag
void S3DModelPiece::DrawStaticLegacyRec() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	S3DModelHelpers::BindLegacyAttrVBOs();

	DrawStaticLegacy(false, false);

	for (const S3DModelPiece* childPiece : children) {
		childPiece->DrawStaticLegacy(false, false);
	}

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