local scriptDirectory = arg[0]:match("^(.*[/\\])") or "./"

local function run(cheats, enabled)
	local lines, live, calls = {}, {}, {}
	local activeUnit
	gadget = {}
	GG = {}
	Game = { mapSizeX = 8192, mapSizeZ = 8192 }
	Engine = { version = "fixture" }
	CMD = { FIRE_STATE = 45 }
	gadgetHandler = {
		IsSyncedCode = function()
			return true
		end,
	}
	WeaponDefs = {
		[1] = { name = "cannon", type = "Cannon" },
		[2] = { name = "missile", type = "MissileLauncher" },
		[3] = { name = "starburst", type = "StarburstLauncher" },
		[4] = { name = "beam", type = "BeamLaser" },
	}
	local names = {
		"different",
		"same",
		"legacy",
		"lua_generic",
		"lua_numbered",
		"missing",
		"invalid",
		"yielding",
		"throwing",
		"unaffected",
		"create_failed",
		"lost",
		"mixed",
	}
	UnitDefs = {}
	for defID, name in ipairs(names) do
		UnitDefs[defID] = { name = name, weapons = { { weaponDef = (defID - 1) % 3 + 1 } } }
	end
	UnitDefs[10].weapons = { { weaponDef = 4 } }
	UnitDefs[13].weapons = { { weaponDef = 4 }, { weaponDef = 1 }, { weaponDef = 2 } }
	local functions = {
		[1] = { QueryWeapon1 = 0, AimFromWeapon1 = 1 },
		[2] = { QueryWeapon1 = 0, AimFromWeapon1 = 1 },
		[3] = { QueryPrimary = 0, AimFromPrimary = 1 },
		[7] = { QueryWeapon1 = 0, AimFromWeapon1 = 1 },
		[8] = { QueryWeapon1 = 0, AimFromWeapon1 = 1 },
		[9] = { QueryWeapon1 = 0, AimFromWeapon1 = 1 },
		[12] = { QueryWeapon1 = 0, AimFromWeapon1 = 1 },
		[13] = { QuerySecondary = 0, AimFromSecondary = 1, QueryWeapon3 = 0, AimFromWeapon3 = 1 },
	}
	Spring = {
		GetModOptions = function()
			return { aimfromaudit = enabled }
		end,
		Echo = function(line)
			lines[#lines + 1] = line
		end,
		IsCheatingEnabled = function()
			return cheats
		end,
		GetCOBScriptID = function(unitID, name)
			return (functions[unitID] or {})[name]
		end,
		CallCOBScript = function(unitID, functionID, returns, parameter)
			assert(returns == 1 and parameter == -1)
			calls[#calls + 1] = unitID
			if unitID == 8 then
				return 1, 0
			end
			if unitID == 9 then
				error("fixture error")
			end
			if unitID == 7 then
				return 0, 99
			end
			return 0, unitID == 2 and 0 or functionID
		end,
		GetUnitScriptPiece = function(_, piece)
			if piece == 0 or piece == 1 then
				return 5 - piece
			end
		end,
		GetUnitPieceInfo = function(_, modelPiece)
			return { name = "piece" .. modelPiece }
		end,
		CreateUnit = function(defID)
			assert(defID ~= 10, "unaffected definitions must not be spawned")
			if defID == 11 then
				return nil
			end
			live[defID] = true
			return defID
		end,
		GetGroundHeight = function()
			return 0
		end,
		GetGaiaTeamID = function()
			return 1
		end,
		MoveCtrl = { Enable = function() end },
		SetUnitBlocking = function() end,
		GiveOrderToUnit = function() end,
		ValidUnitID = function(unitID)
			return live[unitID] and unitID ~= 12
		end,
		GetUnitIsDead = function()
			return false
		end,
		DestroyUnit = function(unitID, explode, reclaim)
			assert(not explode and reclaim)
			live[unitID] = nil
		end,
		UnitScript = {
			CallAsUnit = function(unitID, callback, ...)
				activeUnit = unitID
				return callback(...)
			end,
			GetScriptEnv = function(unitID)
				if unitID == 4 then
					return {
						script = {
							QueryWeapon = function(slot)
								assert(activeUnit == 4 and slot == 1)
								return 1
							end,
							AimFromWeapon = function(slot)
								assert(activeUnit == 4 and slot == 1)
								return 2
							end,
						},
					}
				elseif unitID == 5 then
					return {
						script = {
							QueryWeapon1 = function(...)
								assert(activeUnit == 5 and select("#", ...) == 0)
								return 1
							end,
							AimFromWeapon1 = function(...)
								assert(activeUnit == 5 and select("#", ...) == 0)
								return 1
							end,
						},
					}
				end
			end,
		},
	}
	local loaded = dofile(scriptDirectory .. "dbg_aimfrom_audit.lua")
	if enabled ~= 1 then
		assert(loaded == false and gadget.GameFrame == nil)
		return lines
	end
	gadget:Initialize()
	for frame = 1, 500 do
		gadget:GameFrame(frame)
		if live[1] then
			assert(gadget:UnitPreDamaged(1, 1, 1, 100) == 0)
		end
		assert(gadget:UnitPreDamaged(999, 1, 1, 100) == 100)
	end
	assert(not live[1] and not live[4], "owned samples must be cleaned up")
	return lines
end

assert(#run(true, 0) == 0)
local rejected = table.concat(run(false, 1), "\n")
assert(rejected:find("abort\trequires cheats", 1, true))
local lines = run(true, 1)
local expected = {
	different = "different",
	same = "same",
	legacy = "different",
	lua_generic = "different",
	lua_numbered = "same",
	missing = "unknown",
	invalid = "unknown",
	yielding = "unknown",
	throwing = "unknown",
	mixed = "different",
}
local counts = {}
for _, line in ipairs(lines) do
	local name, slot, category =
		line:match("^AIMFROM_AUDIT\tweapon\t([^\t]+)\t(%d+)\t[^\t]+\t[^\t]+\t[^\t]+\t%d+\t([^\t]+)")
	if name then
		assert(category == expected[name], line)
		counts[name] = (counts[name] or 0) + 1
		if name == "mixed" then
			assert(slot == "2" or slot == "3")
		end
	end
end
for name in pairs(expected) do
	assert(counts[name] == (name == "mixed" and 4 or 2), name)
end
local output = table.concat(lines, "\n")
assert(output:find("unit\tunaffected\tunaffected-type", 1, true))
assert(output:find("unit\tcreate_failed\tcreate-failed", 1, true))
assert(output:find("unit\tlost\tlost-before-final-sample", 1, true))
assert(GG.AimFromAuditComplete)
print(
	"PASS: opt-in/cheat gates, all affected types, COB zero/aliases/status/errors, Lua dispatch/indexing, mixed slots, failures and cleanup"
)
