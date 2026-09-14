-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
-- Run the identical fixture on parent/native/fallback/Lua with parity.lua.
function gadget:GetInfo()
	return {
		name = "Attack movement branches",
		desc = "Deterministic policy scenarios",
		author = "OpenAI Codex",
		layer = -1000000,
		enabled = true,
	}
end
if not gadgetHandler:IsSyncedCode() or Spring.GetModOptions().attackmovementbranches ~= "1" then
	return false
end
local cases = {
	{ "unit clear", "corak", 180 },
	{ "unit out of range", "corak", 700 },
	{ "unit terrain", "corak", 180, terrain = true },
	{ "unit friendly", "corak", 180, blocker = true },
	{ "unit close", "corak", 24 },
	{ "unit hold", "corak", 700, hold = true },
	{ "unit chase", "corak", 180, moving = true },
	{ "unit strafe", "corpyro", 180, blocker = true },
	{ "unit gunship", "armkam", 180, blocker = true },
	{ "unit cannon", "cormort", 450, terrain = true },
	{ "unit missile", "corstorm", 350, terrain = true },
	{ "unit multiweapon", "corcom", 320 },
	{ "unit manual", "corcom", 100, manual = true },
	{ "unit paralyzed target", "armspid", 180, paralyzed = true },
	{ "fight hold", "corak", 180, fight = true, hold = true },
	{ "fight maneuver", "corak", 180, fight = true },
	{ "ground clear", "corak", 180, ground = true },
	{ "ground out of range", "corak", 700, ground = true },
	{ "ground terrain", "cormort", 450, ground = true, terrain = true },
	{ "ground friendly", "corak", 180, ground = true, blocker = true },
	{ "ground gunship", "armkam", 180, ground = true },
	{ "ground manual", "corcom", 100, ground = true, manual = true },
}
local created, shooter, target = {}, nil, nil
local function create(name, x, z, team)
	local id = assert(Spring.CreateUnit(name, x, 200, z, 0, team), name)
	created[#created + 1] = id
	Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, { 0 }, {})
	Spring.GiveOrderToUnit(id, CMD.MOVE_STATE, { 0 }, {})
	return id
end
function gadget:GameFrame(frame)
	if frame < 1 then
		return
	end
	local index, phase = math.floor((frame - 1) / 240) + 1, (frame - 1) % 240
	local c = cases[index]
	if not c then
		return
	end
	if phase == 0 then
		for _, id in ipairs(created) do
			if Spring.ValidUnitID(id) then
				Spring.DestroyUnit(id, false, true)
			end
		end
		created = {}
		Spring.SetGlobalLos(0, true)
		Spring.SetGlobalLos(1, true)
		Spring.SetHeightMapFunc(function()
			for x = 1536, 3072, 8 do
				for z = 1536, 3584, 8 do
					Spring.SetHeightMap(x, z, 200)
				end
			end
			if c.terrain then
				for x = 1920, 2176, 8 do
					for z = 2104, 2160, 8 do
						Spring.SetHeightMap(x, z, 1000)
					end
				end
			end
		end)
		shooter = create(c[2], 2048, 2048, 0)
		target = create("armbanth", 2048, 2048 + c[3], 1)
		Spring.SetUnitHealth(target, 100000)
		if c.paralyzed then
			Spring.SetUnitHealth(target, { paralyze = 1000000 })
		end
		if c.blocker then
			local id = create("armbanth", 2048, 2148, 0)
			Spring.SetUnitCollisionVolumeData(id, 100, 1000, 40, 0, 400, 0, 2, 1, 1)
		end
		Spring.Echo("[AttackBranch]", index, c[1])
	elseif phase == 30 then
		Spring.GiveOrderToUnit(shooter, CMD.MOVE_STATE, { c.hold and 0 or 1 }, {})
		if c.moving then
			Spring.GiveOrderToUnit(target, CMD.MOVE, { 2048, 200, 3000 }, {})
		end
		local params = (c.ground or c.fight) and { 2048, 200, 2048 + c[3] } or { target }
		Spring.GiveOrderToUnit(shooter, c.manual and CMD.MANUALFIRE or c.fight and CMD.FIGHT or CMD.ATTACK, params, {})
	end
end
