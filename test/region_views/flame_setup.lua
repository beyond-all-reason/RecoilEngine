-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
function gadget:GetInfo()
	return { name = "Flame bounds regression", layer = 0, enabled = true }
end
if not gadgetHandler:IsSyncedCode() then
	return
end
function gadget:Initialize()
	Spring.CreateUnit("region_test_unit", 1500, 96, 1500, 0, 0)
end
function gadget:RecvLuaMsg(message)
	local n, x, y, z = message:match("^flare (%d+) ([%d.-]+) ([%d.-]+) ([%d.-]+)$")
	if not n then
		return
	end
	Spring.SpawnCEG("region_flame", tonumber(x), tonumber(y), tonumber(z), 0, 1, 0)
	Spring.SetGameRulesParam("flare_ready", tonumber(n))
end
