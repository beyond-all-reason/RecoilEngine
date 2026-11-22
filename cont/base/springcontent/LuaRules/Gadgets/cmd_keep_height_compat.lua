-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.

function gadget:GetInfo()
	return {
		name = "Legacy command height",
		desc = "Clear the explicit target height option for legacy games",
		layer = 1000000, -- Let game gadgets validate the original issuer first.
		enabled = true,
	}
end

if not gadgetHandler:IsSyncedCode() then
	return false
end

local height = CMD.OPT_KEEP_HEIGHT

function gadget:AllowCommand(unitID, unitDefID, unitTeam, cmdID, params, opts)
	local inserted = cmdID == CMD.INSERT and params[3] and math.bit_and(params[3], height) ~= 0
	if not opts.keepheight and not inserted then
		return true
	end

	if inserted then
		params[3] = params[3] - height
	end
	local bits = opts.coded
	if opts.keepheight then
		bits = bits - height
	end
	Spring.GiveOrderToUnit(unitID, cmdID, params, bits)
	return false
end
