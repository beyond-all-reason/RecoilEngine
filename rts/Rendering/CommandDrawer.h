/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COMMAND_DRAWER_H
#define COMMAND_DRAWER_H

#include <vector>
#include "System/UnorderedSet.hpp"
#include "Sim/Units/BuildInfo.h"

struct Command;
class CCommandAI;
class CAirCAI;
class CBuilderCAI;
class CFactoryCAI;
class CMobileCAI;
class CUnit;
struct SColor;

struct CommandDrawer {
public:
	static CommandDrawer* GetInstance();

	// clear the set after WorldDrawer and MiniMap have both used it
	void Update() { luaQueuedUnitSet.clear(); }

	void Draw(const CCommandAI*, int queueDrawDepth = -1) const;
	void DrawLuaQueuedUnitSetCommands() const;
	void ClearQueuedBuildingSquaresCache();
	void DrawQueuedBuildingSquares(const CBuilderCAI*, const SColor& color);

	void AddLuaQueuedUnit(const CUnit* unit, int queueDrawDepth = 0);

private:
	void DrawCommands(const CCommandAI*, int queueDrawDepth) const;
	void DrawAirCAICommands(const CAirCAI*, int queueDrawDepth) const;
	void DrawBuilderCAICommands(const CBuilderCAI*, int queueDrawDepth) const;
	void DrawFactoryCAICommands(const CFactoryCAI*, int queueDrawDepth) const;
	void DrawMobileCAICommands(const CMobileCAI*, int queueDrawDepth) const;

	void DrawWaitIcon(const Command&) const;
	void DrawDefaultCommand(const Command&, const CUnit*) const;

private:
	using BuildInfoCacheType = std::vector<std::pair<size_t, BuildInfo>>;
	BuildInfoCacheType biCache;
	spring::unordered_set<std::pair<int, int>> luaQueuedUnitSet; //unitID, queueDepth (if > 0)
	static constexpr uint32_t cmdCircleResolution = 100;
};

#define commandDrawer (CommandDrawer::GetInstance())

#endif
