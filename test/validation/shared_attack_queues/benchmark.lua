-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return {
		name = "Shared queue benchmark",
		desc = "Controlled issue/read/stop benchmark",
		author = "OpenAI Codex",
		layer = 1000000,
		enabled = true,
	}
end
if not gadgetHandler:IsSyncedCode() then
	function gadget:RecvFromSynced(name)
		if name == "shared_queue_quit" then
			Spring.SendCommands("quitforce")
		end
	end
	return
end
local attackers, commands = {}, {}
local numAttackers = tonumber(Spring.GetModOptions().sharedqueueattackers) or 1000
local numTargets = tonumber(Spring.GetModOptions().sharedqueuetargets) or 200
local function mark(label)
	Spring.Echo("[SharedQueueBench] " .. label)
end
function gadget:Initialize()
	Spring.SetGlobalLos(0, true)
	Spring.SetGlobalLos(1, true)
end
function gadget:GameFrame(frame)
	if frame == 1 then
		for i = 1, numTargets do
			local id =
				assert(Spring.CreateUnit("armfort", 4000 + (i % 20) * 32, 200, 4000 + math.floor(i / 20) * 32, 0, 1))
			commands[i] = { CMD.ATTACK, { id }, CMD.OPT_SHIFT }
		end
		for i = 1, numAttackers do
			local id =
				assert(Spring.CreateUnit("corak", 1000 + (i % 40) * 32, 200, 1000 + math.floor(i / 40) * 32, 0, 0))
			attackers[i] = id
			Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, { 0 }, 0)
			Spring.GiveOrderToUnit(id, CMD.WAIT, {}, 0)
		end
	elseif frame == 2 then
		mark("pre_issue")
		Spring.GiveOrderArrayToUnitArray(attackers, commands, false)
		mark("post_issue")
		for _, id in ipairs(attackers) do
			local count = Spring.GetUnitCommandCount(id)
			assert(count == numTargets + 1, "unit " .. id .. " queue count " .. count)
		end
	elseif frame == 5 then
		mark("pre_read")
		local count = 0
		for _, id in ipairs(attackers) do
			local queue = Spring.GetUnitCommands(id, -1)
			count = count + #queue
		end
		mark("post_read")
		assert(count == numAttackers * (numTargets + 1))
	elseif frame == 10 then
		mark("pre_stop")
		Spring.GiveOrderToUnitArray(attackers, CMD.STOP, {}, 0)
		mark("post_stop")
		for _, id in ipairs(attackers) do
			assert(Spring.GetUnitCommandCount(id) == 0)
		end
		Spring.Echo("[SharedQueue] PASS")
		SendToUnsynced("shared_queue_quit")
	end
end
