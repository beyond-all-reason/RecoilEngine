-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
local candidate = Spring.GetConfigInt("GhostTestCandidate", 0) == 1
local colorTest
if Spring.GetConfigInt("GhostTestColors", 0) == 1 then
	colorTest = {}
	VFS.Include("ghost_colors.lua", setmetatable({ widget = colorTest }, { __index = _G }))
	colorTest:Initialize()
end
local seen = {}
local failed = false
local function check(ok, message)
	Spring.Echo("GHOST_TEST " .. (ok and "PASS " or "FAIL ") .. message)
	failed = failed or not ok
end
local ghostRenderer
local luaRender = Spring.GetConfigInt("GhostTestLuaRender", 0) == 1
if luaRender then
	ghostRenderer = {}
	local handler = { RemoveWidget = function() error("BAR ghost shader unavailable") end }
	VFS.Include("luaui/Widgets/gfx_ghost_hover.lua", setmetatable({ widget = ghostRenderer, widgetHandler = handler }, { __index = _G }))
	ghostRenderer:Initialize()
	check(not pcall(gl.GhostRaw), "GhostRaw rejects calls outside DrawGhost")
end

function DrawGhost(info)
	if not luaRender then return false end
	local frame = math.floor(Spring.GetGameFrame() / 6)
	local phase = frame >= 200 and "dead" or frame >= 160 and "radar" or "live"
	if info.state == "ghostHighlight" or info.state == "radarGhostHighlight" then
		if not seen["render_" .. phase] then
			seen["render_" .. phase] = true
			check(info.unitID == Spring.GetGameRulesParam("ghost_test_target"), "Lua ghost historical unit ID " .. phase)
			check(info.aimPos[1] == 2060 and info.aimPos[2] == 280 and info.aimPos[3] == 2056, "Lua ghost frozen aimpoint " .. phase)
			check(#info.transform == 16 and #info.color == 4 and #info.teamColor == 4, "Lua ghost render fields " .. phase)
			check(info.transform[13] == 2048 and info.transform[15] == 2048, "Lua ghost stored transform " .. phase)
			check(ghostRenderer:DrawGhost(info) == true, "Lua ghost renderer " .. phase)
			return true
		end
		return ghostRenderer:DrawGhost(info)
	end
	local handled = ghostRenderer:DrawGhost(info)
	if not seen.render_fallback then
		seen.render_fallback = true
		check(handled == false, "Lua ghost renderer fallback")
	end
	return handled
end

function Update()
	local frame = math.floor(Spring.GetGameFrame() / 6)
	if colorTest then colorTest:Update() end
	if candidate and Spring.SetGhostHighlight then
		if (frame >= 130 and frame < 150) or (frame >= 167 and frame < 170) or (frame >= 210 and frame < 230) then
			Spring.SetGhostHighlight(2060, 280, 2056)
		else
			Spring.SetGhostHighlight()
		end
		for _, shot in ipairs({ {125, "live"}, {140, "live_highlight"}, {142, "live_shimmer"}, {165, "radar"}, {168, "radar_highlight"}, {205, "dead"}, {220, "dead_highlight"}, {222, "dead_shimmer"}, {235, "reset"} }) do
			if frame >= shot[1] and not seen[shot[2] .. "_shot"] then
				seen[shot[2] .. "_shot"] = true
				Spring.Echo("GHOST_TEST screenshot " .. shot[2])
				Spring.SendCommands("screenshot png")
			end
		end
	end
	local radarPhase = frame >= 171 and "reacquired" or frame >= 165 and "continuous"
	if candidate and radarPhase and not seen[radarPhase] then
		seen[radarPhase] = true
		local cx, cy = Spring.WorldToScreenCoords(2048, 240, 2048)
		local modelHit, iconHit, iconConsistent = false, false, true
		local counts = {}
		for dx = -60, 60, 3 do
			for dy = -60, 60, 3 do
				local x, y = math.floor(cx + dx), math.floor(cy + dy)
				local ordinary, ordinaryID = Spring.TraceScreenRay(x, y)
				local kind, result, ghostAimPos = Spring.TraceScreenRay(x, y, false, false, false, false, 0, true)
				counts[tostring(ordinary) .. "/" .. tostring(kind)] = (counts[tostring(ordinary) .. "/" .. tostring(kind)] or 0) + 1
				if ordinary == "unit" and ordinaryID == Spring.GetGameRulesParam("ghost_test_target") then
					iconHit = true
					iconConsistent = iconConsistent and kind == "unit" and result == ordinaryID
				end
				if kind == "unit" and result == Spring.GetGameRulesParam("ghost_test_target") and ghostAimPos then
					modelHit = ghostAimPos[1] == 2060 and ghostAimPos[2] == 280 and ghostAimPos[3] == 2056
				end
			end
		end
		for kinds, count in pairs(counts) do Spring.Echo("GHOST_TEST radar_rays " .. kinds .. " " .. count) end
		-- The injected reacquired-radar state does not populate the terrain radar map.
		if radarPhase == "continuous" then
			check(iconHit, "radar icon remains selectable")
		end
		check(iconConsistent, "radar icon keeps unit target")
		check(modelHit, "remembered model resolves to the same radar unit target")
	end
	if frame > 15 and not seen.camera then
		Spring.SetCameraState({ name = "fps", px = 2048, py = 600, pz = 2648, rx = 2.1112, ry = 0, rz = 0 }, 0)
		seen.camera = true
	end
	local phase = frame >= 270 and "revealed"
		or frame >= 200 and "dead"
		or frame >= 120 and "ghost"
		or frame >= 65 and "visible"
	if phase and not seen[phase] then
		seen[phase] = true
		local x, y = Spring.WorldToScreenCoords(2048, 240, 2048)
		local kind, result = Spring.TraceScreenRay(math.floor(x), math.floor(y), false, false, false, false, 0, true)
		Spring.Echo("GHOST_TEST trace " .. phase .. " " .. tostring(kind))
		if phase == "visible" then
			check(kind == "unit", "visible target stays a unit")
		elseif phase == "revealed" then
			check(kind ~= "ghost", "revealed empty site is not targetable")
		elseif candidate then
			check(kind == "ghost", phase .. " ghost is selectable")
			if kind == "ghost" then
				check(result[1] == 2060 and result[2] == 280 and result[3] == 2056, phase .. " uses frozen aimpoint")
			end
		else
			check(kind == "ground", "baseline " .. phase .. " click hits ground")
		end
		if phase == "ghost" and candidate then
			local command
			local w = {}
			local spring = setmetatable({
				GetActiveCommand = function()
					return 1, command
				end,
				GetMouseState = function()
					return math.floor(x), math.floor(y), false
				end,
				IsAboveMiniMap = function()
					return false
				end,
			}, { __index = Spring })
			local tables = setmetatable({
				copy = function(t)
					local copy = {}
					for k, v in pairs(t) do
						copy[k] = v
					end
					return copy
				end,
			}, { __index = table })
			local env = setmetatable(
				{
					widget = w,
					Spring = spring,
					table = tables,
					GameCMD = {
						UNIT_SET_TARGET = Spring.GetGameRulesParam("ghost_test_command"),
						UNIT_SET_TARGET_NO_GROUND = Spring.GetGameRulesParam("ghost_test_command_no_ground"),
					},
					WG = {},
					UnitDefs = {},
					widgetHandler = {
						RegisterGlobal = function() end,
						CommandNotify = function()
							return false
						end,
					},
				},
				{ __index = _G }
			)
			local ignoreSelf = {}
			env.widget = ignoreSelf
			VFS.Include("luaui/Widgets/cmd_no_self_selection.lua", env)
			ignoreSelf:Initialize()
			env.widget = w
			VFS.Include("luaui/Widgets/cmd_ghost_target.lua", env)
			for _, pair in ipairs({
				{ CMD.ATTACK, Spring.GetGameRulesParam("ghost_test_attack") },
				{ env.GameCMD.UNIT_SET_TARGET, Spring.GetGameRulesParam("ghost_test_priority") },
			}) do
				command = pair[1]
				Spring.SelectUnitArray({ pair[2] })
				w:MousePress(math.floor(x), math.floor(y), 1)
				check(w:CommandNotify(command, { 2048, 200, 2048 }, { coded = 0 }), "widget resolves click " .. command)
			end
		end
		local ordinary = Spring.TraceScreenRay(math.floor(x), math.floor(y))
		check(ordinary ~= "ghost", "ordinary trace unchanged " .. phase)
	end
	if frame >= 310 then
		Spring.Echo("GHOST_TEST " .. (failed and "FAILED" or "DONE"))
		Spring.SendCommands("quitforce")
	end
end
