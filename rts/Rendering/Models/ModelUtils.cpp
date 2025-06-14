#include "ModelUtils.h"

#include <cassert>
#include <string>
#include <numeric>

#include "3DModelLog.h"
#include "System/Misc/TracyDefs.h"
#include "Lua/LuaParser.h"

using namespace Skinning;

uint16_t Skinning::GetBoneID(const SVertexData& vert, size_t wi)
{
	return vert.boneIDsLow[wi] | (vert.boneIDsHigh[wi] << 8);
};

void Skinning::ReparentMeshesTrianglesToBones(S3DModel* model, const std::vector<SkinnedMesh>& meshes)
{
	RECOIL_DETAILED_TRACY_ZONE;

	boneWeights.resize(INV_PIECE_NUM + 1);

	for (const auto& mesh : meshes) {
		const auto& verts = mesh.verts;
		const auto& indcs = mesh.indcs;

		for (size_t trID = 0; trID < indcs.size() / 3; ++trID) {

			std::fill(boneWeights.begin(), boneWeights.end(), 0);
			for (size_t vi = 0; vi < 3; ++vi) {
				const auto& vert = verts[indcs[trID * 3 + vi]];

				for (size_t wi = 0; wi < 4; ++wi) {
					boneWeights[GetBoneID(vert, wi)] += vert.boneWeights[wi];
				}
			}

			const auto maxWeightedBoneID = std::distance(
				boneWeights.begin(),
				std::max_element(boneWeights.begin(), boneWeights.end())
			);
			assert(maxWeightedBoneID < INV_PIECE_NUM); // INV_PIECE_NUM - invalid bone

			auto* maxWeightedPiece = model->pieceObjects[maxWeightedBoneID];

			auto& pieceVerts = maxWeightedPiece->GetVerticesVec();
			auto& pieceIndcs = maxWeightedPiece->GetIndicesVec();

			for (size_t vi = 0; vi < 3; ++vi) {
				auto  targVert = verts[indcs[trID * 3 + vi]]; //copy

				// find if targVert is already added
				auto itTargVec = std::find_if(pieceVerts.begin(), pieceVerts.end(), [&targVert](const auto& vert) {
					return targVert.pos.equals(vert.pos) && targVert.normal.equals(vert.normal);
				});

				// new vertex
				if (itTargVec == pieceVerts.end()) {
					// make sure maxWeightedBoneID comes first. It's a must, even if it doesn't exist in targVert.boneIDs!
					const auto boneID0 = GetBoneID(targVert, 0);
					if (boneID0 != maxWeightedBoneID) {
						size_t itPos = 0;
						for (size_t jj = 1; jj < targVert.boneIDsLow.size(); ++jj) {
							if (GetBoneID(targVert, jj) == maxWeightedBoneID) {
								itPos = jj;
								break;
							}
						}
						if (itPos != 0) {
							// swap maxWeightedBoneID so it comes first in the boneIDs array
							std::swap(targVert.boneIDsLow[0], targVert.boneIDsLow[itPos]);
							std::swap(targVert.boneWeights[0], targVert.boneWeights[itPos]);
							std::swap(targVert.boneIDsHigh[0], targVert.boneIDsHigh[itPos]);
						}
						else {
							// maxWeightedBoneID doesn't even exist in this targVert
							// replace the bone with the least weight with maxWeightedBoneID and swap it be first
							targVert.boneIDsLow[3]  = static_cast<uint8_t>((maxWeightedBoneID     ) & 0xFF);
							targVert.boneWeights[3] = 0;
							targVert.boneIDsHigh[3] = static_cast<uint8_t>((maxWeightedBoneID >> 8) & 0xFF);
							std::swap(targVert.boneIDsLow[0], targVert.boneIDsLow[3]);
							std::swap(targVert.boneWeights[0], targVert.boneWeights[3]);
							std::swap(targVert.boneIDsHigh[0], targVert.boneIDsHigh[3]);

							// renormalize weights (optional but nice for debugging)
							const float sumWeights = static_cast<float>(std::reduce(targVert.boneWeights.begin(), targVert.boneWeights.end())) / 255.0;
							for (auto& bw : targVert.boneWeights) {
								bw = static_cast<uint8_t>(math::round(static_cast<float>(bw) / 255.0f / sumWeights));
							}
						}
					}

					pieceIndcs.emplace_back(static_cast<uint32_t>(pieceVerts.size()));
					pieceVerts.emplace_back(std::move(targVert));
				}
				else {
					pieceIndcs.emplace_back(static_cast<uint32_t>(std::distance(
						pieceVerts.begin(),
						itTargVec
					)));
				}
			}
		}
	}

	// transform model space mesh vertices into bone/piece space
	for (auto* piece : model->pieceObjects) {
		if (!piece->HasGeometryData())
			continue;

		if (piece->bposeTransform.IsIdentity())
			continue;

		const auto invTra = piece->bposeTransform.InvertAffineNormalized();

		for (auto& vert : piece->GetVerticesVec()) {
			vert.TransformBy(invTra);
		}
	}
}

void Skinning::ReparentCompleteMeshesToBones(S3DModel* model, const std::vector<SkinnedMesh>& meshes) {
	RECOIL_DETAILED_TRACY_ZONE;

	boneWeights.resize(INV_PIECE_NUM + 1);

	for (const auto& mesh : meshes) {
		const auto& verts = mesh.verts;
		const auto& indcs = mesh.indcs;

		std::fill(boneWeights.begin(), boneWeights.end(), 0);
		for (const auto& vert : verts) {
			for (size_t wi = 0; wi < 4; ++wi) {
				boneWeights[GetBoneID(vert, wi)] += vert.boneWeights[wi];
			}
		}
		const auto maxWeightedBoneID = std::distance(
			boneWeights.begin(),
			std::max_element(boneWeights.begin(), boneWeights.end())
		);
		assert(maxWeightedBoneID < INV_PIECE_NUM); // INV_PIECE_NUM - invalid bone

		auto* maxWeightedPiece = model->pieceObjects[maxWeightedBoneID];

		auto& pieceVerts = maxWeightedPiece->GetVerticesVec();
		auto& pieceIndcs = maxWeightedPiece->GetIndicesVec();
		const auto indexOffset = static_cast<uint32_t>(pieceVerts.size());

		for (auto targVert : verts) { // deliberate copy
			// Unlike ReparentMeshesTrianglesToBones() do not check for already existing vertices
			// Just copy mesh as is. Modelers and assimp should have done necessary dedup for us.

			// make sure maxWeightedBoneID comes first. It's a must, even if it doesn't exist in targVert.boneIDs!
			const auto boneID0 = GetBoneID(targVert, 0);
			if (boneID0 != maxWeightedBoneID) {
				size_t itPos = 0;
				for (size_t jj = 1; jj < targVert.boneIDsLow.size(); ++jj) {
					if (GetBoneID(targVert, jj) == maxWeightedBoneID) {
						itPos = jj;
						break;
					}
				}
				if (itPos != 0) {
					// swap maxWeightedBoneID so it comes first in the boneIDs array
					std::swap(targVert.boneIDsLow[0], targVert.boneIDsLow[itPos]);
					std::swap(targVert.boneWeights[0], targVert.boneWeights[itPos]);
					std::swap(targVert.boneIDsHigh[0], targVert.boneIDsHigh[itPos]);
				}
				else {
					// maxWeightedBoneID doesn't even exist in this targVert
					// replace the bone with the least weight with maxWeightedBoneID and swap it be first
					targVert.boneIDsLow[3] = static_cast<uint8_t>((maxWeightedBoneID) & 0xFF);
					targVert.boneWeights[3] = 0;
					targVert.boneIDsHigh[3] = static_cast<uint8_t>((maxWeightedBoneID >> 8) & 0xFF);
					std::swap(targVert.boneIDsLow[0], targVert.boneIDsLow[3]);
					std::swap(targVert.boneWeights[0], targVert.boneWeights[3]);
					std::swap(targVert.boneIDsHigh[0], targVert.boneIDsHigh[3]);

					// renormalize weights (optional but nice for debugging)
					const float sumWeights = static_cast<float>(std::reduce(targVert.boneWeights.begin(), targVert.boneWeights.end())) / 255.0;
					for (auto& bw : targVert.boneWeights) {
						bw = static_cast<uint8_t>(math::round(static_cast<float>(bw) / 255.0f / sumWeights));
					}
				}
			}

			pieceVerts.emplace_back(std::move(targVert));
		}

		for (const auto indx : indcs) {
			pieceIndcs.emplace_back(indexOffset + indx);
		}
	}

	// transform model space mesh vertices into bone/piece space
	for (auto* piece : model->pieceObjects) {
		if (!piece->HasGeometryData())
			continue;

		if (piece->bposeTransform.IsIdentity())
			continue;

		const auto invTra = piece->bposeTransform.InvertAffineNormalized();

		for (auto& vert : piece->GetVerticesVec()) {
			vert.TransformBy(invTra);
		}
	}
}

void ModelUtils::CalculateModelDimensions(S3DModel* model, S3DModelPiece* piece)
{
	// TODO fix
	const CMatrix44f scaleRotMat = piece->ComposeTransform(ZeroVector, ZeroVector, piece->scale).ToMatrix();

	// cannot set this until parent relations are known, so either here or in BuildPieceHierarchy()
	piece->goffset = scaleRotMat.Mul(piece->offset) + ((piece->parent != nullptr) ? piece->parent->goffset : ZeroVector);

	// update model min/max extents
	model->mins = float3::min(piece->goffset + piece->mins, model->mins);
	model->maxs = float3::max(piece->goffset + piece->maxs, model->maxs);

	piece->SetCollisionVolume(CollisionVolume('b', 'z', piece->maxs - piece->mins, (piece->maxs + piece->mins) * 0.5f));

	// Repeat with children
	for (S3DModelPiece* childPiece : piece->children) {
		CalculateModelDimensions(model, childPiece);
	}
}

void ModelUtils::CalculateModelProperties(S3DModel* model, const LuaTable& modelTable)
{
	RECOIL_DETAILED_TRACY_ZONE;

	model->UpdatePiecesMinMaxExtents();
	CalculateModelDimensions(model, model->GetRootPiece());

	model->mins = modelTable.GetFloat3("mins", model->mins);
	model->maxs = modelTable.GetFloat3("maxs", model->maxs);

	model->radius = modelTable.GetFloat("radius", model->CalcDrawRadius());
	model->height = modelTable.GetFloat("height", model->CalcDrawHeight());

	model->relMidPos = modelTable.GetFloat3("midpos", model->CalcDrawMidPos());
}

void ModelUtils::GetModelParams(const LuaTable& modelTable, ModelParams& modelParams)
{
	RECOIL_DETAILED_TRACY_ZONE;

	auto CondGetLuaValue = [&modelTable]<typename T>(std::optional<T>& value, const std::string& key) {
		if (!modelTable.KeyExists(key))
			return;

		value = modelTable.Get(key, T{});
	};

	CondGetLuaValue(modelParams.mins, "mins");
	CondGetLuaValue(modelParams.maxs, "maxs");

	CondGetLuaValue(modelParams.relMidPos, "midpos");

	CondGetLuaValue(modelParams.radius, "radius");
	CondGetLuaValue(modelParams.height, "height");

	CondGetLuaValue(modelParams.flipTextures, "fliptextures");
	CondGetLuaValue(modelParams.invertTeamColor, "invertteamcolor");
	CondGetLuaValue(modelParams.s3oCompat, "s3ocompat");
}

void ModelUtils::ApplyModelProperties(S3DModel* model, const ModelParams& modelParams)
{
	RECOIL_DETAILED_TRACY_ZONE;

	model->UpdatePiecesMinMaxExtents();
	CalculateModelDimensions(model, model->GetRootPiece());

	// Note the content from Lua table will overwrite whatever has already been defined in modelParams

	model->mins = modelParams.mins.value_or(model->mins);
	model->maxs = modelParams.maxs.value_or(model->maxs);

	// must come after mins / maxs assignment
	model->relMidPos = modelParams.relMidPos.value_or(model->CalcDrawMidPos());

	model->radius = modelParams.radius.value_or(model->CalcDrawRadius());
	model->height = modelParams.height.value_or(model->CalcDrawHeight());
}

void ModelUtils::CalculateNormals(std::vector<SVertexData>& verts, const std::vector<uint32_t>& indcs)
{
	if (indcs.size() < 3)
		return;

	// set the triangle-level S- and T-tangents
	for (size_t i = 0, n = indcs.size(); i < n; i += 3) {

		const auto& v0idx = indcs[i + 0];
		const auto& v1idx = indcs[i + 1];
		const auto& v2idx = indcs[i + 2];

		if (v1idx == INVALID_INDEX || v2idx == INVALID_INDEX) {
			// not a valid triangle, skip
			i += 3; continue;
		}

		auto& v0 = verts[v0idx];
		auto& v1 = verts[v1idx];
		auto& v2 = verts[v2idx];

		const auto& p0 = v0.pos;
		const auto& p1 = v1.pos;
		const auto& p2 = v2.pos;

		const auto p10 = p1 - p0;
		const auto p20 = p2 - p0;

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

	// set the triangle-level S- and T-tangents
	for (size_t i = 0, n = indcs.size(); i < n; i += 3) {

		const auto& v0idx = indcs[i + 0];
		const auto& v1idx = indcs[i + 1];
		const auto& v2idx = indcs[i + 2];

		if (v1idx == INVALID_INDEX || v2idx == INVALID_INDEX) {
			// not a valid triangle, skip
			i += 3; continue;
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
		const float r = (abs(d) < 1e-9f) ? 1.0f : 1.0f / d;

		// note: not necessarily orthogonal to each other
		// or to vertex normal, only to the triangle plane
		const auto sdir = ( p10 * tc20.y - p20 * tc10.y) * r;
		const auto tdir = (-p10 * tc20.x + p20 * tc10.x) * r;

		v0.sTangent += sdir;
		v1.sTangent += sdir;
		v2.sTangent += sdir;

		v0.tTangent += tdir;
		v1.tTangent += tdir;
		v2.tTangent += tdir;
	}

	// set the smoothed per-vertex tangents
	for (size_t i = 0, n = verts.size(); i < n; i++) {
		float3& N = verts[i].normal;
		float3& T = verts[i].sTangent;
		float3& B = verts[i].tTangent; // bi

		N.AssertNaNs(); N.SafeANormalize();
		T.AssertNaNs();
		B.AssertNaNs();

		//const float bitangentAngle = B.dot(N.cross(T)); // dot(B,B')
		//const float handednessSign = Sign(bitangentAngle);

		T = (T - N * N.dot(T));// *handednessSign;
		T.SafeANormalize();

		B = (B - N * N.dot(B) - T * T.dot(N));
		//B = N.cross(T); //probably better
		B.SafeANormalize();
	}
}

void ModelLog::LogModelProperties(const S3DModel& model)
{
	// Verbose logging of model properties
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->name: %s", model.name.c_str());
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->numobjects: %d", model.numPieces);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->radius: %f", model.radius);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->height: %f", model.height);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->mins: (%f,%f,%f)", model.mins[0], model.mins[1], model.mins[2]);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->maxs: (%f,%f,%f)", model.maxs[0], model.maxs[1], model.maxs[2]);
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Model %s Imported.", model.name.c_str());
}
