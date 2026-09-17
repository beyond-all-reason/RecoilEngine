-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
local units = {}
for _, name in ipairs({ "static", "mobile", "fighter", "target", "nano", "factory" }) do
	units[name] = {
		name = name,
		category = "TEST",
		objectName = "fir_tree_small.s3o",
		script = "test.lua", -- Attached by the test gadget.
		maxDamage = 100000,
		power = 100,
		explodeAs = "test",
		selfDestructAs = "test",
		footprintX = 1,
		footprintZ = 1,
		maxWaterDepth = 10000,
		canAttack = name ~= "target" and name ~= "nano" and name ~= "factory",
		canManualFire = true,
		canMove = name == "mobile" or name == "fighter",
		canFly = name == "fighter",
		isFighter = name == "fighter",
		movementClass = name == "mobile" and "TESTBOT" or nil,
		maxVelocity = name ~= "factory" and 3 or 0, -- Factories must be immobile.
		acceleration = 0.1,
		brakeRate = 0.1,
		turnRate = 1000,
		cruiseAlt = 100,
		sightDistance = 2000,
		weapons = (name ~= "target" and name ~= "nano" and name ~= "factory") and { { name = "test" }, { name = "test" } } or {},
		builder = name == "nano" or name == "factory",
		workerTime = 1,
		buildDistance = 500,
		yardmap = name == "factory" and "o" or nil,
		buildoptions = name == "factory" and { "mobile" } or nil,
	}
end
return {
	unitdefs = units,
	weapondefs = { test = {
		name = "test", weaponType = "Cannon", turret = true, range = 2000,
		reloadtime = 60, weaponVelocity = 600, damage = { default = 1 },
		avoidFriendly = false, avoidFeature = false, avoidGround = false,
		fireSubmersed = true,
	} },
	featuredefs = {}, armordefs = {},
	movedefs = { { name = "TESTBOT", footprintX = 1, footprintZ = 1, maxSlope = 60, maxWaterDepth = 10000 } },
}
