-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return { name = "Factory callback regression", desc = "Destroy units during factory completion", author = "OpenAI Codex", layer = 1000000, enabled = true }
end
if not gadgetHandler:IsSyncedCode() then
	function gadget:RecvFromSynced(name)
		if name == "factory_callback_quit" then Spring.SendCommands("quitforce") end
	end
	return
end

local mode = Spring.GetModOptions().factorycallbackcase
local factory, buildee, finishedFrame
local fromFactory = 0
local done = false
local function alive(id)
	return Spring.ValidUnitID(id) and not Spring.GetUnitIsDead(id)
end
function gadget:UnitCreated(id, def, team, builder)
	if factory and builder == factory then buildee = id end
end
function gadget:UnitFinished(id)
	if id ~= buildee then return end
	finishedFrame = Spring.GetGameFrame()
	Spring.Echo("[FactoryCallback] finished " .. mode)
	if mode == "cancel" then
		local command = assert(Spring.GetFactoryCommands(factory, 1)[1])
		Spring.GiveOrderToUnit(factory, CMD.REMOVE, { command.tag }, CMD.OPT_CTRL)
	end
	if mode == "buildee" or mode == "both" then Spring.DestroyUnit(buildee, false, true) end
	-- Damage uses the factory's KillUnit override, as the replay's explosion does.
	if mode == "factory" or mode == "both" then Spring.AddUnitDamage(factory, 1000000) end
end
function gadget:UnitFromFactory(id, def, team, factoryID)
	if id ~= buildee then return end
	assert(factoryID == factory)
	fromFactory = fromFactory + 1
end
function gadget:GameFrame(frame)
	if done then return end
	if frame == 1 then
		Spring.SetTeamResource(0, "ms", 1000000)
		Spring.SetTeamResource(0, "es", 1000000)
		Spring.SetTeamResource(0, "m", 1000000)
		Spring.SetTeamResource(0, "e", 1000000)
		factory = assert(Spring.CreateUnit("corlab", 1800, Spring.GetGroundHeight(1800, 1800), 1800, 0, 0))
		Spring.SetUnitBuildSpeed(factory, 100000)
		Spring.GiveOrderToUnit(factory, -UnitDefNames.corak.id, {}, 0)
	end
	if finishedFrame and frame >= finishedFrame + 10 then
		local ok, err = pcall(function()
			assert(fromFactory == (mode == "normal" and 1 or 0), "unexpected UnitFromFactory count: " .. fromFactory)
			assert(alive(factory) == (mode == "normal" or mode == "buildee" or mode == "cancel"), "unexpected factory survival")
			assert(alive(buildee) == (mode == "normal" or mode == "cancel"), "unexpected buildee survival")
		end)
		Spring.Echo("[FactoryCallback] " .. (ok and "PASS " .. mode or "FAIL " .. tostring(err)))
		done = true
		SendToUnsynced("factory_callback_quit")
	elseif frame > 900 then
		Spring.Echo("[FactoryCallback] FAIL factory did not finish within 900 frames")
		done = true
		SendToUnsynced("factory_callback_quit")
	end
end
