-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return {
		name = "Shared dependency validation",
		desc = "Mixed admission and reentrant target death",
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
local attackers, targets, observed = {}, {}, {}
local armed, nested = false, false
local function snapshot(label)
	local result = {}
	for i, id in ipairs(attackers) do
		local queue = {}
		for _, c in ipairs(Spring.GetUnitCommands(id, -1)) do
			queue[#queue + 1] = c.id .. ":" .. c.tag .. ":" .. c.options.coded .. ":" .. table.concat(c.params, ",")
		end
		result[i] = i .. "=" .. table.concat(queue, "/")
	end
	Spring.Echo("[SharedQueue] " .. label .. " " .. table.concat(result, "|"))
end
function gadget:AllowCommand(id, def, team, cmd, params)
	return not (id == attackers[4] and cmd == CMD.ATTACK and params[1] == targets[2])
end
function gadget:UnitCmdDone(id, def, team, cmd, params, options, tag)
	if armed and observed[id] then
		snapshot("done:" .. observed[id] .. ":" .. cmd .. ":" .. tag)
	end
end
function gadget:UnitCommandEnded(id, cmd, tag, reason)
	if not armed or not observed[id] then
		return
	end
	snapshot("ended:" .. observed[id] .. ":" .. cmd .. ":" .. tag .. ":" .. reason)
	if reason == "targetLost" and not nested then
		nested = true
		-- Modify a later subscriber while the first target is notifying.
		Spring.GiveOrderToUnit(attackers[6], CMD.STOP, {}, 0)
		Spring.GiveOrderArrayToUnitArray(
			{ attackers[2], attackers[6] },
			{ { CMD.ATTACK, { targets[4] }, CMD.OPT_SHIFT } },
			false
		)
		Spring.DestroyUnit(targets[2], false, true)
		snapshot("nested")
	end
end
function gadget:GameFrame(frame)
	if frame == 1 then
		Spring.SetGlobalLos(0, true)
		Spring.SetGlobalLos(1, true)
		for i = 1, 4 do
			targets[i] = assert(Spring.CreateUnit("armfort", 3000 + i * 64, 200, 3000, 0, 1))
		end
		for i = 1, 6 do
			local id = assert(Spring.CreateUnit("corak", 1800 + i * 64, 200, 1800, 0, 0))
			attackers[i], observed[id] = id, i
			Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, { 0 }, 0)
			Spring.GiveOrderToUnit(id, CMD.MOVE_STATE, { 0 }, 0)
		end
		local commands = {
			{ CMD.ATTACK, { targets[1] }, 0 },
			{ CMD.ATTACK, { targets[2] }, CMD.OPT_SHIFT },
			{ CMD.ATTACK, { targets[3] }, CMD.OPT_SHIFT },
		}
		Spring.GiveOrderArrayToUnitArray({ attackers[6], attackers[2], attackers[4] }, commands, false)
		for _, i in ipairs({ 5, 1, 3 }) do
			for _, command in ipairs(commands) do
				Spring.GiveOrderToUnit(attackers[i], command[1], command[2], command[3])
			end
		end
		-- Duplicate cancellation retains historical death registrations.
		Spring.GiveOrderToUnit(attackers[4], CMD.ATTACK, { targets[3] }, CMD.OPT_SHIFT)
		snapshot("issued")
	elseif frame == 30 then
		armed = true
		Spring.DestroyUnit(targets[1], false, true)
	elseif frame == 60 then
		assert(nested, "active target death did not trigger callback")
		snapshot("after-first")
		Spring.DestroyUnit(targets[3], false, true)
	elseif frame == 90 then
		snapshot("after-third")
		Spring.DestroyUnit(targets[4], false, true)
	elseif frame == 120 then
		for _, id in ipairs(attackers) do
			assert(Spring.GetUnitCommandCount(id) == 0)
		end
		snapshot("empty")
		Spring.Echo("[SharedQueue] PASS")
		SendToUnsynced("shared_queue_quit")
	end
end
