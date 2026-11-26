function widget:GetInfo()
	return {
		name    = "AllianceMessages",
		desc    = "Prints a message when /ally changes an alliance with your ally team",
		date    = "Nov, 2026",
		license = "GNU GPL, v2 or later",
		layer   = 0,
		enabled = true,
	}
end

-- After "/ally <allyTeam> <0|1>", every client gets TeamChanged with the sender's team.
-- The alliance is one-way: only the sender's ally team changes its entry for the other ally team.

local allyTeamList = {}
local teamOfAllyTeam = {} -- one team of each ally team, for Spring.AreTeamsAllied
local allied = {} -- allied[a][b] is true when ally team a allies ally team b

local function IsAllied(allyTeamA, allyTeamB)
	-- AreTeamsAllied(t1, t2) reads its arguments in reverse order: it returns whether t2's ally team allies t1's.
	return Spring.AreTeamsAllied(teamOfAllyTeam[allyTeamB], teamOfAllyTeam[allyTeamA])
end

local function ReadAlliances(allyTeamID)
	local row = {}
	for _, otherAllyTeamID in ipairs(allyTeamList) do
		if otherAllyTeamID ~= allyTeamID then
			row[otherAllyTeamID] = IsAllied(allyTeamID, otherAllyTeamID)
		end
	end
	return row
end

function widget:Initialize()
	for _, allyTeamID in ipairs(Spring.GetAllyTeamList()) do
		local teamList = Spring.GetTeamList(allyTeamID)
		if teamList and teamList[1] then
			allyTeamList[#allyTeamList + 1] = allyTeamID
			teamOfAllyTeam[allyTeamID] = teamList[1]
		end
	end
	for _, allyTeamID in ipairs(allyTeamList) do
		allied[allyTeamID] = ReadAlliances(allyTeamID)
	end
end

function widget:TeamChanged(teamID)
	local _, _, _, _, _, fromAllyTeam = Spring.GetTeamInfo(teamID, false)
	local before = allied[fromAllyTeam]
	if not before then
		return
	end

	local now = ReadAlliances(fromAllyTeam)
	allied[fromAllyTeam] = now

	local myAllyTeam = Spring.GetMyAllyTeamID() -- a spectator gets the ally team they view
	for _, whichAllyTeam in ipairs(allyTeamList) do
		if now[whichAllyTeam] ~= before[whichAllyTeam] then
			local change = now[whichAllyTeam] and "allied" or "unallied"
			if fromAllyTeam == myAllyTeam then
				Spring.Echo("Alliance: you have " .. change .. " allyteam " .. whichAllyTeam .. ".")
			elseif whichAllyTeam == myAllyTeam then
				Spring.Echo("Alliance: allyteam " .. fromAllyTeam .. " has " .. change .. " with you.")
			end
		end
	end
end
