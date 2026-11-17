-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
function gadget:GetInfo()
	return { name = "Unit deletion validation", layer = 100000, enabled = true }
end
if gadgetHandler:IsSyncedCode() then
	local targetName = groundFixture and "armllt" or "corhurc"
	local source, target, following, destroyedFrame, deletedFrame
	local notified, neutralSource, unaffectedSource, neutralTarget, otherTarget = {}, nil, nil, nil, nil
	local staleListenerID, reentrantTarget, reentrantTrigger
	local reentrantTargetDeleted, reentrantTriggerDeleted = false, false
	local finishedCommands = 0
	local lifecycleCounts, virtualTargets = {}, {}
	local replacement
	local function CheckLifecycleID(unitID, unitDefID, unitTeam)
		lifecycleCounts[unitID] = (lifecycleCounts[unitID] or 0) + 1
		assert(lifecycleCounts[unitID] == 1, "duplicate lifecycle notification")
		assert(Spring.ValidUnitID(unitID) == preDelete, "wrong validity at lifecycle boundary")
		if preDelete then
			assert(Spring.GetUnitDefID(unitID) == unitDefID)
			assert(Spring.GetUnitTeam(unitID) == unitTeam)
			assert(Spring.GetUnitPosition(unitID))
			assert(Spring.GetUnitIsDead(unitID))
			Spring.DestroyUnit(unitID, false, true, nil, true)
			assert(Spring.ValidUnitID(unitID), "recursive self-deletion invalidated the callback argument")
		end
	end
	function gadget:GameFrame(frame)
		if frame == 100 then SendToUnsynced("validation_timeout") end
		if frame == 1 then
			for _, id in ipairs(Spring.GetAllUnits()) do
				Spring.DestroyUnit(id, false, true)
			end
			source = assert(Spring.CreateUnit(groundFixture and "armstump" or "armfig", 2200, 450, 2000, 1, 0))
			target = assert(Spring.CreateUnit(targetName, 3100, 450, 2200, 1, 1))
			following = assert(Spring.CreateUnit(targetName, 3500, 450, 2400, 1, 1))
			reentrantTarget = assert(Spring.CreateUnit(targetName, 3800, 450, 2600, 1, 1))
			reentrantTrigger = assert(Spring.CreateUnit(targetName, 3900, 450, 2800, 1, 1))
			Spring.MoveCtrl.Enable(target)
			Spring.MoveCtrl.Enable(following)
			Spring.MoveCtrl.Enable(reentrantTarget)
			Spring.MoveCtrl.Enable(reentrantTrigger)
			Spring.GiveOrderToUnit(source, CMD.FIRE_STATE, { 0 }, 0)
			Spring.GiveOrderToUnit(source, CMD.ATTACK, { target }, 0)
			Spring.GiveOrderToUnit(source, CMD.ATTACK, { following }, CMD.OPT_SHIFT)
			virtualTargets = { target, following }
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
			assert(not Spring.ValidUnitID(reentrantTarget))
			assert(not Spring.ValidUnitID(reentrantTrigger) or replacement == reentrantTrigger)
			assert(not Spring.ValidUnitID(target))
			assert(#virtualTargets == 1 and virtualTargets[1] == following)
			assert(replacement and Spring.ValidUnitID(replacement))
			SendToUnsynced("deletion_complete")
		end
		if source and Spring.ValidUnitID(source) then
			local position = { Spring.GetUnitPosition(source) }
			local velocity = { Spring.GetUnitVelocity(source) }
			local values = { frame, Spring.GetUnitHeading(source) }
			for _, value in ipairs(position) do values[#values + 1] = string.format("%.9f", value) end
			for _, value in ipairs(velocity) do values[#values + 1] = string.format("%.9f", value) end
			for _, command in ipairs(Spring.GetUnitCommands(source, -1)) do
				values[#values + 1] = command.id
				values[#values + 1] = command.tag
				for _, parameter in ipairs(command.params) do values[#values + 1] = parameter end
			end
			Spring.Echo("LIFECYCLE_TRACE|" .. table.concat(values, ","))
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
	function gadget:UnitCommand(unitID, unitDefID, unitTeam, commandID, commandParams, commandOptions, commandTag)
		if unitID == source then
			Spring.Echo("LIFECYCLE_DIAGNOSTIC|order," .. Spring.GetGameFrame() .. "," .. commandID .. "," .. commandTag .. "," .. commandOptions.coded .. "," .. table.concat(commandParams, ","))
		end
	end
	function gadget:UnitCmdDone(unitID, unitDefID, unitTeam, commandID, commandParams, commandOptions, commandTag)
		if unitID == source then
			Spring.Echo("LIFECYCLE_TRACE|done," .. Spring.GetGameFrame() .. "," .. commandID)
			Spring.Echo("LIFECYCLE_DIAGNOSTIC|done," .. Spring.GetGameFrame() .. "," .. commandID .. "," .. commandTag .. "," .. commandOptions.coded .. "," .. table.concat(commandParams, ","))
		end
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
		CheckLifecycleID(unitID, unitDefID, unitTeam)
		if unitID == target then
			assert(destroyedFrame and not deletedFrame)
			assert(unitDefID == UnitDefNames[targetName].id and unitTeam == 1)
			local command = Spring.GetUnitCommands(source, 1)[1]
			assert(
				command.id == CMD.ATTACK and command.params[1] == following,
				"Native death dependences have not advanced the queue"
			)
			deletedFrame = Spring.GetGameFrame()
			assert(deletedFrame >= destroyedFrame)
			table.remove(virtualTargets, 1)
			Spring.Echo("LIFECYCLE_TRACE|removed," .. deletedFrame)
			if preDelete then
				assert(Spring.GetUnitWeaponTestTarget(source, 1, target) == false, "finalizing unit remained a valid weapon target")
				for _, commandID in ipairs({ CMD.ATTACK, CMD.GUARD, CMD.RECLAIM, CMD.REPAIR, CMD.CAPTURE, CMD.LOAD_UNITS }) do
					Spring.GiveOrderToUnit(source, commandID, { target }, CMD.OPT_SHIFT)
					Spring.GiveOrderToUnit(source, CMD.INSERT, { 0, commandID, CMD.OPT_SHIFT, target }, CMD.OPT_ALT)
				end
				assert(#Spring.GetUnitCommands(source, -1) == 1, "new target commands were accepted during finalization")
				assert(not Spring.SetUnitTarget(source, target, false, true), "new attack target accepted")
				assert(not Spring.SetUnitTarget(source, target, false, true, 1), "new weapon target accepted")
				local oldQueueSize = #Spring.GetUnitCommands(target, -1)
				assert(not Spring.GiveOrderToUnit(target, CMD.MOVE, { 5000, 450, 5000 }, CMD.OPT_SHIFT))
				assert(not Spring.GiveOrderArrayToUnit(target, { { CMD.MOVE, { 5000, 450, 5000 }, CMD.OPT_SHIFT } }))
				assert(#Spring.GetUnitCommands(target, -1) == oldQueueSize, "finalizing unit received orders")
			end
		elseif unitID == reentrantTarget then
			assert(Spring.ValidUnitID(reentrantTrigger), "nested deletion trigger was already invalid")
			assert(unitDefID == UnitDefNames[targetName].id and unitTeam == 1)
			Spring.DestroyUnit(reentrantTrigger, false, true, nil, true)
			assert(Spring.ValidUnitID(reentrantTarget) == preDelete, "nested deletion invalidated the outer unit")
			replacement = assert(Spring.CreateUnit(targetName, 3900, 450, 2800, 1, 1))
			assert(replacement ~= reentrantTarget, "outer ID was reused during its callback")
			Spring.MoveCtrl.Enable(replacement)
			reentrantTargetDeleted = true
		elseif unitID == reentrantTrigger then
			assert(unitDefID == UnitDefNames[targetName].id and unitTeam == 1)
			reentrantTriggerDeleted = true
		end
	end
else
	function gadget:UnitAttackTargetRemoved()
		error("UnitAttackTargetRemoved must not be delivered unsynced")
	end
	function gadget:Initialize()
		gadgetHandler:AddSyncAction("validation_timeout", function()
			Spring.SendCommands("quitforce")
		end)
		gadgetHandler:AddSyncAction("deletion_complete", function()
			Spring.Echo("UNIT_DELETED_COMPLETE")
			Spring.SendCommands("quitforce")
		end)
	end
	function gadget:UnitDeleted()
		error("UnitDeleted must not be delivered unsynced")
	end
end
