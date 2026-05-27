/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AssParser.h"

#include <regex>
#include <algorithm>
#include <numeric>
#include <optional>
#include <cstdint>

#include "3DModel.hpp"
#include "3DModelDefs.hpp"
#include "3DModelLog.h"
#include "ModelUtils.h"

#include "Lua/LuaParser.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/SpringMath.h"
#include "System/ScopedFPUSettings.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"

#include "lib/assimp/include/assimp/config.h"
#include "lib/assimp/include/assimp/defs.h"
#include "lib/assimp/include/assimp/types.h"
#include "lib/assimp/include/assimp/scene.h"
#include "lib/assimp/include/assimp/postprocess.h"
#include "lib/assimp/include/assimp/Importer.hpp"
#include "lib/assimp/include/assimp/DefaultLogger.hpp"

#include "System/Misc/TracyDefs.h"


#define IS_QNAN(f) (f != f)

// triangulate guarantees the most complex mesh is a triangle
// sortbytype ensure only 1 type of primitive type per mesh is used
static constexpr uint32_t ASS_POSTPROCESS_OPTIONS =
	  aiProcess_RemoveComponent
	| aiProcess_FindInvalidData
	| aiProcess_CalcTangentSpace
	| aiProcess_GenSmoothNormals
	| aiProcess_Triangulate
	| aiProcess_GenUVCoords
	| aiProcess_SortByPType
	| aiProcess_JoinIdenticalVertices
	//| aiProcess_ImproveCacheLocality // FIXME crashes in an assert in VertexTriangleAdjancency.h (date 04/2011)
	| aiProcess_LimitBoneWeights
	| aiProcess_SplitLargeMeshes
	;

static constexpr uint32_t ASS_IMPORTER_OPTIONS =
	  aiComponent_CAMERAS
	| aiComponent_LIGHTS
	| aiComponent_TEXTURES
	| aiComponent_ANIMATIONS
	| aiComponent_MATERIALS
	;
static constexpr uint32_t ASS_LOGGING_OPTIONS =
	  Assimp::Logger::Debugging
	| Assimp::Logger::Info
	| Assimp::Logger::Err
	| Assimp::Logger::Warn
	;



static inline float3 aiVectorToFloat3(const aiVector3D v)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// no-op; AssImp's internal coordinate-system matches Spring's modulo handedness
	return {v.x, v.y, v.z};

	// Blender --> Spring
	// return float3(v.x, v.z, -v.y);
}

static inline CMatrix44f aiMatrixToMatrix(const aiMatrix4x4t<float>& m)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CMatrix44f n;

	n[ 0] = m.a1; n[ 1] = m.a2; n[ 2] = m.a3; n[ 3] = m.a4; // 1st column
	n[ 4] = m.b1; n[ 5] = m.b2; n[ 6] = m.b3; n[ 7] = m.b4; // 2nd column
	n[ 8] = m.c1; n[ 9] = m.c2; n[10] = m.c3; n[11] = m.c4; // 3rd column
	n[12] = m.d1; n[13] = m.d2; n[14] = m.d3; n[15] = m.d4; // 4th column

	// AssImp (row-major, RH) --> Spring (column-major, LH)
	return (n.Transpose());

	// Blender --> Spring
	// return (CMatrix44f(n.GetPos(), n.GetX(), n.GetZ(), -n.GetY()));
}

/*
static float3 aiQuaternionToRadianAngles(const aiQuaternion q1)
{
	const float sqw = q1.w * q1.w;
	const float sqx = q1.x * q1.x;
	const float sqy = q1.y * q1.y;
	const float sqz = q1.z * q1.z;
	// <unit> is 1 if normalised, otherwise correction factor
	const float unit = sqx + sqy + sqz + sqw;
	const float test = q1.x * q1.y + q1.z * q1.w;

	aiVector3D angles;

	if (test > (0.499f * unit)) {
		// singularity at north pole
		angles.x = 2.0f * math::atan2(q1.x, q1.w);
		angles.y = PI * 0.5f;
	} else if (test < (-0.499f * unit)) {
		// singularity at south pole
		angles.x = -2.0f * math::atan2(q1.x, q1.w);
		angles.y = -PI * 0.5f;
	} else {
		angles.x = math::atan2(2.0f * q1.y * q1.w - 2.0f * q1.x * q1.z,  sqx - sqy - sqz + sqw);
		angles.y = math::asin((2.0f * test) / unit);
		angles.z = math::atan2(2.0f * q1.x * q1.w - 2.0f * q1.y * q1.z, -sqx + sqy - sqz + sqw);
	}

	return (aiVectorToFloat3(angles));
}
*/




class AssLogStream : public Assimp::LogStream
{
public:
	void write(const char* message) override {
		LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "Assimp: %s", message);
	}
};





//////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Impl {
	struct MeshData {
		std::vector<SVertexData> verts;
		std::vector<uint32_t> indcs;
	};

	template<typename PieceObject>
	void LoadPieceTransformations(
		PieceObject* piece,
		const S3DModel* model,
		const aiNode* pieceNode,
		const LuaTable& pieceTable,
		const CQuaternion* optRotation
	) {
		RECOIL_DETAILED_TRACY_ZONE;
		aiVector3D aiScaleVec;
		aiVector3D aiTransVec;
		aiQuaternion aiRotateQuat;

		// process transforms
		pieceNode->mTransformation.Decompose(aiScaleVec, aiRotateQuat, aiTransVec);

		// metadata-scaling
		float3 scales{ 1.0f, 1.0f, 1.0f };
		scales = pieceTable.GetFloat3("scale", aiVectorToFloat3(aiScaleVec));
		scales.x = pieceTable.GetFloat("scalex", scales.x);
		scales.y = pieceTable.GetFloat("scaley", scales.y);
		scales.z = pieceTable.GetFloat("scalez", scales.z);

		if (!epscmp(scales.x, scales.y, std::max(scales.x, scales.y) * float3::cmp_eps()) ||
			!epscmp(scales.y, scales.z, std::max(scales.y, scales.z) * float3::cmp_eps()) ||
			!epscmp(scales.z, scales.x, std::max(scales.z, scales.x) * float3::cmp_eps()))
		{
			LOG_SL(LOG_SECTION_MODEL, L_WARNING, "Recoil doesn't support non-uniform scaling");
		}
		piece->scale = scales.x;

		// metadata-translation
		piece->offset = pieceTable.GetFloat3("offset", aiVectorToFloat3(aiTransVec));
		piece->offset.x = pieceTable.GetFloat("offsetx", piece->offset.x);
		piece->offset.y = pieceTable.GetFloat("offsety", piece->offset.y);
		piece->offset.z = pieceTable.GetFloat("offsetz", piece->offset.z);

		// metadata-rotation
		// NOTE:
		//   these rotations are "pre-scripting" but "post-modelling"
		//   together with the (baked) aiRotateQuad they determine the
		//   model's pose *before* any animations execute
		//
		// float3 bakedRotAngles = pieceTable.GetFloat3("rotate", aiQuaternionToRadianAngles(aiRotateQuat) * math::RAD_TO_DEG);
		float3 bakedRotAngles = pieceTable.GetFloat3("rotate", ZeroVector);

		bakedRotAngles.x = pieceTable.GetFloat("rotatex", bakedRotAngles.x);
		bakedRotAngles.y = pieceTable.GetFloat("rotatey", bakedRotAngles.y);
		bakedRotAngles.z = pieceTable.GetFloat("rotatez", bakedRotAngles.z);
		bakedRotAngles *= math::DEG_TO_RAD;

		LOG_SL(LOG_SECTION_PIECE, L_INFO,
			"(%d:%s) Assimp offset (%f,%f,%f), rotate (%f,%f,%f,%f), scale (%f,%f,%f)",
			model->numPieces, piece->name.c_str(),
			aiTransVec.x, aiTransVec.y, aiTransVec.z,
			aiRotateQuat.w, aiRotateQuat.x, aiRotateQuat.y, aiRotateQuat.z,
			aiScaleVec.x, aiScaleVec.y, aiScaleVec.z
		);
		LOG_SL(LOG_SECTION_PIECE, L_INFO,
			"(%d:%s) Relative offset (%f,%f,%f), rotate (%f,%f,%f), scale (%f)",
			model->numPieces, piece->name.c_str(),
			piece->offset.x, piece->offset.y, piece->offset.z,
			bakedRotAngles.x, bakedRotAngles.y, bakedRotAngles.z,
			piece->scale
		);

		// construct 'baked' piece-space transform
		//
		// AssImp order is Translate * Rotate * Scale * v; the
		// translation and scale parts are split into <offset>
		// and <scales> so the baked part reduces to R
		//
		// note: for all non-AssImp models this is identity!
		auto Quat = CQuaternion(aiRotateQuat.x, aiRotateQuat.y, aiRotateQuat.z, aiRotateQuat.w);
		if (optRotation)
			Quat = (*optRotation) * Quat;

		Transform bakedTransform(CQuaternion::FromEulerYPRNeg(-bakedRotAngles) * Quat, ZeroVector, 1.0f);
		piece->SetBakedTransform(bakedTransform);
	}

	std::vector<std::string> GetBoneNames(const aiScene* scene)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		std::vector<std::string> boneNames;
		for (size_t m = 0; m < scene->mNumMeshes; ++m) {
			for (size_t b = 0; b < scene->mMeshes[m]->mNumBones; ++b) {
				std::string boneName(scene->mMeshes[m]->mBones[b]->mName.data);
				auto it = std::find(boneNames.begin(), boneNames.end(), boneName);
				if (it == boneNames.end())
					boneNames.emplace_back(boneName);
			}
		}

		return boneNames;
	}

	std::vector<std::string> GetMeshNames(const aiScene* scene)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		std::vector<std::string> meshNames;
		for (uint32_t m = 0; m < scene->mNumMeshes; ++m) {
			meshNames.emplace_back(scene->mMeshes[m]->mName.data);
		}

		return meshNames;
	}

	const aiNode* FindNodeForMesh(const aiNode* node, uint32_t meshIndex) {
		RECOIL_DETAILED_TRACY_ZONE;
		for (uint32_t i = 0; i < node->mNumMeshes; i++) {
			if (node->mMeshes[i] == meshIndex) return node;
		}
		for (uint32_t i = 0; i < node->mNumChildren; i++) {
			const aiNode* found = FindNodeForMesh(node->mChildren[i], meshIndex);
			if (found) return found;
		}
		return nullptr;
	}

	CMatrix44f GetMeshModelSpaceMatrix(const aiScene* scene, uint32_t meshIndex) {
		RECOIL_DETAILED_TRACY_ZONE;
		const aiNode* node = FindNodeForMesh(scene->mRootNode, meshIndex);
		aiMatrix4x4 transform; // identity by default

		while (node != nullptr) {
			transform = node->mTransformation * transform;
			node = node->mParent;
		}
		return aiMatrixToMatrix(transform); // now in model/world space
	}

	// Pre-compute all node transforms in model space in a single pass
	// Similar to GLTFParser's GetModelTransforms
	spring::unordered_map<const aiNode*, Transform> GetNodeTransforms(const aiScene* scene) {
		RECOIL_DETAILED_TRACY_ZONE;
		spring::unordered_map<const aiNode*, Transform> transforms;

		// Recursive lambda to compute transforms
		// GCC-13 doesn't like recursive lambdas with auto&& self, so we have to declare it separately and capture it by reference
		//auto ComputeTransform = [&](this auto&& self, const aiNode* node, const Transform& parentTransform) -> void {
		auto ComputeTransform = [&](auto&& self, const aiNode* node, const Transform& parentTransform) -> void {
			aiVector3D aiScaleVec, aiTransVec;
			aiQuaternion aiRotateQuat;
			node->mTransformation.Decompose(aiScaleVec, aiRotateQuat, aiTransVec);

			// Create Transform from aiNode's transformation
			Transform nodeTransform(
				CQuaternion(aiRotateQuat.x, aiRotateQuat.y, aiRotateQuat.z, aiRotateQuat.w),
				aiVectorToFloat3(aiTransVec),
				aiScaleVec.x
			);

			// Compute model-space transform & store the transform
			Transform modelSpaceTransform = parentTransform * nodeTransform;
			transforms.emplace(node, modelSpaceTransform);

			// Recursively process children
			for (uint32_t i = 0; i < node->mNumChildren; i++) {
				self(self, node->mChildren[i], modelSpaceTransform);
			}
		};

		ComputeTransform(ComputeTransform, scene->mRootNode, Transform{});

		return transforms;
	}

	std::vector<MeshData> GetModelSpaceMeshes(const aiScene* scene, const S3DModel* model)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		std::vector<MeshData> meshes;

		// Pre-compute all node transforms in a single pass
		const auto nodeTransforms = GetNodeTransforms(scene);

		for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
			const aiMesh* mesh = scene->mMeshes[meshIndex];

			// Find the node containing this mesh and get its pre-computed transform
			const aiNode* meshNode = FindNodeForMesh(scene->mRootNode, meshIndex);
			if (!meshNode) {
				LOG_SL(LOG_SECTION_MODEL, L_ERROR, "Could not find node for mesh %d (name: \"%s\") in scene. Skipping mesh.", meshIndex, mesh->mName.C_Str());
				continue;
			}

			auto it = nodeTransforms.find(meshNode);
			assert(it != nodeTransforms.end());

			auto& [verts, indcs] = meshes.emplace_back();

			LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Fetching mesh %u from scene", meshIndex);
			LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
				"Processing vertices for mesh %u (%u vertices)",
				meshIndex, mesh->mNumVertices);
			LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
				"Normals: %s Tangents/Bitangents: %s TexCoords: %s",
				(mesh->HasNormals() ? "Y" : "N"),
				(mesh->HasTangentsAndBitangents() ? "Y" : "N"),
				(mesh->HasTextureCoords(0) ? "Y" : "N"));

			verts.reserve(mesh->mNumVertices);
			indcs.reserve(mesh->mNumFaces * 3);

			//bones info
			std::vector<std::vector<std::pair<uint16_t, float>>> vertexWeights(mesh->mNumVertices);

			for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
				const aiBone* bone = mesh->mBones[boneIndex];
				for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; weightIndex++) {
					const auto& vertIndex = bone->mWeights[weightIndex].mVertexId;
					const auto& vertWeight = bone->mWeights[weightIndex].mWeight;
					const std::string boneName = std::string(bone->mName.C_Str());


					auto boneID = spring::SafeCast<uint16_t>(model->FindPieceOffset(boneName));
					assert(boneID < INV_PIECE_NUM); // == INV_PIECE_NUM - invalid piece

					vertexWeights[vertIndex].emplace_back(boneID, vertWeight);
				}
			}

			for (auto& vertexWeight : vertexWeights) {
				std::stable_sort(vertexWeight.begin(), vertexWeight.end(), [](const auto& lhs, const auto& rhs) {
					return std::forward_as_tuple(lhs.second, lhs.first) > std::forward_as_tuple(rhs.second, rhs.first);
				});
				vertexWeight.resize(4, std::make_pair(SVertexData::INVALID_BONEID, 0.0f));
			}

			// extract vertex data per mesh
			for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
				const aiVector3D& aiVertex = mesh->mVertices[vertexIndex];

				SVertexData vertex;

				// bones info
				vertex.SetBones(vertexWeights[vertexIndex]);

				// vertex coordinates
				vertex.pos = aiVectorToFloat3(aiVertex);

				if (mesh->HasNormals()) {
					// vertex normal
					const aiVector3D& aiNormal = mesh->mNormals[vertexIndex];

					if (IS_QNAN(aiNormal)) {
						LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Malformed normal (model->name=\"%s\" meshName=\"%s\" vertexIndex=%d x=%f y=%f z=%f)", model->name.c_str(), mesh->mName.C_Str(), vertexIndex, aiNormal.x, aiNormal.y, aiNormal.z);
						vertex.normal = float3{ 0.0f, 1.0f, 0.0f };
					}
					else {
						vertex.normal = (aiVectorToFloat3(aiNormal)).SafeANormalize();
					}
				}
				else {
					vertex.normal = float3{ 0.0f, 1.0f, 0.0f };
				}

				// vertex tangent, x is positive in texture axis
				if (mesh->HasTangentsAndBitangents()) {
					const aiVector3D& aiTangent = mesh->mTangents[vertexIndex];
					const aiVector3D& aiBitangent = mesh->mBitangents[vertexIndex];

					if (IS_QNAN(aiTangent.x) || IS_QNAN(aiTangent.y) || IS_QNAN(aiTangent.z)) {
						LOG_SL(LOG_SECTION_PIECE, L_INFO, "Malformed tangent (model->name=\"%s\" meshName=\"%s\" vertexIndex=%d x=%f y=%f z=%f)", model->name.c_str(), mesh->mName.C_Str(), vertexIndex, aiTangent.x, aiTangent.y, aiTangent.z);
						vertex.tangent = float4{ 1.0f, 0.0f, 0.0f, 0.0f };
					}
					else {
						vertex.tangent = (aiVectorToFloat3(aiTangent)).SafeANormalize();
					}

					if (IS_QNAN(aiBitangent.x) || IS_QNAN(aiBitangent.y) || IS_QNAN(aiBitangent.z)) {
						LOG_SL(LOG_SECTION_PIECE, L_INFO, "Malformed bitangent (model->name=\"%s\" meshName=\"%s\" vertexIndex=%d x=%f y=%f z=%f)", model->name.c_str(), mesh->mName.C_Str(), vertexIndex, aiBitangent.x, aiBitangent.y, aiBitangent.z);
						vertex.tangent.w = 1.0f;
					}
					else {
						const auto B = (aiVectorToFloat3(aiBitangent)).SafeANormalize();
						const float handednessSign = Sign(B.dot(vertex.normal.cross(vertex.tangent)));
						vertex.tangent.w = handednessSign;
					}
				}

				// vertex tex-coords per channel
				for (uint32_t uvChanIndex = 0; uvChanIndex < SVertexData::NUM_MODEL_UVCHANNS; uvChanIndex++) {
					if (!mesh->HasTextureCoords(uvChanIndex))
						break;

					vertex.texCoords[uvChanIndex].x = mesh->mTextureCoords[uvChanIndex][vertexIndex].x;
					vertex.texCoords[uvChanIndex].y = mesh->mTextureCoords[uvChanIndex][vertexIndex].y;
				}

				vertex.TransformBy(it->second);

				verts.push_back(vertex);
			}

			// extract face data
			LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Processing faces for mesh %d (%d faces)", meshIndex, mesh->mNumFaces);

			/*
			 * since aiProcess_SortByPType is being used,
			 * we're sure we'll get only 1 type here,
			 * so combination check isn't needed, also
			 * anything more complex than triangles is
			 * being split thanks to aiProcess_Triangulate
			 */
			for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
				const aiFace& face = mesh->mFaces[faceIndex];

				// some models contain lines (mNumIndices == 2) which
				// we cannot render and they would need a 2nd drawcall)
				if (face.mNumIndices != 3)
					continue;

				for (uint32_t vertexListID = 0; vertexListID < face.mNumIndices; ++vertexListID) {
					indcs.push_back(face.mIndices[vertexListID]);
				}
			}
		}

		return meshes;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

void CAssParser::Init()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// FIXME: non-optimal, maybe compute these ourselves (pre-TL cache size!)
	maxIndices = std::max(globalRendering->glslMaxRecommendedIndices, 1024);
	maxVertices = std::max(globalRendering->glslMaxRecommendedVertices, 1024);

	Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
	// create a logger for debugging model loading issues
	Assimp::DefaultLogger::get()->attachStream(new AssLogStream(), ASS_LOGGING_OPTIONS);
}

void CAssParser::Kill()
{
	RECOIL_DETAILED_TRACY_ZONE;
	Assimp::DefaultLogger::kill();
	LOG_L(L_INFO, "[AssParser::%s] allocated %u pieces", __func__, static_cast<uint32_t>(pieces.size()));
	pieces.clear(); pieces.shrink_to_fit();
}

void CAssParser::Load(S3DModel& model, const std::string& modelFilePath)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Loading model: %s", modelFilePath.c_str());

	const std::string modelPath = FileSystem::GetDirectory(modelFilePath);
	const std::string modelName = FileSystem::GetBasename(modelFilePath);

	CFileHandler file(modelFilePath, SPRING_VFS_ZIP);

	std::vector<uint8_t> fileBuf;
	// load the lua metafile containing properties unique to Recoil models (must return a table)
	std::string metaFileName = modelFilePath + ".lua";

	// try again without the model file extension
	if (!CFileHandler::FileExists(metaFileName, SPRING_VFS_ZIP))
		metaFileName = modelPath + modelName + ".lua";
	if (!CFileHandler::FileExists(metaFileName, SPRING_VFS_ZIP))
		LOG_SL(LOG_SECTION_MODEL, L_INFO, "No meta-file '%s'. Using defaults.", metaFileName.c_str());

	LuaParser metaFileParser(metaFileName, SPRING_VFS_ZIP, SPRING_VFS_ZIP);

	if (!metaFileParser.Execute())
		LOG_SL(LOG_SECTION_MODEL, L_INFO, "'%s': %s. Using defaults.", metaFileName.c_str(), metaFileParser.GetErrorLog().c_str());

	// get the (root-level) model table
	const auto modelTable = metaFileParser.GetRoot();

	if (!modelTable.IsValid())
		LOG_SL(LOG_SECTION_MODEL, L_INFO, "No valid model metadata in '%s' or no meta-file", metaFileName.c_str());


	Assimp::Importer importer;

	// speed-up processing by skipping things we don't need
	importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, ASS_IMPORTER_OPTIONS);
	importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT,   maxVertices);
	importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, maxIndices / 3);

	if (!file.IsBuffered()) {
		const auto fs = file.FileSize();
		if (fs <= 0)
			throw content_error("An assimp model has invalid size of " + std::to_string(fs));

		fileBuf.resize(fs, 0);
		file.Read(fileBuf.data(), fileBuf.size());
	} else {
		fileBuf = std::move(file.GetBuffer());
	}

	if (modelTable.GetBool("nodenamesfromids", false)) {
		assert(FileSystem::GetExtensionLowerCase(modelFilePath) == "dae");
		PreProcessFileBuffer(fileBuf);
	}


	// Read the model file to build a scene object
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Importing model file: %s", modelFilePath.c_str());

	const aiScene* scene = nullptr;

	{
		// ASSIMP spams many SIGFPEs atm in normal & tangent generation
		ScopedDisableFpuExceptions fe;
		scene = importer.ReadFileFromMemory(fileBuf.data(), fileBuf.size(), ASS_POSTPROCESS_OPTIONS);
	}

	if (scene == nullptr)
		throw content_error("[AssimpParser] Model Import: " + std::string(importer.GetErrorString()));

	LOG_SL(LOG_SECTION_MODEL, L_INFO,
		"Processing scene for model: %s (%d meshes / %d materials / %d textures)",
		modelFilePath.c_str(), scene->mNumMeshes, scene->mNumMaterials,
		scene->mNumTextures
	);

	ModelPieceMap pieceMap;
	ParentNameMap parentMap;

	model.name = modelFilePath;
	model.type = MODELTYPE_ASS;

	// Load textures
	FindTextures(&model, scene, modelTable, modelPath, modelName);
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Loading textures. Tex1: '%s' Tex2: '%s'", model.texs[0].c_str(), model.texs[1].c_str());

	textureHandlerS3O.PreloadTexture(
		&model,
		modelTable.GetBool("fliptextures", true),   // "true" is the incorrect default, but has to be retained to be compatible
		modelTable.GetBool("invertteamcolor", true) // "true" is the incorrect default, but has to be retained to be compatible
	);

	// Check if bones exist
	const auto boneNames = Impl::GetBoneNames(scene);
	const auto meshNames = !boneNames.empty() ? Impl::GetMeshNames(scene) : std::vector<std::string>{};

	// Load all pieces in the model
	LOG_SL(LOG_SECTION_MODEL, L_INFO, "Loading pieces from root node '%s'", scene->mRootNode->mName.data);

	if (modelTable.GetBool("s3ocompat", false)) {
		const auto rootPieceRot = CQuaternion(0, 1, 0, 0); // rotate 180 around Y
		LoadPiece(&model, scene->mRootNode, scene, modelTable, meshNames, pieceMap, parentMap, &rootPieceRot);
	} else {
		LoadPiece(&model, scene->mRootNode, scene, modelTable, meshNames, pieceMap, parentMap);
	}

	// Update piece hierarchy based on metadata
	BuildPieceHierarchy(&model, pieceMap, parentMap);

	// skinning support - save mesh data directly to model.skinnedVerts/skinnedIndcs
	if (!meshNames.empty()) {
		const auto meshes = Impl::GetModelSpaceMeshes(scene, &model);

		// Merge all skinned meshes into model.skinnedVerts/skinnedIndcs
		for (const auto& mesh : meshes) {
			const auto vertOffset = model.skinnedVerts.size();
			model.skinnedVerts.insert(model.skinnedVerts.end(), mesh.verts.begin(), mesh.verts.end());
			for (const auto& indx : mesh.indcs) {
				model.skinnedIndcs.push_back(static_cast<uint32_t>(vertOffset + indx));
			}
		}
	}
}


void CAssParser::PreProcessFileBuffer(std::vector<uint8_t>& fileBuffer)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// the Collada specification requires node uid's to be unique
	// (names can be repeated) which certain exporters obey while
	// others do not
	// however, obedient exporters actually make life inconvenient
	// for modellers since assimp's Collada importer extracts node
	// names (aiNode::mName) from the *id* field
	// as a workaround, let the model metadata decide if id's and
	// names should be swapped before assimp processes the buffer
	const std::regex nodePattern{"<node id=\"([a-zA-Z0-9_-]+)\" name=\"([a-zA-Z0-9_-]+)\" type=\"([a-zA-Z]+)\">"};

	std::array<uint8_t, 1024> lineBuffer;
	std::cmatch matchGroups;

	const char* beg = reinterpret_cast<const char*>(fileBuffer.data());
	const char* end = reinterpret_cast<const char*>(fileBuffer.data() + fileBuffer.size());

	if (strstr(beg, "COLLADA") == nullptr)
		return;

	for (size_t i = 0, n = fileBuffer.size(); i < n; ) {
		matchGroups = std::cmatch{};

		if (!std::regex_search(beg + i, matchGroups, nodePattern))
			break;

		const std::string   id = matchGroups[1].str();
		const std::string name = matchGroups[2].str();
		const std::string type = matchGroups[3].str();

		assert(matchGroups[0].first  >= beg && matchGroups[0].first  < end);
		assert(matchGroups[0].second >= beg && matchGroups[0].second < end);

		// just swap id and name fields; preserves line length
		memset(lineBuffer.data(), 0, lineBuffer.size());
		snprintf(reinterpret_cast<char*>(lineBuffer.data()), lineBuffer.size(), "<node id=\"%s\" name=\"%s\" type=\"%s\">", name.c_str(), id.c_str(), type.c_str());
		memcpy(const_cast<char*>(matchGroups[0].first), lineBuffer.data(), matchGroups[0].length());

		i = matchGroups[0].second - beg;
	}
}

void CAssParser::LoadPieceTransformations(
	SAssPiece* piece,
	const S3DModel* model,
	const aiNode* pieceNode,
	const LuaTable& pieceTable,
	const CQuaternion* optRotation
) {
	RECOIL_DETAILED_TRACY_ZONE;
	Impl::LoadPieceTransformations<SAssPiece>(piece, model, pieceNode, pieceTable, optRotation);
}

void CAssParser::SetPieceName(
	SAssPiece* piece,
	const S3DModel* model,
	const aiNode* pieceNode,
	ModelPieceMap& pieceMap
) {
	RECOIL_DETAILED_TRACY_ZONE;
	assert(piece->name.empty());
	piece->name = std::string(pieceNode->mName.data);

	if (piece->name.empty()) {
		if (piece == model->GetRootPiece()) {
			// root is always the first piece created, so safe to assign this
			piece->name = "$$root$$";
			return;
		}

		piece->name = "$$piece$$";
	}

	// find a new name if none given or if a piece with the same name already exists
	ModelPieceMap::const_iterator it = pieceMap.find(piece->name);

	for (uint32_t i = 0; it != pieceMap.end(); i++) {
		const std::string newPieceName = piece->name + IntToString(i, "%02i");

		if ((it = pieceMap.find(newPieceName)) == pieceMap.end()) {
			piece->name = newPieceName; break;
		}
	}

	assert(piece->name != "SpringHeight");
	assert(piece->name != "SpringRadius");
}

void CAssParser::SetPieceParentName(
	SAssPiece* piece,
	const S3DModel* model,
	const aiNode* pieceNode,
	const LuaTable& pieceTable,
	ParentNameMap& parentMap
) {
	RECOIL_DETAILED_TRACY_ZONE;
	// parent was updated in GetPieceTableRecursively
	if (parentMap.find(piece->name) != parentMap.end())
		return;

	// Get parent name from metadata or model
	if (pieceTable.KeyExists("parent")) {
		parentMap[piece->name] = pieceTable.GetString("parent", "");
		return;
	}

	if (pieceNode->mParent == nullptr)
		return;

	if (pieceNode->mParent->mParent != nullptr) {
		// parent is not the root
		parentMap[piece->name] = std::string(pieceNode->mParent->mName.data);
	} else {
		// parent is the root (which must already exist)
		assert(model->GetRootPiece() != nullptr);
		parentMap[piece->name] = (model->GetRootPiece())->name;
	}
}

void CAssParser::LoadPieceGeometry(SAssPiece* piece, const S3DModel* model, const aiNode* pieceNode, const aiScene* scene)
{
    RECOIL_DETAILED_TRACY_ZONE;

    // Get vertex data from node meshes
    for (uint32_t meshListIndex = 0; meshListIndex < pieceNode->mNumMeshes; ++meshListIndex) {
        const uint32_t meshIndex = pieceNode->mMeshes[meshListIndex];
        const aiMesh* mesh = scene->mMeshes[meshIndex];

        LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Fetching mesh %d from scene", meshIndex);
        LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
            "Processing vertices for mesh %d (%d vertices)",
            meshIndex, mesh->mNumVertices);
        LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
            "Normals: %s Tangents/Bitangents: %s TexCoords: %s",
            (mesh->HasNormals() ? "Y" : "N"),
            (mesh->HasTangentsAndBitangents() ? "Y" : "N"),
            (mesh->HasTextureCoords(0) ? "Y" : "N"));

        auto& verts = piece->tmpVerts;
        auto& indcs = piece->tmpIndcs;

        verts.reserve(verts.size() + mesh->mNumVertices);
        indcs.reserve(indcs.size() + mesh->mNumFaces * 3);

        const uint32_t baseOffset = verts.size();

        // extract vertex data per mesh
        for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
            const aiVector3D& aiVertex = mesh->mVertices[vertexIndex];

            SVertexData vertex;

            // vertex coordinates
            vertex.pos = aiVectorToFloat3(aiVertex);

            if (mesh->HasNormals()) {
                // vertex normal
                const aiVector3D& aiNormal = mesh->mNormals[vertexIndex];

                if (IS_QNAN(aiNormal)) {
                    LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Malformed normal (model->name=\"%s\" piece->name=\"%s\" vertexIndex=%d x=%f y=%f z=%f)", model->name.c_str(), piece->name.c_str(), vertexIndex, aiNormal.x, aiNormal.y, aiNormal.z);
                    vertex.normal = float3{ 0.0f, 1.0f, 0.0f };
                }
                else {
                    vertex.normal = (aiVectorToFloat3(aiNormal)).SafeANormalize();
                }
            }
            else {
                vertex.normal = float3{ 0.0f, 1.0f, 0.0f };
            }

            // vertex tangent, x is positive in texture axis
            if (mesh->HasTangentsAndBitangents()) {
                const aiVector3D& aiTangent = mesh->mTangents[vertexIndex];
                const aiVector3D& aiBitangent = mesh->mBitangents[vertexIndex];

                if (IS_QNAN(aiTangent.x) || IS_QNAN(aiTangent.y) || IS_QNAN(aiTangent.z)) {
                    LOG_SL(LOG_SECTION_PIECE, L_INFO, "Malformed tangent (model->name=\"%s\" piece->name=\"%s\" vertexIndex=%d x=%f y=%f z=%f)", model->name.c_str(), piece->name.c_str(), vertexIndex, aiTangent.x, aiTangent.y, aiTangent.z);
                    vertex.tangent = float4{ 1.0f, 0.0f, 0.0f, 0.0f };
                } else {
                    vertex.tangent = (aiVectorToFloat3(aiTangent)).SafeANormalize();
                }

                if (IS_QNAN(aiBitangent.x) || IS_QNAN(aiBitangent.y) || IS_QNAN(aiBitangent.z)) {
                    LOG_SL(LOG_SECTION_PIECE, L_INFO, "Malformed bitangent (model->name=\"%s\" piece->name=\"%s\" vertexIndex=%d x=%f y=%f z=%f)", model->name.c_str(), piece->name.c_str(), vertexIndex, aiBitangent.x, aiBitangent.y, aiBitangent.z);
                    vertex.tangent.w = 1.0f;
                } else {
                    const auto B = (aiVectorToFloat3(aiBitangent)).SafeANormalize();
                    const float handednessSign = Sign(B.dot(vertex.normal.cross(vertex.tangent)));
                    vertex.tangent.w = handednessSign;
                }
            }

            // vertex tex-coords per channel
            for (uint32_t uvChanIndex = 0; uvChanIndex < SVertexData::NUM_MODEL_UVCHANNS; uvChanIndex++) {
                if (!mesh->HasTextureCoords(uvChanIndex))
                    break;

                vertex.texCoords[uvChanIndex].x = mesh->mTextureCoords[uvChanIndex][vertexIndex].x;
                vertex.texCoords[uvChanIndex].y = mesh->mTextureCoords[uvChanIndex][vertexIndex].y;
            }

            verts.push_back(vertex);
        }

        // extract face data
        LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Processing faces for mesh %d (%d faces)", meshIndex, mesh->mNumFaces);

        /*
         * since aiProcess_SortByPType is being used,
         * we're sure we'll get only 1 type here,
         * so combination check isn't needed, also
         * anything more complex than triangles is
         * being split thanks to aiProcess_Triangulate
         */
        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
            const aiFace& face = mesh->mFaces[faceIndex];

            // some models contain lines (mNumIndices == 2) which
            // we cannot render and they would need a 2nd drawcall)
            if (face.mNumIndices != 3)
                continue;

            for (uint32_t vertexListID = 0; vertexListID < face.mNumIndices; ++vertexListID) {
                indcs.push_back(baseOffset + face.mIndices[vertexListID]);
            }
        }
    }
}

// Not efficient, but there aren't that many pieces
// So fast anyway
static LuaTable GetPieceTableRecursively(
	const LuaTable& table,
	const std::string& name,
	const std::string& parentName,
	CAssParser::ParentNameMap& parentMap)
{
	RECOIL_DETAILED_TRACY_ZONE;
	LuaTable ret = table.SubTable(name);
	if (ret.IsValid()) {
		if (!parentName.empty())
			parentMap[name] = parentName;
		return ret;
	}

	std::vector<std::string> keys;
	table.GetKeys(keys);
	for (const std::string& key: keys) {
		ret = GetPieceTableRecursively(table.SubTable(key), name, key, parentMap);
		if (ret.IsValid())
			break;
	}
	return ret;
}


SAssPiece* CAssParser::AllocPiece()
{
	RECOIL_DETAILED_TRACY_ZONE;
	return static_cast<SAssPiece*>(AllocPieceImpl());
}

SAssPiece* CAssParser::LoadPiece(
	S3DModel* model,
	const aiNode* pieceNode,
	const aiScene* scene,
	const LuaTable& modelTable,
	const std::vector<std::string>& skipList,
	ModelPieceMap& pieceMap,
	ParentNameMap& parentMap,
	const CQuaternion* optRotation
) {
	RECOIL_DETAILED_TRACY_ZONE;
	if (std::find(skipList.begin(), skipList.end(), std::string(pieceNode->mName.data)) != skipList.end())
		return nullptr;

	++model->numPieces;

	SAssPiece* piece = AllocPiece();

	if (pieceNode->mParent == nullptr) {
		// set the model's root piece ASAP, needed in SetPiece*Name
		assert(pieceNode == scene->mRootNode);
		model->AddPiece(piece);
	}

	SetPieceName(piece, model, pieceNode, pieceMap);
	piece->SetParentModel(model);

	LOG_SL(LOG_SECTION_PIECE, L_INFO, "Converting node '%s' to piece '%s' (%d meshes).", pieceNode->mName.data, piece->name.c_str(), pieceNode->mNumMeshes);

	// Load additional piece properties from metadata
	const LuaTable& pieceTable = GetPieceTableRecursively(modelTable.SubTable("pieces"), piece->name, "", parentMap);

	if (pieceTable.IsValid())
		LOG_SL(LOG_SECTION_PIECE, L_INFO, "Found metadata for piece '%s'", piece->name.c_str());

	LoadPieceTransformations(piece, model, pieceNode, pieceTable, optRotation);
	LoadPieceGeometry(piece, model, pieceNode, scene);
	SetPieceParentName(piece, model, pieceNode, pieceTable, parentMap);

	{
		// operator[] creates an empty string if piece is not in map
		const auto parentNameIt = parentMap.find(piece->name);
		const std::string& parentName = (parentNameIt != parentMap.end())? (parentNameIt->second).c_str(): "[null]";

		// Verbose logging of piece properties
		LOG_SL(LOG_SECTION_PIECE, L_INFO, "Loaded model piece: %s with %d meshes", piece->name.c_str(), pieceNode->mNumMeshes);
		LOG_SL(LOG_SECTION_PIECE, L_INFO, "piece->name: %s", piece->name.c_str());
		LOG_SL(LOG_SECTION_PIECE, L_INFO, "piece->parent: %s", parentName.c_str());
	}

	// Recursively process all child pieces
	for (uint32_t i = 0; i < pieceNode->mNumChildren; ++i) {
		LoadPiece(model, pieceNode->mChildren[i], scene, modelTable, skipList, pieceMap, parentMap);
	}

	pieceMap[piece->name] = piece;
	return piece;
}


// Because of metadata overrides we don't know the true hierarchy until all pieces have been loaded
void CAssParser::BuildPieceHierarchy(S3DModel* model, ModelPieceMap& pieceMap, const ParentNameMap& parentMap)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const char* fmt1 = "Missing piece '%s' declared as parent of '%s'.";
	const char* fmt2 = "Missing root piece (parent of orphan '%s')";

	// loop through all pieces and create missing hierarchy info
	for (auto it = pieceMap.cbegin(); it != pieceMap.cend(); ++it) {
		SAssPiece* piece = static_cast<SAssPiece*>(it->second);

		if (piece == model->GetRootPiece()) {
			assert(piece->parent == nullptr);
			assert(model->GetRootPiece() == piece);
			continue;
		}

		const auto parentNameIt = parentMap.find(piece->name);

		if (parentNameIt != parentMap.end()) {
			const std::string& parentName = parentNameIt->second;
			const auto pieceIt = pieceMap.find(parentName);

			// re-assign this piece to a different parent
			if (pieceIt != pieceMap.end()) {
				piece->parent = pieceIt->second;
				piece->parent->children.push_back(piece);
			} else {
				LOG_SL(LOG_SECTION_PIECE, L_ERROR, fmt1, parentName.c_str(), piece->name.c_str());
			}

			continue;
		}

		// piece with no named parent that isn't the root (orphaned)
		// link it to the root piece which has already been pre-added
		if ((piece->parent = model->GetRootPiece()) == nullptr) {
			LOG_SL(LOG_SECTION_PIECE, L_ERROR, fmt2, piece->name.c_str());
		} else {
			piece->parent->children.push_back(piece);
		}
	}

	model->FlattenPieceTree(model->GetRootPiece());
	model->SetPieceMatrices();
}

static std::string FindTexture(std::string testTextureFile, const std::string& modelPath, const std::string& fallback)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (testTextureFile.empty())
		return fallback;

	// blender denotes relative paths with "//..", remove it
	if (testTextureFile.starts_with("//.."))
		testTextureFile = testTextureFile.substr(4);

	if (CFileHandler::FileExists(testTextureFile, SPRING_VFS_ZIP_FIRST))
		return testTextureFile;

	if (CFileHandler::FileExists("unittextures/" + testTextureFile, SPRING_VFS_ZIP_FIRST))
		return "unittextures/" + testTextureFile;

	if (CFileHandler::FileExists(modelPath + testTextureFile, SPRING_VFS_ZIP_FIRST))
		return modelPath + testTextureFile;

	return fallback;
}


static std::string FindTextureByRegex(const std::string& regex_path, const std::string& regex)
{
	RECOIL_DETAILED_TRACY_ZONE;
	//FIXME instead of ".*" only check imagetypes!
	const std::vector<std::string>& files = CFileHandler::FindFiles(regex_path, regex + ".*");

	if (!files.empty())
		return FindTexture(FileSystem::GetFilename(files[0]), "", "");

	return "";
}


void CAssParser::FindTextures(
	S3DModel* model,
	const aiScene* scene,
	const LuaTable& modelTable,
	const std::string& modelPath,
	const std::string& modelName
) {
	RECOIL_DETAILED_TRACY_ZONE;
	// 1. try to find by name (lowest priority)
	model->texs[0] = FindTextureByRegex("unittextures/", modelName);

	if (model->texs[0].empty()) model->texs[0] = FindTextureByRegex("unittextures/", modelName + "1");
	if (model->texs[1].empty()) model->texs[1] = FindTextureByRegex("unittextures/", modelName + "2");
	if (model->texs[0].empty()) model->texs[0] = FindTextureByRegex(modelPath, "tex1");
	if (model->texs[1].empty()) model->texs[1] = FindTextureByRegex(modelPath, "tex2");
	if (model->texs[0].empty()) model->texs[0] = FindTextureByRegex(modelPath, "diffuse");
	if (model->texs[1].empty()) model->texs[1] = FindTextureByRegex(modelPath, "glow"); // lowest-priority name

	// 2. gather model-defined textures of first material (medium priority)
	if (scene->mNumMaterials > 0) {
		constexpr uint32_t texTypes[] = {
			aiTextureType_SPECULAR,
			aiTextureType_UNKNOWN,
			aiTextureType_DIFFUSE,
			/*
			// TODO: support these too (we need to allow constructing tex1 & tex2 from several sources)
			aiTextureType_EMISSIVE,
			aiTextureType_HEIGHT,
			aiTextureType_NORMALS,
			aiTextureType_SHININESS,
			aiTextureType_OPACITY,
			*/
		};
		for (uint32_t texType: texTypes) {
			aiString textureFile;
			if (scene->mMaterials[0]->Get(AI_MATKEY_TEXTURE(texType, 0), textureFile) != aiReturn_SUCCESS)
				continue;

			assert(textureFile.length > 0);
			model->texs[0] = FindTexture(textureFile.data, modelPath, model->texs[0]);
		}
	}

	// 3. try to load from metafile (highest priority)
	model->texs[0] = FindTexture(modelTable.GetString("tex1", ""), modelPath, model->texs[0]);
	model->texs[1] = FindTexture(modelTable.GetString("tex2", ""), modelPath, model->texs[1]);
}


