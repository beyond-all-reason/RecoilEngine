-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
local candidate = Spring.GetConfigInt("GhostTestCandidate", 0) == 1
local seen = {}
local failed = false
local function check(ok, message)
	Spring.Echo("GHOST_TEST " .. (ok and "PASS " or "FAIL ") .. message)
	failed = failed or not ok
end
local ghostRenderer
local hoverDrawActive = true
local hoverPhase, hoverX, hoverY, radarX, radarY
local luaRender = Spring.GetConfigInt("GhostTestLuaRender", 0) == 1
if luaRender then
	ghostRenderer = {}
	local handler = {
		RemoveWidget = function() error("BAR ghost shader unavailable") end,
		ModalAllows = function() return true end,
		UpdateCallIn = function(_, name) assert(name == "DrawGhost"); hoverDrawActive = true end,
		RemoveCallIn = function(_, name) assert(name == "DrawGhost"); hoverDrawActive = false end,
	}
	local spring = setmetatable({
		GetActiveCommand = function() return 1, hoverPhase and CMD.ATTACK end,
		GetMouseState = function() return hoverX, hoverY, false, false end,
		IsGUIHidden = function() return false end,
		IsAboveMiniMap = function() return false end,
	}, { __index = Spring })
	VFS.Include("luaui/Widgets/gfx_ghost_hover.lua", setmetatable({
		widget = ghostRenderer, widgetHandler = handler, Spring = spring,
		GameCMD = VFS.Include("modules/customcommands.lua").GameCMD,
	}, { __index = _G }))
	ghostRenderer:Initialize()
	check(not pcall(gl.GhostRaw), "GhostRaw rejects calls outside DrawGhost")
end

function DrawGhost(info)
	if Spring.GetMyAllyTeamID() == 2 then
		local frame = math.floor(Spring.GetGameFrame() / 6)
		local phase = frame >= 200 and "dead" or "live"
		local x, y, z = Spring.GetGhostPosition(info.ghostID)
		if x == 2100 and not seen["observer_draw_" .. phase] then
			check(info.ghostID == Spring.GetGhostsInCylinder(x, z, 0)[1] and y == 400 and z == 2080,
				"second observer DrawGhost " .. phase)
			seen["observer_draw_" .. phase] = true
		end
	end
	if not luaRender then return false end
	local handled = hoverDrawActive and ghostRenderer:DrawGhost(info)
	local x, y, z = Spring.GetGhostPosition(info.ghostID)
	if hoverPhase and x == 2060 then
		if not seen["render_" .. hoverPhase] then
			seen["render_" .. hoverPhase] = true
			local sighting = Spring.GetGhostsInCylinder(2060, 2056, 1)[1]
			check(info.ghostID == sighting, "Lua ghost sighting ID " .. hoverPhase)
			check(y == 280 and z == 2056, "Lua ghost frozen aimpoint " .. hoverPhase)
			check(#info.teamColor == 4, "Lua ghost team color " .. hoverPhase)
			check(handled == true, "Lua ghost renderer " .. hoverPhase)
			local other = setmetatable({ ghostID = -1 }, { __index = info })
			check(ghostRenderer:DrawGhost(other) == false, "same aimpoint with another sighting ID uses fallback " .. hoverPhase)
		end
	elseif not seen.render_fallback then
		seen.render_fallback = true
		check(handled == false, "Lua ghost renderer fallback")
	end
	return handled
end

local function readMinimapIcon(x, z, allyTeam)
	local texture = assert(gl.CreateTexture(32, 32, {fbo = true}))
	local pixels = {}
	gl.RenderToTexture(texture, function()
		gl.Clear(GL.COLOR_BUFFER_BIT, 0, 0, 0, 0)
		gl.Translate(-1, -1, 0)
		gl.Scale(2, 2, 1)
		gl.DrawMiniMapIcons(x - 16, z - 16, x + 16, z + 16, 6, allyTeam, false, false)
		for _, row in ipairs(gl.ReadPixels(0, 0, 32, 32)) do
			for _, rgba in ipairs(row) do
				pixels[#pixels + 1] = string.char(math.floor(rgba[1]*255+.5), math.floor(rgba[2]*255+.5), math.floor(rgba[3]*255+.5))
			end
		end
	end)
	gl.DeleteTexture(texture)
	return table.concat(pixels)
end

local initialIcon, radarIcon
local emptyIcon = string.rep(string.char(0), 32 * 32 * 3)
local probePhases = {[40] = "before transfer", [50] = "after transfer", [60] = "after destruction", [85] = "after reveal"}
local radarPhases = {[164] = "continuous", [172] = "reacquired"}

function DrawScreen()
	if not candidate then return end
	local frame = math.floor(Spring.GetGameFrame() / 6)
	local observer = Spring.GetMyAllyTeamID()
	local phase = observer == 2 and probePhases[frame] or observer == 0 and radarPhases[frame]
	if not phase or seen["minimap_" .. phase] then return end
	seen["minimap_" .. phase] = true
	if observer == 2 then
		local image = readMinimapIcon(3296, 3200, observer)
		if phase == "before transfer" then initialIcon = image end
		local expected = phase == "after reveal" and emptyIcon or initialIcon
		check(initialIcon and initialIcon ~= emptyIcon and image == expected, "second observer minimap " .. phase)
	else
		local image = readMinimapIcon(2048, 2048, observer)
		if phase == "continuous" then radarIcon = image end
		check(image ~= emptyIcon and image == radarIcon, "remembered radar minimap " .. phase)
	end
end

-- The second observer saw the same building after its aimpoint was changed.
local function checkObserverRay(frame)
	if not seen.camera then
		Spring.SetCameraState({ name = "fps", px = 2048, py = 600, pz = 2648, rx = 2.1112, ry = 0, rz = 0 }, 0)
		seen.camera = true
		return
	end
	local phase = frame >= 270 and "only remaining ghost" or frame >= 200 and "dead ghost" or frame >= 120 and "live ghost"
	if phase and not seen[phase] then
		seen[phase] = true
		local x, y = Spring.WorldToScreenCoords(2048, 240, 2048)
		local kind, id, aim = Spring.TraceScreenRay(math.floor(x), math.floor(y), false, false, false, false, 0, true)
		local ghosts = Spring.GetGhostsInCylinder(2100, 2080, 1)
		local gx, gy, gz = Spring.GetGhostPosition(ghosts[1])
		check(kind == "ghost" and aim == nil and #ghosts == 1 and id == ghosts[1] and gx == 2100 and gy == 400 and gz == 2080,
			"second observer " .. phase)
		check(not pcall(Spring.GetGhostsInCylinder, 2060, 2056, 1, 0), "cannot query another observer")
	end
end

local function clickModel(x, y, attackUnit, setTargetUnit)
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
				GHOST_ATTACK = Spring.GetGameRulesParam("ghost_test_attack_cmd"),
				GHOST_SET_TARGET = Spring.GetGameRulesParam("ghost_test_set_cmd"),
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
		{ CMD.ATTACK, attackUnit },
		{ env.GameCMD.UNIT_SET_TARGET, setTargetUnit },
	}) do
		command = pair[1]
		Spring.SelectUnitArray({ pair[2] })
		w:MousePress(math.floor(x), math.floor(y), 1)
		check(w:CommandNotify(command, { 2048, 200, 2048 }, { coded = 0 }), "widget resolves click " .. command)
	end
end

local function withArity(...)
	return select("#", ...), ...
end

function Update()
	local frame = math.floor(Spring.GetGameFrame() / 6)
	if Spring.GetMyAllyTeamID() == 2 then
		if frame > 15 then checkObserverRay(frame) end
		if frame >= 310 then
			Spring.Echo("GHOST_TEST " .. (failed and "FAILED" or "DONE"))
			Spring.SendCommands("quitforce")
		end
		return
	end
	if candidate and frame >= 65 and not seen.sentRaw then
		seen.sentRaw = true
		Spring.SelectUnitArray({Spring.GetGameRulesParam("ghost_test_attack")})
		Spring.GiveOrder(CMD.INSERT, {0, CMD.ATTACK, CMD.OPT_KEEP_HEIGHT, 2050, 900, 2056}, CMD.OPT_ALT)
		Spring.SelectUnitArray({Spring.GetGameRulesParam("ghost_test_priority")})
		Spring.GiveOrder(Spring.GetGameRulesParam("ghost_test_command"), {2050, 900, 2056}, CMD.OPT_KEEP_HEIGHT)
	end
	if candidate and frame >= 80 and not seen.checkedRaw then
		seen.checkedRaw = true
		local cmd = Spring.GetUnitCommands(Spring.GetGameRulesParam("ghost_test_attack"), 1)[1]
		local _, _, pos = Spring.GetUnitWeaponTarget(Spring.GetGameRulesParam("ghost_test_priority"), 1)
		check(cmd and cmd.params[1] == 2050 and cmd.params[2] == 200 and not cmd.options.keepheight,
			"LuaUI inserted height clamped")
		check(type(pos) == "table" and pos[1] == 2050 and pos[2] == 200, "LuaUI set-target height clamped")
	end
	if luaRender then
		hoverPhase = (frame >= 130 and frame < 150 and "live") or (frame >= 167 and frame < 170 and "radar") or (frame >= 210 and frame < 230 and "dead")
		if hoverPhase == "radar" and radarX then
			hoverX, hoverY = radarX, radarY
		else
			local x, y = Spring.WorldToScreenCoords(2048, 240, 2048)
			hoverX, hoverY = math.floor(x), math.floor(y)
		end
		ghostRenderer:Update()
	end
	if candidate then
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
		local modelHit, iconConsistent = false, true
		local iconX, iconY
		local counts = {}
		local sighting = Spring.GetGhostsInCylinder(2060, 2056, 1)[1]
		local targetID = Spring.GetGameRulesParam("ghost_test_target")
		local _, _, _, contact = Spring.GetGhostPosition(sighting)
		local exactArity = true
		for dx = -60, 60, 3 do
			for dy = -60, 60, 3 do
				local x, y = math.floor(cx + dx), math.floor(cy + dy)
				local ordinary, ordinaryID = Spring.TraceScreenRay(x, y)
				local n, kind, result = withArity(Spring.TraceScreenRay(x, y, false, false, false, false, 0, true))
				if kind == "unit" or kind == "ghost" then exactArity = exactArity and n == 2 end
				counts[tostring(ordinary) .. "/" .. tostring(kind)] = (counts[tostring(ordinary) .. "/" .. tostring(kind)] or 0) + 1
				if ordinary == "unit" and ordinaryID == targetID then
					iconX, iconY = x, y
					iconConsistent = iconConsistent and kind == "unit" and result == ordinaryID
				end
				if kind == "ghost" and result == targetID and contact == targetID then
					modelHit = true
					radarX, radarY = x, y
				end
			end
		end
		for kinds, count in pairs(counts) do Spring.Echo("GHOST_TEST radar_rays " .. kinds .. " " .. count) end
		-- The injected reacquired-radar state does not populate the terrain radar map.
		if radarPhase == "continuous" then
			check(iconX ~= nil, "radar icon remains selectable")
			if luaRender and iconX then
				hoverPhase, hoverX, hoverY = "radar", iconX, iconY
				ghostRenderer:Update()
				check(not hoverDrawActive, "radar icon leaves ghost highlight off")
				hoverPhase = nil
			end
		end
		check(iconConsistent, "radar icon keeps unit target")
		check(modelHit, "radar model returns its unit " .. radarPhase)
		check(exactArity, "two trace results " .. radarPhase)
		if radarPhase == "continuous" and modelHit then
			clickModel(radarX, radarY, Spring.GetGameRulesParam("ghost_test_radar_attack"), Spring.GetGameRulesParam("ghost_test_radar_priority"))
		end
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
		local kind, id, result = Spring.TraceScreenRay(math.floor(x), math.floor(y), false, false, false, false, 0, true)
		Spring.Echo("GHOST_TEST trace " .. phase .. " " .. tostring(kind))
		if phase == "visible" then
			check(kind == "unit", "visible target stays a unit")
		elseif phase == "revealed" then
			check(kind ~= "ghost", "revealed empty site is not targetable")
		elseif candidate then
			check(kind == "ghost", phase .. " ghost is selectable")
			if kind == "ghost" then
				local gx, gy, gz = Spring.GetGhostPosition(id)
				check(result == nil and id == Spring.GetGhostsInCylinder(2060, 2056, 1)[1] and gx == 2060 and gy == 280 and gz == 2056, phase .. " uses frozen aimpoint")
			end
		else
			check(kind == "ground", "baseline " .. phase .. " click hits ground")
		end
		if phase == "ghost" and candidate then
			clickModel(x, y, Spring.GetGameRulesParam("ghost_test_attack"), Spring.GetGameRulesParam("ghost_test_priority"))
		end
		local ordinary = Spring.TraceScreenRay(math.floor(x), math.floor(y))
		check(ordinary ~= "ghost", "ordinary trace unchanged " .. phase)
	end
	if frame >= 310 then
		Spring.Echo("GHOST_TEST " .. (failed and "FAILED" or "DONE"))
		Spring.SendCommands("quitforce")
	end
end
