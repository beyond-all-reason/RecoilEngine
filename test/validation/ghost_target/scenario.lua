-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return { name = "Ghost target validation", author = "BAR contributors", layer = -100000, enabled = true }
end
if not gadgetHandler:IsSyncedCode() then
	return false
end
local target, attack, priority, nuke
local function report(name, value)
	Spring.Echo("GHOST_TEST " .. name .. " " .. tostring(value))
end
function gadget:GameFrame(frame)
	if frame == 1 then
		for _, id in ipairs(Spring.GetAllUnits()) do
			Spring.DestroyUnit(id, false, true)
		end
	elseif frame == 15 then
		Spring.SetHeightMapFunc(function()
			for x = 1536, 2560, 8 do
				for z = 1536, 2560, 8 do
					Spring.SetHeightMap(x, z, 200)
				end
			end
		end)
		target = assert(Spring.CreateUnit("cordoom", 2048, 200, 2048, 0, 1))
		attack = assert(Spring.CreateUnit("armrock", 2048, 200, 1750, 0, 0))
		priority = assert(Spring.CreateUnit("armrock", 2100, 200, 1750, 0, 0))
		for _, id in ipairs({ attack, priority }) do
			Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, { 0 }, 0)
			Spring.GiveOrderToUnit(id, CMD.MOVE_STATE, { 0 }, 0)
			Spring.SetUnitSensorRadius(id, "los", 0)
			Spring.SetUnitSensorRadius(id, "airLos", 0)
		end
		nuke = assert(Spring.CreateUnit("armsilo", 2300, 200, 1750, 0, 0))
		Spring.SetUnitSensorRadius(nuke, "los", 0)
		Spring.SetUnitSensorRadius(nuke, "airLos", 0)
		Spring.SetGameRulesParam("ghost_test_attack", attack)
		Spring.SetGameRulesParam("ghost_test_priority", priority)
		Spring.SetGameRulesParam("ghost_test_command", GameCMD.UNIT_SET_TARGET)
		Spring.SetGameRulesParam("ghost_test_command_no_ground", GameCMD.UNIT_SET_TARGET_NO_GROUND)
		Spring.SetUnitLosMask(target, 0, 15)
		Spring.SetUnitLosState(target, 0, 15)
		Spring.SetUnitMidAndAimPos(target, 2048, 240, 2048, 2060, 280, 2056)
		Spring.SetGameRulesParam("ghost_test_target", target)
		Spring.SetUnitHealth(target, 1000000)
	elseif frame == 40 then
		Spring.GiveOrderToUnit(attack, CMD.ATTACK, { 2060, 280, 2056 }, CMD.OPT_KEEP_HEIGHT or 1)
		Spring.GiveOrderToUnit(priority, GameCMD.UNIT_SET_TARGET, { 2060, 280, 2056 }, CMD.OPT_KEEP_HEIGHT or 1)
		Spring.GiveOrderToUnit(nuke, CMD.ATTACK, { 2060, 280, 2056 }, CMD.OPT_KEEP_HEIGHT or 1)
	elseif frame == 50 then
		report("nuke_y", Spring.GetUnitCommands(nuke, 1)[1].params[2])
		local commands = Spring.GetUnitCommands(attack, 1)
		report("attack_y", commands[1] and commands[1].params[2])
		local _, _, pos = Spring.GetUnitWeaponTarget(priority, 1)
		report("settarget_y", type(pos) == "table" and pos[2])
		Spring.GiveOrderToUnit(attack, CMD.ATTACK, { 2060, 280, 2056 }, 0)
		Spring.GiveOrderToUnit(priority, GameCMD.UNIT_SET_TARGET, { 2060, 280, 2056 }, 0)
	elseif frame == 60 then
		report("ground_attack_y", Spring.GetUnitCommands(attack, 1)[1].params[2])
		local _, _, pos = Spring.GetUnitWeaponTarget(priority, 1)
		report("ground_settarget_y", type(pos) == "table" and pos[2])
	elseif frame == 90 then
		Spring.SetUnitLosState(target, 0, 4)
	elseif frame == 110 then
		Spring.SetUnitMidAndAimPos(target, 2048, 240, 2048, 2100, 400, 2080)
	elseif frame == 150 then
		report("click_attack_y", Spring.GetUnitCommands(attack, 1)[1].params[2])
		local _, _, pos = Spring.GetUnitWeaponTarget(priority, 1)
		report("click_settarget_y", type(pos) == "table" and pos[2])
	elseif frame == 160 then
		Spring.SetUnitLosState(target, 0, 14)
	elseif frame == 170 then
		Spring.SetUnitLosState(target, 0, 6)
	elseif frame == 175 then
		Spring.SetUnitLosState(target, 0, 4)
	elseif frame == 180 then
		Spring.DestroyUnit(target, false, true)
	elseif frame == 240 then
		Spring.SetGlobalLos(0, true)
	elseif frame == 300 then
		report("synced_done", true)
	end
end
