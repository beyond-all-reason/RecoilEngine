-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

function widget:GetInfo()
	return {
		name = "Spectator Commands",
		desc = "Grants full view and selection on elimination and handles /specfullview and /specteam",
		layer = 0,
		enabled = true,
	}
end

local wasSpectating

local function specFullView(_, _, words)
	local spectating, fullView = Spring.GetSpectatingState()
	if not spectating then
		return false
	end

	local view, select = not fullView, not fullView
	if words[1] then
		-- The low bits select full view (1) and full selection (2).
		local mode = tonumber(words[1]:match("^[+-]?%d+")) or 0
		view = mode % 2 == 1
		select = mode % 4 >= 2
	end

	Spring.SetSpectatorFullView(view)
	Spring.SetSpectatorFullSelect(select)
	return true
end

local function specTeam(_, _, words)
	local teamID = words[1] and tonumber(words[1]:match("^[+-]?%d+"))
	return teamID ~= nil and Spring.SetSpectatorTeam(teamID)
end

function widget:Initialize()
	wasSpectating = Spring.GetSpectatingState()
	widgetHandler:AddAction("specteam", specTeam, nil, "tp")
	widgetHandler:AddAction("specfullview", specFullView, nil, "tp")
end

function widget:PlayerChanged(playerID)
	if playerID ~= Spring.GetMyPlayerID() then
		return
	end

	local spectating = Spring.GetSpectatingState()
	if spectating == wasSpectating then
		return
	end

	-- View and selection changes can trigger PlayerChanged again.
	wasSpectating = spectating
	if spectating then
		Spring.SetSpectatorFullCommunications(true)
		Spring.SetSpectatorFullView(true)
		Spring.SetSpectatorFullSelect(true)
	end
end

function widget:Shutdown()
	widgetHandler:RemoveAction("specteam")
	widgetHandler:RemoveAction("specfullview")
end
