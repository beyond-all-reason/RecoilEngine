-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
-- Minimal UI: issue one controlled network batch, with no BAR widgets loaded.
local sent = false
function Update()
	local a = Spring.GetGameRulesParam("sq_a")
	if sent or not a then
		return
	end
	local b = Spring.GetGameRulesParam("sq_b")
	local targetA = Spring.GetGameRulesParam("sq_ta")
	local targetB = Spring.GetGameRulesParam("sq_tb")
	sent = true
	assert(Spring.GiveOrderArrayToUnitArray({ a, b }, {
		{ CMD.WAIT, {}, 0 },
		{ CMD.ATTACK, { targetA }, CMD.OPT_SHIFT },
		{ CMD.ATTACK, { targetB }, CMD.OPT_SHIFT },
	}, false))
end
