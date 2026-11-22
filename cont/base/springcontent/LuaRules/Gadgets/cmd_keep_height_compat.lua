-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.

function gadget:GetInfo()
	return {
		name = "Legacy command height",
		desc = "Clear the explicit target height option for legacy games",
		layer = 0,
		enabled = true,
	}
end

if not gadgetHandler:IsSyncedCode() then
	return false
end

local height = CMD.OPT_KEEP_HEIGHT

function gadget:AllowCommand(unitID, unitDefID, unitTeam, cmdID, params, opts)
	local inserted = cmdID == CMD.INSERT and params[3] and math.bit_and(params[3], height) ~= 0
	if opts.keepheight or inserted then
		opts.clearKeepHeight = true
	end
	return true
end
