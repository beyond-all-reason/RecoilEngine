-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return { name = "Factory creation regression", desc = "Factory creation lifetime", author = "OpenAI Codex", layer = 1000000, enabled = true }
end
if not gadgetHandler:IsSyncedCode() then
	function gadget:RecvFromSynced(name)
		if name == "factory_callback_quit" then Spring.SendCommands("quitforce") end
	end
	return
end
local mode = Spring.GetModOptions().factorycallbackcase
local factory, first, triggered, triggerFrame
local created, destroyed = {}, {}
local function alive(id)
	return id and Spring.ValidUnitID(id) and not Spring.GetUnitIsDead(id)
end
function gadget:UnitCreated(id, def, team, builder)
	if not factory or builder ~= factory then return end
	created[#created + 1] = id
	Spring.Echo('[FactoryCreation] created', id, def, Spring.GetGameFrame())
	if triggered then return end
	triggered, first, triggerFrame = true, id, Spring.GetGameFrame()
	if mode == 'lua-created' then Spring.DestroyUnit(factory, false, true) end
	if mode == 'alt-created' then Spring.GiveOrderToUnit(factory, -UnitDefNames.corthud.id, {}, CMD.OPT_ALT) end
end
function gadget:UnitDestroyed(id)
	destroyed[id] = true
	if id == factory then
		Spring.Echo('[FactoryCreation] factory died', Spring.GetGameFrame())
		triggerFrame = Spring.GetGameFrame()
	end
end
function gadget:GameFrame(frame)
	if frame == 1 then
		Spring.SetTeamResource(0, 'ms', 1000000)
		Spring.SetTeamResource(0, 'es', 1000000)
		Spring.SetTeamResource(0, 'm', 1000000)
		Spring.SetTeamResource(0, 'e', 1000000)
		factory = assert(Spring.CreateUnit('corlab', 1800, Spring.GetGroundHeight(1800,1800), 1800, 0, 0))
		Spring.SetUnitBuildSpeed(factory, 100)
		Spring.GiveOrderToUnit(factory, -UnitDefNames.corak.id, {}, 0)
	end
	if triggerFrame and frame >= triggerFrame + 30 then
		local ok, err = pcall(function()
			if mode == 'normal' then
				assert(alive(factory) and alive(first), 'normal build lost')
			elseif mode == 'alt-created' then
				assert(alive(factory), 'factory unexpectedly died')
				assert(not alive(first), 'cancelled nanoframe survived')
			else
				assert(not alive(factory), 'factory did not die')
				for _, id in ipairs(created) do
					Spring.Echo('[FactoryCreation] survivor', id, alive(id))
					assert(not alive(id), 'orphan nanoframe survived: '..id)
				end
			end
		end)
		Spring.Echo('[FactoryCallback] '..(ok and 'PASS '..mode or 'FAIL '..tostring(err)))
		SendToUnsynced('factory_callback_quit')
	elseif frame > 1800 then
		Spring.Echo('[FactoryCallback] FAIL timeout '..mode)
		SendToUnsynced('factory_callback_quit')
	end
end
