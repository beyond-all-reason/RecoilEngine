-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
function gadget:GetInfo()
	return { name = "Sensor partition regression", layer = 0, enabled = true }
end
if not gadgetHandler:IsSyncedCode() then
	return
end
local isolated = Spring.GetModOptions().sensor_test ~= "whole-map"
local emitter, target, localTarget, sonarProbe
local proximityProbe, proximityLocal
local pings = {}
local acknowledgements = {}
local loadedCheckFrame
local clients = tonumber(Spring.GetModOptions().fixture_clients) or 1
local checks, failed = 0, false
local function Check(name, value)
	checks = checks + 1
	if not value then
		failed = true
	end
	Spring.Echo("[SENSOR_REGIONS] ASSERT " .. name .. " pass=" .. tostring(not not value))
end
local regions =
	{ { 0, 0, 2048, 1536 }, { 3072, 0, 5120, 1536 }, { 0, 2560, 2048, 4096 }, { 3072, 2560, 5120, 4096 } }
local Z = 1024
local function Sensor(id, kind, value)
	Spring.SetUnitSensorRadius(id, kind, value)
end
local function Spawn(x, team)
	local id = assert(Spring.CreateUnit("region_sensor_unit", x, 96, Z, 0, team))
	for _, kind in ipairs({ "los", "airLos", "radar", "sonar", "seismic", "radarJammer", "sonarJammer" }) do
		Sensor(id, kind, 0)
	end
	return id
end
local function Coverage(prefix, localX, foreignX)
	Check(prefix .. "_local_los", Spring.IsPosInLos(localX, 96, Z, 0))
	Check(prefix .. "_local_airlos", Spring.IsPosInAirLos(localX, 1000, Z, 0))
	Check(prefix .. "_local_radar", Spring.IsPosInRadar(localX, 96, Z, 0))
	Check(prefix .. "_local_sonar", Spring.IsPosInRadar(localX, -100, Z, 0))
	Check(prefix .. "_foreign_los", Spring.IsPosInLos(foreignX, 96, Z, 0) == not isolated)
	Check(prefix .. "_foreign_airlos", Spring.IsPosInAirLos(foreignX, 1000, Z, 0) == not isolated)
	Check(prefix .. "_foreign_radar", Spring.IsPosInRadar(foreignX, 96, Z, 0) == not isolated)
	Check(prefix .. "_foreign_sonar", Spring.IsPosInRadar(foreignX, -100, Z, 0) == not isolated)
end
function gadget:AllowUnitCloak(id, enemy)
	return enemy == nil
end
function gadget:UnitSeismicPing(x, y, z, strength, ally, unit)
	if ally == 0 and unit then
		pings[unit] = (pings[unit] or 0) + 1
	end
end
function gadget:RecvLuaMsg(message, player)
	if message ~= "sensor_fixture_ready" or Spring.GetGameFrame() < 130 then
		return
	end
	acknowledgements[player] = true
	local count = 0
	for _ in pairs(acknowledgements) do
		count = count + 1
	end
	if count == clients and not Spring.GetGameRulesParam("sensor_finish_at") then
		Spring.SetGameRulesParam("sensor_finish_at", Spring.GetGameFrame() + 60, { public = true })
	end
end
local function Witness(frame)
	local state = { tostring(frame) }
	for _, id in ipairs(Spring.GetAllUnits()) do
		local x, y, z = Spring.GetUnitPosition(id)
		state[#state + 1] = string.format("%d:%.3f:%.3f:%.3f:%.3f", id, x, y, z, Spring.GetUnitHealth(id))
	end
	Spring.Echo("[SENSOR_WITNESS] " .. table.concat(state, ";"))
end
function gadget:Initialize()
	if isolated then
		Check("synced_api_available", type(Spring.SetSensorRegions) == "function")
		if Spring.SetSensorRegions then
			Check("aligned_regions_accepted", Spring.SetSensorRegions(regions))
			Check(
				"overlap_rejected",
				not Spring.SetSensorRegions({ { 0, 0, 4096, 4096 }, { 3072, 0, 5120, 4096 } })
			)
			Check("unaligned_rejected", not Spring.SetSensorRegions({ { 0, 0, 2049, 4096 } }))
			Check("nonfinite_rejected", not Spring.SetSensorRegions({ { 0, 0, math.huge, 4096 } }))
			Check("nan_rejected", not Spring.SetSensorRegions({ { 0, 0, 0 / 0, 4096 } }))
			Check("extra_arguments_rejected", not pcall(Spring.SetSensorRegions, regions, true))
			Check("malformed_rejected", not pcall(Spring.SetSensorRegions, { { 0, 0, 5120 } }))
		end
	elseif Spring.SetSensorRegions then
		Check("initial_clear", Spring.SetSensorRegions())
	end
	emitter = Spawn(1800, 0)
	target = Spawn(3400, Spring.GetGaiaTeamID())
	localTarget = Spawn(1700, Spring.GetGaiaTeamID())
	for _, kind in ipairs({ "los", "airLos", "radar", "sonar", "seismic" }) do
		Sensor(emitter, kind, 4096)
	end
end
function gadget:GameFrame(f)
	-- The saving process pauses at frame 130. Only the fresh loading process
	-- advances through this stage and creates an emitter against restored state.
	if f == 140 and Spring.GetModOptions().fixture_save_load == "1" then
		emitter = Spawn(1800, 0)
		for _, kind in ipairs({ "los", "airLos", "radar", "sonar", "seismic" }) do
			Sensor(emitter, kind, 4096)
		end
		loadedCheckFrame = f + 30
	end
	if loadedCheckFrame and f == loadedCheckFrame then
		Coverage("loaded", 1700, 3400)
		Check("loaded_configuration_stays_immutable", not Spring.SetSensorRegions())
		Spring.Echo(failed and "SENSOR_SAVELOAD_FAIL" or "SENSOR_SAVELOAD_PASS", checks)
		Spring.SetGameRulesParam("sensor_load_passed", failed and 0 or 1, { public = true })
	end
	local finish = Spring.GetGameRulesParam("sensor_finish_at")
	if f % 30 == 0 and (not finish or f <= finish) then
		Witness(f)
	end
	if f == 1 then
		Spring.GiveOrderToUnit(target, CMD.MOVE, { 3550, 96, Z }, 0)
		Spring.GiveOrderToUnit(localTarget, CMD.MOVE, { 1850, 96, Z }, 0)
	end
	if f == 20 then
		Coverage("source", 1700, 3400)
		Check("other_row_los", Spring.IsPosInLos(1700, 96, 3000, 0) == not isolated)
		Check("other_row_airlos", Spring.IsPosInAirLos(1700, 1000, 3000, 0) == not isolated)
		Check("other_row_radar", Spring.IsPosInRadar(1700, 96, 3000, 0) == not isolated)
		Check("local_seismic_ping", (pings[localTarget] or 0) > 0)
		Check("foreign_seismic_ping", ((pings[target] or 0) > 0) == not isolated)
		local state = Spring.GetUnitLosState(target, 0)
		Check("foreign_unit_state", not not (state and state.los) == not isolated)
		if Spring.SetSensorRegions then
			Check("late_change_rejected", not Spring.SetSensorRegions())
		end
		sonarProbe = Spawn(3400, 0)
		Sensor(sonarProbe, "sonar", 4096)
		Sensor(emitter, "radarJammer", 4096)
		Sensor(emitter, "sonarJammer", 4096)
	end
	if f == 25 then
		Check("local_jammer", select(4, Spring.GetPositionLosState(1700, 96, Z, 0)))
		Check("foreign_jammer", not not select(4, Spring.GetPositionLosState(3400, 96, Z, 0)) == not isolated)
		Check("local_sonar_jammer", not Spring.IsPosInRadar(1700, -100, Z, 0))
		Check("foreign_sonar_jammer", Spring.IsPosInRadar(3400, -100, Z, 0) == isolated)
		Sensor(emitter, "radarJammer", 0)
		Sensor(emitter, "sonarJammer", 0)
		Spring.SetUnitPosition(emitter, 3400, Z)
	end
	if f == 45 then
		Coverage("travelled", 3500, 1700)
		Spring.SetUnitPosition(emitter, 2500, Z)
	end
	if f == 65 then
		Check("void_los", Spring.IsPosInLos(1700, 96, Z, 0) == not isolated)
		Check("void_radar", Spring.IsPosInRadar(3400, 96, Z, 0) == not isolated)
		Spring.DestroyUnit(emitter, false, true)
		Spring.DestroyUnit(sonarProbe, false, true)
	end
	if f == 80 then
		Spring.DestroyUnit(localTarget, false, true)
		proximityProbe = Spawn(1800, 0)

		Spring.SetUnitCloak(proximityProbe, true, 3000)
	end
	if f == 85 then
		Check(
			"proximity_foreign_enemy",
			(Spring.GetUnitNearestEnemy(proximityProbe, 4096, false) ~= nil) == not isolated
		)
	end
	if f == 100 then
		Check("foreign_enemy_cannot_decloak", Spring.GetUnitIsCloaked(proximityProbe) == isolated)
		proximityLocal = Spawn(1700, Spring.GetGaiaTeamID())
		Check(
			"proximity_local_enemy",
			Spring.GetUnitNearestEnemy(proximityProbe, 4096, false) == proximityLocal
		)
	end
	if f == 120 then
		Check("local_enemy_decloaks", not Spring.GetUnitIsCloaked(proximityProbe))
		Spring.DestroyUnit(proximityProbe, false, true)
		Spring.DestroyUnit(proximityLocal, false, true)
	end
	if f == 130 then
		Check("removed_los", not Spring.IsPosInLos(1700, 96, Z, 0))
		Check("removed_radar", not Spring.IsPosInRadar(3400, 96, Z, 0))
		Check("removed_sonar", not Spring.IsPosInRadar(3400, -100, Z, 0))
		Spring.SetGameRulesParam("sensor_test_passed", failed and 0 or 1, { public = true })
		Spring.Echo(failed and "SENSOR_REGIONS_FAIL" or "SENSOR_REGIONS_PASS", checks)
	end
end
