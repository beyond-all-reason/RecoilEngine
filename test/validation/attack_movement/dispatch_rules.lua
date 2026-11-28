-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return {
		name = "Attack movement dispatch validation",
		desc = "Opt-in two-handle checks",
		author = "OpenAI Codex",
		layer = -1000000,
		enabled = true,
	}
end
if Spring.GetModOptions().attackmovementvalidation ~= "dispatch" then
	return false
end
if not gadgetHandler:IsSyncedCode() then
	function gadget:RecvFromSynced(name)
		if name == "attack_dispatch_speed" then
			Spring.SendCommands("setspeed 1000")
		end
		if name == "attack_dispatch_quit" then
			Spring.SendCommands("quitforce")
		end
	end
	return
end
local cases = {
	"handled",
	"false",
	"nil",
	"replace",
	"targetDeath",
	"ownerDeath",
	"finishSuccessor",
	"groundCoordinates",
	"sensorCoordinates",
}
local shooter, target, active, calls, finished
local function cleanup(id)
	if id and Spring.ValidUnitID(id) then
		Spring.DestroyUnit(id, false, true)
	end
end
function gadget:AttackCommandMovement(id, _, _, _, targetID, x, y, z)
	if id ~= shooter then
		return false
	end
	calls = calls + 1
	assert(type(x) == "number" and type(y) == "number" and type(z) == "number", "missing target coordinates")
	if active == "groundCoordinates" then
		local _, _, _, cx, cy, cz = Spring.GetUnitCurrentCommand(id)
		assert(targetID == nil and x == cx and y == cy and z == cz, "ground command coordinates changed")
		return true
	end
	if active == "sensorCoordinates" then
		assert(targetID == target)
		local _, _, _, mx, my, mz = Spring.GetUnitPosition(target, true)
		-- Forced radar-only visibility and error size 1 make the expected
		-- midpoint offset exact. Interpolation is frozen below.
		assert(x == mx + 64 and y == my + 16 and z == mz - 32, "sensor error not passed to callback")
		return true
	end
	if active == "finishSuccessor" then
		Spring.UnitFinishCommand(id)
		assert(Spring.GetUnitCurrentCommand(id) == CMD.MOVE, "queued MOVE lost")
		return false
	end
	if active == "handled" then
		return true
	end
	if active == "nil" then
		return
	end
	if active == "false" then
		return false
	end
	if active == "replace" then
		Spring.GiveOrderToUnit(id, CMD.STOP, {}, {})
	end
	if active == "targetDeath" then
		Spring.DestroyUnit(target, false, true)
	end
	if active == "ownerDeath" then
		Spring.DestroyUnit(id, false, true)
	end
	return false
end
function gadget:GameFrame(frame)
	if frame < 1 or finished then
		return
	end
	local index = math.floor((frame - 1) / 120) + 1
	local phase = (frame - 1) % 120
	if not cases[index] then
		finished = true
		Spring.Echo("[AttackValidation] PASS dispatch=" .. #cases)
		SendToUnsynced("attack_dispatch_quit")
		return
	end
	if phase == 0 then
		assert(Spring.GetGameRulesParam("attack_dispatch_gaia_ready") == 1, "LuaGaia fixture missing")
		SendToUnsynced("attack_dispatch_speed")
		cleanup(shooter)
		cleanup(target)
		active, calls = cases[index], 0
		Spring.SetGlobalLos(0, true)
		shooter = assert(Spring.CreateUnit("corak", 2048, Spring.GetGroundHeight(2048, 2048), 2048, 0, 0))
		Spring.SetGameRulesParam("attack_dispatch_shooter", shooter)
		target = assert(Spring.CreateUnit("armbanth", 2048, Spring.GetGroundHeight(2048, 2248), 2248, 0, 1))
		Spring.GiveOrderToUnit(shooter, CMD.FIRE_STATE, { 0 }, {})
		Spring.GiveOrderToUnit(target, CMD.FIRE_STATE, { 0 }, {})
		Spring.SetUnitWeaponState(shooter, 1, "reloadState", 1000000)
		if active == "finishSuccessor" then
			Spring.SetUnitHealth(shooter, { paralyze = 100000 })
		end
		Spring.SetGameRulesParam("attack_dispatch_gaia", 0)
		Spring.SetGameRulesParam("attack_dispatch_later", 0)
	elseif phase == 30 then
		if active == "sensorCoordinates" then
			Spring.SetUnitLosMask(target, 0, 15)
			Spring.SetUnitLosState(target, 0, 2) -- radar only
			Spring.SetRadarErrorParams(0, 1)
			Spring.SetUnitPosErrorParams(target, 64, 16, -32, 0, 0, 0, 1000000, 0, true)
		end
		Spring.GiveOrderToUnit(
			shooter,
			CMD.ATTACK,
			active == "groundCoordinates" and { 2048, 200, 2248 } or { target },
			{}
		)
		if active == "finishSuccessor" then
			Spring.GiveOrderToUnit(shooter, CMD.MOVE, { 2048, 200, 3000 }, { "shift" })
			Spring.SetUnitHealth(shooter, { paralyze = 0 })
		end
	elseif phase == 119 then
		local gaia = Spring.GetGameRulesParam("attack_dispatch_gaia") or 0
		local later = Spring.GetGameRulesParam("attack_dispatch_later") or 0
		assert(calls > 0, "Rules did not run: " .. active)
		if active == "false" or active == "nil" then
			assert(gaia == calls, "fallback did not reach Gaia exactly once")
			assert(later == calls, "fallback did not reach the later Rules gadget exactly once")
		else
			assert(gaia == 0, "Gaia ran after handling/invalidation: " .. active)
			assert(later == 0, "later Rules gadget ran after handling/invalidation: " .. active)
		end
		Spring.Echo("[AttackValidationDispatch]", active, calls, gaia, later)
	end
end
for _, name in ipairs({ "GameFrame", "AttackCommandMovement" }) do
	local run = gadget[name]
	gadget[name] = function(...)
		local ok, result = pcall(run, ...)
		if not ok then
			finished = true
			Spring.Echo("[AttackValidation] FAIL " .. tostring(result))
			SendToUnsynced("attack_dispatch_quit")
			return true
		end
		return result
	end
end
