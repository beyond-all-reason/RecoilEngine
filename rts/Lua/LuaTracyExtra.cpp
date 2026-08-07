/* This file is part of the Recoil engine (GPL v2 or later). */

#include "LuaTracyExtra.h"

#include "LuaInclude.h"
#include "LuaUtils.h"

#include "System/Misc/TracyDefs.h"
#include "System/Config/ConfigHandler.h"
#include "Rendering/GL/myGL.h" // gated Tracy GPU-zone macros (TracyGpuZoneTransient)
#include <common/TracyQueue.hpp>

#include <functional>
#include <set>
#include <string>

CONFIG(bool, TracyLuaGpuZones)
	.defaultValue(false)
	.description("Register tracy.GpuProfile for per-widget/gadget GPU zones (dev/profiling only)");

/* Tracy seems to want unique, unchanging strings to be passed to
 * its API, so we need to immanentize the ephemeral Lua strings
 * by storing them.
 *
 * NB: strings here are never really cleaned up, but the use case assumes
 * that they live a long time and there's just a handful of them. */
static std::set <std::string, std::less<>> tracyLuaPlots;

static const std::string& GetImmanentPlotName(const char *plotName)
{
	const auto plot = tracyLuaPlots.find(plotName);
	if (plot != tracyLuaPlots.end())
		return *plot;

	return *tracyLuaPlots.emplace(plotName).first;
}

/*** Configure custom appearance for a Tracy plot for use in debugging or profiling
 *
 * @function tracy.LuaTracyPlotConfig
 * @param plotName string name of the plot to customize
 * @param plotFormatType "Number"|"Percentage"|"Memory"|nil (Default: `"Number"`)
 * @param stepwise boolean? (Default: `true`) stepwise chart
 * @param fill boolean? (Default: `false`) whether to fill color
 * @param color integer? (Default: `0xFFFFFF`) uint32 number as BGR color
 */

static int LuaTracyPlotConfig(lua_State* L)
{
	const auto plotName             = luaL_checkstring(L, 1);
	const auto plotFormatTypeString = luaL_optstring(L, 2, "");
	const auto stepwise             = luaL_optboolean(L, 3, true);
	const auto fill                 = luaL_optboolean(L, 4, false);
	const uint32_t color            = luaL_optint(L, 5, 0xFFFFFF); // white

	tracy::PlotFormatType plotFormatType;
	switch (plotFormatTypeString[0]) {
		case 'p': case 'P': plotFormatType = tracy::PlotFormatType::Percentage; break;
		case 'm': case 'M': plotFormatType = tracy::PlotFormatType::Memory;     break;
		default:            plotFormatType = tracy::PlotFormatType::Number;     break;
	}

	TracyPlotConfig(GetImmanentPlotName(plotName).c_str(), plotFormatType, stepwise, fill, color);
	return 0;
}


/*** Update a Tracy plot with a value
 *
 * @function tracy.LuaTracyPlot
 * @param plotName string Which LuaPlot should be updated
 * @param plotValue number the number to show on the Tracy plot
 */
static int LuaTracyPlot(lua_State* L)
{
	const auto plotName  = luaL_checkstring(L, 1);
	const auto plotValue = luaL_checkfloat (L, 2);

	TracyPlot(GetImmanentPlotName(plotName).c_str(), plotValue);
	return 0;
}

bool LuaTracyExtra::PushEntries(lua_State* L)
{
	LuaPushNamedCFunc(L, "LuaTracyPlot"      , LuaTracyPlot      );
	LuaPushNamedCFunc(L, "LuaTracyPlotConfig", LuaTracyPlotConfig);
	return true;
}

/*** Profile the GPU work submitted by a function as one Tracy GPU zone
 *
 * @function tracy.GpuProfile
 * @param name string zone label (e.g. the widget/gadget name)
 * @param fn function the call to profile
 * @param ... any arguments forwarded to `fn`
 * @return ... whatever `fn` returns
 *
 * Wraps `fn(...)` in a GPU zone whose begin/end are emitted in C++ around a
 * protected call, so the LIFO GPU query stream stays balanced even if `fn`
 * errors. Meant to be driven from a handler's central per-addon dispatch, not
 * called by individual widgets/gadgets. Measures GPU execution time of what was
 * submitted (shaders, overdraw) — not CPU/Lua cost. Only present when the
 * `TracyLuaGpuZones` config is set; a transparent passthrough under HEADLESS or
 * when no GPU profiling context exists.
 */
static int LuaTracyGpuProfile(lua_State* L)
{
	[[maybe_unused]] const char* zoneName = luaL_checkstring(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	// stack: [name, fn, arg1 .. argN]; lua_pcall consumes fn + its N args and
	// leaves fn's results above `name` (which stays at index 1).
	const int numArgs = lua_gettop(L) - 2;

	int callStatus;
	{
		// Begin/end bracket the protected call in C++ (RAII), so the query
		// stream is always balanced: a Lua error is caught by lua_pcall and
		// never longjmps past this scope. `active` is false when no GPU context
		// exists (e.g. no ARB_timer_query), which avoids a null-context deref.
		#if !defined(HEADLESS) && defined(TRACY_ENABLE)
		TracyGpuZoneTransient(___luaGpuProfileZone, zoneName, tracy::GetGpuCtx().ptr != nullptr);
		#endif
		callStatus = lua_pcall(L, numArgs, LUA_MULTRET, 0);
	}

	if (callStatus != 0)
		return lua_error(L); // re-raise now that the zone has closed

	return lua_gettop(L) - 1; // fn's results (everything above `name`)
}

bool LuaTracyExtra::PushGpuProfile(lua_State* L)
{
	if (configHandler == nullptr || !configHandler->GetBool("TracyLuaGpuZones"))
		return false;

	LuaPushNamedCFunc(L, "GpuProfile", LuaTracyGpuProfile);
	return true;
}
