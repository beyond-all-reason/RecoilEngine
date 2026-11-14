-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
-- Integration with #3333; opt-in only, never part of replay parity.
function gadget:GetInfo()
	return { name = "Attack goal ownership validation", desc = "Command-owned and independent Lua goals", author = "OpenAI Codex", layer = -1000000, enabled = true }
end
if not gadgetHandler:IsSyncedCode() or Spring.GetModOptions().attackmovementownership ~= "1" then
	return false
end
local cases = { "attack", "independent", "raw", "successor" }
local passed = 0
local shooter, target, kind
local fixed = Spring.GetModOptions().attackmovementstopfixed == "1"
function gadget:AttackCommandMovement(id)
	if id ~= shooter then return false end
	Spring.SetUnitMoveGoal(id, 2748, Spring.GetGroundHeight(2748, 2048), 2048, 8, nil, kind == "raw")
	return true
end
function gadget:GameFrame(frame)
	if frame == 1 then
		Spring.SetGlobalLos(0, true)
		Spring.SetGlobalLos(1, true)
		Spring.SetHeightMapFunc(function()
			for x = 1536, 3072, 8 do
				for z = 1536, 3584, 8 do Spring.SetHeightMap(x, z, 200) end
			end
		end)
	end
	local index = math.floor((frame - 1) / 240) + 1
	local phase = (frame - 1) % 240
	if phase == 0 and cases[index] then
		if shooter and Spring.ValidUnitID(shooter) then Spring.DestroyUnit(shooter, false, true) end
		if target and Spring.ValidUnitID(target) then Spring.DestroyUnit(target, false, true) end
		kind = cases[index]
		shooter = assert(Spring.CreateUnit("armstump", 2048, Spring.GetGroundHeight(2048, 2048), 2048, "east", 0))
		target = assert(Spring.CreateUnit("corak", 2748, Spring.GetGroundHeight(2748, 2048), 2048, "west", 1))
		Spring.GiveOrderToUnit(shooter, CMD.FIRE_STATE, { 0 }, 0)
		Spring.GiveOrderToUnit(target, CMD.FIRE_STATE, { 0 }, 0)
		Spring.GiveOrderToUnit(shooter, CMD.ATTACK, { target }, 0)
		if kind == "successor" then
			Spring.GiveOrderToUnit(shooter, CMD.MOVE, { 2048, 0, 3048 }, { "shift" })
		end
	elseif phase == 90 and cases[index] then
		assert(Spring.GetUnitMoveTypeData(shooter).progressState == "active", kind .. " must be moving before deletion")
		if kind == "independent" then
			-- Same location, deliberately set outside AttackCommandMovement.
			Spring.SetUnitMoveGoal(shooter, 2748, Spring.GetGroundHeight(2748, 2048), 2048, 8)
		end
		Spring.DestroyUnit(target, false, true, nil, true)
	elseif phase == 110 and cases[index] then
		assert(not Spring.ValidUnitID(target), "target must have been deleted")
		local cmd = Spring.GetUnitCurrentCommand(shooter)
		assert(cmd == (kind == "successor" and CMD.MOVE or nil), kind .. " unexpected command after deletion")
		local active = Spring.GetUnitMoveTypeData(shooter).progressState == "active"
		assert(active == (kind ~= "attack" or not fixed), kind .. " wrong movement ownership")
		passed = passed + 1
		Spring.Echo("[AttackOwnership] PASS " .. kind)
	elseif frame == 961 then
		assert(passed == #cases, "incomplete ownership checks")
		Spring.Echo("[AttackValidation] PASS ownership=4")
	end
end
