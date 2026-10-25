-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
local inside, nested, done = false, 0, false
function LayoutButtons(x, y, count, commands)
	if inside then
		nested = nested + 1
	end
	return "Region views", x, y, {}, {}, {}, {}, {}, {}, {}, {}
end
function DrawScreen()
	if done or Spring.GetGameFrame() < 3 then
		return
	end
	done = true
	Spring.SelectUnitArray({ Spring.GetGameRulesParam("view_home") })
	inside = true
	Spring.LoadCtrlPanelConfig("framealpha 0\nxicons 5\nyicons 3\n")
	inside = false
	if nested > 0 then
		Spring.Echo("LAYOUT_STACK_PASS nested=" .. nested)
	else
		Spring.Echo("LAYOUT_STACK_FAIL nested layout was not exercised")
	end
	Spring.SendCommands("quitforce")
end
