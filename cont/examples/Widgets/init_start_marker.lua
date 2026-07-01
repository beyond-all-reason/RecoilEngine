function widget:GetInfo()
  return {
    name      = "Start Point Adder",
    desc      = "Add team start points once the game begins",
    author    = "abma",
    date      = "July 05, 2011",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
    enabled   = true  --  loaded by default?
  }
end

function widget:Initialize()
	local _, _, spec = Engine.Shared.GetPlayerInfo(Engine.Unsynced.GetMyPlayerID())
	if spec then
		widgetHandler:RemoveWidget()
		return false
	end
end

function widget:Update()
	if (Engine.Shared.GetGameSeconds() > 0) then
		local x, y, z = Engine.Shared.GetTeamStartPosition(Engine.Unsynced.GetMyTeamID())
		local id=Engine.Unsynced.GetMyPlayerID()
		Engine.Unsynced.MarkerAddPoint(x, y, z, "Start " .. id )
		widgetHandler:RemoveWidget()
	end
end
