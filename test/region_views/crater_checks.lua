-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
local reported, saveTime
function Update()
	if Spring.GetGameFrame() == 0 and Spring.GetMyPlayerID() == 0 then
		local n = 0
		for _, id in ipairs(Spring.GetPlayerList()) do
			if select(2, Spring.GetPlayerInfo(id, false)) then
				n = n + 1
			end
		end
		if n == (tonumber(Spring.GetModOptions().fixture_clients) or 1) then
			Spring.SendCommands("forcestart")
		end
	end
	if
		not reported
		and (
			Spring.GetGameRulesParam("crater_passed") ~= nil
			or Spring.GetConfigInt("RegionFixtureReloaded", 0) == 1
		)
	then
		reported = true
		assert(Spring.SetMapDamageRegions == nil, "crater regions must be synced-only")
		local r = Spring.GetMyPlayerID() == 0 and { 0, 0, 2048, 4096 } or { 2560, 0, 5120, 4096 }
		assert(Spring.SetWorldDrawBounds(unpack(r)))
		Spring.Echo("[CRATER_LOCAL_VIEW] " .. table.concat(r, ","))
		if Spring.GetModOptions().fixture_save_load == "1" then
			if Spring.GetConfigInt("RegionFixtureReloaded", 0) == 1 then
				Spring.SendCommands("pause 0")
			else
				Spring.SendCommands("pause 1", "save crater-regions")
				saveTime = Spring.GetDrawSeconds()
			end
		else
			Spring.SendLuaRulesMsg("crater_ready")
		end
	end
	if
		saveTime
		and Spring.GetDrawSeconds() - saveTime > 1
		and VFS.FileExists("Saves/crater-regions.ssf", VFS.RAW_ONLY)
	then
		Spring.SendCommands("quitforce")
	end
	if Spring.GetGameRulesParam("crater_loaded") ~= nil then
		Spring.SendCommands("quitforce")
	end
	local finish = Spring.GetGameRulesParam("crater_finish")
	if finish and Spring.GetGameFrame() >= finish then
		Spring.SendCommands("quitforce")
	end
end
function GameSetup()
	return true, true
end
function DrawScreen()
	if Spring.GetGameFrame() == 0 then
		Update()
	end
end
