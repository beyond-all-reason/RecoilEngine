-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return {
		name = "Shared queue repeat validation",
		desc = "Repeat through native FinishCommand",
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
local attackers, indices, remaining = {}, {}, {}
local function snapshot(id, label)
	local result = {}
	for _, command in ipairs(Spring.GetUnitCommands(id, -1)) do
		result[#result + 1] = command.id
			.. ":"
			.. command.tag
			.. ":"
			.. command.options.coded
			.. ":"
			.. table.concat(command.params, ",")
	end
	Spring.Echo("[SharedQueue] " .. label .. ":" .. indices[id] .. ":" .. table.concat(result, "|"))
end
function gadget:AttackCommandMovement(id)
	if remaining[id] and remaining[id] > 0 then
		remaining[id] = remaining[id] - 1
		Spring.SetUnitAttackMovement(id, "finish")
		return true
	end
	return false
end
function gadget:UnitCommandEnded(id, cmd, tag, reason)
	if indices[id] then
		snapshot(id, "ended:" .. cmd .. ":" .. tag .. ":" .. reason)
	end
end
function gadget:UnitCmdDone(id, def, team, cmd, params, options, tag)
	if indices[id] then
		snapshot(id, "done:" .. cmd .. ":" .. tag)
	end
end
function gadget:Initialize()
	Spring.SetGlobalLos(0, true)
	Spring.SetGlobalLos(1, true)
end
function gadget:GameFrame(frame)
	if frame == 1 then
		local targets = {}
		for i = 1, 2 do
			targets[i] = assert(Spring.CreateUnit("armbanth", 2800 + i * 80, 200, 3000, 0, 1))
		end
		for i = 1, 2 do
			local id = assert(Spring.CreateUnit("corak", 1800 + i * 80, 200, 1800, 0, 0))
			attackers[i], indices[id] = id, i
			Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, { 0 }, 0)
			Spring.GiveOrderToUnit(id, CMD.MOVE_STATE, { 0 }, 0)
			Spring.GiveOrderToUnit(id, CMD.REPEAT, { 1 }, 0)
			remaining[id] = i == 1 and 8 or 3
		end
		Spring.GiveOrderArrayToUnitArray(
			attackers,
			{ { CMD.ATTACK, { targets[1] }, 0 }, { CMD.ATTACK, { targets[2] }, CMD.OPT_SHIFT } },
			false
		)
		for _, id in ipairs(attackers) do
			snapshot(id, "issued")
		end
	elseif frame == 150 then
		for _, id in ipairs(attackers) do
			assert(remaining[id] == 0, "finish callback did not run enough times")
			assert(Spring.GetUnitCommandCount(id) == 2, "repeat changed queue length")
			snapshot(id, "repeated")
		end
		Spring.GiveOrderToUnit(attackers[1], CMD.STOP, {}, 0)
		assert(Spring.GetUnitCommandCount(attackers[1]) == 0)
		assert(Spring.GetUnitCommandCount(attackers[2]) == 2)
		Spring.Echo("[SharedQueue] PASS")
		SendToUnsynced("shared_queue_quit")
	end
end
