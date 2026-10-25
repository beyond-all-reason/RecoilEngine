--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    game_start_position.lua
--  brief:   Lua version of the engine start position selecter
--
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  For start position type 2 (choose in game) this does what the engine does:
--  left click the map to choose a start position, click "Ready" to ready up,
--  and a team that never chose a position gets one near the centre of its
--  start box. It does nothing for the other start position types.
--
--  To use it, copy it to LuaRules/Gadgets/ and set
--  system.useStartPositionSelecter = false in gamedata/modrules.lua,
--  otherwise the engine selecter takes the clicks this gadget passes on.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:GetInfo()
	return {
		name      = "Start Position",
		desc      = "Choose start positions in game, like the engine selecter",
		license   = "GNU GPL, v2 or later",
		-- synced: GameStart before the gadgets that spawn start units
		-- unsynced: map clicks after the other gadgets, like the engine selecter
		layer     = gadgetHandler:IsSyncedCode() and -1000 or 1000,
		enabled   = true
	}
end

if (Game.startPosType ~= 2) then
	return false
end

local PLAYER_RDYSTATE_FORCED = 2

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (gadgetHandler:IsSyncedCode()) then

-- CTeam::SetDefaultStartPos
local function GetDefaultStartPos(teamID, numTeams)
	local allyTeamID = select(6, Spring.GetTeamInfo(teamID, false))
	local xmin, zmin, xmax, zmax = Spring.GetAllyTeamStartBox(allyTeamID)
	local offset = (teamID - numTeams) * 4 * Game.squareSize
	return offset + (xmin + xmax) * 0.5, offset + (zmin + zmax) * 0.5
end

function gadget:AllowStartPosition(playerID, teamID, readyState)
	if (readyState == PLAYER_RDYSTATE_FORCED) then
		SendToUnsynced("StartPositionForced", playerID, teamID)
	end
	return true
end

function gadget:GameStart()
	local gaiaTeamID = Spring.GetGaiaTeamID()
	local teams = Spring.GetTeamList()
	for i = 1, #teams do
		local teamID = teams[i]
		local x, y, z, valid = Spring.GetTeamStartPosition(teamID)
		if (teamID ~= gaiaTeamID) and (not valid) then
			local dx, dz = GetDefaultStartPos(teamID, #teams)
			-- while the engine still sets the default itself, a team
			-- only gets here when that default is outside the map
			if (math.abs(x - dx) > 0.5) or (math.abs(z - dz) > 0.5) then
				Spring.SetTeamStartPosition(teamID, dx, y, dz)
			end
		end
	end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

else

if (Spring.IsReplay()) then
	return false
end

local READY_BOX = {x1 = 0.71, y1 = 0.72, x2 = 0.81, y2 = 0.76} -- screen fractions
local LINE_HEIGHT = 1.25 -- about font->GetLineHeight()

local WHITE     = {1.0, 1.0, 1.0, 1.0}
local RED       = {1.0, 0.2, 0.2, 1.0}
local GREEN     = {0.2, 1.0, 0.2, 1.0}
local YELLOW    = {0.8, 0.8, 0.2, 1.0}
local CYAN      = {0.0, 0.9, 0.9, 1.0}
local LIGHT_RED = {1.0, 0.5, 0.5, 1.0}

local fontSize = Spring.GetConfigInt("FontSize", 23)
local myPlayerID = Spring.GetMyPlayerID()

local pickPos = {0, 0, 0}
local picked = false
local done = false -- the engine selecter has deleted itself
local started = (Spring.GetGameFrame() > 0)

local setupState
local setupPlayerStates
local setupDrawFrame = -2

local function GetDrawFrame()
	local low, high = Spring.GetDrawFrame()
	return low + high * 65536
end

local function IsActive()
	if (not done) and Spring.GetSpectatingState() then
		done = true
	end
	return (not done)
end

-- GameSetup reaches this gadget only while no LuaUI handles it,
-- and only until the start countdown ends
local function IsSetupShown()
	return (GetDrawFrame() - setupDrawFrame <= 1)
end

local function InReadyBox(x, y)
	local vsx, vsy = Spring.GetViewGeometry()
	local fx, fy = x / vsx, y / vsy
	return (fx > READY_BOX.x1) and (fx < READY_BOX.x2) and (fy > READY_BOX.y1) and (fy < READY_BOX.y2)
end

-- CStartPosSelecter::Ready
local function Ready()
	local x, y, z, valid = Spring.GetTeamStartPosition(Spring.GetMyTeamID())
	if (not valid) then
		-- do not ready up before a position was chosen
		if (not picked) then
			return false
		end
		x, y, z = pickPos[1], pickPos[2], pickPos[3]
	end
	Spring.RequestStartPosition(x, y, z, true)
	done = true
	return true
end

function gadget:Initialize()
	-- the engine selecter deletes itself after Lua forces the player ready
	gadgetHandler:AddSyncAction("StartPositionForced", function(_, playerID, teamID)
		if (playerID == myPlayerID) and (teamID == Spring.GetMyTeamID()) then
			done = true
		end
	end)
end

function gadget:GameStart()
	started = true
end

-- CStartPosSelecter::MousePress
function gadget:MousePress(x, y, button)
	if (button ~= 1) or Spring.IsGUIHidden() or (not IsActive()) then
		return false
	end
	if started then
		done = true
		return false
	end
	-- the engine minimap takes clicks before the selecter
	if Spring.IsAboveMiniMap(x, y) then
		return false
	end
	if IsSetupShown() and InReadyBox(x, y) then
		return (not Ready())
	end

	local _, pos = Spring.TraceScreenRay(x, y, true, false, false, true)
	if (pos == nil) then
		return true
	end

	-- like the engine, erase map marks near the previous position
	-- (near the map origin on the first click)
	Spring.MarkerErasePosition(pickPos[1], pickPos[2], pickPos[3])
	pickPos = {pos[1], pos[2], pos[3]}
	picked = true
	Spring.RequestStartPosition(pos[1], pos[2], pos[3], false)
	return true
end

-- the gadget handler clears its mouse owner only in MouseRelease
function gadget:MouseRelease(x, y, button)
end

-- replaces the engine text and player list, and hides the engine ready box
function gadget:GameSetup(state, ready, playerStates)
	setupState = state
	setupPlayerStates = playerStates
	setupDrawFrame = GetDrawFrame()
	return true
end

-- GameSetupDrawer::Draw
local function DrawSetup(vsx, vsy)
	gl.Color(WHITE)
	gl.Text(setupState, 0.3 * vsx, 0.7 * vsy, fontSize, "o")

	local numPlayers = 0
	for playerID in pairs(setupPlayerStates) do
		numPlayers = math.max(numPlayers, playerID + 1)
	end

	-- player 0 at the bottom, "Players:" at the top
	local lineHeight = fontSize * LINE_HEIGHT
	for a = 0, numPlayers do
		local name, color = "Players:", WHITE
		if (a < numPlayers) then
			local active, spectator
			name, active, spectator = Spring.GetPlayerInfo(a, false)
			if spectator then
				color = active and CYAN or LIGHT_RED
			elseif (not active) then
				color = RED
			elseif (setupPlayerStates[a] == "notready") then
				color = YELLOW
			else
				color = GREEN
			end
		end
		gl.Color(color)
		gl.Text(name, 10, 0.5 * vsy - 0.5 * lineHeight * numPlayers + lineHeight * a, fontSize, "o")
	end
end

-- CStartPosSelecter::Draw
local function DrawReadyBox(vsx, vsy)
	local x1, y1 = READY_BOX.x1 * vsx, READY_BOX.y1 * vsy
	local x2, y2 = READY_BOX.x2 * vsx, READY_BOX.y2 * vsy
	-- the engine draws the box twice
	local guiAlpha = Spring.GetConfigFloat("GuiOpacity", 0.8)
	local alpha = 1 - (1 - guiAlpha) * (1 - guiAlpha)
	local mx, my = Spring.GetMouseState()
	if InReadyBox(mx, my) then
		gl.Color(0.7, 0.2, 0.2, alpha)
	else
		gl.Color(0.7, 0.7, 0.2, alpha)
	end
	gl.Rect(x1, y1, x2, y2)

	local textSize = 0.9 * math.min((x2 - x1) / gl.GetTextWidth("Ready"), (y2 - y1) / LINE_HEIGHT)
	gl.Color(WHITE)
	gl.Text("Ready", 0.5 * (x1 + x2), 0.5 * (y1 + y2), textSize, "ocv")
end

function gadget:DrawScreen(vsx, vsy)
	if (not IsSetupShown()) then
		return
	end
	DrawSetup(vsx, vsy)
	if IsActive() and (not started) then
		DrawReadyBox(vsx, vsy)
	end
	gl.Color(WHITE)
end

end
