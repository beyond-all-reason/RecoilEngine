-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
-- Runs without AttackCommandMovement or the later orientation/candidate API.
function gadget:GetInfo()
	return { name = "Query-only friendly filters", layer = 0, enabled = true }
end
if Spring.GetModOptions().attackmovementvalidation ~= "filters" then return false end
if not gadgetHandler:IsSyncedCode() then
	function gadget:RecvFromSynced(name)
		if name == "filters_done" then Spring.SendCommands("quitforce") end
	end
	return
end
local shooter, blocker
local flags = Game.collisionFlags
local function create(name, z)
	local id = assert(Spring.CreateUnit(name, 2048, 200, z, 0, 0))
	Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, { 0 }, {})
	Spring.GiveOrderToUnit(id, CMD.MOVE_STATE, { 0 }, {})
	return id
end
local function query(mask)
	return Spring.GetUnitWeaponTryTarget(shooter, 1, 2048, 230, 2348, { avoidFlags = mask })
end
function gadget:GameFrame(frame)
	if frame == 1 then
		Spring.SetGlobalLos(0, true)
		Spring.SetHeightMapFunc(function()
			for x = 1536, 2560, 8 do
				for z = 1536, 2560, 8 do Spring.SetHeightMap(x, z, 200) end
			end
		end)
		shooter = create("armllt", 2048)
	elseif frame == 30 then
		assert(query(flags.noGround + flags.noFeatures), "clear baseline blocked")
		blocker = create("armbanth", 2198)
		Spring.SetUnitCollisionVolumeData(blocker, 200, 1200, 200, 0, 500, 0, 2, 1, 1)
	elseif frame == 60 or frame == 90 then
		local mobile = frame == 60
		local mask = flags.noGround + flags.noFeatures
		local clear, reason, kind, id = query(mask)
		assert(not clear and reason == "friendly" and kind == "unit" and id == blocker, "missing friendly blocker")
		assert(query(mask + (mobile and flags.noMobileFriendlies or flags.noStaticFriendlies)), "matching filter did not skip blocker")
		assert(not query(mask + (mobile and flags.noStaticFriendlies or flags.noMobileFriendlies)), "opposite filter skipped blocker")
		assert(not query(mask), "override leaked into next query")
		if mobile then
			Spring.DestroyUnit(blocker, false, true)
			blocker = create("armfus", 2198)
			Spring.SetUnitCollisionVolumeData(blocker, 200, 1200, 200, 0, 500, 0, 2, 1, 1)
		else
			Spring.Echo("[AttackValidation] PASS filters=2")
			SendToUnsynced("filters_done")
		end
	end
end
