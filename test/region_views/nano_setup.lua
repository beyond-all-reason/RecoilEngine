-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
function gadget:GetInfo()
	return { name = "Native nano construction fixture", layer = 0, enabled = true }
end
if not gadgetHandler:IsSyncedCode() then
	return
end
local targets, builders = {}, {}
function gadget:Initialize()
	Spring.SetTeamResource(0, "metal", 10000)
	Spring.SetTeamResource(0, "energy", 10000)
	for i, z in ipairs({ 1300, 3200 }) do
		local builder = Spring.CreateUnit("region_nano_builder", 1500, 96, z, 0, 0)
		local target = Spring.CreateUnit("region_test_unit", 2700, 96, z, 0, 0, true)
		targets[i] = target
		builders[i] = builder
		Spring.SetUnitHealth(target, { build = 0.1 })
		Spring.GiveOrderToUnit(builder, CMD.REPAIR, { target }, 0)
	end
end
function gadget:GameFrame(f)
	for _, id in ipairs(targets) do
		Spring.SetUnitHealth(id, { build = 0.1 })
	end
	if f == 45 then
		for i, id in ipairs(builders) do
			local hp, mhp, para, capture, build = Spring.GetUnitHealth(targets[i])
			Spring.Echo(
				"NANO_FIXTURE builder="
					.. id
					.. " power="
					.. tostring(Spring.GetUnitCurrentBuildPower(id))
					.. " targetBuild="
					.. build
					.. " orders="
					.. #Spring.GetUnitCommands(id, -1)
			)
		end
	end
end
