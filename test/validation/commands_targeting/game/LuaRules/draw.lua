-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function Update()
	if Spring.GetGameFrame() < 0 then
		Spring.SendCommands("forcestart")
	end
end
function RecvFromSynced(message)
	if message == "commands_targeting_done" then
		local hidden = Spring.ClearCommandsTargeting == nil and Spring.ClearTargetingFromUnit == nil
		local readable = type(Spring.GetCommandsTargeting) == "function"
		Spring.Echo(hidden and readable and "COMMANDS_TARGETING UNSYNCED PASS" or "COMMANDS_TARGETING FAIL: unsynced API exposure")
		Spring.SendCommands("quitforce")
	end
end
