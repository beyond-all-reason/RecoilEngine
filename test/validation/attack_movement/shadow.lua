-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
-- Validation only: execute two side-effect-free policy instances, then let
-- native movement run. The diagnostic engine logs the actual native actions.
function gadget:GetInfo()
	return { name = "Attack policy shadow", author = "OpenAI Codex", layer = 100000, enabled = true }
end
if not gadgetHandler:IsSyncedCode() then
	return false
end
local policies, actions = {}, {}
local function record(name, ...)
	local row = { name }
	for i = 1, select("#", ...) do
		local value = select(i, ...)
		row[#row + 1] = type(value) == "number" and string.format("%.9g", value) or tostring(value)
	end
	actions[#actions + 1] = table.concat(row, " ")
end
for _, accepted in ipairs({ true, false }) do
	local api = setmetatable({}, { __index = Spring })
	api.GetModOptions = function()
		return { attackmovementmode = "lua" }
	end
	api.SetUnitMoveGoal = function(_, x, y, z, radius, speed)
		record("move", x, y, z, radius, speed or "default")
	end
	api.ClearUnitGoal = function()
		record("stop")
	end
	api.SetUnitFacingTarget = function(_, x, y, z, radius)
		record("face", x, y, z, radius)
	end
	api.FinishUnitAttackCommand = function()
		record("finish")
	end
	api.SetUnitTarget = function(_, a, b, c, d, e)
		if type(b) == "boolean" then
			record("assignUnit", a, c and 1 or 0, b and 1 or 0, accepted and 1 or 0)
		else
			record("assignGround", a, b, c, e and 1 or 0, d and 1 or 0, accepted and 1 or 0)
		end
		return accepted
	end

	local instance = {}
	local env = setmetatable({ Spring = api, gadget = instance }, { __index = getfenv() })
	local chunk = assert(loadstring(VFS.LoadFile("test/attack_policy.lua")))
	setfenv(chunk, env)
	chunk()
	policies[#policies + 1] = instance
end
function gadget:AttackCommandMovement(...)
	for index, policy in ipairs(policies) do
		actions = {}
		assert(policy:AttackCommandMovement(...) == true)
		Spring.Echo("[AttackShadow" .. index .. "] " .. table.concat(actions, " | "))
	end
	return false
end
function gadget:UnitDestroyed(unitID)
	for _, policy in ipairs(policies) do
		policy:UnitDestroyed(unitID)
	end
end
