-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return {
		name = "Shared queue save validation",
		desc = "Full game save/load queue roundtrip",
		author = "OpenAI Codex",
		layer = 1000000,
		enabled = true,
	}
end
if not gadgetHandler:IsSyncedCode() then
	function gadget:RecvFromSynced(name)
		if name == "shared_queue_save" then
			Spring.SendCommands("save shared-queues")
		end
		if name == "shared_queue_quit" then
			Spring.SendCommands("quitforce")
		end
	end
	return
end
local attackers, expected = {}, {}
local function snapshot(id)
	local result = {}
	for _, c in ipairs(Spring.GetUnitCommands(id, -1)) do
		result[#result + 1] = c.id .. ":" .. c.tag .. ":" .. c.options.coded .. ":" .. table.concat(c.params, ",")
	end
	return table.concat(result, "|")
end
function gadget:GameFrame(frame)
	if frame == 1 then
		Spring.SetGlobalLos(0, true)
		Spring.SetGlobalLos(1, true)
		local commands = {}
		for i = 1, 10 do
			local id = assert(Spring.CreateUnit("armfort", 4000 + i * 64, 200, 4000, 0, 1))
			commands[i] = { CMD.ATTACK, { id }, CMD.OPT_SHIFT }
		end
		for i = 1, 3 do
			local id = assert(Spring.CreateUnit("corak", 1800 + i * 80, 200, 1800, 0, 0))
			attackers[i] = id
			Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, { 0 }, 0)
			Spring.GiveOrderToUnit(id, CMD.WAIT, {}, 0)
		end
		Spring.GiveOrderArrayToUnitArray(attackers, commands, false)
		local queue = Spring.GetUnitCommands(attackers[2], -1)
		Spring.GiveOrderToUnit(attackers[2], CMD.REMOVE, { queue[5].tag }, 0)
		for i, id in ipairs(attackers) do
			assert(Spring.GetUnitCommandCount(id) == (i == 2 and 10 or 11))
			expected[i] = snapshot(id)
		end
	elseif frame == 5 then
		SendToUnsynced("shared_queue_save")
	elseif frame == 90 then
		for i, id in ipairs(attackers) do
			assert(snapshot(id) == expected[i], "queue roundtrip mismatch")
			Spring.Echo("[SharedQueue] restored:" .. i .. ":" .. snapshot(id))
		end
		Spring.GiveOrderToUnit(attackers[1], CMD.STOP, {}, 0)
		assert(Spring.GetUnitCommandCount(attackers[1]) == 0)
		assert(snapshot(attackers[3]) == expected[3])
	elseif frame == 100 then
		local queue = Spring.GetUnitCommands(attackers[3], -1)
		Spring.DestroyUnit(queue[2].params[1], false, true)
	elseif frame == 110 then
		assert(Spring.GetUnitCommandCount(attackers[1]) == 0)
		assert(Spring.GetUnitCommandCount(attackers[2]) == 9)
		assert(Spring.GetUnitCommandCount(attackers[3]) == 10)
		for i = 2, 3 do
			Spring.Echo("[SharedQueue] target-died:" .. i .. ":" .. snapshot(attackers[i]))
		end
		Spring.Echo("[SharedQueue] PASS")
		SendToUnsynced("shared_queue_quit")
	end
end
