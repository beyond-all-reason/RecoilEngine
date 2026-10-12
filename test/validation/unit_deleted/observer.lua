-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
function gadget:GetInfo()
	return { name = "Unit deletion validation", layer = 100000, enabled = true }
end
if gadgetHandler:IsSyncedCode() then
	local source, target, following, destroyedFrame, deletedFrame
	local notified, neutralSource, unaffectedSource, neutralTarget, otherTarget = {}, nil, nil, nil, nil
	local staleListenerID, reentrantTarget, reentrantTrigger
	local reentrantTargetDeleted, reentrantTriggerDeleted = false, false
	local finishedCommands = 0
	function gadget:GameFrame(frame)
		if frame == 1 then
			for _, id in ipairs(Spring.GetAllUnits()) do
				Spring.DestroyUnit(id, false, true)
			end
			source = assert(Spring.CreateUnit("armfig", 2200, 450, 2000, 1, 0))
			target = assert(Spring.CreateUnit("corhurc", 3100, 450, 2200, 1, 1))
			following = assert(Spring.CreateUnit("corhurc", 3500, 450, 2400, 1, 1))
			reentrantTarget = assert(Spring.CreateUnit("corhurc", 3800, 450, 2600, 1, 1))
			reentrantTrigger = assert(Spring.CreateUnit("corhurc", 3900, 450, 2800, 1, 1))
			Spring.MoveCtrl.Enable(target)
			Spring.MoveCtrl.Enable(following)
			Spring.MoveCtrl.Enable(reentrantTarget)
			Spring.MoveCtrl.Enable(reentrantTrigger)
			Spring.GiveOrderToUnit(source, CMD.FIRE_STATE, { 0 }, 0)
			Spring.GiveOrderToUnit(source, CMD.ATTACK, { target }, 0)
			Spring.GiveOrderToUnit(source, CMD.ATTACK, { following }, CMD.OPT_SHIFT)
		elseif frame == 10 then
			neutralSource = assert(Spring.CreateUnit("armfig", 2200, 450, 3000, 1, 0))
			staleListenerID = assert(Spring.CreateUnit("armfig", 2200, 450, 3100, 1, 0))
			unaffectedSource = assert(Spring.CreateUnit("armfig", 2200, 450, 3200, 1, 0))
			neutralTarget = assert(Spring.CreateUnit("corhurc", 3100, 450, 3000, 1, 1))
			otherTarget = assert(Spring.CreateUnit("corhurc", 3100, 450, 3300, 1, 1))
			for _, id in ipairs({neutralSource, staleListenerID, unaffectedSource, neutralTarget, otherTarget}) do
				Spring.MoveCtrl.Enable(id)
				Spring.MoveCtrl.SetPosition(id, 2400 + id % 400, 700, 3000)
				Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, { 0 }, 0)
			end
			for _, id in ipairs({neutralSource, staleListenerID, unaffectedSource}) do
				Spring.GiveOrderToUnit(id, CMD.ATTACK, { otherTarget }, 0)
				Spring.GiveOrderToUnit(id, CMD.ATTACK, { neutralTarget }, CMD.OPT_SHIFT)
			end
			Spring.SetUnitTarget(neutralSource, neutralTarget, false, true)
			Spring.SetUnitTarget(staleListenerID, neutralTarget, false, true)
			Spring.SetUnitTarget(unaffectedSource, otherTarget, false, true)
			Spring.SetUnitNeutral(neutralTarget, true)
			assert(notified[neutralSource] == 1 and not notified[unaffectedSource], "wrong affected-unit set")
			assert(not Spring.ValidUnitID(staleListenerID), "callback-deleted listener was not removed")
			assert(not notified[staleListenerID], "deleted listener was notified")
			local retained = Spring.GetUnitCommands(unaffectedSource, -1)
			assert(#retained == 2 and retained[2].params[1] == neutralTarget, "unaffected queue was pruned")
			Spring.SetUnitNeutral(neutralTarget, true)
			assert(notified[neutralSource] == 1, "repeated neutralization without a current target notified twice")
			Spring.SetUnitNeutral(neutralTarget, false)
			-- A virtual queue must also receive the event without a matching native order.
			-- Returning its reference tag must not finish it or execute its successor.
			Spring.GiveOrderToUnit(neutralSource, CMD.MOVE, { 5000, 700, 5000 }, CMD.OPT_SHIFT)
			finishedCommands = 0
			Spring.SetUnitTarget(neutralSource, neutralTarget, false, true)
			Spring.SetUnitNeutral(neutralTarget, true)
			assert(notified[neutralSource] == 2, "virtual pending target missed notification")
			local successor = Spring.GetUnitCommands(neutralSource, -1)
			assert(#successor == 1 and successor[1].id == CMD.MOVE, "returned tag was not erased")
			assert(finishedCommands == 0, "tag erasure invoked FinishCommand")
		elseif frame == 30 then
			Spring.DestroyUnit(target, false, false)
		elseif frame == 50 then
			Spring.DestroyUnit(reentrantTarget, false, true, nil, true)
		elseif frame == 90 then
			assert(destroyedFrame and deletedFrame, "Missing lifecycle notifications")
			assert(reentrantTargetDeleted and reentrantTriggerDeleted, "Missing nested deletion notifications")
			assert(not Spring.ValidUnitID(reentrantTarget) and not Spring.ValidUnitID(reentrantTrigger))
			SendToUnsynced("deletion_complete")
		end
	end
	function gadget:UnitAttackTargetRemoved(unitID, unitDefID, unitTeam, targetID)
		assert((unitID == neutralSource or unitID == staleListenerID) and targetID == neutralTarget, "unexpected target-removal notification")
		if unitID == neutralSource and Spring.ValidUnitID(staleListenerID) then
			Spring.DestroyUnit(staleListenerID, false, true, nil, true)
		end
		assert(unitDefID == UnitDefNames.armfig.id and unitTeam == 0)
		local queue = Spring.GetUnitCommands(unitID, -1)
		assert(queue[1].params[1] == otherTarget, "notification did not follow native queue cleanup")
		notified[unitID] = (notified[unitID] or 0) + 1
		if notified[unitID] == 2 then
			return { queue[1].tag, queue[1].tag, -1 }
		end
	end
	function gadget:UnitCmdDone(unitID)
		if unitID == neutralSource then
			finishedCommands = finishedCommands + 1
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
		elseif unitID == reentrantTarget then
			assert(not Spring.ValidUnitID(reentrantTarget), "UnitDeleted ran before removal")
			assert(Spring.ValidUnitID(reentrantTrigger), "nested deletion trigger was already invalid")
			assert(unitDefID == UnitDefNames.corhurc.id and unitTeam == 1)
			Spring.DestroyUnit(reentrantTrigger, false, true, nil, true)
			reentrantTargetDeleted = true
		elseif unitID == reentrantTrigger then
			assert(not Spring.ValidUnitID(reentrantTrigger), "nested UnitDeleted ran before removal")
			assert(unitDefID == UnitDefNames.corhurc.id and unitTeam == 1)
			reentrantTriggerDeleted = true
		end
	end
else
	function gadget:UnitAttackTargetRemoved()
		error("UnitAttackTargetRemoved must not be delivered unsynced")
	end
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
