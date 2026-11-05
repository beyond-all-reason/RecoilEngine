-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
-- Observer shared verbatim by the parent, native, fallback and Lua runs.
function gadget:GetInfo()
	return {
		name = "Attack movement parity",
		desc = "Opt-in frame comparison",
		author = "OpenAI Codex",
		layer = -1000001,
		enabled = true,
	}
end
local options = Spring.GetModOptions()
if options.attackmovementvalidation ~= "replay" then
	return false
end
local finish = tonumber(options.attackmovementendframe) or 36000
local observe = options.attackmovementparity ~= "0"
if not gadgetHandler:IsSyncedCode() then
	local start, previous
	function gadget:RecvFromSynced(name, frame)
		if name == "attack_parity_tick" then
			local now = Spring.GetTimerMicros()
			if not start then
				start = now
			end
			if previous then
				Spring.Echo(
					string.format("[AttackTiming] %d %.6f", frame, Spring.DiffTimers(now, previous, true, true))
				)
			end
			previous = now
			if frame == 1 then
				Spring.SendCommands("setspeed 1000")
			end
			if frame == finish then
				Spring.Echo(string.format("[AttackElapsed] %.6f", Spring.DiffTimers(now, start, true, true)))
				Spring.SendCommands("quitforce")
			end
		end
	end
	return
end
-- Reuse the numeric buffer and pack native float values in C. Textifying every
-- field every frame creates enough temporary strings to overwhelm synced GC.
local values, count, weaponCounts = {}, 0, {}
local function add(...)
	for i = 1, select("#", ...) do
		local v = select(i, ...)
		if type(v) == "table" then
			add(#v)
			for j = 1, #v do
				add(v[j])
			end
		else
			count = count + 1
			values[count] = type(v) == "number" and v or (v == true and 1 or v == false and 0 or -1e30)
		end
	end
end
local function digest()
	for i = count + 1, #values do
		values[i] = nil
	end
	return VFS.CalculateHash(VFS.PackF32(values), 1)
end
local function snapshot(frame)
	count = 0
	local ids = Spring.GetAllUnits()
	table.sort(ids)
	add(#ids)
	for _, id in ipairs(ids) do
		local def = Spring.GetUnitDefID(id)
		add(id, def, Spring.GetUnitTeam(id), Spring.GetUnitHeading(id))
		add(Spring.GetUnitPosition(id))
		add(Spring.GetUnitVelocity(id))
		add(Spring.GetUnitHealth(id))
		add(Spring.GetUnitExperience(id))
		local commands = Spring.GetUnitCommands(id, -1)
		add(#commands)
		for _, cmd in ipairs(commands) do
			add(cmd.id, cmd.tag, cmd.options.coded, cmd.params)
		end
		if not weaponCounts[def] then
			weaponCounts[def] = #UnitDefs[def].weapons
		end
		add(weaponCounts[def])
		for w = 1, weaponCounts[def] do
			add(Spring.GetUnitWeaponState(id, w))
			add(Spring.GetUnitWeaponTarget(id, w))
		end
	end
	local units = digest()
	count = 0
	ids = Spring.GetAllProjectiles()
	table.sort(ids)
	add(#ids)
	for _, id in ipairs(ids) do
		add(id, Spring.GetProjectileDefID(id))
		add(Spring.GetProjectilePosition(id))
		add(Spring.GetProjectileVelocity(id))
	end
	Spring.Echo(string.format("[AttackParity] %d %s %s", frame, units, digest()))
end
function gadget:GameFrame(frame)
	if frame < 1 or frame > finish then
		return
	end
	if observe then
		local ok, err = pcall(snapshot, frame)
		if not ok then
			Spring.Echo("[AttackValidation] FAIL " .. tostring(err))
			SendToUnsynced("attack_parity_tick", finish)
			return
		end
	end
	if frame == finish then
		Spring.Echo("[AttackValidation] PASS replay")
	end
	SendToUnsynced("attack_parity_tick", frame)
end
