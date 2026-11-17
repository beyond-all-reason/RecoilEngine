function gadget:GetInfo()
	return { name = "Compact lifecycle parity", layer = -100000, enabled = true }
end

if not gadgetHandler:IsSyncedCode() then
	function gadget:RecvFromSynced(action)
		if action == "compact_validation_complete" then
			Spring.Echo("COMPACT_VALIDATION_COMPLETE")
			Spring.SendCommands("quitforce")
		elseif action == "compact_validation_timeout" then
			Spring.SendCommands("quitforce")
		end
	end
	return
end

local sources, targets, labels = {}, {}, {}
local sawCompact = false
local neutralSources = {}
local projectiles, damageEvents, deaths = 0, 0, 0
local endFrame = validationEndFrame
local targetName = groundFixture and "armllt" or "corhurc"
local sourceName = groundFixture and "armstump" or "armfig"
local function label(unitID)
	return labels[unitID] or tostring(unitID)
end
local function emit(prefix, values)
	Spring.Echo(prefix .. "|" .. table.concat(values, ","))
end
local function issueList(unitID)
	if useCompact then
		Spring.GiveOrderToUnit(unitID, GameCMD.ATTACK_TARGETS, targets, 0)
	else
		for index, targetID in ipairs(targets) do
			Spring.GiveOrderToUnit(unitID, CMD.ATTACK, { targetID }, index == 1 and 0 or CMD.OPT_SHIFT)
		end
	end
	Spring.GiveOrderToUnit(unitID, CMD.MOVE, { 4500, 450, 2100 }, CMD.OPT_SHIFT)
end
local function pendingTarget(unitID)
	local command = Spring.GetUnitCommands(unitID, 2)[2]
	if command.id == GameCMD.ATTACK_TARGETS then
		return GG.GetPendingAttackTarget(unitID, command.params[1])
	end
	assert(command.id == CMD.ATTACK)
	return command.params[1]
end
local function snapshot(frame)
	for sourceIndex, unitID in ipairs(sources) do
		local values = { "frame", frame, sourceIndex, Spring.GetUnitHeading(unitID) }
		for _, value in ipairs({ Spring.GetUnitPosition(unitID) }) do
			values[#values + 1] = string.format("%.9f", value)
		end
		for _, value in ipairs({ Spring.GetUnitVelocity(unitID) }) do
			values[#values + 1] = string.format("%.9f", value)
		end
		local queue = Spring.GetUnitCommands(unitID, -1)
		local raw = { "queue", frame, sourceIndex }
		for _, command in ipairs(queue) do
			raw[#raw + 1] = command.id
			raw[#raw + 1] = command.tag
			raw[#raw + 1] = command.options.coded
			for _, parameter in ipairs(command.params) do raw[#raw + 1] = parameter end
			if command.id == GameCMD.ATTACK_TARGETS then
				assert(useCompact and #command.params == 1 and command.params[1] < 0, "not a compact reference")
				sawCompact = true
			end
		end
		emit("COMPACT_RAW", raw)
		local front = queue[1]
		values[#values + 1] = front and front.id or -1
		if front then
			for _, parameter in ipairs(front.params) do values[#values + 1] = parameter end
		end
		local targetType, userTarget, weaponTarget = Spring.GetUnitWeaponTarget(unitID, 1)
		values[#values + 1] = targetType or -1
		values[#values + 1] = tostring(userTarget)
		if targetType == 1 then values[#values + 1] = weaponTarget end
		emit("COMPACT_BEHAVIOR", values)
	end
end

function gadget:GameFrame(frame)
	if frame > endFrame then
		if frame == endFrame + 50 then SendToUnsynced("compact_validation_timeout") end
		return
	end
	if frame == 1 then
		assert(Spring.RegisterCommand, "missing #3431 dependency")
		assert(Script.GetCallInList()[lifecycleCallin], "missing lifecycle capability")
		assert(GG.GetPendingAttackTarget and GG.GetUnitAttackTargetList, "real #8935 controller missing")
		for _, unitID in ipairs(Spring.GetAllUnits()) do Spring.DestroyUnit(unitID, false, true) end
		for index = 1, 2 do
			local unitID = assert(Spring.CreateUnit(sourceName, 2200, 450, 2000 + (index - 1) * 500, 1, 0))
			sources[index] = unitID
			labels[unitID] = "source" .. index
			Spring.GiveOrderToUnit(unitID, CMD.FIRE_STATE, { 0 }, 0)
			if not groundFixture then Spring.GiveOrderToUnit(unitID, CMD.IDLEMODE, { 0 }, 0) end
			if scenario ~= "combat" then Spring.SetUnitWeaponState(unitID, 1, { reloadState = 100000 }) end
		end
		for index = 1, 3 do
			local unitID = assert(Spring.CreateUnit(targetName, 3100 + (index - 1) * 400, 450, 2200, 1, 1))
			targets[index] = unitID
			labels[unitID] = "target" .. index
			Spring.MoveCtrl.Enable(unitID)
			if scenario == "combat" and groundFixture then
				local targetX = 3100 + (index - 1) * 400
				Spring.MoveCtrl.SetPosition(unitID, targetX, Spring.GetGroundHeight(targetX, 2200), 2200)
			end
			Spring.GiveOrderToUnit(unitID, CMD.FIRE_STATE, { 0 }, 0)
			Spring.SetUnitLosMask(unitID, 0, { los = true, radar = true })
			Spring.SetUnitLosState(unitID, 0, { los = true, radar = true })
		end
		for _, unitID in ipairs(sources) do issueList(unitID) end
	elseif scenario == "combat" then
	elseif frame == 30 then
		if scenario == "neutral" then
			Spring.SetUnitTarget(sources[1], targets[2], false, true)
			Spring.SetUnitTarget(sources[2], targets[1], false, true)
			Spring.SetUnitNeutral(targets[2], true)
			assert(neutralSources[sources[1]] == targets[2], "affected source missed neutralization")
			assert(not neutralSources[sources[2]], "unaffected source received neutralization")
			assert(pendingTarget(sources[1]) == targets[3], "affected pending queue was not pruned")
			assert(pendingTarget(sources[2]) == targets[2], "unaffected pending queue was pruned")
		elseif scenario == "recycle" then
			Spring.DestroyUnit(targets[2], false, true, nil, true)
			local replacement = assert(Spring.CreateUnit(targetName, 7000, 450, 3000, 1, 1))
			assert(replacement == targets[2], "fixture did not recycle the pending ID")
			Spring.MoveCtrl.Enable(replacement)
			labels[replacement] = "replacement"
		else
			Spring.DestroyUnit(targets[scenario == "pending" and 2 or 1], false, false)
		end
	elseif frame == 45 then
		if scenario == "neutral" then
			Spring.SetUnitNeutral(targets[2], false)
		elseif scenario == "recycle" then
			Spring.DestroyUnit(targets[1], false, false)
		else
			Spring.DestroyUnit(targets[3], false, false)
		end
	elseif frame == 60 then
		local targetIndex = scenario == "active" and 2 or scenario == "recycle" and 3 or 1
		Spring.DestroyUnit(targets[targetIndex], false, false)
	elseif frame == 75 and scenario == "neutral" then
		Spring.DestroyUnit(targets[3], false, false)
	elseif frame == 90 and scenario == "neutral" then
		Spring.DestroyUnit(targets[2], false, false)
	end
	if #sources > 0 then snapshot(frame) end
	if frame == endFrame then
		assert(sawCompact == useCompact, "compact mode silently fell back to native")
		if scenario == "combat" then
			assert(projectiles > 0 and damageEvents > 0 and deaths > 0, "combat did not fire, damage and delete a target")
			emit("COMPACT_BEHAVIOR", { "combat", projectiles, damageEvents, deaths })
		else
			for _, unitID in ipairs(sources) do
				for _, command in ipairs(Spring.GetUnitCommands(unitID, -1)) do
					assert(command.id ~= GameCMD.ATTACK_TARGETS and command.id ~= CMD.ATTACK, "stale target survived")
				end
			end
		end
		SendToUnsynced("compact_validation_complete")
	end
end

function gadget:UnitCmdDone(unitID, unitDefID, unitTeam, commandID, commandParams, commandOptions, commandTag)
	if not labels[unitID] or not labels[unitID]:find("source", 1, true) then return end
	local raw = { "done", Spring.GetGameFrame(), label(unitID), commandID, commandTag, commandOptions.coded }
	for _, parameter in ipairs(commandParams) do raw[#raw + 1] = parameter end
	emit("COMPACT_RAW", raw)
	if commandID == CMD.ATTACK or commandID == CMD.MOVE then
		local values = { "done", Spring.GetGameFrame(), label(unitID), commandID }
		for _, parameter in ipairs(commandParams) do values[#values + 1] = parameter end
		emit("COMPACT_BEHAVIOR", values)
	end
end

function gadget:UnitDeleted(unitID)
	if labels[unitID] then
		if labels[unitID]:find("target", 1, true) then deaths = deaths + 1 end
		assert(Spring.ValidUnitID(unitID) == (lifecycleCallin == "UnitPreDelete"))
		emit("COMPACT_BEHAVIOR", { "deleted", Spring.GetGameFrame(), label(unitID) })
	end
end

function gadget:UnitAttackTargetRemoved(unitID, unitDefID, unitTeam, targetID)
	if labels[unitID] then
		neutralSources[unitID] = targetID
		emit("COMPACT_BEHAVIOR", { "neutral", Spring.GetGameFrame(), label(unitID), label(targetID) })
	end
end

function gadget:Initialize()
	if scenario == "combat" then
		for _, weapon in ipairs(UnitDefs[UnitDefNames[sourceName].id].weapons) do
			Script.SetWatchProjectile(weapon.weaponDef, true)
		end
	end
end

function gadget:ProjectileCreated(projectileID, ownerID, weaponDefID)
	if scenario == "combat" and labels[ownerID] then
		projectiles = projectiles + 1
		emit("COMPACT_BEHAVIOR", { "shot", Spring.GetGameFrame(), label(ownerID), weaponDefID, projectileID })
	end
end

function gadget:UnitDamaged(unitID, unitDefID, unitTeam, damage, paralyzer, weaponDefID, projectileID, attackerID)
	if scenario == "combat" and labels[unitID] then
		damageEvents = damageEvents + 1
		emit("COMPACT_BEHAVIOR", { "damage", Spring.GetGameFrame(), label(unitID), string.format("%.9f", damage), weaponDefID or -1, label(attackerID) })
	end
end
