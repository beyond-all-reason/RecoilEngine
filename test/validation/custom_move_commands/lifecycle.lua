-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
-- Replaces LuaRules/main.lua and draw.lua to exercise whole-environment reloads.

local commandID = 34567
local abandonedID = commandID + 1
local shutdownID = commandID + 2

if Script.GetSynced() then
	local phase = Spring.GetGameRulesParam("registration_phase") or 0
	local requested = false

	function Initialize()
		if phase == 0 then
			Spring.RegisterCommand(commandID, { movement = true })
			Spring.RegisterCommand(abandonedID, {})
		else
			-- Both true and false registrations must be removed on full reload.
			Spring.RegisterCommand(commandID, { movement = false })
			Spring.RegisterCommand(abandonedID, { movement = true })
			Spring.RegisterCommand(shutdownID, {})
			assert(not pcall(Spring.RegisterCommand, commandID, {}), "Accepted duplicate ID")
			assert(not pcall(Spring.RegisterCommand, commandID, { movement = true }), "Changed registered ID")
		end
	end

	function Shutdown()
		if phase == 0 then
			-- Cleanup must include registrations made during Shutdown.
			Spring.RegisterCommand(shutdownID, { movement = true })
		end
	end

	function GameFrame(frame)
		assert(frame < 900, "Registration lifecycle validation timed out")
		if requested then
			return
		end
		requested = true
		if phase == 0 then
			Spring.SetGameRulesParam("registration_phase", 1)
			SendToUnsynced("registration_reload")
		else
			Spring.SetGameRulesParam("registration_unsynced_requested", 1)
			SendToUnsynced("registration_reload_unsynced")
		end
	end

	function RecvLuaMsg(message)
		if message ~= "registration_unsynced_reloaded" then
			return
		end
		assert(phase == 1)
		-- Reloading only the unsynced half must preserve synced registrations.
		assert(not pcall(Spring.RegisterCommand, commandID, {}))
		assert(not pcall(Spring.RegisterCommand, abandonedID, { movement = true }))
		assert(not pcall(Spring.RegisterCommand, shutdownID, {}))
		SendToUnsynced("registration_complete")
	end
else
	function Initialize()
		if Spring.GetGameRulesParam("registration_unsynced_requested") == 1 then
			Spring.SendLuaRulesMsg("registration_unsynced_reloaded")
		end
	end

	function RecvFromSynced(action)
		if action == "registration_reload" then
			Spring.SendCommands({ "cheat", "luarules reload" })
		elseif action == "registration_reload_unsynced" then
			Spring.SendCommands("luarules reloadunsynced")
		elseif action == "registration_complete" then
			Spring.Echo("CUSTOM_MOVE_LIFECYCLE_COMPLETE")
			Spring.SendCommands("quitforce")
		end
	end
end

Initialize()
