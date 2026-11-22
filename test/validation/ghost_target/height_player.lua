-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
-- Send real player packets with explicit deadlines through the unsynced API.
function RecvFromSynced(action, direct, inserted)
	if action == "height_done" then
		Spring.SendCommands("quitforce")
	elseif action == "height_orders" then
		CallAsTeam(0, function()
			local bits = CMD.OPT_KEEP_HEIGHT + CMD.OPT_SHIFT
			assert(Spring.GiveOrderToUnit(direct, CMD.ATTACK, {3600, 900, 3200}, bits, 160))
			assert(Spring.GiveOrderToUnit(inserted, CMD.INSERT,
				{0, CMD.ATTACK, bits, 3800, 900, 3200}, CMD.OPT_ALT + CMD.OPT_KEEP_HEIGHT, 240))
		end)
	end
end
