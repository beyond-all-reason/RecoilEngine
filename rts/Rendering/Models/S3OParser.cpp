/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cctype>
#include <stdexcept>

#include "S3OParser.h"
#include "3DModel.hpp"
#include "ModelUtils.h"
#include "s3o.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Exceptions.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/byteorder.h"

#include "System/Misc/TracyDefs.h"



void CS3OParser::Kill() {
	RECOIL_DETAILED_TRACY_ZONE;
	LOG_L(L_INFO, "[S3OParser::%s] allocated %u pieces", __func__, static_cast<uint32_t>(pieces.size()));
	pieces.clear(); pieces.shrink_to_fit();
}

void CS3OParser::Load(S3DModel& model, const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;

	auto fileBuf = LoadFromFile(name);

	if (fileBuf.size() < sizeof(S3OHeader))
		throw content_error("[S3OParser] corrupted header for model-file " + name);

	S3OHeader header;
	memcpy(&header, fileBuf.data(), sizeof(header));
	header.swap();

	model.name = name;
	model.type = MODELTYPE_S3O;
	model.numPieces = 0;
	model.texs[0] = (header.texture1 == 0)? "" : (char*) &fileBuf[header.texture1];
	model.texs[1] = (header.texture2 == 0)? "" : (char*) &fileBuf[header.texture2];

	textureHandlerS3O.PreloadTexture(
		&model,
		false,
		false
	);

	model.FlattenPieceTree(LoadPiece(&model, nullptr, fileBuf, header.rootPiece));
	model.SetPieceMatrices();

	// set after the extrema are known
	if (header.radius > 0.01f)
		model.modelParams.radius = header.radius;

	if (header.height > 0.01f)
		model.modelParams.height = header.height;

	model.modelParams.relMidPos = float3(header.midx, header.midy, header.midz);
}


SS3OPiece* CS3OParser::AllocPiece()
{
	RECOIL_DETAILED_TRACY_ZONE;
	return static_cast<SS3OPiece*>(AllocPieceImpl());
}

SS3OPiece* CS3OParser::LoadPiece(S3DModel* model, SS3OPiece* parent, std::vector<uint8_t>& buf, int offset)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if ((offset + sizeof(Piece)) > buf.size())
		throw content_error("[S3OParser] corrupted piece for model-file " + model->name);

	model->numPieces++;

	// retrieve piece data
	Piece* fp = reinterpret_cast<Piece*>(&buf[offset]); fp->swap();

	// (fp->xxxCount > 0) check rationale: apparently widely used s3o tools have a bug when fp->xxx might point outside of buffer
	// this bug only manifests itself when launching spring in debug build with bounds checking (MSVC does it by default)
	// Since s3o assets with such bugs is uncountable, let's workaround it in the code.
	Vertex* vertexList = fp->numVertices > 0 ? reinterpret_cast<Vertex*>(&buf[fp->vertices]) : nullptr;
	const int* indexList = fp->vertexTableSize > 0 ? reinterpret_cast<int*>(&buf[fp->vertexTable]) : nullptr;
	const int* childList = fp->numchildren > 0 ? reinterpret_cast<int*>(&buf[fp->children]) : nullptr;

	// create piece
	SS3OPiece* piece = AllocPiece();

	piece->offset.x = fp->xoffset;
	piece->offset.y = fp->yoffset;
	piece->offset.z = fp->zoffset;
	piece->primType = fp->primitiveType;
	piece->name = (char*) &buf[fp->name];
	piece->parent = parent;
	piece->SetParentModel(model);

	// retrieve vertices
	piece->tmpVerts.resize(fp->numVertices);
	for (int a = 0; a < fp->numVertices; ++a) {
		Vertex* v = vertexList++;
		v->swap();

		SVertexData sv;
		sv.pos = float3(v->xpos, v->ypos, v->zpos);
		sv.normal = float3(v->xnormal, v->ynormal, v->znormal);

		if (sv.normal.CheckNaNs()) {
			sv.normal.SafeANormalize();
		} else {
			sv.normal = ZeroVector;
		}

		sv.texCoords[0] = float2(v->texu, v->texv);
		sv.texCoords[1] = float2(v->texu, v->texv);

		piece->tmpVerts[a] = sv;
	}

	// retrieve draw indices
	piece->tmpIndcs.resize(fp->vertexTableSize);
	for (int a = 0; a < fp->vertexTableSize; ++a) {
		piece->tmpIndcs[a] = swabDWord(*(indexList++));
	}

	// post process the piece
	{
		piece->Trianglize();
		piece->SetVertexTangents();
	}

	// load children pieces
	piece->children.reserve(fp->numchildren);

	for (int a = 0; a < fp->numchildren; ++a) {
		int childOffset = swabDWord(*(childList++));
		SS3OPiece* childPiece = LoadPiece(model, piece, buf, childOffset);
		piece->children.push_back(childPiece);
	}

	return piece;
}

void SS3OPiece::Trianglize()
{
	RECOIL_DETAILED_TRACY_ZONE;
	switch (primType) {
		case S3O_PRIMTYPE_TRIANGLES: {
		} break;
		case S3O_PRIMTYPE_TRIANGLE_STRIP: {
			if (tmpIndcs.size() < 3) {
				primType = S3O_PRIMTYPE_TRIANGLES;
				tmpIndcs.clear();
				return;
			}

			decltype(tmpIndcs) newIndices;
			newIndices.resize(tmpIndcs.size() * 3); // each index (can) create a new triangle

			for (size_t i = 0; (i + 2) < tmpIndcs.size(); ++i) {
				// tmpIndcs can contain end-of-strip markers (-1U)
				if (tmpIndcs[i + 0] == -1 || tmpIndcs[i + 1] == -1 || tmpIndcs[i + 2] == -1)
					continue;

				newIndices.push_back(tmpIndcs[i + 0]);
				newIndices.push_back(tmpIndcs[i + 1]);
				newIndices.push_back(tmpIndcs[i + 2]);
			}

			primType = S3O_PRIMTYPE_TRIANGLES;
			tmpIndcs.swap(newIndices);
		} break;
		case S3O_PRIMTYPE_QUADS: {
			if (tmpIndcs.size() % 4 != 0) {
				primType = S3O_PRIMTYPE_TRIANGLES;
				tmpIndcs.clear();
				return;
			}

			decltype(tmpIndcs) newIndices;
			const size_t oldCount = tmpIndcs.size();
			newIndices.resize(oldCount + oldCount / 2); // 4 indices become 6

			for (size_t i = 0, j = 0; i < tmpIndcs.size(); i += 4) {
				newIndices[j++] = tmpIndcs[i + 0];
				newIndices[j++] = tmpIndcs[i + 1];
				newIndices[j++] = tmpIndcs[i + 2];

				newIndices[j++] = tmpIndcs[i + 0];
				newIndices[j++] = tmpIndcs[i + 2];
				newIndices[j++] = tmpIndcs[i + 3];
			}

			primType = S3O_PRIMTYPE_TRIANGLES;
			tmpIndcs.swap(newIndices);
		} break;

		default: {
		} break;
	}
}

void SS3OPiece::SetVertexTangents()
{
	ModelUtils::CalculateTangents(tmpVerts, tmpIndcs);
}
