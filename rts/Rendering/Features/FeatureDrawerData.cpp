#include "FeatureDrawerData.h"

#include "System/Config/ConfigHandler.h"
#include "System/StringHash.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Common/ModelDrawerHelpers.h"

#include "System/Misc/TracyDefs.h"

CONFIG(float, FeatureDrawDistance)
.defaultValue(6000.0f)
.minimumValue(0.0f)
.description("Maximum distance at which features will be drawn.");

CONFIG(float, FeatureFadeDistance)
.defaultValue(4500.0f)
.minimumValue(0.0f)
.description("Distance at which features will begin to fade from view.");


void CFeatureDrawerData::RenderFeaturePreCreated(const CFeature* feature)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (feature->def->drawType != DRAWTYPE_MODEL)
		return;

	UpdateObject(feature, true);
}

//TODO remove
void CFeatureDrawerData::RenderFeatureCreated(const CFeature* feature)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(
		feature->def->drawType != DRAWTYPE_MODEL ||
		std::find(unsortedObjects.begin(), unsortedObjects.end(), feature) != unsortedObjects.end()
	);
}

void CFeatureDrawerData::RenderFeatureDestroyed(const CFeature* feature)
{
	RECOIL_DETAILED_TRACY_ZONE;
	DelObject(feature, feature->def->drawType == DRAWTYPE_MODEL);
	LuaObjectDrawer::SetObjectLOD(const_cast<CFeature*>(feature), LUAOBJ_FEATURE, 0);
}

CFeatureDrawerData::CFeatureDrawerData(bool& mtModelDrawer_)
	: CFeatureDrawerDataBase("[CFeatureDrawerData]", 313373, mtModelDrawer_)
{
	RECOIL_DETAILED_TRACY_ZONE;
	eventHandler.AddClient(this); //cannot be done in CModelRenderDataConcept, because object is not fully constructed
	configHandler->NotifyOnChange(this, { "FeatureDrawDistance", "FeatureFadeDistance" });

	featureDrawDistance = configHandler->GetFloat("FeatureDrawDistance");
	featureFadeDistance = std::min(configHandler->GetFloat("FeatureFadeDistance"), featureDrawDistance);
}

CFeatureDrawerData::~CFeatureDrawerData()
{
	RECOIL_DETAILED_TRACY_ZONE;
	configHandler->RemoveObserver(this);
}

void CFeatureDrawerData::ConfigNotify(const std::string& key, const std::string& value)
{
	RECOIL_DETAILED_TRACY_ZONE;
	switch (hashStringLower(key.c_str())) {
	case hashStringLower("FeatureDrawDistance"): {
		featureDrawDistance = std::strtof(value.c_str(), nullptr);
	} break;
	case hashStringLower("FeatureFadeDistance"): {
		featureFadeDistance = std::strtof(value.c_str(), nullptr);
	} break;
	default: {} break;
	}

	featureDrawDistance = std::max(0.0f, featureDrawDistance);
	featureFadeDistance = std::max(0.0f, featureFadeDistance);
	featureFadeDistance = std::min(featureFadeDistance, featureDrawDistance);

	LOG_L(L_INFO, "[FeatureDrawer::%s] {draw,fade}distance set to {%f,%f}", __func__, featureDrawDistance, featureFadeDistance);
}

void CFeatureDrawerData::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	UpdateDrawFlagsCameras();

	const auto updateBody = [this](size_t k) {
		UpdateDrawPos(unsortedObjects[k]);
		UpdateCommon(k);
	};

	if (mtModelDrawer) {
		for_mt_chunk(0, unsortedObjects.size(), updateBody, MT_CHUNK_SIZE_UPDT_MIN, MT_CHUNK_SIZE_UPDT_MAX);
	}
	else {
		for (size_t k = 0; k < unsortedObjects.size(); ++k)
			updateBody(k);
	}
}

bool CFeatureDrawerData::IsAlpha(const CFeature* co) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (co->drawAlpha < 1.0f);
}

void CFeatureDrawerData::UpdateObjectDrawFlags(CSolidObject* o) const
{
	RECOIL_DETAILED_TRACY_ZONE;

	CFeature* f = static_cast<CFeature*>(o);

	// drawAlpha keeps its value while the player camera does not see the feature
	uint8_t drawFlag = DrawFlags::SO_NODRAW_FLAG;
	float drawAlpha = f->drawAlpha;

	if (!f->noDraw && !f->IsInVoid() && (f->IsInLosForAllyTeam(gu->myAllyTeam) || gu->spectatingFullView)) {
		const float drawRadius = f->GetDrawRadius();

		if (camPlayer->InView(f->drawMidPos, drawRadius)) {
			const float camDist = (f->drawPos - camPlayer->GetPos()).Length();

			if (!f->alphaFade) {
				// special case for non-fading features
				drawFlag = DrawFlags::SO_OPAQUE_FLAG;
				drawAlpha = 1.0f;
			} else if (camDist > featureDrawDistance) {
				// too far, don't draw at all
				drawAlpha = 0.0f;
			} else if (camDist < featureFadeDistance) {
				// close enough to draw solid
				drawAlpha = 1.0f;
				drawFlag = DrawFlags::SO_OPAQUE_FLAG;

				if (f->IsInWater())
					drawFlag |= DrawFlags::SO_REFRAC_FLAG;
			} else if (featureDrawDistance == featureFadeDistance) {
				// fading is disabled, just don't draw
				drawAlpha = 0.0f;
			} else {
				drawAlpha = std::max(0.0f, 1.0f - (camDist - featureFadeDistance) / (featureDrawDistance - featureFadeDistance));
				drawFlag = DrawFlags::SO_ALPHAF_FLAG;

				if (f->IsInWater())
					drawFlag |= DrawFlags::SO_REFRAC_FLAG;
			}
		}

		if (camUWRefl != nullptr && drawAlpha > 0.0f && (drawFlag & (DrawFlags::SO_OPAQUE_FLAG | DrawFlags::SO_ALPHAF_FLAG)) != 0) {
			if (camUWRefl->InView(f->drawMidPos, drawRadius) && CModelDrawerHelper::ObjectVisibleReflection(f->drawMidPos, camUWRefl->GetPos(), drawRadius))
				drawFlag |= DrawFlags::SO_REFLEC_FLAG;
		}

		if (camShadow != nullptr && drawAlpha > 0.0f && camShadow->InView(f->drawMidPos, drawRadius))
			drawFlag |= ((drawAlpha < 1.0f) ? DrawFlags::SO_SHTRAN_FLAG : DrawFlags::SO_SHOPAQ_FLAG);
	}

	spring::StoreIfChanged(f->drawAlpha, drawAlpha);
	spring::StoreIfChanged(f->drawFlag, drawFlag);

	if (f->alwaysUpdateMat || (drawFlag > DrawFlags::SO_NODRAW_FLAG && drawFlag < DrawFlags::SO_DRICON_FLAG)) {
		const CMatrix44f drawMat = f->ComposeMatrix(f->drawPos);

		if (std::memcmp(&drawMat, &f->GetTransformMatrixRef(false), sizeof(drawMat)) != 0)
			f->SetTransform(drawMat, false);
	}
}

void CFeatureDrawerData::UpdateDrawPos(CFeature* f)
{
	RECOIL_DETAILED_TRACY_ZONE;
	spring::StoreIfChanged(f->drawPos, f->GetDrawPos(globalRendering->timeOffset));
	spring::StoreIfChanged(f->drawMidPos, f->GetMdlDrawMidPos());
}