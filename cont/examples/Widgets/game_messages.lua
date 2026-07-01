function widget:GetInfo()
  return {
    name      = "TeamDiedMessages",
    desc      = "Prints a message when a team die",
    author    = "abma & zwzsg",
    date      = "Sep, 2013",
    license   = "GNU GPL, v2 or later",
    layer     = 5,
    enabled   = true
  }
end

local sec = "game_messages.lua" -- log section
local msgsfile = "gamedata/messages.tdf"

local luaMsgs = {}

function widget:Initialize()
	local TDF = VFS.Include('gamedata/parse_tdf.lua')
	if (not VFS.FileExists(msgsfile)) then
		Engine.Shared.Log(sec, LOG.INFO, "gamedata/messages.tdf doesn't exist.")
		return
	end

	local tdfMsgs, err = TDF.Parse(msgsfile)
	if (tdfMsgs == nil) then
		Engine.Shared.Log(sec, LOG.ERROR, 'Error parsing '.. msgsfile  .. err)
	end
	if (type(tdfMsgs.messages) ~= 'table') then
		Engine.Shared.Log(sec, LOG.ERROR, 'Missing "messages" section in ' .. msgsfile )
	end

	for label, msgs in pairs(tdfMsgs.messages) do
		if ((type(label) == 'string') and (type(msgs)  == 'table')) then
			local msgArray = {}
			for _, msg in pairs(msgs) do
				if (type(msg) == 'string') then
					table.insert(msgArray, msg)
				end
			end
			luaMsgs[label:lower()] = msgArray -- lower case keys
		end
	end
end

local function Translate(msg)
	msg = msg:lower()
	if luaMsgs and type(luaMsgs[msg]) == "table" then
		local msgs = luaMsgs[msg]
		return msgs[ math.random( #msgs ) ]
	end
	Engine.Shared.Log(sec, LOG.DEBUG, string.format("Missing translation for %s", msg))
	return msg
end

function widget:TeamDied(teamID)
	local playerNames
	local _, leaderPlayerId, isDead, isAiTeam = Engine.Shared.GetTeamInfo(teamID)
	if isAiTeam then
		local skirmishAIID, aiName, hostingPlayerID, shortName, version, options = Engine.Shared.GetAIInfo(teamID)
		if aiName~="UNKNOWN" and aiName:lower():sub(1,3)~="bot" and aiName:len()>1 then
			playerNames=aiName
		elseif shortName~="UNKNOWN" then
			playerNames=shortName
		elseif hostingPlayerID~=Engine.Unsynced.GetMyPlayerID() then
			local ownerName=Engine.Shared.GetPlayerInfo(hostingPlayerID)
			if ownerName then
				playerNames=ownerName.."'s bot"
			end
		end
	end
	for _,playerId in ipairs(Engine.Shared.GetPlayerList(teamID,true)) do
		playerNames=(playerNames and playerNames..", " or "")..Engine.Shared.GetPlayerInfo(playerId)
	end
	msg = string.format(Translate("Team%i(%s) is no more"), teamID, playerNames or "")
	Engine.Shared.Echo(msg)
end