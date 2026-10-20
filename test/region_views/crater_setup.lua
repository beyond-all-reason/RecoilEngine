-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
function gadget:GetInfo()
	return { name = "Native crater region regression", layer = 0, enabled = true }
end
if not gadgetHandler:IsSyncedCode() then
	return
end
local isolated = Spring.GetModOptions().sensor_test ~= "whole-map"
local checks, failed = 0, false
local buildings, initial, heights = {}, {}, {}
local points = { { 1600, 2048 }, { 2304, 2048 }, { 2800, 2048 }, { 1600, 3200 } }
local regions = { { 0, 0, 2048, 4096 }, { 2560, 0, 5120, 4096 } }
local acks = {}
local function Check(name, value)
	checks = checks + 1
	failed = failed or not value
	Spring.Echo("[CRATER_REGIONS] ASSERT " .. name .. " pass=" .. tostring(not not value))
end
local function Remember()
	for i, p in ipairs(points) do
		heights[i] = Spring.GetGroundHeight(p[1], p[2])
	end
	for i, id in ipairs(buildings) do
		initial[i] = select(2, Spring.GetUnitPosition(id))
	end
end
local function Explode(x, z)
	Spring.SpawnExplosion(x, Spring.GetGroundHeight(x, z), z, 0, 1, 0, {
		damages = { [0] = 20000, craterMult = 1 },
		craterAreaOfEffect = 1400,
		damageAreaOfEffect = 1,
		damageGround = true,
	})
end
local function Verify(prefix)
	local changed = {}
	for i, p in ipairs(points) do
		changed[i] = math.abs(Spring.GetGroundHeight(p[1], p[2]) - heights[i]) > 0.01
	end
	Check(prefix .. "_local_terrain_changes", changed[1])
	Check(prefix .. "_barrier_terrain", changed[2] == not isolated)
	Check(prefix .. "_foreign_terrain", changed[3] == not isolated)
	local y1 = select(2, Spring.GetUnitPosition(buildings[1]))
	local y2 = select(2, Spring.GetUnitPosition(buildings[2]))
	Check(prefix .. "_local_building_moves", math.abs(y1 - initial[1]) > 0.01)
	Check(prefix .. "_foreign_building", (math.abs(y2 - initial[2]) > 0.01) == not isolated)
	local y3 = select(2, Spring.GetUnitPosition(buildings[3]))
	Check(prefix .. "_straddling_building", (math.abs(y3 - initial[3]) > 0.01) == not isolated)
end
function gadget:Initialize()
	if isolated then
		Check("synced_api_available", type(Spring.SetMapDamageRegions) == "function")
		if Spring.SetMapDamageRegions then
			Check("configured", Spring.SetMapDamageRegions(regions))
			Check(
				"overlap_rejected",
				not Spring.SetMapDamageRegions({ { 0, 0, 4096, 4096 }, { 2560, 0, 5120, 4096 } })
			)
			Check("unaligned_rejected", not Spring.SetMapDamageRegions({ { 0, 0, 2049, 4096 } }))
			Check("nan_rejected", not Spring.SetMapDamageRegions({ { 0, 0, 0 / 0, 4096 } }))
			Check("malformed_rejected", not pcall(Spring.SetMapDamageRegions, { { 0, 0, 5120 } }))
			-- Even pre-frame initialization cannot reconfigure a queued crater.
			Spring.SpawnExplosion(
				512,
				96,
				512,
				0,
				1,
				0,
				{ damages = { [0] = 100, craterMult = 1 }, craterAreaOfEffect = 16, damageGround = true }
			)
			Check("pending_crater_reconfiguration_rejected", not Spring.SetMapDamageRegions())
		end
	end
	for i, x in ipairs({ 1792, 2704, 2048 }) do
		local id = assert(Spring.CreateUnit("region_test_unit", x, 96, 2048, 0, 0))
		Spring.SetUnitBlocking(id, true, true, true, true, true, true, true)
		buildings[i] = id
	end
end
function gadget:RecvLuaMsg(message, player)
	if message ~= "crater_ready" then
		return
	end
	acks[player] = true
	local n = 0
	for _ in pairs(acks) do
		n = n + 1
	end
	if n == (tonumber(Spring.GetModOptions().fixture_clients) or 1) then
		Spring.SetGameRulesParam("crater_finish", Spring.GetGameFrame() + 90, { public = true })
	end
end
function gadget:GameFrame(f)
	if
		f % 30 == 0
		and (not Spring.GetGameRulesParam("crater_finish") or f <= Spring.GetGameRulesParam("crater_finish"))
	then
		local state = { tostring(f) }
		for _, p in ipairs(points) do
			state[#state + 1] = string.format("%.5f", Spring.GetGroundHeight(p[1], p[2]))
		end
		Spring.Echo("[CRATER_WITNESS] " .. table.concat(state, ","))
	end
	if f == 10 then
		if Spring.SetMapDamageRegions then
			Check("late_change_rejected", not Spring.SetMapDamageRegions())
		end
		Remember()
		Explode(1904, 2048)
	end
	if f == 35 then
		Verify("impact")
		Remember()
		Explode(2304, 2048)
	end
	if f == 60 then
		local unchanged = true
		for i, p in ipairs(points) do
			unchanged = unchanged and Spring.GetGroundHeight(p[1], p[2]) == heights[i]
		end
		Check("void_impact", unchanged == isolated)
		Spring.Echo(failed and "CRATER_REGIONS_FAIL" or "CRATER_REGIONS_PASS", checks)
		Spring.SetGameRulesParam("crater_passed", failed and 0 or 1, { public = true })
	end
	-- The source process pauses at 60. Only the fresh loading process advances.
	if f == 80 and Spring.GetModOptions().fixture_save_load == "1" then
		Remember()
		Explode(1904, 2048)
	end
	if f == 105 and Spring.GetModOptions().fixture_save_load == "1" then
		Verify("loaded")
		Spring.Echo(failed and "CRATER_SAVELOAD_FAIL" or "CRATER_SAVELOAD_PASS", checks)
		Spring.SetGameRulesParam("crater_loaded", failed and 0 or 1, { public = true })
	end
end
