#include "ModelUtils.h"

#include <ranges>
#include <cassert>
#include <string>
#include <numeric>

#include "3DModelLog.h"
#include "3DModelDefs.hpp"
#include "3DModel.hpp"
#include "3DModelPiece.hpp"
#include "System/ContainerUtil.h"
#include "System/Misc/TracyDefs.h"
#include "Lua/LuaParser.h"

void ModelUtils::CalculateModelProperties(S3DModel* model)
{
	RECOIL_DETAILED_TRACY_ZONE;

	model->FinalizeLoad();

	const auto& modelParams = model->modelParams;

	// Note the content from Lua table will overwrite whatever has already been defined in modelParams
	model->aabb.mins = modelParams.mins.value_or(model->aabb.mins);
	model->aabb.maxs = modelParams.maxs.value_or(model->aabb.maxs);

	// must come after mins / maxs assignment
	model->relMidPos = modelParams.relMidPos.value_or(model->CalcDrawMidPos());

	model->radius = modelParams.radius.value_or(model->CalcDrawRadius());
	model->height = modelParams.height.value_or(model->CalcDrawHeight());
}

void ModelUtils::GetModelParams(const LuaTable& modelTable, ModelParams& modelParams)
{
	RECOIL_DETAILED_TRACY_ZONE;

	auto CondGetLuaValue = [&modelTable]<typename T>(std::optional<T>& value, const std::string& key) {
		if (!modelTable.KeyExists(key))
			return;

		value = modelTable.Get(key, T{});
	};

	CondGetLuaValue(modelParams.texs[0], "tex1");
	CondGetLuaValue(modelParams.texs[1], "tex2");

	CondGetLuaValue(modelParams.mins, "mins");
	CondGetLuaValue(modelParams.maxs, "maxs");

	CondGetLuaValue(modelParams.relMidPos, "midpos");

	CondGetLuaValue(modelParams.radius, "radius");
	CondGetLuaValue(modelParams.height, "height");

	CondGetLuaValue(modelParams.flipTextures, "fliptextures");
	CondGetLuaValue(modelParams.invertTeamColor, "invertteamcolor");
	CondGetLuaValue(modelParams.s3oCompat, "s3ocompat");
}

void ModelUtils::TransferPiecesToSkinnedMesh(S3DModel* model)
{
	RECOIL_DETAILED_TRACY_ZONE;

	// Reserve space
	{
		const auto totalVerts = std::ranges::fold_left(
			model->pieceObjects, model->skinnedVerts.size(),
			[](auto acc, auto* piece) {
				return acc + piece->tmpVerts.size();
			}
		);
		const auto totalIndcs = std::ranges::fold_left(
			model->pieceObjects, model->skinnedIndcs.size(),
			[](auto acc, auto* piece) {
				return acc + piece->tmpIndcs.size();
			}
		);

		model->skinnedVerts.reserve(totalVerts);
		model->skinnedIndcs.reserve(totalIndcs);
		model->shatterIndcs.reserve(totalIndcs * S3DModelPiecePart::SHATTER_VARIATIONS);
	}

	for (size_t pieceIdx = 0; pieceIdx < model->pieceObjects.size(); ++pieceIdx) {
		auto* piece = model->pieceObjects[pieceIdx];

		auto& pieceVerts = piece->tmpVerts;
		auto& pieceIndcs = piece->tmpIndcs;

		// Record count for this piece in skinnedVerts/skinnedIndcs, which will be used for piece->HasGeometryData() and piece->SetEmitters()
		piece->relVertCnt = static_cast<uint32_t>(pieceVerts.size());
		piece->relIndxCnt = static_cast<uint32_t>(pieceIndcs.size());

		piece->SetEmitters();

		if (!piece->HasGeometryData())
			continue;

		// Create shatter pieces first - this populates piece->tmpShIndcs
		piece->CreateShatterPieces();

		// Record relative offset for this piece in skinnedVerts/skinnedIndcs/shatterIndcs
		piece->relVertOff = static_cast<uint32_t>(model->skinnedVerts.size());
		piece->relIndxOff = static_cast<uint32_t>(model->skinnedIndcs.size());

		auto& pieceShIndcs = piece->tmpShIndcs;

		// Transform verts
		for (const auto& vert : pieceVerts) {
			auto& transformedVertex = model->skinnedVerts.emplace_back(vert.TransformBy(piece->bposeTransform));
			if (transformedVertex.boneIDs == SVertexData::DEFAULT_BONEIDS) {
				assert(transformedVertex.boneWeights == SVertexData::DEFAULT_BONEWEIGHTS);
				// Assign pieceIndex to first bone ID
				transformedVertex.boneIDs[0] = static_cast<uint16_t>(pieceIdx);
			}
		}

		// Copy and adjust indices
		spring::AppendRange(model->skinnedIndcs,
			pieceIndcs | std::views::transform([baseVertNum = piece->relVertOff](auto v) { return baseVertNum + v; }) // add baseVertNum to indices
		);

		// Copy and adjust shatter indices
		spring::AppendRange(model->shatterIndcs,
			pieceShIndcs | std::views::transform([baseVertNum = piece->relVertOff](auto v) { return baseVertNum + v; }) // add baseVertNum to indices
		);

		// makes no sense to keep them?
		pieceVerts = {};
		pieceIndcs = {};
		pieceShIndcs = {};
	}
}

void ModelUtils::CheckNormalAndTangent(const S3DModel* model)
{
	RECOIL_DETAILED_TRACY_ZONE;

	const auto& verts = model->skinnedVerts;

	uint32_t numBadNormals = 0;
	uint32_t numBadTangents = 0;

	static constexpr float sqThreshold = Square(0.9f);

	for (const auto& vert : verts) {
		numBadNormals  += (vert.normal.SqLength()  < sqThreshold);
		numBadTangents += (vert.tangent.SqLength() < sqThreshold);
	}

	if (numBadNormals > 0 || numBadTangents > 0) {
		LOG_L(L_DEBUG, "[%s] model \"%s\" has %u vertices, %u with invalid normals, %u with invalid tangents",
			__func__, model->name.c_str(), static_cast<uint32_t>(verts.size()), numBadNormals, numBadTangents);
	}
}

void ModelUtils::CalculateNormals(std::vector<SVertexData>& verts, const std::vector<uint32_t>& indcs)
{
	if (indcs.size() < 3)
		return;

	for (size_t i = 0, n = indcs.size(); i < n; i += 3) {

		const auto& v0idx = indcs[i + 0];
		const auto& v1idx = indcs[i + 1];
		const auto& v2idx = indcs[i + 2];

		if (v0idx == INVALID_INDEX || v1idx == INVALID_INDEX || v2idx == INVALID_INDEX) {
			// not a valid triangle, skip
			continue;
		}

		auto& v0 = verts[v0idx];
		auto& v1 = verts[v1idx];
		auto& v2 = verts[v2idx];

		const auto& p0 = v0.pos;
		const auto& p1 = v1.pos;
		const auto& p2 = v2.pos;

		const auto p10 = p1 - p0;
		const auto p20 = p2 - p0;

		//not normalized on purpose, so a bigger triangle contributes more to the normal than a smaller one
		const auto N = p10.cross(p20);

		v0.normal += N;
		v1.normal += N;
		v2.normal += N;
	}

	// set the smoothed per-vertex tangents
	for (size_t i = 0, n = verts.size(); i < n; i++) {
		float3& N = verts[i].normal;

		N.AssertNaNs();

		const float sql = N.SqLength();
		if likely(N.CheckNaNs() && sql > float3::nrm_eps())
			N *= math::isqrt(sql);
		else
			N = float3{ 0.0f, 1.0f, 0.0f };
	}
}

void ModelUtils::CalculateTangents(std::vector<SVertexData>& verts, const std::vector<uint32_t>& indcs)
{
	if (indcs.size() < 3)
		return;

	std::vector<float3> tTangent(verts.size(), float3{});
	// set the triangle-level S- and T-tangents
	for (size_t i = 0, n = indcs.size(); i < n; i += 3) {

		const auto& v0idx = indcs[i + 0];
		const auto& v1idx = indcs[i + 1];
		const auto& v2idx = indcs[i + 2];

		if (v1idx == INVALID_INDEX || v2idx == INVALID_INDEX) {
			// not a valid triangle, skip
			continue;
		}

		auto& v0 = verts[v0idx];
		auto& v1 = verts[v1idx];
		auto& v2 = verts[v2idx];

		const auto& p0 = v0.pos;
		const auto& p1 = v1.pos;
		const auto& p2 = v2.pos;

		const auto& tc0 = v0.texCoords[0];
		const auto& tc1 = v1.texCoords[0];
		const auto& tc2 = v2.texCoords[0];

		const auto p10 = p1 - p0;
		const auto p20 = p2 - p0;

		const auto tc10 = tc1 - tc0;
		const auto tc20 = tc2 - tc0;

		// if d is 0, texcoors are degenerate
		const float d = (tc10.x * tc20.y - tc20.x * tc10.y);
		if (math::fabsf(d) < 1e-9)
			continue; // garbage, skip it

		const float r = 1.0f / d;
		// note: not necessarily orthogonal to each other
		// or to vertex normal, only to the triangle plane
		const auto sdir = ( p10 * tc20.y - p20 * tc10.y) * r;
		const auto tdir = (-p10 * tc20.x + p20 * tc10.x) * r;

		v0.tangent += sdir;
		v1.tangent += sdir;
		v2.tangent += sdir;

		tTangent[v0idx] += tdir;
		tTangent[v1idx] += tdir;
		tTangent[v2idx] += tdir;
	}

	// set the smoothed per-vertex tangents
	for (size_t i = 0, n = verts.size(); i < n; i++) {
		auto& N = verts[i].normal;
		auto& T = verts[i].tangent;
		auto& B = tTangent[i];

		N.AssertNaNs(); N.SafeANormalize();
		T.AssertNaNs();
		B.AssertNaNs();

		// Gram-Schmidt: orthogonalize T against N
		T = (T - N * N.dot(T));
		T.SafeANormalize();

		const float handednessSign = Sign(B.dot(N.cross(T)));
		T.w = handednessSign;
	}
}

void ModelLog::LogModelProperties(const S3DModel& model)
{
	// Verbose logging of model properties
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->name: %s", model.name.c_str());
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->numobjects: %d", model.numPieces);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->radius: %f", model.radius);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->height: %f", model.height);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->mins: (%f,%f,%f)", model.aabb.mins[0], model.aabb.mins[1], model.aabb.mins[2]);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->maxs: (%f,%f,%f)", model.aabb.maxs[0], model.aabb.maxs[1], model.aabb.maxs[2]);
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Model %s Imported.", model.name.c_str());
}
