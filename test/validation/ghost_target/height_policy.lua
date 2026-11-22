-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return {name = "Command height fixture", layer = 0, enabled = true}
end

local function check()
	local id = assert(Spring.CreateUnit("probe", 2000, 200, 1800, 0, 0))
	local preserve = Spring.GetModOptions().heightpolicy == "override"
	local height = CMD.OPT_KEEP_HEIGHT
	local give = Spring.GiveOrderToUnit
	local function expect(index, cmdID, x, options, flagged)
		local cmd = assert(Spring.GetUnitCommands(id, -1)[index], "missing command")
		local y = flagged and 900 or math.max(0, Spring.GetGroundHeight(x, 2000))
		assert(cmd.id == cmdID and cmd.params[1] == x and cmd.params[3] == 2000, "queue order")
		assert(math.abs(cmd.params[2] - y) < 0.01, "expected height " .. y .. ", got " .. cmd.params[2])
		assert(cmd.options.coded == options and cmd.options.keepheight == flagged, "command options")
		return cmd.tag
	end
	give(id, CMD.ATTACK, {2000, 900, 2000}, 0)
	local tag = expect(1, CMD.ATTACK, 2000, 0, false)
	local opts = CMD.OPT_SHIFT + CMD.OPT_CTRL
	local expected = opts + (preserve and height or 0)
	give(id, CMD.ATTACK, {2200, 900, 2000}, opts + height)
	expect(2, CMD.ATTACK, 2200, expected, preserve)
	give(id, CMD.INSERT, {tag, CMD.MANUALFIRE, opts + height, 2100, 900, 2000}, CMD.OPT_RIGHT + height)
	expect(2, CMD.MANUALFIRE, 2100, expected, preserve)
	give(id, CMD.INSERT, {0, CMD.ATTACK, opts + height, 2300, 900, 2000}, CMD.OPT_ALT)
	expect(1, CMD.ATTACK, 2300, expected, preserve)
	assert(expect(2, CMD.ATTACK, 2000, 0, false) == tag)
	expect(3, CMD.MANUALFIRE, 2100, expected, preserve)
	expect(4, CMD.ATTACK, 2200, expected, preserve)
	assert(#Spring.GetUnitCommands(id, -1) == 4, "duplicate command")
end

function gadget:GameFrame(frame)
	if frame ~= 5 then return end
	local ok, err = pcall(check)
	Spring.Echo(ok and "HEIGHT_TEST PASS" or "HEIGHT_TEST FAIL " .. tostring(err))
	SendToUnsynced("height_done")
end
