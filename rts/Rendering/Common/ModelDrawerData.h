#pragma once

#include <vector>
#include <array>
#include <cstring>
#include <functional>

#include <unordered_map>

#include "System/EventClient.h"
#include "System/EventHandler.h"
#include "System/ContainerUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/Threading/ThreadPool.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Models/ModelsMemStorage.h"
#include "Rendering/Models/ModelRenderContainer.h"
#include "Rendering/Models/3DModel.hpp"
#include "Rendering/Env/IWater.h"
#include "Map/ReadMap.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Game/CameraHandler.h"

class CModelDrawerDataConcept : public CEventClient {
public:
	CModelDrawerDataConcept(const std::string& ecName, int ecOrder)
		: CEventClient(ecName, ecOrder, false)
	{};
	virtual ~CModelDrawerDataConcept() {
		eventHandler.RemoveClient(this);
		autoLinkedEvents.clear();
	};
public:
	bool GetFullRead() const override { return true; }
	int  GetReadAllyTeam() const override { return AllAccessTeam; }
protected:
	// the per-frame passes mostly recompute unchanged values; storing them anyway moves
	// the cache line to the storing core, a different one each frame in MT passes
	template<typename V>
	static bool StoreIfChanged(V& dst, const V& src) {
		if (std::memcmp(&dst, &src, sizeof(V)) == 0)
			return false;

		dst = src;
		return true;
	}
protected:
	static constexpr int MT_CHUNK_OR_MIN_CHUNK_SIZE_SMMA = 128;
	static constexpr int MT_CHUNK_OR_MIN_CHUNK_SIZE_UPDT = 256;
};


template <typename T>
class CModelDrawerDataBase : public CModelDrawerDataConcept
{
public:
	using ObjType = T;
public:
	CModelDrawerDataBase(const std::string& ecName, int ecOrder, bool& mtModelDrawer_);
	virtual ~CModelDrawerDataBase() override;
public:
	void Update() override = 0;
protected:
	virtual bool IsAlpha(const T* co) const = 0;
private:
	void AddObject(const T* co, bool add); //never to be called directly! Use UpdateObject() instead!
protected:
	void DelObject(const T* co, bool del);
	void UpdateObject(const T* co, bool init);
protected:
	// k is the object's index into unsortedObjects
	void UpdateCommon(size_t k);
	virtual void UpdateObjectDrawFlags(CSolidObject* o) const = 0;

	// per-frame inputs of UpdateObjectDrawFlags, set before the per-object pass
	void UpdateDrawFlagsCameras();

	const CCamera* camPlayer = nullptr;
	const CCamera* camUWRefl = nullptr; // nullptr if there is no reflection pass
	const CCamera* camShadow = nullptr; // nullptr if models cast no shadows
private:
	// render data of unsortedObjects[k], so the per-frame passes need no lookups
	struct ObjectData {
		ScopedTransformMemAlloc transformAlloc;
		size_t uniformsOffset = 0;
		int32_t lastSyncedFrameUpload = std::numeric_limits<int32_t>::lowest(); // update at least once before the sim starts
	};

	void UpdateObjectTrasform(const T* o, ObjectData& od);
	void UpdateObjectUniforms(const T* o, const ObjectData& od);
public:
	const std::vector<T*>& GetUnsortedObjects() const { return unsortedObjects; }
	const ModelRenderContainer<T>& GetModelRenderer(int modelType) const { return modelRenderers[modelType]; }

	void ClearPreviousDrawFlags() { for (auto object : unsortedObjects) object->previousDrawFlag = 0; }

	const ScopedTransformMemAlloc& GetObjectTransformMemAlloc(const T* o) const {
		const auto it = objectIndices.find(o);
		return (it != objectIndices.end()) ? objectsData[it->second].transformAlloc : ScopedTransformMemAlloc::Dummy();
	}
protected:
	std::array<ModelRenderContainer<T>, MODELTYPE_CNT> modelRenderers;

	std::vector<T*> unsortedObjects;
private:
	std::vector<ObjectData> objectsData;
	spring::unordered_map<const T*, size_t> objectIndices;
protected:
	bool& mtModelDrawer;
};

using CUnitDrawerDataBase = CModelDrawerDataBase<CUnit>;
using CFeatureDrawerDataBase = CModelDrawerDataBase<CFeature>;

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline CModelDrawerDataBase<T>::CModelDrawerDataBase(const std::string& ecName, int ecOrder, bool& mtModelDrawer_)
	: CModelDrawerDataConcept(ecName, ecOrder)
	, mtModelDrawer(mtModelDrawer_)
{
	for (auto& mr : modelRenderers) { mr.Clear(); }
}

template<typename T>
inline CModelDrawerDataBase<T>::~CModelDrawerDataBase()
{
	unsortedObjects.clear();
	objectsData.clear();
	objectIndices.clear();
}

template<typename T>
inline void CModelDrawerDataBase<T>::AddObject(const T* co, bool add)
{
	T* o = const_cast<T*>(co);

	if (o->model != nullptr) {
		modelRenderers[MDL_TYPE(o)].AddObject(o);
	}

	if (!add)
		return;

	objectIndices.emplace(o, unsortedObjects.size());
	unsortedObjects.emplace_back(o);

	const uint32_t numMatrices = ((o->model ? o->model->numPieces : 0) + 1u) * 2;
	objectsData.push_back({ ScopedTransformMemAlloc(numMatrices), modelUniformsStorage.AddObject(co) });
}

template<typename T>
inline void CModelDrawerDataBase<T>::DelObject(const T* co, bool del)
{
	T* o = const_cast<T*>(co);

	if (o->model != nullptr) {
		modelRenderers[MDL_TYPE(o)].DelObject(o);
	}

	if (!del)
		return;

	const auto it = objectIndices.find(o);
	if (it == objectIndices.end())
		return;

	// move the last object into the hole, like spring::VectorErase
	const size_t k = it->second;
	objectIndices.erase(it);

	if (k + 1 != unsortedObjects.size()) {
		unsortedObjects[k] = unsortedObjects.back();
		objectsData[k] = std::move(objectsData.back()); // swaps the transform allocs, pop_back frees o's
		objectIndices[unsortedObjects[k]] = k;
	}

	unsortedObjects.pop_back();
	objectsData.pop_back();

	modelUniformsStorage.DelObject(co);
}

template<typename T>
inline void CModelDrawerDataBase<T>::UpdateObject(const T* co, bool init)
{
	DelObject(co, false);
	AddObject(co, init );
}


template<typename T>
inline void CModelDrawerDataBase<T>::UpdateObjectTrasform(const T* o, ObjectData& od)
{
	// check if already uploaded
	if (od.lastSyncedFrameUpload >= gs->frameNum)
		return;

	ScopedTransformMemAlloc& stma = od.transformAlloc;

	const auto& tmPrev = o->preFrameTra;
	const auto  tmCurr = Transform::FromMatrix(o->GetTransformMatrix(true)); //synced transform

	// conditionally update new and prev synced positions
	stma.UpdateIfChanged(0, tmPrev);
	stma.UpdateIfChanged(1, tmCurr);

	for (int i = 0; i < o->localModel.pieces.size(); ++i) {
		const LocalModelPiece& lmp = o->localModel.pieces[i];

		const auto& lmpTransform = lmp.GetModelSpaceTransform(); //forces dirty / wasUpdated recalculation if no other method called it yet

		if likely(!lmp.GetWasUpdated())
			continue;

		if unlikely(!lmp.GetScriptVisible()) {
			stma.UpdateForced(2 * (1 + i) + 0, Transform::Zero());
			stma.UpdateForced(2 * (1 + i) + 1, Transform::Zero());
			lmp.ResetWasUpdated();
			continue;
		}

		stma.UpdateForced(2 * (1 + i) + 0, lmp.GetEffectivePrevModelSpaceTransform());
		stma.UpdateForced(2 * (1 + i) + 1, lmpTransform);

		lmp.ResetWasUpdated();
	}

	od.lastSyncedFrameUpload = gs->frameNum;
}

template<typename T>
inline void CModelDrawerDataBase<T>::UpdateObjectUniforms(const T* o, const ObjectData& od)
{
	auto& uni = modelUniformsStorage.GetUniformsAt(od.uniformsOffset);

	bool changed = StoreIfChanged(uni.drawFlag, o->drawFlag);

	if (gu->spectatingFullView || o->IsInLosForAllyTeam(gu->myAllyTeam)) {
		changed |= StoreIfChanged(uni.id, static_cast<uint16_t>(o->id));
		changed |= StoreIfChanged(uni.teamID, static_cast<uint8_t>(o->team));
		// TODO remove drawPos, replace with pos
		changed |= StoreIfChanged(uni.drawPos, float4{ o->drawPos, o->heading * math::PI / SPRING_MAX_HEADING });
		changed |= StoreIfChanged(uni.speed, o->speed);
		changed |= StoreIfChanged(uni.maxHealth, o->maxHealth);
		changed |= StoreIfChanged(uni.health, o->health);
	}

	if (changed)
		modelUniformsStorage.SetUpdate(od.uniformsOffset);
}

template<typename T>
inline void CModelDrawerDataBase<T>::UpdateDrawFlagsCameras()
{
	camPlayer = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);
	camUWRefl = IWater::GetWater()->CanDrawReflectionPass() ? CCameraHandler::GetCamera(CCamera::CAMTYPE_UWREFL) : nullptr;
	camShadow = ((shadowHandler.shadowGenBits & CShadowHandler::SHADOWGEN_BIT_MODEL) != 0) ? CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW) : nullptr;
}

template<typename T>
inline void CModelDrawerDataBase<T>::UpdateCommon(size_t k)
{
	T* o = unsortedObjects[k];
	ObjectData& od = objectsData[k];

	assert(o);
	StoreIfChanged(o->previousDrawFlag, o->drawFlag);
	UpdateObjectDrawFlags(o);

	if (o->alwaysUpdateMat || (o->drawFlag > DrawFlags::SO_NODRAW_FLAG && o->drawFlag < DrawFlags::SO_DRICON_FLAG))
		UpdateObjectTrasform(o, od);

	UpdateObjectUniforms(o, od);
}