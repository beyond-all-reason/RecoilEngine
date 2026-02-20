#include "LocalModel.hpp"

#include "3DModel.hpp"
#include "3DModelPiece.hpp"
#include "3DModelDefs.hpp"
#include "System/Misc/TracyDefs.h"

CR_BIND(LocalModel, )
CR_REG_METADATA(LocalModel, (
	CR_MEMBER(pieces),

	CR_MEMBER(boundingVolume),
	CR_IGNORED(luaMaterialData),
	CR_MEMBER(needsBoundariesRecalc)
))

/** ****************************************************************************************************
 * LocalModel
 */

void LocalModel::DrawPieces() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (const auto& p : pieces) {
		p.Draw();
	}
}

void LocalModel::DrawPiecesLOD(uint32_t lod) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!luaMaterialData.ValidLOD(lod))
		return;

	for (const auto& p: pieces) {
		p.DrawLOD(lod);
	}
}

void LocalModel::SetLODCount(uint32_t lodCount)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(Initialized());

	luaMaterialData.SetLODCount(lodCount);
	pieces[0].SetLODCount(lodCount);
}


void LocalModel::SetModel(const S3DModel* model, bool initialize)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// make sure we do not get called for trees, etc
	assert(model != nullptr);
	assert(model->numPieces >= 1);

	if (!initialize) {
		assert(pieces.size() == model->numPieces);

		// PostLoad; only update the pieces
		for (size_t n = 0; n < pieces.size(); n++) {
			S3DModelPiece* omp = model->GetPiece(n);

			pieces[n].original = omp;
		}

		pieces[0].UpdateChildTransformRec(true);
		UpdateBoundingVolume();
		return;
	}

	assert(pieces.empty());

	pieces.clear();
	pieces.reserve(model->numPieces);

	CreateLocalModelPieces(model->GetRootPiece());

	// must recursively update matrices here too: for features
	// LocalModel::Update is never called, but they might have
	// baked piece rotations (in the case of .dae)
	pieces[0].UpdateChildTransformRec(false);

	for (auto& piece : pieces) {
		piece.SavePrevModelSpaceTransform();
	}

	UpdateBoundingVolume();

	assert(pieces.size() == model->numPieces);
}

LocalModelPiece* LocalModel::CreateLocalModelPieces(const S3DModelPiece* mpParent)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LocalModelPiece* lmpChild = nullptr;

	// construct an LMP(mp) in-place
	pieces.emplace_back(mpParent);
	LocalModelPiece* lmpParent = &pieces.back();

	lmpParent->SetLModelPieceIndex(pieces.size() - 1);
	lmpParent->SetScriptPieceIndex(pieces.size() - 1);
	lmpParent->SetLocalModel(this);

	// the mapping is 1:1 for Lua scripts, but not necessarily for COB
	// CobInstance::MapScriptToModelPieces does the remapping (if any)
	assert(lmpParent->GetLModelPieceIndex() == lmpParent->GetScriptPieceIndex());

	for (const S3DModelPiece* mpChild: mpParent->children) {
		lmpChild = CreateLocalModelPieces(mpChild);
		lmpChild->SetParent(lmpParent);
		lmpParent->AddChild(lmpChild);
	}

	return lmpParent;
}


void LocalModel::UpdateBoundingVolume()
{
	ZoneScoped;

	// bounding-box extrema (local space)
	float3 bbMins = DEF_MIN_SIZE;
	float3 bbMaxs = DEF_MAX_SIZE;

	for (const auto& lmPiece: pieces) {
		const auto& tra = lmPiece.GetModelSpaceTransform();
		const S3DModelPiece* piece = lmPiece.original;

		// skip empty pieces or bounds will not be sensible
		if (!piece->HasGeometryData())
			continue;

		// transform only the corners of the piece's bounding-box
		const float3 pMins = piece->mins;
		const float3 pMaxs = piece->maxs;
		const float3 verts[8] = {
			// bottom
			float3(pMins.x,  pMins.y,  pMins.z),
			float3(pMaxs.x,  pMins.y,  pMins.z),
			float3(pMaxs.x,  pMins.y,  pMaxs.z),
			float3(pMins.x,  pMins.y,  pMaxs.z),
			// top
			float3(pMins.x,  pMaxs.y,  pMins.z),
			float3(pMaxs.x,  pMaxs.y,  pMins.z),
			float3(pMaxs.x,  pMaxs.y,  pMaxs.z),
			float3(pMins.x,  pMaxs.y,  pMaxs.z),
		};

		for (const float3& v: verts) {
			const float3 vertex = tra * v;

			bbMins = float3::min(bbMins, vertex);
			bbMaxs = float3::max(bbMaxs, vertex);
		}
	}

	// note: offset is relative to object->pos
	boundingVolume.InitBox(bbMaxs - bbMins, (bbMaxs + bbMins) * 0.5f);

	needsBoundariesRecalc = false;
}