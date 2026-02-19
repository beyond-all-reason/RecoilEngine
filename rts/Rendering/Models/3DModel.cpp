/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "3DModel.hpp"

#include <algorithm>

#include "3DModelMisc.hpp"
#include "3DModelPiece.hpp"
#include "System/Misc/TracyDefs.h"


/******************************************************************************/
/******************************************************************************/

S3DModel& S3DModel::operator= (S3DModel&& m) noexcept {
	name = std::move(m.name);
	texs[0] = std::move(m.texs[0]);
	texs[1] = std::move(m.texs[1]);

	id = m.id;
	numPieces = m.numPieces;
	textureType = m.textureType;

	type = m.type;

	radius = m.radius;
	height = m.height;

	mins = m.mins;
	maxs = m.maxs;
	relMidPos = m.relMidPos;

	indxStart = m.indxStart;
	indxCount = m.indxCount;

	pieceObjects.swap(m.pieceObjects);

	for (auto po : pieceObjects)
		po->SetParentModel(this);

	loadStatus = m.loadStatus;
	uploaded = m.uploaded;

	std::swap(traAlloc, m.traAlloc);

	return *this;
}

S3DModelPiece* S3DModel::FindPiece(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = std::find_if(pieceObjects.begin(), pieceObjects.end(), [&name](const S3DModelPiece* piece) {
		return piece->name == name;
	});
	if (it == pieceObjects.end())
		return nullptr;

	return *it;
}

const S3DModelPiece* S3DModel::FindPiece(const std::string& name) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = std::find_if(pieceObjects.begin(), pieceObjects.end(), [&name](const S3DModelPiece* piece) {
		return piece->name == name;
		});
	if (it == pieceObjects.end())
		return nullptr;

	return *it;
}

size_t S3DModel::FindPieceOffset(const std::string& name) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = std::find_if(pieceObjects.begin(), pieceObjects.end(), [&name](const S3DModelPiece* piece) {
		return piece->name == name;
	});

	if (it == pieceObjects.end())
		return size_t(-1);

	return std::distance(pieceObjects.begin(), it);
}

void S3DModel::SetPieceMatrices()
{
	auto* rootPiece = GetRootPiece();
	rootPiece->SetPieceTransform(Transform());

	// use this occasion and copy bpose matrices
	for (size_t i = 0; i < pieceObjects.size(); ++i) {
		const auto* po = pieceObjects[i];
		traAlloc.UpdateForced(i, po->bposeTransform);
	}
}

void S3DModel::FlattenPieceTree(S3DModelPiece* root)
{
	assert(root != nullptr);

	pieceObjects.clear();
	pieceObjects.reserve(numPieces);

	// force mutex just in case this is called from modelLoader.ProcessVertices()
	// TODO: pass to S3DModel if it is created from LoadModel(ST) or from ProcessVertices(MT)
	traAlloc = ScopedTransformMemAlloc(numPieces);

	std::vector<S3DModelPiece*> stack = { root };

	while (!stack.empty()) {
		S3DModelPiece* p = stack.back();

		stack.pop_back();
		pieceObjects.push_back(p);

		// add children in reverse for the correct DF traversal order
		for (size_t n = 0; n < p->children.size(); n++) {
			auto* child = p->children[p->children.size() - n - 1];
			stack.push_back(child);
		}
	}
}

void S3DModel::DrawStatic() const
{
	S3DModelHelpers::BindLegacyAttrVBOs();

	// draw pieces in their static bind-pose (ie. without script-transforms)
	for (const S3DModelPiece* pieceObj : pieceObjects) {
		pieceObj->DrawStaticLegacy(false, true);
	}

	S3DModelHelpers::UnbindLegacyAttrVBOs();
}

void S3DModel::UpdatePiecesMinMaxExtents()
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (auto* piece : pieceObjects) {
		for (const auto& vertex : piece->GetVerticesVec()) {
			piece->mins = float3::min(piece->mins, vertex.pos);
			piece->maxs = float3::max(piece->maxs, vertex.pos);
		}
	}
}