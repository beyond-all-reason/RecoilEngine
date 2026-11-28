-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return { name = "Command repeat validation", desc = "Invalid attacks and repeat controls", author = "OpenAI Codex", layer = math.huge, enabled = true }
end

if gadgetHandler:IsSyncedCode() then
	local subjects, names, counts, targets = {}, {}, {}, {}
	local crashTarget, transportTarget, transport
	local function spawn(name, x, z, team)
		local id = assert(Spring.CreateUnit(name, x, Spring.GetGroundHeight(x, z), z, "east", team or 0))
		Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, { 0 }, 0)
		return id
	end
	local function subject(label, name, x, z)
		local id = spawn(name, x, z)
		subjects[label], names[id], counts[id] = id, label, 0
		Spring.GiveOrderToUnit(id, CMD.REPEAT, { 1 }, 0)
		assert(Spring.GetUnitStates(id)["repeat"], label .. " must support repeat")
		return id
	end
	function gadget:UnitCmdDone(id, _, _, cmd)
		if names[id] and (cmd == CMD.ATTACK or cmd == CMD.REPAIR) then
			counts[id] = counts[id] + 1
		end
	end
	function gadget:GameFrame(frame)
		if frame == 1 then
			for _, id in ipairs(Spring.GetAllUnits()) do Spring.DestroyUnit(id, false, true) end
			for i, name in ipairs({ "armllt", "armstump", "armfig", "armcom" }) do
				subject("self_" .. name, name, 1800, 2000 + 400 * i)
				subject("lost_" .. name, name, 3000, 2000 + 400 * i)
				local target = spawn(name == "armfig" and "corhurc" or "armstump", 3500, 2000 + 400 * i, 1)
				targets["lost_" .. name] = target
				Spring.SetUnitLosMask(target, 0, 15)
				Spring.SetUnitLosState(target, 0, 15)
			end
			subject("crashing", "armfig", 2000, 4000)
			crashTarget = spawn("corhurc", 4600, 4000, 1)
			Spring.GiveOrderToUnit(crashTarget, CMD.MOVE, { 5000, 1000, 4000 }, 0)
			subject("transported", "armstump", 2000, 4400)
			transport = spawn("armatlas", 4000, 4400, 1)
			transportTarget = spawn("armstump", 4000, 4600, 1)
			subject("area_repeat", "armcom", 2000, 5000)
			subject("area_persistent", "armcom", 2400, 5000)
			subject("successor", "armstump", 2000, 5500)
			targets.successor = spawn("armstump", 3000, 5500, 1)
			Spring.SetUnitLosMask(targets.successor, 0, 15)
			Spring.SetUnitLosState(targets.successor, 0, 15)
			subject("repeat_off", "armstump", 2000, 6000)
			Spring.GiveOrderToUnit(subjects.repeat_off, CMD.REPEAT, { 0 }, 0)
		end
		-- Keep the native crashing state alive independently of ground impact.
		if crashTarget then
			assert(Spring.ValidUnitID(crashTarget) and not Spring.GetUnitIsDead(crashTarget))
			Spring.SetUnitPosition(crashTarget, 4600, 1000, 4000)
			Spring.SetUnitVelocity(crashTarget, 0, 0, 0)
		end
		-- Prevent combat damage without changing the subjects' native command AIs.
		for _, id in pairs(subjects) do
			local def = UnitDefs[Spring.GetUnitDefID(id)]
			for weapon = 1, #def.weapons do
				Spring.SetUnitWeaponState(id, weapon, { reloadState = frame + 10000 })
			end
		end
		if frame == 60 then
			for label, id in pairs(subjects) do
				if label:sub(1, 5) == "self_" or label == "repeat_off" then
					Spring.GiveOrderToUnit(id, CMD.ATTACK, { id }, 0)
				elseif label:sub(1, 5) == "lost_" then
					Spring.GiveOrderToUnit(id, CMD.ATTACK, { targets[label] }, 0)
				end
			end
			Spring.GiveOrderToUnit(subjects.crashing, CMD.ATTACK, { crashTarget }, 0)
			Spring.UnitAttach(transport, transportTarget, 1, true)
			assert(Spring.GetUnitTransporter(transportTarget) == transport)
			Spring.GiveOrderToUnit(subjects.transported, CMD.ATTACK, { transportTarget }, 0)
			Spring.GiveOrderToUnit(subjects.area_repeat, CMD.REPAIR, { 1600, 0, 5000, 50 }, 0)
			Spring.GiveOrderToUnit(subjects.area_persistent, CMD.REPAIR, { 1600, 0, 5000, 50 }, { "alt" })
			local id = subjects.successor
			Spring.GiveOrderToUnit(id, CMD.WAIT, {}, 0)
			Spring.GiveOrderToUnit(id, CMD.ATTACK, { id }, { "shift" })
			Spring.GiveOrderToUnit(id, CMD.ATTACK, { targets.successor }, { "shift" })
			Spring.GiveOrderToUnit(id, CMD.WAIT, {}, 0)
		elseif frame == 90 then
			assert(Spring.SetUnitCrashing(crashTarget, true), "target must start crashing")
			for label, target in pairs(targets) do
				if label:sub(1, 5) == "lost_" then Spring.SetUnitLosState(target, 0, 0) end
			end
		elseif frame == 300 then
			assert(Spring.GetUnitMoveTypeData(crashTarget).aircraftState == "crashing")
			assert(Spring.GetUnitTransporter(transportTarget) == transport)
			for label, id in pairs(subjects) do
				local q = Spring.GetUnitCommands(id, -1)
				SendToUnsynced("command_repeat", label, counts[id], #q, q[1] and q[1].params[1] or -1)
				if label == "successor" then
					assert(q[1] and q[1].id == CMD.ATTACK and q[1].params[1] == targets.successor)
				end
			end
			SendToUnsynced("command_repeat", "finished", frame)
		end
	end
else
	local output
	function gadget:Initialize()
		output = assert(io.open("command-repeat.csv", "w"))
		gadgetHandler:AddSyncAction("command_repeat", function(_, ...)
			local values = { ... }
			for i, value in ipairs(values) do values[i] = tostring(value) end
			output:write(table.concat(values, ","), "\n")
			output:flush()
		end)
	end
	function gadget:GameFrame(frame)
		if frame == 5 then Spring.SendCommands({ "setmaxspeed 20", "setminspeed 20" }) end
		if frame == 305 then Spring.SendCommands("quitforce") end
	end
	function gadget:Shutdown()
		if output then output:close() end
	end
end
