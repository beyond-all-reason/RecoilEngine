-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

function gadget:GetInfo()
	return { name = "Custom movement command validation", layer = 100000, enabled = true }
end

local config = VFS.Include("custom_move_config.lua")
local customID = 34567
if gadgetHandler:IsSyncedCode() then
	local source
	local firstMoveFinished
	local descriptionIndex
	local x, z = 2400, 2000
	local function description(marked)
		return { id = customID, type = CMDTYPE.ICON_MAP, name = "Controller", hidden = true, moveCommand = marked }
	end
	local function insertDescription(unitID, marked)
		Spring.InsertUnitCmdDesc(unitID, description(marked))
		return Spring.FindUnitCmdDesc(unitID, customID)
	end
	local function readFlag(unitID, index)
		return Spring.GetUnitCmdDescs(unitID, index, index)[1].moveCommand
	end
	function gadget:Initialize()
		gadgetHandler:RegisterCMDID(customID)
	end
	function gadget:GameFrame(frame)
		if frame == 1 then
			for _, id in ipairs(Spring.GetAllUnits()) do
				Spring.DestroyUnit(id, false, true)
			end
		elseif frame == 5 then
			source = assert(Spring.CreateUnit("armfav", x, Spring.GetGroundHeight(x, z), z, 1, 0))
			local other = assert(Spring.CreateUnit("armfav", x - 300, Spring.GetGroundHeight(x - 300, z), z, 1, 0))
			-- A marked descriptor on another unit must not affect this unit.
			local otherIndex = insertDescription(other, true)
			assert(readFlag(other, otherIndex) == true)
			local marked = config.mode == "marked"
				or config.mode == "cleared"
				or config.mode == "removed"
				or config.mode == "renamed"
			descriptionIndex = insertDescription(source, marked)
			assert(readFlag(source, descriptionIndex) == marked)
			Spring.GiveOrderToUnit(source, CMD.FIRE_STATE, { 0 }, 0)
			Spring.GiveOrderToUnit(other, CMD.FIRE_STATE, { 0 }, 0)
		elseif frame == 10 then
			Spring.GiveOrderToUnit(source, CMD.MOVE, { x + 225, Spring.GetGroundHeight(x + 225, z), z }, 0)
			local command = config.mode == "native" and CMD.MOVE or customID
			Spring.GiveOrderToUnit(
				source,
				command,
				{ x + 600, Spring.GetGroundHeight(x + 600, z + 200), z + 200 },
				CMD.OPT_SHIFT
			)
		elseif frame == 11 then
			-- Change descriptions while the controller is already queued.
			local index = descriptionIndex
			if config.mode == "edited" then
				Spring.EditUnitCmdDesc(source, index, { moveCommand = true })
				assert(readFlag(source, index) == true)
			elseif config.mode == "cleared" or config.mode == "removed" or config.mode == "renamed" then
				if config.mode == "cleared" then
					Spring.EditUnitCmdDesc(source, index, { moveCommand = false })
					assert(readFlag(source, index) == false)
				elseif config.mode == "removed" then
					Spring.RemoveUnitCmdDesc(source, index)
					assert(Spring.FindUnitCmdDesc(source, customID) == nil)
				else
					Spring.EditUnitCmdDesc(source, index, { id = customID + 1 })
					assert(Spring.FindUnitCmdDesc(source, customID) == nil)
				end
			end
		end
	end
	function gadget:CommandFallback(unitID, unitDefID, team, command, params, options, tag)
		if command ~= customID then
			return false
		end
		Spring.GiveOrderToUnit(unitID, CMD.INSERT, { 0, CMD.MOVE, 0, params[1], params[2], params[3] }, CMD.OPT_ALT)
		Spring.GiveOrderToUnit(unitID, CMD.REMOVE, { tag }, CMD.OPT_INTERNAL)
		return true, false
	end
	function gadget:UnitCmdDone(unitID, unitDefID, team, command)
		if unitID == source and command == CMD.MOVE and not firstMoveFinished then
			firstMoveFinished = Spring.GetGameFrame()
			SendToUnsynced("move_finished", firstMoveFinished)
		end
	end
	function gadget:GameFramePost(frame)
		if source then
			local px, py, pz = Spring.GetUnitPosition(source)
			local vx, vy, vz = Spring.GetUnitVelocity(source)
			SendToUnsynced(
				"move_state",
				frame,
				string.format(
					"%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%d",
					px,
					py,
					pz,
					vx,
					vy,
					vz,
					Spring.GetUnitHeading(source)
				)
			)
		end
		if frame == config.finish then
			assert(firstMoveFinished, "First Move never finished")
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
		gadgetHandler:AddSyncAction("move_finished", function(_, frame)
			local file = assert(io.open("first-move.txt", "w"))
			file:write(frame, "\n")
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
