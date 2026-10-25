-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
function gadget:GetInfo()
	return { name = "View rectangle fixture", layer = 0, enabled = true }
end
if not gadgetHandler:IsSyncedCode() then
	return
end
function gadget:Initialize()
	-- Distinct icons on both sides of the gap expose stale/cross-region drawing.
	local home = assert(Spring.CreateUnit("region_test_unit", 1500, 96, 1250, 0, 0))
	local frontier = assert(Spring.CreateUnit("region_test_unit", 2500, 96, 3250, 0, 0))
	local edge = assert(Spring.CreateUnit("region_test_unit", 998, 96, 1400, 0, 0))
	Spring.SetGameRulesParam("view_edge", edge, { public = true })
	Spring.SetGameRulesParam("view_home", home, { public = true })
	Spring.SetGameRulesParam("view_frontier", frontier, { public = true })
end
