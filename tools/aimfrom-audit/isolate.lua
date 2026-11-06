function gadget:GetInfo()
	return { name = "Audit isolation harness", layer = -99999, enabled = true, handler = true }
end

if not gadgetHandler:IsSyncedCode() then
	function gadget:Initialize()
		gadgetHandler.actionHandler.AddSyncAction(gadget, "aimfrom_audit_cheat", function()
			Spring.SendCommands("cheat")
			Spring.SendCommands("setspeed 100")
		end)
		gadgetHandler.actionHandler.AddSyncAction(gadget, "aimfrom_audit_quit", function()
			Spring.Quit()
		end)
	end
	function gadget:GameFrame() end
	return
end

function gadget:GameFrame(frame)
	if frame == 1 then
		if not Spring.IsCheatingEnabled() then
			SendToUnsynced("aimfrom_audit_cheat")
		end
		local remove = {}
		local audit = gadgetHandler:FindGadget("AimFrom compatibility audit")
		local scripts = gadgetHandler:FindGadget("Lua unit script framework")
		for _, loaded in ipairs(gadgetHandler.gadgets) do
			if loaded ~= self and loaded ~= audit and loaded ~= scripts then
				remove[#remove + 1] = loaded
			end
		end
		for _, loaded in ipairs(remove) do
			gadgetHandler:RemoveGadget(loaded)
		end
		for _, unitID in ipairs(Spring.GetAllUnits()) do
			Spring.DestroyUnit(unitID, false, true)
		end
	end
	if GG.AimFromAuditComplete or frame > 20000 then
		SendToUnsynced("aimfrom_audit_quit")
	end
end
