-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

function gadget:GetInfo()
	return { name = "Custom movement command validation", layer = 100000, enabled = true }
end

local config = VFS.Include("custom_move_config.lua")
local customID = 34567
local otherID = customID + 1
if gadgetHandler:IsSyncedCode() then
	local units = {}
	local targets = {}
	local attackMode = config.mode == "attack_native" or config.mode == "attack_controller"
	local finished = {}
	local positions = { { 2400, 2000 }, { 2100, 1600 } }
	local descriptionIndex
	function gadget:Initialize()
		gadgetHandler:RegisterCMDID(customID)
		gadgetHandler:RegisterCMDID(otherID)
		if not Spring.RegisterCommand then
			assert(attackMode, "Command registration unavailable")
			return
		end
		for _, id in ipairs({ CMD.STOP, CMD.WAIT, CMD.MOVE, -1, 1000, customID + 0.5, 2 ^ 40, math.huge, 0 / 0 }) do
			local ok, err = pcall(Spring.RegisterCommand, id, { movement = true })
			assert(not ok, "Accepted invalid command ID: " .. tostring(id) .. ": " .. tostring(err))
		end
		local function rejects(id, properties)
			assert(not pcall(Spring.RegisterCommand, id, properties), "Accepted invalid registration")
		end
		local defaultsID = customID + 2
		Spring.RegisterCommand(defaultsID, {})
		Spring.RegisterCommand(defaultsID, { movement = false })
		rejects(defaultsID, { movement = true })
		-- Rejected mutations must leave the original registration intact.
		Spring.RegisterCommand(defaultsID, {})
		for _, properties in ipairs({
			{ movement = 1 },
			{ movement = "true" },
			{ moving = true },
			{ ["movement\0extra"] = true },
			{ [1] = true },
			false,
		}) do
			rejects(customID, properties)
		end
		rejects(customID, nil)
		if config.mode == "default" then
			Spring.RegisterCommand(customID, {})
		elseif config.mode ~= "native" and config.mode ~= "unregistered" then
			Spring.RegisterCommand(customID, { movement = true })
		end
		if config.mode == "duplicate" then
			rejects(customID, { movement = false })
			rejects(customID, {})
			rejects(customID, { movement = true, unknown = true })
			Spring.RegisterCommand(customID, { movement = true })
		end
	end
	function gadget:GameFrame(frame)
		if frame == 1 then
			for _, id in ipairs(Spring.GetAllUnits()) do
				Spring.DestroyUnit(id, false, true)
			end
		elseif frame == 5 then
			for index, pos in ipairs(positions) do
				local x, z = pos[1], pos[2]
				units[index] = assert(Spring.CreateUnit("armfav", x, Spring.GetGroundHeight(x, z), z, 1, 0))
				Spring.GiveOrderToUnit(units[index], CMD.FIRE_STATE, { 0 }, 0)
				assert(Spring.FindUnitCmdDesc(units[index], customID) == nil)
				if attackMode then
					targets[index] = {}
					for target = 1, 2 do
						local tx, tz = x + 1200, z + target * 200
						local id = assert(Spring.CreateUnit("corak", tx, Spring.GetGroundHeight(tx, tz), tz, 1, 1))
						targets[index][target] = id
						Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, { 0 }, 0)
					end
				end
			end
			if config.mode == "removed" or config.mode == "renamed" then
				Spring.InsertUnitCmdDesc(
					units[1],
					{ id = customID, type = CMDTYPE.ICON_MAP, name = "Controller", hidden = true }
				)
				descriptionIndex = assert(Spring.FindUnitCmdDesc(units[1], customID))
			end
		elseif frame == 10 then
			local command = config.mode == "native" and CMD.MOVE or customID
			if config.mode == "other_id" then
				command = otherID
			end
			for index, pos in ipairs(positions) do
				local x, z = pos[1], pos[2]
				Spring.GiveOrderToUnit(units[index], CMD.MOVE, { x + 225, Spring.GetGroundHeight(x + 225, z), z }, 0)
				if config.mode == "attack_native" then
					for _, target in ipairs(targets[index]) do
						Spring.GiveOrderToUnit(units[index], CMD.ATTACK, { target }, CMD.OPT_SHIFT)
					end
				elseif config.mode == "attack_controller" then
					Spring.GiveOrderToUnit(units[index], assert(GameCMD.ATTACK_TARGETS), targets[index], CMD.OPT_SHIFT)
				else
					Spring.GiveOrderToUnit(
						units[index],
						command,
						{ x + 600, Spring.GetGroundHeight(x + 600, z + 200), z + 200 },
						CMD.OPT_SHIFT
					)
				end
			end
		elseif frame == 11 then
			if config.mode == "duplicate" then
				assert(not pcall(Spring.RegisterCommand, customID, {}))
				Spring.RegisterCommand(customID, { movement = true })
			end
			if config.mode == "removed" then
				Spring.RemoveUnitCmdDesc(units[1], descriptionIndex)
			elseif config.mode == "renamed" then
				Spring.EditUnitCmdDesc(units[1], descriptionIndex, { id = otherID })
			end
		end
	end
	function gadget:CommandFallback(unitID, unitDefID, team, command, params, options, tag)
		if command ~= customID and command ~= otherID then
			return false
		end
		Spring.GiveOrderToUnit(unitID, CMD.INSERT, { 0, CMD.MOVE, 0, params[1], params[2], params[3] }, CMD.OPT_ALT)
		Spring.GiveOrderToUnit(unitID, CMD.REMOVE, { tag }, CMD.OPT_INTERNAL)
		return true, false
	end
	function gadget:UnitCmdDone(unitID, unitDefID, team, command)
		for index, id in ipairs(units) do
			if unitID == id and command == CMD.MOVE and not finished[index] then
				finished[index] = Spring.GetGameFrame()
				SendToUnsynced("move_finished", index, finished[index])
			end
		end
	end
	function gadget:GameFramePost(frame)
		if #units > 0 then
			local states = {}
			for _, id in ipairs(units) do
				local px, py, pz = Spring.GetUnitPosition(id)
				local vx, vy, vz = Spring.GetUnitVelocity(id)
				states[#states + 1] = string.format(
					"%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%d",
					px,
					py,
					pz,
					vx,
					vy,
					vz,
					Spring.GetUnitHeading(id)
				)
			end
			SendToUnsynced("move_state", frame, table.concat(states, "\t"))
		end
		if frame == config.finish then
			assert(finished[1] and finished[2], "First Move never finished")
			SendToUnsynced("move_complete", frame)
		end
	end
else
	local trace
	function gadget:Initialize()
		trace = assert(io.open("movement.tsv", "w"))
		gadgetHandler:AddSyncAction("move_state", function(_, frame, state)
			trace:write(frame, "\t", state, "\n")
		end)
		gadgetHandler:AddSyncAction("move_finished", function(_, index, frame)
			local file = assert(io.open("first-move.txt", "a"))
			file:write(index, "\t", frame, "\n")
			file:close()
		end)
		gadgetHandler:AddSyncAction("move_complete", function(_, frame)
			trace:flush()
			Spring.Echo("CUSTOM_MOVE_COMPLETE", frame)
			Spring.SendCommands("quitforce")
		end)
	end
	function gadget:Shutdown()
		if trace then
			trace:close()
		end
	end
end
