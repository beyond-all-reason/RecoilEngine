-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return {
		name = "Shared attack queue validation",
		desc = "Baseline/candidate queue and event comparison",
		author = "OpenAI Codex",
		layer = 1000000,
		enabled = true,
	}
end
if not gadgetHandler:IsSyncedCode() then
	function gadget:RecvFromSynced(name, a, b, targetA, targetB)
		if name == "shared_queue_quit" then
			Spring.SendCommands("quitforce")
		end
	end
	return
end
local attackers, targets, observed = {}, {}, {}
local phase, nested = "", false
local function emit(text)
	Spring.Echo("[SharedQueue] " .. text)
end
local function pack(command)
	local result = { command.id, command.tag, command.options.coded }
	for _, value in ipairs(command.params) do
		result[#result + 1] = value
	end
	return table.concat(result, ",")
end
local function snapshot(label)
	for i, id in ipairs(attackers) do
		local commands = Spring.GetUnitCommands(id, -1)
		assert(#commands == Spring.GetUnitCommandCount(id))
		local text = {}
		for j, command in ipairs(commands) do
			text[#text + 1] = pack(command)
			local cid, options, tag = Spring.GetUnitCurrentCommand(id, j)
			assert(cid == command.id and options == command.options.coded and tag == command.tag)
			assert(select(3, Spring.GetUnitCurrentCommand(id, j - #commands - 1)) == command.tag)
		end
		emit(label .. ":" .. i .. ":" .. table.concat(text, "|"))
	end
end
function gadget:AllowCommand(id, def, team, cmd, params)
	if not observed[id] then
		return true
	end
	emit("allow:" .. phase .. ":" .. observed[id] .. ":" .. cmd .. ":" .. table.concat(params, ","))
	if phase == "admission" and cmd == CMD.ATTACK and params[1] == targets[3] then
		if id == attackers[2] then
			return false
		end
		if id == attackers[1] and not nested then
			nested = true
			Spring.GiveOrderArrayToUnitArray({ id }, { { CMD.ATTACK, { targets[6] }, CMD.OPT_SHIFT } }, false)
		end
	end
	return true
end
function gadget:UnitCommand(id, def, team, cmd, params)
	if observed[id] then
		emit("command:" .. phase .. ":" .. observed[id] .. ":" .. cmd .. ":" .. table.concat(params, ","))
	end
end
function gadget:UnitCmdDone(id, def, team, cmd, params, options, tag)
	if observed[id] then
		emit("done:" .. phase .. ":" .. observed[id] .. ":" .. cmd .. ":" .. tag)
	end
end
function gadget:UnitCommandEnded(id, cmd, tag, reason)
	if observed[id] then
		emit("ended:" .. phase .. ":" .. observed[id] .. ":" .. cmd .. ":" .. tag .. ":" .. reason)
	end
end
function gadget:Initialize()
	Spring.SetGlobalLos(0, true)
	Spring.SetGlobalLos(1, true)
	gadgetHandler:RegisterAllowCommand(CMD.ANY)
end
function gadget:GameFrame(frame)
	if frame == 1 then
		for i = 1, 6 do
			targets[i] = assert(Spring.CreateUnit("armbanth", 2800 + i * 80, 200, 3000, 0, 1))
		end
		for i, name in ipairs({ "corak", "corak", "armliche" }) do
			local id = assert(Spring.CreateUnit(name, 1800 + i * 80, 200, 1800, 0, 0))
			attackers[i] = id
			Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, { 0 }, 0)
			Spring.GiveOrderToUnit(id, CMD.MOVE_STATE, { 0 }, 0)
			Spring.GiveOrderToUnit(id, CMD.WAIT, {}, 0)
			observed[id] = i
		end
		phase = "admission"
		local commands = {}
		for i = 1, 5 do
			commands[i] = { CMD.ATTACK, { targets[i] }, CMD.OPT_SHIFT }
		end
		Spring.GiveOrderArrayToUnitArray(attackers, commands, false)
		snapshot("admitted")
	elseif frame == 2 then
		phase = "remove_insert"
		local queue = Spring.GetUnitCommands(attackers[1], -1)
		Spring.GiveOrderToUnit(attackers[1], CMD.REMOVE, { queue[3].tag }, 0)
		Spring.GiveOrderToUnit(attackers[1], CMD.INSERT, { 3, CMD.MOVE, CMD.OPT_SHIFT, 1900, 200, 1900 }, CMD.OPT_ALT)
		Spring.GiveOrderToUnit(attackers[2], CMD.INSERT, { 0, CMD.ATTACK, CMD.OPT_SHIFT, targets[6] }, CMD.OPT_ALT)
		snapshot("edited")
	elseif frame == 3 then
		phase = "duplicate"
		Spring.GiveOrderArrayToUnitMap(
			{ [attackers[1]] = true, [attackers[2]] = true },
			{ { CMD.ATTACK, { targets[4] }, CMD.OPT_SHIFT } }
		)
		snapshot("duplicate")
	elseif frame == 4 then
		phase = "target_death"
		Spring.DestroyUnit(targets[5], false, true)
		snapshot("dead")
	elseif frame == 5 then
		phase = "advance"
		Spring.GiveOrderToUnit(attackers[1], CMD.REMOVE, { CMD.WAIT }, CMD.OPT_ALT)
		Spring.GiveOrderToUnit(attackers[3], CMD.WAIT, {}, 0)
		snapshot("advance")
	elseif frame == 40 then
		phase = "stop"
		snapshot("progress")
		Spring.GiveOrderToUnit(attackers[1], CMD.STOP, {}, 0)
		snapshot("stopped")
		phase = "network"
		Spring.SetGameRulesParam("sq_a", attackers[1])
		Spring.SetGameRulesParam("sq_b", attackers[3])
		Spring.SetGameRulesParam("sq_ta", targets[1])
		Spring.SetGameRulesParam("sq_tb", targets[2])
	elseif frame == 80 then
		snapshot("network")
		assert(Spring.GetUnitCommandCount(attackers[1]) == 3, "network batch not received")
		emit("PASS")
		SendToUnsynced("shared_queue_quit")
	end
end
