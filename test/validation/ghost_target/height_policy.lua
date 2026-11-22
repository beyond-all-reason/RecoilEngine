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

local direct, inserted
local allowed, accepted = {}, {}
local keep = Spring.GetModOptions().heightpolicy == "override"

function gadget:AllowCommand(id, _, _, _, _, _, _, player, synced, fromLua)
	if id == direct or id == inserted then
		assert(player == 0 and synced and not fromLua, "unexpected issuer in AllowCommand")
		allowed[id] = (allowed[id] or 0) + 1
	end
	return true
end

function gadget:UnitCommand(id, _, _, cmd, params, opts, _, player, synced, fromLua)
	if id == direct or id == inserted then
		assert(not accepted[id], "duplicate UnitCommand")
		accepted[id] = {cmd = cmd, player = player, synced = synced, fromLua = fromLua, bits = opts.coded, inner = params[3]}
	end
end

local function testFrame(frame)
	if frame == 5 then
		check()
		direct = assert(Spring.CreateUnit("mobileprobe", 2000, 200, 1800, 0, 0))
		inserted = assert(Spring.CreateUnit("mobileprobe", 2150, 200, 1800, 0, 0))
		SendToUnsynced("height_orders", direct, inserted)
	elseif frame == 160 then
		for _, id in ipairs({direct, inserted}) do
			local event = assert(accepted[id], "missing player order")
			local n = allowed[id]
			local kind = id == direct and "ATTACK" or "INSERT"
			Spring.Echo("HEIGHT_TEST " .. kind .. " playerID=" .. event.player .. " AllowCommand calls=" .. n)
			assert(event.cmd == (id == direct and CMD.ATTACK or CMD.INSERT))
			assert(event.player == 0 and event.synced and not event.fromLua and n == 1, "original player order")
			local heightBit = keep and CMD.OPT_KEEP_HEIGHT or 0
			assert(event.bits == (id == direct and CMD.OPT_SHIFT or CMD.OPT_ALT) + heightBit)
			if id == inserted then assert(event.inner == CMD.OPT_SHIFT + heightBit) end
			local cmd = assert(Spring.GetUnitCommands(id, 1)[1], "order expired early")
			assert(cmd.id == CMD.ATTACK and cmd.options.coded == CMD.OPT_SHIFT + heightBit)
		end
	elseif frame == 177 then
		assert(#Spring.GetUnitCommands(direct, 1) == 0, "direct attack missed its timeout")
		assert(#Spring.GetUnitCommands(inserted, 1) == 1, "inserted attack expired early")
	elseif frame == 240 then
		assert(#Spring.GetUnitCommands(inserted, 1) == 1, "inserted attack expired before its deadline")
	elseif frame == 257 then
		assert(#Spring.GetUnitCommands(inserted, 1) == 0, "inserted attack missed its timeout")
		Spring.Echo("HEIGHT_TEST PASS direct timeout 160, inserted timeout 240")
		SendToUnsynced("height_done")
	end
end

function gadget:GameFrame(frame)
	local ok, err = pcall(testFrame, frame)
	if not ok then
		Spring.Echo("HEIGHT_TEST FAIL " .. tostring(err))
		SendToUnsynced("height_done")
	end
end
