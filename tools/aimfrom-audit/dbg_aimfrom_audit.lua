function gadget:GetInfo()
	return {
		name = "AimFrom compatibility audit",
		desc = "Opt-in AimFrom/Query script-piece sampling",
		author = "Codex",
		layer = -100000,
		enabled = true,
	}
end

if not gadgetHandler:IsSyncedCode() then
	return false
end

if tostring(Spring.GetModOptions().aimfromaudit) ~= "1" then
	return false
end

local affected = { Cannon = true, MissileLauncher = true, StarburstLauncher = true }
local legacy = { "Primary", "Secondary", "Tertiary" }
local pending, definitions = {}, {}
local owned = {}
local cursor, finished = 1, false

local function emit(...)
	local fields = { ... }
	for index, value in ipairs(fields) do
		fields[index] = tostring(value):gsub("[\t\r\n]", " ")
	end
	Spring.Echo("AIMFROM_AUDIT\t" .. table.concat(fields, "\t"))
end

local function query(unitID, weaponNum, prefix, environment)
	local ok, piece
	if environment then
		local script = environment.script or {}
		local callback = script[prefix]
		local numbered = false
		if type(callback) ~= "function" then
			callback = script[prefix .. weaponNum]
			numbered = true
		end
		if type(callback) ~= "function" then
			return nil, "missing"
		end
		if numbered then
			ok, piece = pcall(Spring.UnitScript.CallAsUnit, unitID, callback)
		else
			ok, piece = pcall(Spring.UnitScript.CallAsUnit, unitID, callback, weaponNum)
		end
		if ok and type(piece) == "number" then
			piece = piece - 1
		end
	else
		local name = prefix .. weaponNum
		local functionID = Spring.GetCOBScriptID(unitID, name)
		if not functionID and legacy[weaponNum] then
			name = (prefix == "QueryWeapon" and "Query" or "AimFrom") .. legacy[weaponNum]
			functionID = Spring.GetCOBScriptID(unitID, name)
		end
		if not functionID then
			return nil, "missing"
		end
		local status
		ok, status, piece = pcall(Spring.CallCOBScript, unitID, functionID, 1, -1)
		if not ok then
			return nil, "error:" .. tostring(status)
		end
		if ok and status ~= 0 then
			return nil, "unfinished:" .. tostring(status)
		end
	end
	if not ok then
		return nil, "error:" .. tostring(piece)
	end
	if type(piece) ~= "number" or piece < 0 or piece ~= math.floor(piece) then
		return nil, "invalid:" .. tostring(piece)
	end
	local modelPiece = Spring.GetUnitScriptPiece(unitID, piece)
	if not modelPiece then
		return nil, "unmapped:" .. piece
	end
	local info = Spring.GetUnitPieceInfo(unitID, modelPiece)
	return modelPiece, (info and info.name or "?") .. ":script=" .. piece .. ":model=" .. modelPiece
end

local function sample(entry, frame)
	local unitID, definition = entry.unitID, UnitDefs[entry.defID]
	local environment = Spring.UnitScript.GetScriptEnv and Spring.UnitScript.GetScriptEnv(unitID)
	for weaponNum, weapon in ipairs(definition.weapons) do
		local weaponDef = WeaponDefs[weapon.weaponDef]
		if affected[weaponDef.type] then
			local queryPiece, queryInfo = query(unitID, weaponNum, "QueryWeapon", environment)
			local aimPiece, aimInfo = query(unitID, weaponNum, "AimFromWeapon", environment)
			local category = "unknown"
			if queryPiece and aimPiece then
				category = queryPiece ~= aimPiece and "different" or "same"
			end
			emit(
				"weapon",
				definition.name,
				weaponNum,
				weaponDef.name,
				weaponDef.type,
				environment and "Lua" or "COB",
				frame - entry.created,
				category,
				queryInfo,
				aimInfo
			)
		end
	end
end

function gadget:Initialize()
	for defID in pairs(UnitDefs) do
		definitions[#definitions + 1] = defID
	end
	table.sort(definitions)
	emit("begin", #definitions, Game.gameVersion or "unknown", Engine.version or "unknown")
end

function gadget:UnitPreDamaged(unitID, unitDefID, unitTeam, damage)
	if owned[unitID] then
		return 0, 0
	end
	return damage, 1
end

function gadget:GameFrame(frame)
	if finished or frame < 2 then
		return
	end
	if not Spring.IsCheatingEnabled() then
		if frame < 300 then
			return
		end
		emit("abort", "requires cheats and a disposable test game")
		finished = true
		return
	end
	for index = #pending, 1, -1 do
		local entry = pending[index]
		local age = frame - entry.created
		if not Spring.ValidUnitID(entry.unitID) or Spring.GetUnitIsDead(entry.unitID) then
			emit("unit", UnitDefs[entry.defID].name, "lost-before-final-sample")
			owned[entry.unitID] = nil
			table.remove(pending, index)
		elseif age == 1 or age == 30 then
			sample(entry, frame)
			if age == 30 then
				Spring.DestroyUnit(entry.unitID, false, true)
				owned[entry.unitID] = nil
				table.remove(pending, index)
			end
		end
	end
	if #pending == 0 then
		local created = 0
		while cursor <= #definitions and created < 8 do
			local defID = definitions[cursor]
			cursor = cursor + 1
			local definition, candidate = UnitDefs[defID], false
			for _, weapon in ipairs(definition.weapons) do
				candidate = candidate or affected[WeaponDefs[weapon.weaponDef].type] == true
			end
			if not candidate then
				emit("unit", definition.name, "unaffected-type")
			else
				local positionX = Game.mapSizeX * 0.5 + created * 128
				local positionZ = Game.mapSizeZ * 0.5
				local unitID = Spring.CreateUnit(
					defID,
					positionX,
					Spring.GetGroundHeight(positionX, positionZ),
					positionZ,
					0,
					Spring.GetGaiaTeamID()
				)
				if unitID then
					owned[unitID] = true
					Spring.MoveCtrl.Enable(unitID)
					Spring.SetUnitBlocking(unitID, false)
					Spring.GiveOrderToUnit(unitID, CMD.FIRE_STATE, { 0 }, 0)
					pending[#pending + 1] = { unitID = unitID, defID = defID, created = frame }
					emit("unit", definition.name, "candidate", definition.scriptName or "unknown")
				else
					emit("unit", definition.name, "create-failed")
				end
				created = created + 1
			end
		end
		if cursor > #definitions and #pending == 0 then
			emit("complete", #definitions)
			GG.AimFromAuditComplete = true
			finished = true
		end
	end
end
