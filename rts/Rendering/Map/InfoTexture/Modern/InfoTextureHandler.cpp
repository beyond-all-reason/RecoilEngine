/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "InfoTextureHandler.h"
#include "AirLos.h"
#include "Combiner.h"
#include "Height.h"
#include "Los.h"
#include "Metal.h"
#include "MetalExtraction.h"
#include "Path.h"
#include "Radar.h"
#include "Rendering/GlobalRendering.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/StringHash.h"

#include "System/Misc/TracyDefs.h"




CInfoTextureHandler::CInfoTextureHandler()
{
	AddInfoTexture(infoTex = new CInfoTextureCombiner());
	AddInfoTexture(new CLosTexture());
	AddInfoTexture(new CAirLosTexture());
	AddInfoTexture(new CMetalTexture());
	AddInfoTexture(new CMetalExtractionTexture());
	AddInfoTexture(new CRadarTexture());
	AddInfoTexture(new CHeightTexture());
	AddInfoTexture(new CPathTexture());
	// TODO?
	//AddInfoTexture(new CHeatTexture());
	//AddInfoTexture(new CFlowTexture());
	//AddInfoTexture(new CPathCostTexture());
}


CInfoTextureHandler::~CInfoTextureHandler()
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (auto& pitex: infoTextures) {
		delete pitex.second;
	}
	for (auto& pitex: allyTeamInfoTextures) {
		delete pitex.second.tex;
	}
	infoTextureHandler = nullptr;
}


void CInfoTextureHandler::AddInfoTexture(CModernInfoTexture* itex)
{
	RECOIL_DETAILED_TRACY_ZONE;
	infoTextures[itex->GetName()] = itex;
}


const CInfoTexture* CInfoTextureHandler::GetInfoTextureConst(const std::string& name) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	static const CDummyInfoTexture dummy;

	const auto it = infoTextures.find(name);

	if (it != infoTextures.end())
		return it->second;

	return &dummy;
}

CInfoTexture* CInfoTextureHandler::GetInfoTexture(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (const_cast<CInfoTexture*>(GetInfoTextureConst(name)));
}


CInfoTexture* CInfoTextureHandler::GetInfoTexture(const std::string& name, int allyTeam)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!teamHandler.IsValidAllyTeam(allyTeam))
		return nullptr;

	const std::string key = name + "_" + std::to_string(allyTeam);

	const auto it = allyTeamInfoTextures.find(key);

	if (it != allyTeamInfoTextures.end()) {
		AllyTeamInfoTexture& entry = it->second;

		// if updates were paused for lack of requests, refresh inline
		// so the requester does not sample stale data this frame
		if (entry.tex != nullptr && (globalRendering->drawFrame - entry.lastRequestFrame) > ALLYTEAM_TEX_UPDATE_TIMEOUT)
			UpdateAllyTeamInfoTexture(entry.tex);

		entry.lastRequestFrame = globalRendering->drawFrame;
		return entry.tex;
	}

	// constructing (and initially updating) these binds FBO's and changes
	// the viewport; save and restore both since creation can be triggered
	// by a Lua texture-parse in the middle of a draw pass
	GLint prevFBO = 0;
	GLint prevViewport[4];
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	CModernInfoTexture* itex = nullptr;

	try {
		switch (hashString(name.c_str())) {
			case hashString("los"   ): { itex = new CLosTexture(allyTeam);    } break;
			case hashString("airlos"): { itex = new CAirLosTexture(allyTeam); } break;
			case hashString("radar" ): {
				// the radar texture samples its allyteam's los-texture, create that first
				if (GetInfoTexture("los", allyTeam) != nullptr)
					itex = new CRadarTexture(allyTeam);
			} break;
			default: {
				// per-allyteam variants exist only for the LOS-derived textures
				return nullptr;
			} break;
		}

		if (itex != nullptr)
			itex->Update(); // avoids one frame of uninitialized texture data
	} catch (const opengl_error&) {
		LOG_L(L_ERROR, "[CInfoTextureHandler::%s] could not create info-texture \"%s\"", __func__, key.c_str());
		itex = nullptr;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

	// also insert nullptr on failure, prevents retrying every request
	allyTeamInfoTextures[key] = { itex, globalRendering->drawFrame };
	return itex;
}


void CInfoTextureHandler::UpdateAllyTeamInfoTexture(CModernInfoTexture* itex) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// updating binds FBO's and changes the viewport; save and restore both
	// since this can be triggered by a Lua texture-parse mid draw-pass
	GLint prevFBO = 0;
	GLint prevViewport[4];
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	itex->Update();

	glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}


bool CInfoTextureHandler::IsEnabled() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (infoTex->IsEnabled());
}


void CInfoTextureHandler::DisableCurrentMode()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (returnToLOS && (GetMode() != "los")) {
		// return to LOS-mode if it was active before
		SetMode("los");
	} else {
		// otherwise disable overlay entirely
		SetMode("");
	}
}


void CInfoTextureHandler::SetMode(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	returnToLOS &= (name !=      ""); // NOLINT(readability-container-size-empty)
	returnToLOS |= (name ==   "los");
	inMetalMode  = (name == "metal");

	infoTex->SwitchMode(name);
}


void CInfoTextureHandler::ToggleMode(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (infoTex->GetMode() == name)
		return (DisableCurrentMode());

	SetMode(name);
}


const std::string& CInfoTextureHandler::GetMode() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (infoTex->GetMode());
}

const std::vector<std::string> CInfoTextureHandler::GetModes() const
{
	std::vector<string> modes;
	modes.reserve(infoTextures.size());

	for(const auto& [mode, tex]: infoTextures)
		modes.push_back(mode);

	return modes;
}

bool CInfoTextureHandler::HasMode(const std::string& name) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return infoTextures.contains(name);
}


GLuint CInfoTextureHandler::GetCurrentInfoTexture() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (infoTex->GetTexture());
}

int2 CInfoTextureHandler::GetCurrentInfoTextureSize() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return (infoTex->GetTexSize());
}


void CInfoTextureHandler::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	for (auto& [name, tex] : infoTextures) {
		// force first update except for combiner; hides visible uninitialized texmem
		if ((firstUpdate && tex != infoTex) || tex->IsUpdateNeeded())
			tex->Update();
	}

	for (auto& [name, entry] : allyTeamInfoTextures) {
		// can hold nullptr entries for textures that failed to create
		if (entry.tex == nullptr)
			continue;
		// pause textures nothing has requested recently; the request
		// path refreshes them inline again when they come back in use
		if ((globalRendering->drawFrame - entry.lastRequestFrame) > ALLYTEAM_TEX_UPDATE_TIMEOUT)
			continue;
		if (entry.tex->IsUpdateNeeded())
			entry.tex->Update();
	}

	firstUpdate = false;
}

