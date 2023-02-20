/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SMF_GROUND_DRAWER_H_
#define _SMF_GROUND_DRAWER_H_

#include "Map/BaseGroundDrawer.h"
#include "Map/SMF/SMFRenderState.h"
#include "Rendering/GL/GeometryBuffer.h"
#include "Rendering/GL/LightHandler.h"
#include "Rendering/GL/RenderBuffersFwd.h"
#include "Rendering/GL/VAO.h"

#include <memory>


class CSMFReadMap;
class IMeshDrawer;
struct ISMFRenderState;

namespace Shader {
	struct IProgramObject;
}

enum {
	SMF_MESHDRAWER_LEGACY = 0,
	SMF_MESHDRAWER_BASIC  = 1,
	SMF_MESHDRAWER_ROAM   = 2,
	SMF_MESHDRAWER_LAST   = 3,
};


/**
 * Map drawer implementation for the CSMFReadMap map system.
 */
class CSMFGroundDrawer : public CBaseGroundDrawer
{
public:
	CSMFGroundDrawer(CSMFReadMap* rm);
	~CSMFGroundDrawer();

	friend class CSMFReadMap;

	void Draw(const DrawPass::e& drawPass);
	void DrawDeferredPass(const DrawPass::e& drawPass, bool alphaTest);
	void DrawForwardPass(const DrawPass::e& drawPass, bool alphaTest);
	void DrawShadowPass();

	void Update();
	void UpdateRenderState();
	void SunChanged();

	void SetLuaShader(const LuaMapShaderData*);
	void SetDrawDeferredPass(bool b) {
		if ((drawDeferred = b)) {
			drawDeferred &= UpdateGeometryBuffer(false);
		}
	}

	void SetupBigSquare(const int bigSquareX, const int bigSquareY);


	void IncreaseDetail() { SetDetail(groundDetail + 1); }
	void DecreaseDetail() { SetDetail(groundDetail - 1); }
	void SetDetail(int newGroundDetail);
	int GetGroundDetail(const DrawPass::e& drawPass = DrawPass::Normal) const;

	const CSMFReadMap* GetReadMap() const { return smfMap; }
	      CSMFReadMap* GetReadMap()       { return smfMap; }
	const GL::LightHandler* GetLightHandler() const { return &lightHandler; }
	      GL::LightHandler* GetLightHandler()       { return &lightHandler; }

	const GL::GeometryBuffer* GetGeometryBuffer() const { return &geomBuffer; }
	      GL::GeometryBuffer* GetGeometryBuffer()       { return &geomBuffer; }

	IMeshDrawer* GetMeshDrawer() { return meshDrawer; }
	IMeshDrawer* SwitchMeshDrawer(int wantedMode = -1);

private:
	ISMFRenderState* SelectRenderState(const DrawPass::e& drawPass);

	inline void DrawBorder(const DrawPass::e drawPass);

	bool HaveLuaRenderState() const;
	bool UpdateGeometryBuffer(bool init);

	bool alwaysDispatchEvents = false;
protected:
	CSMFReadMap* smfMap;
	IMeshDrawer* meshDrawer;

	int drawerMode;
	int groundDetail;

	// [0] := fallback shader-less rendering path
	// [1] := default shader-driven rendering path
	// [2] := custom shader-driven rendering path (via Lua)
	// [3] := currently selected state (shared by deferred pass)
	std::array<ISMFRenderState*, RENDER_STATE_CNT> smfRenderStates;

	GL::LightHandler lightHandler;
	GL::GeometryBuffer geomBuffer;

	Shader::IProgramObject* borderShader = nullptr;
};

#endif // _SMF_GROUND_DRAWER_H_
