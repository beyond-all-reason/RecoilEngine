-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
function gadget:GetInfo()
	return { name = "Unit deletion validation", layer = 100000, enabled = true }
end
if gadgetHandler:IsSyncedCode() then
	local source, target, following, destroyedFrame, deletedFrame
	function gadget:GameFrame(frame)
		if frame == 1 then
			for _, id in ipairs(Spring.GetAllUnits()) do
				Spring.DestroyUnit(id, false, true)
			end
			source = assert(Spring.CreateUnit("armfig", 2200, 450, 2000, 1, 0))
			target = assert(Spring.CreateUnit("corhurc", 3100, 450, 2200, 1, 1))
			following = assert(Spring.CreateUnit("corhurc", 3500, 450, 2400, 1, 1))
			Spring.MoveCtrl.Enable(target)
			Spring.MoveCtrl.Enable(following)
			Spring.GiveOrderToUnit(source, CMD.FIRE_STATE, { 0 }, 0)
			Spring.GiveOrderToUnit(source, CMD.ATTACK, { target }, 0)
			Spring.GiveOrderToUnit(source, CMD.ATTACK, { following }, CMD.OPT_SHIFT)
		elseif frame == 30 then
			Spring.DestroyUnit(target, false, false)
		elseif frame == 90 then
			assert(destroyedFrame and deletedFrame, "Missing lifecycle notifications")
			SendToUnsynced("deletion_complete")
		end
	end
	function gadget:UnitDestroyed(unitID)
		if unitID == target then
			destroyedFrame = Spring.GetGameFrame()
			assert(Spring.ValidUnitID(target), "UnitDestroyed must precede deletion")
			assert(not deletedFrame)
		end
	end
	function gadget:UnitDeleted(unitID, unitDefID, unitTeam)
		if unitID == target then
			assert(destroyedFrame and not deletedFrame)
			assert(not Spring.ValidUnitID(target), "UnitDeleted ran before removal")
			assert(unitDefID == UnitDefNames.corhurc.id and unitTeam == 1)
			local command = Spring.GetUnitCommands(source, 1)[1]
			assert(
				command.id == CMD.ATTACK and command.params[1] == following,
				"Native death dependences have not advanced the queue"
			)
			deletedFrame = Spring.GetGameFrame()
			assert(deletedFrame >= destroyedFrame)
		end
	end
else
	function gadget:Initialize()
		gadgetHandler:AddSyncAction("deletion_complete", function()
			Spring.Echo("UNIT_DELETED_COMPLETE")
			Spring.SendCommands("quitforce")
		end)
	end
	function gadget:UnitDeleted()
		error("UnitDeleted must not be delivered unsynced")
	end
end
