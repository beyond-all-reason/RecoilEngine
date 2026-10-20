-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
function gadget:GetInfo()
	return { name = "Draw bounds fixture", layer = 0, enabled = true }
end
if not gadgetHandler:IsSyncedCode() then
	return
end
local projectiles = {}
local homeUnit, homeProjectile
function gadget:Initialize()
	for _, region in ipairs({ { "home", 1300 }, { "frontier", 3200 } }) do
		local name, z = region[1], region[2]
		local unit = assert(Spring.CreateUnit("region_test_unit", 1700, 96, z, 0, 0))
		local feature = assert(Spring.CreateFeature("region_test_feature", 2000, 96, z))
		local projectile = assert(Spring.SpawnProjectile(WeaponDefNames.draw_bounds_projectile.id, {
			pos = { 2600, 230, z },
			speed = { 0, 0, 0 },
			gravity = 0,
			owner = unit,
			team = 0,
			ttl = 10000,
		}))
		projectiles[projectile] = { 2600, 230, z }
		if name == "home" then
			homeUnit, homeProjectile = unit, projectile
		end
		Spring.SetGameRulesParam(name .. "_unit", unit, { public = true })
		Spring.SetGameRulesParam(name .. "_feature", feature, { public = true })
		Spring.SetGameRulesParam(name .. "_projectile", projectile, { public = true })
		assert(Spring.SpawnCEG("draw_bounds_particle", 2300, 230, z, 0, 1, 0))
		assert(Spring.SpawnCEG("draw_bounds_flash", 2900, 96, z, 0, 1, 0))
	end
end
function gadget:GameFrame(frame)
	for id, pos in pairs(projectiles) do
		Spring.SetProjectilePosition(id, pos[1], pos[2], pos[3])
		Spring.SetProjectileVelocity(id, 0, 0, 0)
	end
end

function gadget:RecvLuaMsg(message)
	if message == "draw_bounds_offmap" then
		Spring.MoveCtrl.Enable(homeUnit)
		Spring.MoveCtrl.SetPosition(homeUnit, -128, 220, 1300)
		projectiles[homeProjectile] = { -128, 230, 1600 }
	end
end
