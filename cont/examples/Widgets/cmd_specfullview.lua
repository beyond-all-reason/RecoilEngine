-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

function widget:GetInfo()
	return {
		name = "Spectator Full View",
		desc = "Provides /specfullview [0-3] for spectators",
		layer = 0,
		enabled = true,
	}
end

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

function widget:Initialize()
	widgetHandler:AddAction("specfullview", specFullView, nil, "tp")
end

function widget:Shutdown()
	widgetHandler:RemoveAction("specfullview")
end
