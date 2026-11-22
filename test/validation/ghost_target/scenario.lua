-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return { name = "Ghost target validation", author = "BAR contributors", layer = -100000, enabled = true }
end
if not gadgetHandler:IsSyncedCode() then
	local reported = false
	function gadget:DrawGhost(info)
		if not reported then
			assert(type(info.ghostID) == "number" and #info.teamColor == 4)
			Spring.Echo("GHOST_TEST PASS unsynced gadget DrawGhost")
			reported = true
		end
		return false
	end
	function gadget:RecvFromSynced(name)
		if name == "ghost_test_save" and Spring.GetConfigInt("GhostTestSave", 0) == 1 then
			Spring.SendCommands("save ghost_observers")
		end
		if name == "ghost_test_finish" and Spring.GetConfigInt("GhostTestHeadless", 0) == 1 then
			Spring.Echo("GHOST_TEST DONE")
			Spring.SendCommands("quitforce")
		end
	end
	return
end
local target, attack, priority, nuke
local radarAttack, radarPriority, lateTarget
local firstGhost
local vetoAttack
function gadget:Initialize()
	gadgetHandler:RegisterAllowCommand(CMD.ATTACK)
end
function gadget:AllowCommand(unitID, _, _, _, params)
	return unitID ~= vetoAttack or #params ~= 1
end
local function report(name, value)
	Spring.Echo("GHOST_TEST " .. name .. " " .. tostring(value))
end
-- Query a tiny cylinder around the remembered aimpoint (offset from the base).
local function checkSighting(allyTeam, x, y, z, expected)
	local ghosts = Spring.GetGhostsInCylinder(x, z, 1, allyTeam)
	assert(#ghosts == (expected and 1 or 0), "unexpected ghost count for allyTeam " .. allyTeam)
	if expected then
		local ax, ay, az = Spring.GetGhostPosition(ghosts[1], allyTeam)
		assert(ghosts[1] >= Game.maxUnits and ax == x and ay == y and az == z)
	end
end
local function checkOrders(probe, foreignID)
	local ally = 2
	local shooter = assert(Spring.CreateUnit("armrock", 2100, 200, 1800, 0, ally))
	local setter = assert(Spring.CreateUnit("armrock", 2150, 200, 1800, 0, ally))
	for _, id in ipairs({shooter, setter}) do
		Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, {0}, 0)
	end
	local order = Spring.GiveOrderToUnit
	local function queue()
		return Spring.GetUnitCommands(shooter, -1)
	end
	local function xyz(p, height)
		return #p == 3 and p[1] == 2020 and p[2] == height and p[3] == 2050
	end
	local function currentSet()
		local list = assert(GG.GetUnitTargetList(setter))
		assert(#list == 1)
		return list[1].target
	end
	order(shooter, CMD.MOVE, {1600, 200, 1600}, 0)
	order(shooter, CMD.MOVE, {1750, 200, 1600}, CMD.OPT_SHIFT)
	local anchors = queue()
	local function inserted(command, bits, ...)
		order(shooter, CMD.INSERT, {anchors[2].tag, command, bits, ...}, 0)
	end
	inserted(CMD.ATTACK, CMD.OPT_KEEP_HEIGHT, 2020, 999, 2050)
	local raw = queue()
	assert(xyz(raw[2].params, 200) and not raw[2].options.keepheight, "embedded keepheight must be normalized")
	order(shooter, CMD.REMOVE, {raw[2].tag}, 0)
	Spring.SetUnitLosState(probe, ally, 4)
	local rows = Spring.GetGhostsInCylinder(2020, 2050, 0, ally)
	assert(#rows == 2 and rows[1] ~= rows[2])
	local ghostID = rows[2]
	assert(not Spring.GetGhostPosition(foreignID, ally) and not Spring.GetGhostPosition(ghostID, 0))
	for _, invalid in ipairs({foreignID, 0, -1, 0.5}) do
		inserted(GameCMD.GHOST_ATTACK, 0, invalid)
		order(setter, GameCMD.GHOST_SET_TARGET, {invalid}, 0)
	end
	inserted(GameCMD.GHOST_ATTACK, 0, ghostID, 999)
	assert(#queue() == 2 and not GG.GetUnitTargetList(setter), "invalid requests changed targets")
	inserted(GameCMD.GHOST_ATTACK, CMD.OPT_SHIFT, ghostID)
	order(setter, CMD.INSERT, {0, GameCMD.GHOST_SET_TARGET, 0, ghostID}, CMD.OPT_ALT)
	local function checkAttack(isContact)
		local q = queue()
		assert(#q == 3 and q[1].tag == anchors[1].tag and q[3].tag == anchors[2].tag, "queue order")
		assert(q[2].id == CMD.ATTACK and q[2].options.shift, "attack type and shift")
		if isContact then
			assert(#q[2].params == 1 and q[2].params[1] == probe and not q[2].options.keepheight, "unit attack")
			assert(currentSet() == probe, "set-target contact")
		else
			assert(q[2].options.keepheight and xyz(q[2].params, 350), "ghost attack XYZ")
			assert(xyz(currentSet(), 350), "set-target ghost XYZ")
		end
	end
	checkAttack(false)
	vetoAttack = shooter
	local originalTag = queue()[2].tag
	Spring.SetUnitLosState(probe, ally, 6)
	local retained = queue()[2]
	assert(retained.tag == originalTag and xyz(retained.params, 350), "vetoed unit attack removed ghost order")
	assert(currentSet() == probe)
	vetoAttack = nil
	Spring.SetUnitLosState(probe, ally, 4)
	Spring.SetUnitLosState(probe, ally, 6)
	checkAttack(true)
	Spring.SetUnitLosState(probe, ally, 4)
	checkAttack(false)
	Spring.SetUnitLosState(probe, ally, 15)
	checkAttack(true)
	assert(not Spring.GetGhostPosition(ghostID, ally))
	-- Start a fresh ordinary unit attack and priority target while the unit is visible.
	order(shooter, CMD.REMOVE, {queue()[2].tag}, 0)
	inserted(CMD.ATTACK, CMD.OPT_SHIFT, probe)
	order(setter, GameCMD.UNIT_SET_TARGET, {probe}, 0)
	order(shooter, GameCMD.GHOST_ATTACK, {ghostID}, 0)
	order(setter, GameCMD.GHOST_SET_TARGET, {ghostID}, 0)
	checkAttack(true)
	Spring.SetUnitLosState(probe, ally, 4)
	checkAttack(false)
	assert(Spring.GetGhostsInCylinder(2020, 2050, 0, ally)[2] ~= ghostID)
	-- Two retained sightings now share all three coordinates and the same
	-- historical unit ID. Only the newly attached one may resolve on contact.
	Spring.SetUnitLeavesGhost(probe, false, true)
	Spring.SetUnitLeavesGhost(probe, true)
	Spring.SetUnitLosState(probe, ally, 15)
	Spring.SetUnitLosState(probe, ally, 4)
	local identical = Spring.GetGhostsInCylinder(2020, 2050, 0, ally)
	assert(#identical == 3 and select(2, Spring.GetGhostPosition(identical[2], ally)) == select(2, Spring.GetGhostPosition(identical[3], ally)))
	local latest = identical[3]
	order(shooter, CMD.INSERT, {2, GameCMD.GHOST_ATTACK, 0, latest}, CMD.OPT_ALT)
	order(setter, GameCMD.GHOST_SET_TARGET, {latest}, CMD.OPT_SHIFT)
	Spring.SetUnitLosState(probe, ally, 15)
	local after = queue()
	assert(#after == 4 and xyz(after[2].params, 350) and after[3].params[1] == probe and #after[3].params == 1)
	local targets = GG.GetUnitTargetList(setter)
	assert(#targets == 2 and xyz(targets[1].target, 350) and targets[2].target == probe)
	report("orders_recontact", true)
end

-- All three recipients get their first targeting order after this observer lost radar.
local function checkLateOrders(target, allyTeam, x, y, z)
	local function shooter(offset)
		local id = assert(Spring.CreateUnit("armrock", 2250 + offset, 200, 2000 - 50 * allyTeam, 0, allyTeam))
		Spring.SetUnitSensorRadius(id, "los", 0)
		Spring.SetUnitSensorRadius(id, "airLos", 0)
		return id
	end
	local direct, inserted, setter = shooter(0), shooter(50), shooter(100)
	local ghostID = assert(Spring.GetGhostsInCylinder(x, z, 1, allyTeam)[1])
	assert(not select(4, Spring.GetGhostPosition(ghostID, allyTeam)))
	Spring.GiveOrderToUnit(direct, CMD.ATTACK, {target}, 0)
	Spring.GiveOrderToUnit(inserted, CMD.MOVE, {1700, 200, 1600}, 0)
	local anchor = Spring.GetUnitCommands(inserted, 1)[1].tag
	Spring.GiveOrderToUnit(inserted, CMD.INSERT, {anchor, CMD.ATTACK, CMD.OPT_SHIFT, target}, 0)
	Spring.GiveOrderToUnit(setter, GameCMD.UNIT_SET_TARGET, {target}, 0)
	local function atSighting(p)
		return p and #p == 3 and p[1] == x and p[2] == y and p[3] == z
	end
	local cmd = Spring.GetUnitCommands(direct, 1)[1]
	assert(cmd and cmd.id == CMD.ATTACK and cmd.options.keepheight and atSighting(cmd.params), "late attack")
	local queue = Spring.GetUnitCommands(inserted, -1)
	assert(#queue == 2 and queue[2].tag == anchor and queue[1].id == CMD.ATTACK
		and queue[1].options.shift and queue[1].options.keepheight and atSighting(queue[1].params), "late insert")
	local entry = assert(GG.GetUnitTargetList(setter))[1]
	assert(entry.ghostID == ghostID and atSighting(entry.target), "late Set Target")
	Spring.SetUnitLosState(target, allyTeam, 6)
	local directCmd = Spring.GetUnitCommands(direct, 1)[1]
	queue = Spring.GetUnitCommands(inserted, -1)
	assert(#directCmd.params == 1 and directCmd.params[1] == target, "late attack on recontact")
	assert(#queue == 2 and queue[2].tag == anchor and #queue[1].params == 1 and queue[1].params[1] == target, "late insert on recontact")
	assert(GG.GetUnitTargetList(setter)[1].target == target, "late Set Target on recontact")
	report("late_unit_orders_allyteam_" .. allyTeam, true)
end

-- Both observable sites are empty. Their sightings must expire together, regardless of hidden survival.
local probes, probeGhosts, sites = {}, {}, {}
local function checkEmptySites(frame)
	local ally = 2
	if frame == 30 then
		for i, x in ipairs({3200, 3296}) do
			local u = assert(Spring.CreateUnit("armrock", x, Spring.GetGroundHeight(x, 3200), 3200, 0, 1))
			probes[i] = u
			Spring.SetUnitLeavesGhost(u, true)
			Spring.SetUnitLosMask(u, ally, 15)
			Spring.SetUnitLosState(u, ally, 15)
			Spring.SetUnitLosState(u, ally, 4)
			sites[i] = {Spring.GetUnitPosition(u)}
			probeGhosts[i] = assert(Spring.GetGhostsInCylinder(x, 3200, 32, ally)[1])
		end
	elseif frame == 45 then
		Spring.SetUnitPosition(probes[1], 4500, 4200)
		Spring.TransferUnit(probes[2], 0, false)
		assert(Spring.GetUnitTeam(probes[2]) == 0 and Spring.GetGhostPosition(probeGhosts[2], ally))
	elseif frame == 55 then
		Spring.DestroyUnit(probes[2], false, true)
	elseif frame == 65 then
		for _, id in ipairs(probeGhosts) do assert(Spring.GetGhostPosition(id, ally), "hidden transfer and destruction must preserve sighting") end
		assert(Spring.CreateUnit("armflea", 3248, Spring.GetGroundHeight(3248, 3232), 3232, 0, ally))
	elseif frame == 80 then
		for i, p in ipairs(sites) do
			assert(select(2, Spring.GetPositionLosState(p[1], p[2], p[3], ally)), "site must be in LOS")
			assert(not Spring.GetGhostPosition(probeGhosts[i], ally), "empty site retained its sighting")
		end
		local x, y, z = Spring.GetUnitPosition(probes[1])
		assert(x and not select(2, Spring.GetPositionLosState(x, y, z, ally)), "moved unit must still be hidden")
		report("empty_sites", true)
	end
end

function gadget:GameFrame(frame)
	-- Leave enough wall time for screenshot capture and screen-ray sweeps.
	if frame % 6 ~= 0 then return end
	frame = frame / 6
	if Spring.GetGhostsInCylinder then checkEmptySites(frame) end
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
		radarAttack = assert(Spring.CreateUnit("armrock", 1930, 200, 1750, 0, 0))
		radarPriority = assert(Spring.CreateUnit("armrock", 1980, 200, 1750, 0, 0))
		Spring.SetGameRulesParam("ghost_test_radar_attack", radarAttack)
		Spring.SetGameRulesParam("ghost_test_radar_priority", radarPriority)
		for _, id in ipairs({ attack, priority, radarAttack, radarPriority }) do
			Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, { 0 }, 0)
			Spring.GiveOrderToUnit(id, CMD.MOVE_STATE, { 0 }, 0)
			Spring.SetUnitSensorRadius(id, "los", 0)
			Spring.SetUnitSensorRadius(id, "airLos", 0)
		end
		nuke = assert(Spring.CreateUnit("armsilo", 2300, 200, 1750, 0, 0))
		Spring.SetUnitSensorRadius(nuke, "los", 0)
		Spring.SetUnitSensorRadius(nuke, "airLos", 0)
		if GameCMD.GHOST_ATTACK then
			Spring.SetGameRulesParam("ghost_test_attack_cmd", GameCMD.GHOST_ATTACK)
			Spring.SetGameRulesParam("ghost_test_set_cmd", GameCMD.GHOST_SET_TARGET)
		end
		Spring.SetGameRulesParam("ghost_test_attack", attack)
		Spring.SetGameRulesParam("ghost_test_priority", priority)
		Spring.SetGameRulesParam("ghost_test_command", GameCMD.UNIT_SET_TARGET)
		Spring.SetGameRulesParam("ghost_test_command_no_ground", GameCMD.UNIT_SET_TARGET_NO_GROUND)
		Spring.SetUnitLosMask(target, 2, 15)
		Spring.SetUnitLosState(target, 2, 15)
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
	elseif frame == 115 then
		Spring.SetUnitLosState(target, 2, 4)
	elseif frame == 120 or frame == 200 or frame == 270 then
		if Spring.GetGhostsInCylinder then
			checkSighting(0, 2060, 280, 2056, frame < 270)
			checkSighting(0, 2100, 400, 2080, false)
			checkSighting(2, 2060, 280, 2056, false)
			checkSighting(2, 2100, 400, 2080, true)
			if frame == 120 and Spring.GetGhostPosition then
				firstGhost = Spring.GetGhostsInCylinder(2060, 2056, 1, 0)[1]
				local second = Spring.GetGhostsInCylinder(2100, 2080, 1, 2)[1]
				assert(firstGhost ~= second and not Spring.GetGhostPosition(firstGhost, 2) and not Spring.GetGhostPosition(second, 0))
				Spring.SetGameRulesParam("ghost_test_foreign_id", firstGhost)
				Spring.GiveOrderToUnit(nuke, GameCMD.GHOST_ATTACK, { firstGhost }, 0)
				assert(Spring.GetUnitCommands(nuke, 1)[1].params[2] == 200)
			end
			if frame == 200 then
				assert(firstGhost and attack and priority, "missing saved scenario state")
				local cmd = assert(Spring.GetUnitCommands(attack, 1)[1])
				assert(cmd.options.keepheight and cmd.params[2] == 280, "hidden destruction changed the attack")
				assert(GG.GetUnitTargetList(priority)[1].ghostID == firstGhost, "hidden destruction changed Set Target")
			end
			if frame == 270 then
				assert(not Spring.GetUnitCommands(attack, 1)[1], "expired attack remained")
				assert(not GG.GetUnitTargetList(priority), "expired Set Target remained")
				report("synced_observers", true)
			end
		end
	elseif frame == 125 then
		Spring.SetUnitPosition(target, 3600, 3500)
	elseif frame == 150 then
		local command = Spring.GetUnitCommands(attack, 1)[1]
		report("click_attack_y", command and command.params[2])
		local _, _, pos = Spring.GetUnitWeaponTarget(priority, 1)
		report("click_settarget_y", type(pos) == "table" and pos[2])
	elseif frame == 151 and firstGhost then
		Spring.GiveOrderToUnit(attack, GameCMD.GHOST_ATTACK, { firstGhost }, 0)
		Spring.GiveOrderToUnit(priority, GameCMD.GHOST_SET_TARGET, { firstGhost }, 0)
	elseif frame == 155 then
		Spring.SetUnitPosition(target, 2048, 2048)
	elseif frame == 160 then
		Spring.SetUnitLosState(target, 0, 14)
		if firstGhost then
			assert(Spring.GetUnitCommands(attack, 1)[1].params[1] == target)
			assert(GG.GetUnitTargetList(priority)[1].target == target)
		end
	elseif frame == 170 then
		Spring.SetUnitLosState(target, 0, 6)
	elseif (frame == 174 or frame == 177) and firstGhost then
		local cmd = Spring.GetUnitCommands(radarAttack, 1)[1]
		local list = GG.GetUnitTargetList(radarPriority)
		local entry = list and list[1]
		if frame == 174 then
			report("radar_model_click_attack_unit", cmd and cmd.id == CMD.ATTACK and #cmd.params == 1 and cmd.params[1] == target)
			report("radar_model_click_set_unit", entry and entry.target == target)
		else
			report("radar_model_click_attack_ghost", cmd and cmd.id == CMD.ATTACK and cmd.options.keepheight and cmd.params[2] == 280)
			report("radar_model_click_set_ghost", entry and entry.ghostID == firstGhost)
		end
	elseif frame == 175 then
		Spring.SetUnitLosState(target, 0, 4)
	elseif frame == 180 then
		Spring.SetUnitPosition(target, 3600, 3500)
		Spring.DestroyUnit(target, false, true)
	elseif frame == 190 then
		SendToUnsynced("ghost_test_save")
	elseif frame == 240 then
		Spring.SetGlobalLos(0, true)
	elseif frame == 280 and Spring.GetGhostsInCylinder then
		-- Detach one sighting, then take another of the same still-living unit.
		local probe = assert(Spring.CreateUnit("cordoom", 2048, 200, 2048, 0, 1))
		Spring.SetUnitLosMask(probe, 2, 15)
		Spring.SetUnitLosState(probe, 2, 15)
		Spring.SetUnitMidAndAimPos(probe, 2048, 240, 2048, 2020, 300, 2050)
		Spring.SetUnitLosState(probe, 2, 4)
		Spring.SetUnitLeavesGhost(probe, false, true)
		Spring.SetUnitLeavesGhost(probe, true)
		Spring.SetUnitLosState(probe, 2, 15)
		Spring.SetUnitMidAndAimPos(probe, 2048, 240, 2048, 2020, 350, 2050)
		Spring.SetUnitLosState(probe, 2, 4)
		local rows = Spring.GetGhostsInCylinder(2020, 2050, 0, 2)
		assert(#rows == 2)
		local heights = {}
		for _, id in ipairs(rows) do
			local _, y = Spring.GetGhostPosition(id, 2)
			heights[y] = true
		end
		assert(heights[300] and heights[350])
		Spring.SetUnitLosState(probe, 2, 15)
		checkSighting(2, 2020, 300, 2050, true)
		report("separate_sightings", true)
		checkOrders(probe, Spring.GetGameRulesParam("ghost_test_foreign_id"))
	elseif frame == 282 and Spring.GetGhostPosition then
		Spring.SetGlobalLos(0, false)
		lateTarget = assert(Spring.CreateUnit("cordoom", 2400, 200, 2256, 0, 1))
		for _, allyTeam in ipairs({0, 2}) do
			Spring.SetUnitLosMask(lateTarget, allyTeam, 15)
			Spring.SetUnitLosState(lateTarget, allyTeam, 15)
			Spring.SetUnitMidAndAimPos(lateTarget, 2400, 240, 2256, 2412, 280 + 60 * allyTeam, 2264)
			Spring.SetUnitLosState(lateTarget, allyTeam, 14)
		end
	elseif frame == 284 and lateTarget then
		Spring.SetUnitLosState(lateTarget, 0, 4)
		Spring.SetUnitLosState(lateTarget, 2, 4)
	elseif frame == 286 and lateTarget then
		checkLateOrders(lateTarget, 0, 2412, 280, 2264)
		checkLateOrders(lateTarget, 2, 2412, 400, 2264)
	elseif frame == 300 then
		report("synced_done", true)
		SendToUnsynced("ghost_test_finish")
	end
end
