-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
-- Keep a custom command at the front while supplying the same explicit unit
-- target as a native Attack. The custom parameter deliberately is not a unit ID.
function gadget:GetInfo()
	return { name = "Custom aircraft attack validation", layer = 100000, enabled = true }
end

local config = VFS.Include("custom_move_config.lua")
local customID = 34567
if gadgetHandler:IsSyncedCode() then
	local plane, target
	function gadget:Initialize()
		gadgetHandler:RegisterCMDID(customID)
		local mode = config.mode
		if mode == "air_registered" then
			Spring.RegisterCommand(customID, { attack = true })
			assert(not pcall(Spring.RegisterCommand, customID, { attack = false }))
		elseif mode == "air_false" then
			Spring.RegisterCommand(customID, { attack = false, movement = true })
		elseif mode == "air_default" then
			Spring.RegisterCommand(customID, { movement = true })
		elseif mode == "air_wrong_type" then
			Spring.RegisterCommand(customID, { attack = "true", movement = true })
		end
	end
	function gadget:CommandFallback(_, _, _, cmdID)
		if cmdID == customID then
			return true, false
		end
		return false
	end
	function gadget:GameFrame(frame)
		if frame == 1 then
			for _, id in ipairs(Spring.GetAllUnits()) do
				Spring.DestroyUnit(id, false, true)
			end
			plane = assert(Spring.CreateUnit("armfig", 2200, Spring.GetGroundHeight(2200, 2000) + 450, 2000, 1, 0))
			target = assert(Spring.CreateUnit("corhurc", 3100, Spring.GetGroundHeight(3100, 2200) + 450, 2200, 1, 1))
			Spring.MoveCtrl.Enable(target)
			Spring.SetUnitHealth(target, 1000000)
			Spring.SetUnitArmored(target, true, 0)
			Spring.GiveOrderToUnit(plane, CMD.IDLEMODE, { 0 }, 0)
			Spring.GiveOrderToUnit(plane, CMD.MOVE, { 2400, 450, 2000 }, 0)
		elseif frame == 90 then
			if config.mode == "air_native" then
				Spring.GiveOrderToUnit(plane, CMD.ATTACK, { target }, 0)
			else
				Spring.GiveOrderToUnit(plane, customID, { -1 }, 0)
			end
		end
		if frame >= 90 then
			Spring.SetUnitTarget(plane, target, false, true)
		end
	end
	function gadget:GameFramePost(frame)
		if frame >= 90 and frame <= config.finish then
			local x, y, z = Spring.GetUnitPosition(plane)
			local vx, vy, vz = Spring.GetUnitVelocity(plane)
			SendToUnsynced(
				"air_state",
				frame,
				string.format(
					"%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%d",
					x,
					y,
					z,
					vx,
					vy,
					vz,
					Spring.GetUnitHeading(plane)
				)
			)
		end
		if frame == config.finish then
			SendToUnsynced("air_complete")
		end
	end
else
	local file
	function gadget:Initialize()
		file = assert(io.open("aircraft.tsv", "w"))
		gadgetHandler:AddSyncAction("air_state", function(_, frame, state)
			file:write(frame, "\t", state, "\n")
		end)
		gadgetHandler:AddSyncAction("air_complete", function()
			file:flush()
			Spring.Echo("CUSTOM_AIR_COMPLETE")
			Spring.SendCommands("quitforce")
		end)
	end
	function gadget:Shutdown()
		if file then
			file:close()
		end
	end
end
