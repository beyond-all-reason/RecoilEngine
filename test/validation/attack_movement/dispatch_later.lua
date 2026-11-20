-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return {
		name = "Attack movement later handler",
		desc = "Detect dispatch after a preceding gadget invalidates the command",
		author = "OpenAI Codex",
		layer = -999999,
		enabled = true,
	}
end
if not gadgetHandler:IsSyncedCode() or Spring.GetModOptions().attackmovementvalidation ~= "dispatch" then
	return false
end
function gadget:AttackCommandMovement(id)
	if id ~= Spring.GetGameRulesParam("attack_dispatch_shooter") then
		return false
	end
	Spring.SetGameRulesParam("attack_dispatch_later", (Spring.GetGameRulesParam("attack_dispatch_later") or 0) + 1)
	return false
end
