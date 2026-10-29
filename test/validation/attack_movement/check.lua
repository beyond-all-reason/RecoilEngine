-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return {
		name = "Attack movement validation",
		desc = "Opt-in integration checks",
		author = "OpenAI Codex",
		layer = -1000000,
		enabled = true,
	}
end
local options = Spring.GetModOptions()
local mode = options.attackmovementvalidation
if not mode then
	return false
end
if not gadgetHandler:IsSyncedCode() then
	function gadget:RecvFromSynced(name)
		if name == "attack_validation_speed" then
			Spring.SendCommands("setspeed 1000")
		end
		if name == "attack_validation_quit" then
			Spring.SendCommands("quitforce")
		end
	end
	return
end
local endFrame = tonumber(options.attackmovementendframe) or 36000
local cases, created, features = {}, {}, {}
local active, shooter, target, observed
local blockerIDs, mobileBlocker, staticBlocker
local completed = 0
local validReasons = {
	clear = true,
	notChecked = true,
	invalidTarget = true,
	range = true,
	terrain = true,
	friendly = true,
	neutral = true,
	feature = true,
	blocked = true,
}
local function terrain(height, x1, z1, x2, z2)
	Spring.SetHeightMapFunc(function()
		for x = x1, x2, 8 do
			for z = z1, z2, 8 do
				Spring.SetHeightMap(x, z, height)
			end
		end
	end)
end
local function create(name, x, z, team)
	local id = assert(Spring.CreateUnit(name, x, Spring.GetGroundHeight(x, z), z, 0, team), name)
	created[#created + 1] = id
	Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, { 0 }, {})
	Spring.GiveOrderToUnit(id, CMD.MOVE_STATE, { 0 }, {})
	for w = 1, #UnitDefs[Spring.GetUnitDefID(id)].weapons do
		Spring.SetUnitWeaponState(id, w, "reloadState", 1000000)
	end
	return id
end
local function startCase(case)
	for _, id in ipairs(created) do
		if Spring.ValidUnitID(id) then
			Spring.DestroyUnit(id, false, true)
		end
	end
	for _, id in ipairs(features) do
		Spring.DestroyFeature(id)
	end
	created, features = {}, {}
	blockerIDs, mobileBlocker, staticBlocker = {}, nil, nil
	terrain(200, 1536, 1536, 3072, 3072)
	active, observed = case, false
	shooter = create(case.weapon, 2048, 2048, 0)
	target = create("armbanth", 2048, 2248, 1)
	if case.blocker == "terrain" then
		terrain(1000, 1920, 2104, 2176, 2184)
	elseif case.blocker == "static" or case.blocker == "mobileStatic" or case.blocker == "staticMobile" then
		local names = case.blocker == "static" and { "armfus" }
			or case.blocker == "mobileStatic" and { "armbanth", "armfus" }
			or { "armfus", "armbanth" }
		for i, name in ipairs(names) do
			local offset = #names == 1 and 0 or (i == 1 and -20 or 20)
			local id = create(name, 2048, (case.weapon == "armmerl" and 2048 or 2148) + offset, 0)
			Spring.SetUnitCollisionVolumeData(id, 200, 1200, 200, 0, 500, 0, 2, 1, 1)
			blockerIDs[id] = true
			if name == "armfus" then
				staticBlocker = id
			else
				mobileBlocker = id
			end
		end
	elseif case.blocker == "friendly" or case.blocker == "neutral" or case.blocker == "mixed" then
		local blocker = create("armbanth", 2048, case.weapon == "armmerl" and 2048 or 2148, 0)
		blockerIDs[blocker] = true
		mobileBlocker = blocker
		Spring.SetUnitCollisionVolumeData(blocker, 200, 1200, 200, 0, 500, 0, 2, 1, 1)
		if case.blocker == "mixed" then
			terrain(1000, 1920, 2104, 2176, 2184)
			Spring.MoveCtrl.Enable(blocker)
			Spring.MoveCtrl.SetPosition(blocker, 2048, 200, 2148)
		end
		if case.blocker == "neutral" then
			Spring.TransferUnit(blocker, 1, false)
			Spring.SetUnitNeutral(blocker, true)
		end
	elseif case.blocker == "feature" then
		local id =
			assert(Spring.CreateFeature("armbanth_dead", 2048, 200, case.weapon == "armmerl" and 2048 or 2148, 0, 0))
		features[#features + 1] = id
		blockerIDs[id] = true
		Spring.SetFeatureCollisionVolumeData(id, 200, 1200, 200, 0, 500, 0, 2, 1, 1)
	end
end
function gadget:Initialize()
	assert(Spring.TestUnitAttackWeapon and Spring.GetUnitAttackTargetPosition)
	if mode ~= "blockers" then
		gadgetHandler:RemoveCallIn("AttackCommandMovement")
		return
	end
	Spring.SetGlobalLos(0, true)
	Spring.SetGlobalLos(1, true)
	for _, weapon in ipairs({ "corak", "cormort", "corstorm", "corban", "armmerl" }) do
		for _, blocker in ipairs({
			"clear",
			"friendly",
			"neutral",
			"feature",
			"terrain",
			"mixed",
			"static",
			"mobileStatic",
			"staticMobile",
		}) do
			if not (weapon == "armmerl" and (blocker == "terrain" or blocker == "mixed")) then
				for _, ground in ipairs({ false, true }) do
					cases[#cases + 1] = { weapon = weapon, blocker = blocker, ground = ground }
				end
			end
		end
	end
end
local function query(unitID, orientation, avoidFlags, position, heading, useMuzzle)
	local clear, reason, kind, id = Spring.TestUnitAttackWeapon(unitID, 1, {
		orientation = orientation,
		avoidFlags = avoidFlags,
		position = position,
		heading = heading,
		useMuzzle = useMuzzle,
	})
	return { clear = clear, reason = reason, kind = kind, id = id }
end
local function same(a, b, message)
	assert(a.clear == b.clear and a.reason == b.reason and a.kind == b.kind and a.id == b.id, message)
end
function gadget:AttackCommandMovement(unitID, _, _, _, targetID)
	if unitID ~= shooter then
		return false
	end
	Spring.ClearUnitGoal(unitID, false)
	assert((targetID ~= nil) == not active.ground)
	local flags = Game.collisionFlags
	local friendlyMask = flags.noGround + flags.noNeutrals + flags.noFeatures + flags.noCloaked
	local terrainMask = flags.noUnits + flags.noFeatures + flags.noCloaked
	local storedFlags = Spring.GetUnitWeaponState(unitID, 1, "avoidFlags")
	local selected = active.ground and "heading" or "rotate"
	local headingResult
	for _, orientation in ipairs({ "rotate", "heading" }) do
		local native = query(unitID, orientation)
		Spring.SetUnitWeaponState(unitID, 1, "avoidFlags", storedFlags + 768)
		same(native, query(unitID, orientation), "stored high bits enabled query-only filtering")
		Spring.SetUnitWeaponState(unitID, 1, "avoidFlags", storedFlags)
		local all = query(unitID, orientation, 0)
		local friendly = query(unitID, orientation, friendlyMask)
		local terrainResult = query(unitID, orientation, terrainMask)
		local static = query(unitID, orientation, friendlyMask + flags.noMobileFriendlies)
		local mobile = query(unitID, orientation, friendlyMask + flags.noStaticFriendlies)
		local neither = query(unitID, orientation, friendlyMask + flags.noMobileFriendlies + flags.noStaticFriendlies)
		if orientation == selected then
			local friendlyExpected = (active.blocker ~= "neutral" and (mobileBlocker or staticBlocker)) and "friendly"
				or "clear"
			local terrainExpected = (active.blocker == "terrain" or active.blocker == "mixed") and "terrain" or "clear"
			assert(friendly.reason == friendlyExpected, "friendly-only: " .. friendly.reason)
			assert(terrainResult.reason == terrainExpected, "terrain-only: " .. terrainResult.reason)
			assert(static.reason == (staticBlocker and "friendly" or "clear"), "static-only filter")
			assert(
				mobile.reason == ((mobileBlocker and active.blocker ~= "neutral") and "friendly" or "clear"),
				"mobile-only filter"
			)
			assert(neither.reason == "clear", "both friendly subtypes excluded")
			if staticBlocker then
				assert(static.id == staticBlocker)
			end
			if mobileBlocker and active.blocker ~= "neutral" then
				assert(mobile.id == mobileBlocker)
			end
			local expected = staticBlocker and "friendly" or (active.blocker == "mixed" and "terrain" or active.blocker)
			if all.reason == expected then
				observed = true
			end
		end
		for _, result in ipairs({ native, all, friendly, terrainResult, static, mobile, neither }) do
			assert(validReasons[result.reason] and result.clear == (result.reason == "clear"))
			if result.reason == "friendly" or result.reason == "neutral" or result.reason == "feature" then
				assert(
					result.kind == (result.reason == "feature" and "feature" or "unit") and blockerIDs[result.id],
					"wrong blocker identity"
				)
			else
				assert(result.kind == nil and result.id == nil, "stale blocker identity")
			end
		end
		same(native, query(unitID, orientation), "filter leaked into default query")
		if orientation == "heading" then
			headingResult = all
		end
	end
	assert(storedFlags == Spring.GetUnitWeaponState(unitID, 1, "avoidFlags"))
	local x, y, z = Spring.GetUnitPosition(unitID)
	local unitHeading = Spring.GetUnitHeading(unitID)
	local vectors = { Spring.GetUnitWeaponVectors(unitID, 1) }
	same(query(unitID, "heading", 0, { x, y, z }, 0), headingResult, "candidate disagrees with current heading query")
	if active.blocker == "clear" and (active.weapon == "corak" or active.weapon == "cormort") then
		local below = query(unitID, "heading", 0, { x, y - 80, z }, 0)
		local filtered = query(unitID, "heading", flags.noGround + terrainMask, { x, y - 80, z }, 0)
		assert(below.reason == "terrain" and below.kind == nil and below.id == nil)
		assert(filtered.clear, "noGround must exclude underground source")
	end
	local sidestep = active.blocker == "friendly" or active.blocker == "static" or active.blocker == "feature"
	local setVolume = active.blocker == "feature" and Spring.SetFeatureCollisionVolumeData
		or Spring.SetUnitCollisionVolumeData
	if sidestep then
		for id in pairs(blockerIDs) do
			setVolume(
				id,
				active.weapon == "armmerl" and 200 or 40,
				1200,
				active.weapon == "armmerl" and 200 or 20,
				0,
				500,
				0,
				2,
				1,
				1
			)
		end
		local blocked = query(unitID, "heading", 0, { x, y, z }, 0)
		assert(not blocked.clear and blocked.id ~= nil, "narrow blocker must obstruct current pose")
	end
	local side = query(unitID, "heading", 0, { x + 160, y, z + 300 }, -16384)
	if sidestep then
		assert(side.clear and side.kind == nil and side.id == nil, "sidestep: " .. side.reason)
		for id in pairs(blockerIDs) do
			setVolume(id, 200, 1200, 200, 0, 500, 0, 2, 1, 1)
		end
	end
	query(unitID, "heading", 0, { x, y, z }, 0, true)
	local far = query(unitID, "heading", 0, { 100, y, 100 }, 0)
	assert(far.reason == "range" and far.kind == nil and far.id == nil)
	local nx, ny, nz = Spring.GetUnitPosition(unitID)
	assert(x == nx and y == ny and z == nz and unitHeading == Spring.GetUnitHeading(unitID), "query moved unit")
	local after = { Spring.GetUnitWeaponVectors(unitID, 1) }
	for i, value in ipairs(vectors) do
		assert(value == after[i], "query changed weapon vectors")
	end
	same(query(unitID, "heading", 0), headingResult, "candidate leaked into default query")
	assert(not pcall(Spring.TestUnitAttackWeapon, unitID, 0))
	for _, options in ipairs({
		{ avoidFlags = 1024 },
		{ orientation = "unknown" },
		{ useMuzzle = "yes" },
		{ orientation = "heading", position = { x, y, z } },
		{ orientation = "heading", heading = 32768 },
		{ orientation = "heading", position = { 0 / 0, y, z } },
		{ orientation = "heading", position = { x, math.huge, z } },
		{ orientation = "rotate", position = { x, y, z } },
	}) do
		assert(not pcall(Spring.TestUnitAttackWeapon, unitID, 1, options), "invalid query accepted")
	end
	assert(not pcall(Spring.TestUnitAttackWeapon, target, 1))
	assert(not pcall(Spring.GetUnitAttackTargetPosition, target))
	return true
end
local function snapshot(frame)
	local sum, weighted, ids = 1, 0, Spring.GetAllUnits()
	table.sort(ids)
	for _, id in ipairs(ids) do
		local x, y, z = Spring.GetUnitPosition(id)
		local cmd, opts, tag = Spring.GetUnitCurrentCommand(id)
		local value =
			string.format("%d:%.9g:%.9g:%.9g:%s:%s:%s;", id, x, y, z, tostring(cmd), tostring(opts), tostring(tag))
		-- Engine Lua uses float numbers: keep both checksum sums below 2^24.
		for i = 1, #value do
			sum = (sum + value:byte(i)) % 65521
			weighted = (weighted + sum) % 65521
		end
	end
	Spring.Echo(
		string.format("[AttackValidationSnapshot] frame=%d units=%d digest=%.0f:%.0f", frame, #ids, sum, weighted)
	)
end
function gadget:GameFrame(frame)
	if frame <= 0 then
		return
	end
	if frame == 1 then
		SendToUnsynced("attack_validation_speed")
	end
	if mode == "blockers" then
		local index = math.floor((frame - 1) / 90) + 1
		local phase = (frame - 1) % 90
		if not cases[index] then
			assert(completed == #cases)
			Spring.Echo("[AttackValidation] PASS blockers=" .. #cases)
			SendToUnsynced("attack_validation_quit")
			return
		end
		if phase == 0 then
			startCase(cases[index])
		end
		if phase == 30 then
			local params = active.ground and { 2048, 200, 2248 } or { target }
			Spring.GiveOrderToUnit(shooter, CMD.ATTACK, params, {})
			Spring.Echo(
				"[AttackValidationOrder]",
				shooter,
				Spring.GetUnitCurrentCommand(shooter),
				Spring.GetUnitIsStunned(shooter)
			)
		end
		if phase == 89 then
			assert(
				observed,
				active.weapon .. ": expected " .. active.blocker .. (active.ground and " ground" or " unit")
			)
			assert(not pcall(Spring.GetUnitAttackTargetPosition, shooter))
			assert(not pcall(Spring.TestUnitAttackWeapon, shooter, 1))
			completed = completed + 1
		end
	else
		if frame % 1800 == 0 then
			snapshot(frame)
		end
		if frame >= endFrame then
			Spring.Echo("[AttackValidation] PASS replay")
			SendToUnsynced("attack_validation_quit")
		end
	end
end

-- A failed check must stop the accelerated run rather than produce an error loop.
for _, name in ipairs({ "GameFrame", "AttackCommandMovement" }) do
	local run = gadget[name]
	gadget[name] = function(...)
		local ok, result = pcall(run, ...)
		if not ok then
			Spring.Echo("[AttackValidation] FAIL " .. tostring(result))
			SendToUnsynced("attack_validation_quit")
			return true
		end
		return result
	end
end
