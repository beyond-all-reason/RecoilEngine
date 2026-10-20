-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
local initialized, reported = false, false
local saveTime
function Update()
	if Spring.GetGameFrame() == 0 and Spring.GetMyPlayerID() == 0 then
		local count = 0
		for _, id in ipairs(Spring.GetPlayerList()) do
			if select(2, Spring.GetPlayerInfo(id, false)) then
				count = count + 1
			end
		end
		if count == (tonumber(Spring.GetModOptions().fixture_clients) or 1) then
			Spring.SendCommands("forcestart")
		end
	end
	if not initialized and Spring.GetGameFrame() > 0 then
		initialized = true
		assert(Spring.SetSensorRegions == nil, "sensor regions must not be an unsynced control")
		local r = Spring.GetMyPlayerID() % 2 == 0 and { 0, 0, 2048, 1536 } or { 3072, 0, 5120, 1536 }
		assert(Spring.SetWorldDrawBounds(unpack(r)))
		assert(Spring.SetMiniMapView(unpack(r)))
		Spring.Echo("[SENSOR_LOCAL_VIEW] " .. table.concat(r, ","))

		Spring.SendCommands("setspeed 4")
	end
	if
		not reported
		and (
			Spring.GetGameRulesParam("sensor_test_passed") ~= nil
			or Spring.GetConfigInt("RegionFixtureReloaded", 0) == 1
		)
	then
		reported = true
		if
			Spring.GetModOptions().fixture_save_load == "1"
			or Spring.GetConfigInt("RegionFixtureReloaded", 0) == 1
		then
			if Spring.GetConfigInt("RegionFixtureReloaded", 0) == 1 then
				Spring.SendCommands("pause 0")
			else
				Spring.SendCommands("pause 1", "save sensor-regions")
				saveTime = Spring.GetDrawSeconds()
			end
		else
			Spring.SendLuaRulesMsg("sensor_fixture_ready")
		end
	end
	if
		saveTime
		and Spring.GetDrawSeconds() - saveTime > 1
		and VFS.FileExists("Saves/sensor-regions.ssf", VFS.RAW_ONLY)
	then
		Spring.SendCommands("quitforce")
	end
	if Spring.GetGameRulesParam("sensor_load_passed") ~= nil then
		Spring.SendCommands("quitforce")
	end
	local finish = Spring.GetGameRulesParam("sensor_finish_at")
	if finish and Spring.GetGameFrame() >= finish then
		Spring.SendCommands("quitforce")
	end
end

-- Allow the minimal fixture to start without a lobby UI.
function GameSetup()
	return true, true
end
function DrawScreen()
	if Spring.GetGameFrame() == 0 then
		Update()
	end
end
