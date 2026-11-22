/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <array>
#include <ranges>
#include <unordered_map>

#include "System/float3.h"
#include "System/float4.h"
#include "System/Matrix44f.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Rendering/Common/ModelDrawerData.h"
#include "Rendering/UnitDefImage.h"
#include "Game/GlobalUnsynced.h"

struct S3DModel;
class CUnitDrawer;
struct UnitDef;

namespace GL {
	struct GeometryBuffer;
}

// Frozen at LOS loss. Never read a hidden unit to resolve a player's target.
struct GhostTarget {
	CR_DECLARE_STRUCT(GhostTarget)
	float sightingKey = 0;
	CMatrix44f modelTransform;
	float4 teamColor;
	std::string modelName;
	const S3DModel* GetModel() const;
	mutable const S3DModel* model = nullptr;
	float3 midPos;
	float radius = 0.0f;
	size_t iconIndex = 0;
	CMatrix44f selectionTransform;
	CollisionVolume selectionVolume;
	bool selectable = false;
	bool minimapIcon = true;
};

class GhostSolidObject {
	CR_DECLARE_STRUCT(GhostSolidObject)
public:
	void PostLoad();
	GhostTarget target;
	// color identity captured when the ghost was created; a ghost keeps the color it was last
	// seen under. team drives the legacy (GLSL) team-color path, paletteIndex drives the GL4
	// per-instance palette (equal to team unless a custom Lua color palette was assigned).
	uint8_t team;
	uint16_t paletteIndex;


	ScopedTransformMemAlloc worldTransformAlloc;
};

class CUnitDrawerData : public CUnitDrawerDataBase {
public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) override {
		return
			eventName == "RenderUnitPreCreated" ||
			eventName == "RenderUnitCreated" || eventName == "RenderUnitDestroyed" ||
			eventName == "UnitEnteredRadar"  || eventName == "UnitEnteredLos"      ||
			eventName == "UnitLeftRadar"     || eventName == "UnitLeftLos"         ||
			eventName == "PlayerChanged";
	}

	void RenderUnitPreCreated(const CUnit* unit) override;
	void RenderUnitCreated(const CUnit* unit, int cloaked) override;
	void RenderUnitDestroyed(const CUnit* unit) override;

	void UnitEnteredRadar(const CUnit* unit, int allyTeam) override;
	void UnitLeftRadar(const CUnit* unit, int allyTeam) override { UnitEnteredRadar(unit, allyTeam); }

	void UnitEnteredLos(const CUnit* unit, int allyTeam) override;
	void UnitLeftLos(const CUnit* unit, int allyTeam) override;

	void PlayerChanged(int playerID) override;

	bool UpdateUnitGhosts(const CUnit* unit);
	void UnitLeavesGhostChanged(const CUnit* unit);
	bool TraceGhost(const float3& start, const float3& dir, float length, float& ghostID, int& unitID, const CUnit* excludeUnit) const;
public:
	class TempDrawUnit {
		CR_DECLARE_STRUCT(TempDrawUnit)
	public:
		const UnitDef* GetUnitDef() const;
		int unitDefId;

		int team;
		int facing;
		int timeout;

		float3 pos;
		float rotation;

		bool drawAlpha;
		bool drawBorder;
	private:
		mutable const UnitDef* unitDef;
	};
	// a still-alive building that left an observer's LOS. The unit is drawn as a ghost using the
	// color it was last seen under (snapshotted here), so it does not silently recolor if the live
	// unit changes team while out of LOS. team feeds the legacy (GLSL) team-color path, paletteIndex
	// feeds the GL4 per-instance palette (equal to team unless a custom Lua palette was assigned).
	struct LiveGhostBuilding {
		CR_DECLARE_STRUCT(LiveGhostBuilding)
		CUnit* unit = nullptr;
		GhostTarget target;
		uint16_t paletteIndex = 0;
		uint8_t team = 0;
	};
	struct SavedData {
		CR_DECLARE_STRUCT(SavedData)

		/// AI unit ghosts
		std::array< std::vector<TempDrawUnit>, MODELTYPE_CNT> tempOpaqueUnits;
		std::array< std::vector<TempDrawUnit>, MODELTYPE_CNT> tempAlphaUnits;

		/// buildings that were in LOS_PREVLOS when they died and not in LOS since
		std::vector<std::array<std::vector<GhostSolidObject*>, MODELTYPE_CNT>> deadGhostBuildings;

		/// buildings that left LOS but are still alive
		std::vector<std::array<std::vector<LiveGhostBuilding>, MODELTYPE_CNT>> liveGhostBuildings;
	};
public:
	CUnitDrawerData(bool& mtModelDrawer_);
	virtual ~CUnitDrawerData();
public:
	void SetUnitIconDist(float dist) {
		unitIconDist = dist;
		iconLength = unitIconDist * unitIconDist * 750.0f;
	}

	// IconsAsUI
	float GetUnitIconScaleUI() const { return iconScale; }
	float GetUnitIconFadeStart() const { return iconFadeStart; }
	float GetUnitIconFadeVanish() const { return iconFadeVanish; }
	void SetUnitIconScaleUI(float scale) { iconScale = std::clamp(scale, 0.1f, 10.0f); }
	void SetUnitIconFadeStart(float scale) { iconFadeStart = std::clamp(scale, 1.0f, 10000.0f); }
	void SetUnitIconFadeVanish(float scale) { iconFadeVanish = std::clamp(scale, 1.0f, 10000.0f); }

	// *UnitDefImage
	void SetUnitDefImage(const UnitDef* unitDef, const std::string& texName);
	void SetUnitDefImage(const UnitDef* unitDef, unsigned int texID, int xsize, int ysize);
	uint32_t GetUnitDefImage(const UnitDef* unitDef);
public:
	void Update() override;
	bool IsAlpha(const CUnit* co) const override { return co->IsCloaked(); }
public:
	void AddTempDrawUnit(const TempDrawUnit& tempDrawUnit);

	void UpdateGhostedBuildings();
	void UpdateUnitIconsByUnitDef(const UnitDef* ud);
public:
	void UpdateCurrentUnitIcon(const CUnit* unit);
	/// icon shown to an arbitrary viewer (same rules that maintain currentIconIndex for the local view)
	size_t GetUnitIconIndex(const CUnit* unit, int allyTeam, bool fullView) const;

	const auto& GetUnitDefImages() const { return unitDefImages; }
	      auto& GetUnitDefImages() { return unitDefImages; }

	const auto& GetTempOpaqueDrawUnits(int modelType) const { return savedData.tempOpaqueUnits[modelType]; }
	const auto& GetTempAlphaDrawUnits(int modelType) const { return  savedData.tempAlphaUnits[modelType]; }

	auto GetDeadGhostBuildings(int allyTeam) const {
		assert((unsigned)allyTeam < savedData.deadGhostBuildings.size());
		return std::views::join(savedData.deadGhostBuildings[allyTeam]);
	}

	const auto& GetDeadGhostBuildings(int allyTeam, int modelType) const {
		assert((unsigned)allyTeam < savedData.deadGhostBuildings.size());
		return savedData.deadGhostBuildings[allyTeam][modelType];
	}
	const auto& GetLiveGhostBuildings(int allyTeam, int modelType) const {
		assert((unsigned)allyTeam < savedData.liveGhostBuildings.size());
		return savedData.liveGhostBuildings[allyTeam][modelType];
	}

	// world transform slot (in transformsMemStorage) for a live ghost building; INVALID_INDEX if none
	size_t GetLiveGhostTransform(float ghostID) const {
		const auto it = liveGhostTransforms.find(ghostID);
		return (it != liveGhostTransforms.end()) ? it->second.first.GetOffset(false) : TransformsMemStorage::INVALID_INDEX;
	}

	auto*       GetSavedData()       { return &savedData; }
	const auto* GetSavedData() const { return &savedData; }
protected:
	void UpdateObjectDrawFlags(CSolidObject* o) const override;
private:
	void UpdateTempDrawUnits(std::vector<TempDrawUnit>& tempDrawUnits);

	void UpdateUnitIconState(CUnit* unit);
	void UpdateUnitIconStateScreen(CUnit* unit);
	static void UpdateDrawPos(CUnit* unit);

	/// Returns true if the given unit should be drawn as icon in the current frame.
	bool DrawAsIconByDistance(const CUnit* unit, const float sqUnitCamDist) const;
	//bool DrawAsIconScreen(CUnit* unit) const;
public:
	// lengths & distances
	float unitIconDist;
	float iconLength;

	//icons
	bool iconHideWithUI = true;
	bool sortUnitIconsByDepth = false;
	float ghostIconDimming = 0.5f;

	// IconsAsUI
	bool useScreenIcons = false;
	float iconZoomDist;
	float iconSizeBase = 32.0f;
	float iconScale = 1.0f;
	float iconFadeStart = 3000.0f;
	float iconFadeVanish = 1000.0f;

	void ConfigNotify(const std::string& key, const std::string& value);
private:
	SavedData savedData;

	std::vector<UnitDefImage> unitDefImages;

	S3DModel* GetUnitModel(const CUnit* unit) const;
	void RemoveDeadGhost(GhostSolidObject* gso, std::vector<GhostSolidObject*>& dgb, int index);

	// rebuilds the per-unit world transform slots for live ghost buildings of the local allyTeam.
	// scan-based so it self-heals across savegame load, allyTeam/spectator changes and leavesGhost toggles.
	void UpdateLiveGhostTransforms();
	// maps sighting ID -> { RAII-owned world transform slot, last sweep stamp seen }
	std::unordered_map<float, std::pair<ScopedTransformMemAlloc, int>> liveGhostTransforms;
	int liveGhostSweepStamp = 0;

	// icons
	bool useDistToGroundForIcons;
	float sqCamDistToGroundForIcons;

	// IconsAsUI
	static constexpr float iconSizeMult = 0.005f; // 1/200
};
