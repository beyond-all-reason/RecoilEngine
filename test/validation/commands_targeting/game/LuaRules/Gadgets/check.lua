-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return { name = "Commands targeting validation", layer = 0, enabled = true }
end
if not gadgetHandler:IsSyncedCode() then return false end

local ATTACK_CMDS = { CMD.ATTACK, CMD.FIGHT, CMD.MANUALFIRE }
local failed, checks = false, 0
local attackers, all, expected = {}, {}, {}
local a, b, c, queued, split, weaponOnly, ground, empty, cmdOnly, auto, autoTarget, nano, nano2, factory
local function check(ok, message)
	checks = checks + 1
	if not ok then
		failed = true
		Spring.Echo("COMMANDS_TARGETING FAIL: " .. message)
	end
end
local function target(u, slot)
	local kind, _, id = Spring.GetUnitWeaponTarget(u, slot or 1)
	return kind == 1 and id or nil
end
local function create(name, x, z, team)
	local y = math.max(0, Spring.GetGroundHeight(x, z)) + 200
	local u = assert(Spring.CreateUnit(name, x, y, z, 0, team))
	all[#all + 1] = u
	-- Strafing-aircraft command AI pauses while MoveCtrl is active.
	if name ~= "fighter" then
		Spring.MoveCtrl.Enable(u)
		Spring.MoveCtrl.SetPosition(u, x, y, z)
	end
	Spring.SetUnitAlwaysVisible(u, true)
	Spring.GiveOrderToUnit(u, CMD.FIRE_STATE, { 0 }, 0)
	Spring.UnitScript.CreateScript(u, {
		QueryWeapon = function() return 1 end,
		AimFromWeapon = function() return 1 end,
		AimWeapon = function(w) Spring.SetUnitWeaponState(u, w, "aimReady", 1) end,
		BlockShot = function() return true end, -- Keep targets alive and queues stable.
	})
	return u
end
local function attack(u, id, shift)
	Spring.GiveOrderToUnit(u, CMD.ATTACK, { id }, shift and { "shift" } or 0)
end
-- For factories this reads the orders passed on to new units, like the API under test.
local function commands(u)
	return Spring.GetUnitCommands(u, -1)
end
local function matches(cmd, ids)
	if #cmd.params ~= 1 or cmd.params[1] ~= a then return false end
	for _, id in ipairs(ids) do
		if cmd.id == id then return true end
	end
	return false
end
-- Remember which commands must survive and how many the clearing call must remove.
local function remember(u)
	local keep, removed = {}, 0
	for _, cmd in ipairs(commands(u)) do
		if matches(cmd, ATTACK_CMDS) then
			removed = removed + 1
		else
			keep[#keep + 1] = cmd.tag
		end
	end
	expected[u] = keep
	return removed
end
local function checkQueues()
	for u, tags in pairs(expected) do
		local queue = commands(u)
		check(#queue == #tags, "remaining queue length for " .. u)
		for i, tag in ipairs(tags) do
			check(queue[i] and queue[i].tag == tag, "remaining command order/tag for " .. u)
		end
	end
end
-- Expected getter result for one unit, derived from its visible queue.
local function expectedTags(u, ids, includeQueued)
	local tags = {}
	if u == factory and not includeQueued then return tags end
	for i, cmd in ipairs(commands(u)) do
		if (includeQueued or i == 1) and matches(cmd, ids) then tags[#tags + 1] = cmd.tag end
	end
	return tags
end
local function checkGetter(result, ids, includeQueued, readable, label)
	local units, expectedUnits = 0, 0
	for _ in pairs(result) do units = units + 1 end
	for _, u in ipairs(all) do
		local tags = readable(u) and expectedTags(u, ids, includeQueued) or {}
		local got = result[u] or {}
		if #tags > 0 then expectedUnits = expectedUnits + 1 end
		local same = #tags == #got
		for i = 1, #tags do same = same and tags[i] == got[i] end
		check(same, label .. ": tags for unit " .. u)
	end
	check(units == expectedUnits, label .. ": no unexpected units")
end
local function everyone() return true end
local function team(t) return function(u) return Spring.GetUnitTeam(u) == t end end

function gadget:Initialize()
	check(type(Spring.ClearCommandsTargeting) == "function", "missing ClearCommandsTargeting API")
	check(type(Spring.ClearTargetingFromUnit) == "function", "missing ClearTargetingFromUnit API")
	check(type(Spring.GetCommandsTargeting) == "function", "missing GetCommandsTargeting API")
end

function gadget:GameFrame(frame)
	if failed then SendToUnsynced("commands_targeting_done"); return end
	local x, z = Game.mapSizeX / 2, Game.mapSizeZ / 2
	if frame == 1 then
		a = create("target", x + 300, z, 1)
		b = create("target", x + 300, z + 100, 1)
		c = create("target", x + 300, z + 200, 1)
		for i, name in ipairs({ "static", "mobile", "fighter" }) do
			attackers[i] = create(name, x, z + (i - 1) * 100, i == 3 and 2 or 0)
		end
		queued = create("static", x - 50, z, 0)
		split = create("static", x - 100, z, 0)
		weaponOnly = create("static", x - 150, z, 2)
		ground = create("static", x - 200, z, 0)
		empty = create("static", x - 250, z, 0)
		cmdOnly = create("static", x - 300, z, 0)
		factory = create("factory", x - 400, z, 0)
		nano = create("nano", x + 300, z - 200, 1)
		nano2 = create("nano", x + 300, z - 300, 1)
		-- Out of range of B/C: automatic acquisition can only choose this target.
		auto = create("static", 200, 200, 0)
		autoTarget = create("target", 500, 200, 1)
		Spring.GiveOrderToUnit(auto, CMD.FIRE_STATE, { 2 }, 0)
		Spring.SetUnitHealth(a, 50000) -- Keeps the repair order busy.
	elseif frame == 32 then
		for _, u in ipairs(attackers) do
			attack(u, a); attack(u, b, true); attack(u, c, true)
			-- INSERT preserves a duplicate rather than toggling the earlier order.
			Spring.GiveOrderToUnit(u, CMD.INSERT, { -1, CMD.ATTACK, 0, a }, { "alt" })
		end
		attack(queued, b); attack(queued, a, true)
		Spring.GiveOrderToUnit(queued, CMD.MANUALFIRE, { a }, { "shift" })
		Spring.GiveOrderToUnit(queued, CMD.FIGHT, { a }, { "shift" })
		attack(split, b)
		Spring.GiveOrderToUnit(ground, CMD.ATTACK, { x + 300, 300, z }, 0)
		attack(ground, a, true)
		attack(empty, a)
		attack(cmdOnly, a)
		-- Orders for the units the factory will build (factories cannot take ATTACK themselves).
		Spring.GiveOrderToUnit(factory, CMD.FIGHT, { a }, 0)
		Spring.GiveOrderToUnit(factory, CMD.FIGHT, { b }, { "shift" })
		Spring.GiveOrderToUnit(nano, CMD.REPAIR, { a }, 0)
		Spring.GiveOrderToUnit(nano2, CMD.REPAIR, { a }, 0)
	elseif frame == 64 then
		check(Script.GetFullCtrl(), "synced LuaRules has full control")
		check(Spring.ClearCommandsTargeting(-1, ATTACK_CMDS) == nil, "invalid ID rejected by ClearCommandsTargeting")
		check(Spring.ClearTargetingFromUnit(-1) == nil, "invalid ID rejected by ClearTargetingFromUnit")
		check(Spring.GetCommandsTargeting(-1, ATTACK_CMDS) == nil, "invalid ID rejected by GetCommandsTargeting")
		check(not pcall(Spring.ClearCommandsTargeting, a, "attack"), "bad command list rejected")
		check(not pcall(Spring.GetCommandsTargeting, a), "missing command list rejected")
		for _, u in ipairs(attackers) do check(target(u) == a, "active attack on A before clearing") end
		check(target(queued) == b, "queued-only attacker still targets B")
		check(target(cmdOnly) == a, "command-only attacker targets A")
		local queueBefore = commands(queued)
		check(#queueBefore == 4, "queued ATTACK/MANUALFIRE/FIGHT fixture")
		check(#commands(factory) == 2 and #Spring.GetFactoryCommands(factory, -1) == 0, "factory fixture")
		local task, taskTarget = Spring.GetUnitWorkerTask(nano)
		check(task == CMD.REPAIR and taskTarget == a, "repair fixture")
		for _, t in ipairs({ 0, 1, Script.NO_ACCESS_TEAM }) do
			CallAsTeam({ ctrl = t }, function()
				check(not Script.GetFullCtrl(), "restricted context lacks full control")
				check(Spring.ClearCommandsTargeting(a, ATTACK_CMDS) == nil, "restricted ClearCommandsTargeting rejected")
				check(Spring.ClearTargetingFromUnit(a) == nil, "restricted ClearTargetingFromUnit rejected")
			end)
			check(target(attackers[1]) == a and #commands(queued) == #queueBefore, "denied call has no effects")
		end
		-- Getter: active orders only, then all queued orders, then restricted reads.
		local active = Spring.GetCommandsTargeting(a, CMD.ATTACK)
		checkGetter(active, { CMD.ATTACK }, false, everyone, "active attacks")
		check(active[attackers[1]] and #active[attackers[1]] == 1 and active[queued] == nil and active[factory] == nil, "active attack fixture")
		local whole = Spring.GetCommandsTargeting(a, ATTACK_CMDS, true)
		checkGetter(whole, ATTACK_CMDS, true, everyone, "queued attacks")
		check(whole[attackers[1]] and #whole[attackers[1]] == 2 and whole[queued] and #whole[queued] == 3, "queued attack fixture")
		check(whole[factory] and #whole[factory] == 1 and whole[nano] == nil, "factory orders reported, repair not")
		check(#Spring.GetCommandsTargeting(a, CMD.REPAIR, true)[nano] == 1, "repair order reported")
		for _, t in ipairs({ 0, 1, 2 }) do
			CallAsTeam({ read = t }, function()
				checkGetter(Spring.GetCommandsTargeting(a, ATTACK_CMDS, true), ATTACK_CMDS, true, team(t), "read as team " .. t)
			end)
		end
		Spring.SetUnitTarget(split, a, false, true, 1)
		Spring.SetUnitTarget(weaponOnly, a, false, true, 1)
		check(target(split, 1) == a and target(split, 2) == b, "independent weapon targets fixture")
		check(target(weaponOnly) == a and #commands(weaponOnly) == 0, "weapon-only target fixture")
		check(target(auto) == autoTarget, "automatic target acquired")
		local removed = 0
		for _, u in ipairs(all) do removed = removed + remember(u) end
		local neutral = Spring.GetUnitNeutral(a)
		-- Unit scripts retain full control in CallAsUnit, just like their LuaRules owner.
		Spring.UnitScript.CallAsUnit(a, function()
			check(Script.GetFullCtrl(), "unit script context retains full control")
			check(Spring.ClearCommandsTargeting(a, {}) == 0, "empty command list removes nothing")
			check(Spring.ClearTargetingFromUnit(autoTarget) == true and target(auto) == nil, "automatic target cleared")
		end)
		check(Spring.ClearCommandsTargeting(a, ATTACK_CMDS) == removed, "removed command count")
		checkQueues()
		check(next(Spring.GetCommandsTargeting(a, ATTACK_CMDS, true)) == nil, "no matching commands remain")
		-- Finishing an attack order releases its own target like a completed order would,
		-- but targets without a matching order are the other function's job.
		check(target(cmdOnly) == nil, "finished attack order released its target")
		check(target(split, 1) == a and target(weaponOnly) == a, "independent targets survive command removal")
		check(Spring.ClearTargetingFromUnit(a) == true, "targets cleared")
		check(Spring.GetUnitNeutral(a) == neutral, "neutral state unchanged")
		for _, u in ipairs(all) do
			for w = 1, #UnitDefs[Spring.GetUnitDefID(u)].weapons do
				check(target(u, w) ~= a, "no weapon still targets A")
			end
		end
		check(target(split, 2) == b, "other weapon target preserved")
		check(Spring.GetUnitWeaponTarget(ground, 1) == 2, "ground target preserved")
		check(select(2, Spring.GetUnitWorkerTask(nano)) == a, "repair survives attack clearing")
		-- Reference behaviour: the same removal through CMD.REMOVE.
		Spring.GiveOrderToUnit(nano2, CMD.REMOVE, { commands(nano2)[1].tag }, 0)
		expected[nano2] = {}
		check(#commands(nano2) == 0 and Spring.GetUnitWorkerTask(nano2) == nil, "CMD.REMOVE reference stops repairing")
		check(Spring.ClearCommandsTargeting(a, CMD.REPAIR) == 1, "single command ID removes the repair order")
		check(#commands(nano) == 0 and Spring.GetUnitWorkerTask(nano) == nil, "repair order removed and builder stopped")
		expected[nano] = {}
		check(Spring.ClearCommandsTargeting(a, ATTACK_CMDS) == 0, "repeated call removes nothing")
		checkQueues()
	elseif frame == 96 then
		for _, u in ipairs(attackers) do check(target(u) == b, "next attack B initialized") end
		check(#commands(empty) == 0 and target(empty) == nil, "sole attack cleared")
		check(#commands(cmdOnly) == 0 and target(cmdOnly) == nil, "command-only attacker idle")
		check(#commands(factory) == 1 and commands(factory)[1].params[1] == b, "factory keeps its other order")
		check(target(auto) == autoTarget, "automatic reacquisition remains possible")
		check(Spring.GetUnitWorkerTask(nano) == nil and Spring.GetUnitWorkerTask(nano2) == nil, "builders stay idle")
		Spring.DestroyUnit(a, false, true)
	elseif frame == 128 then
		for _, u in ipairs(attackers) do check(target(u) == b, "A's deletion does not skip B") end
		attack(weaponOnly, b)
	elseif frame == 160 then
		check(target(weaponOnly) == b, "cleared attacker accepts new orders")
		if not failed then Spring.Echo("COMMANDS_TARGETING PASS checks=" .. checks) end
		SendToUnsynced("commands_targeting_done")
	end
end
