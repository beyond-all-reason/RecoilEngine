function gadget:GetInfo()
  return {
    name      = "LeavesGhost Updater",
    desc      = "Removes radar icon wobble for mobile units with leavesGhost and makes them leave 'dead ghosts' behind",
    layer     = 0,
    enabled   = true
  }
end


if (gadgetHandler:IsSyncedCode()) then
  local isTransportable = {}
  for unitDefID, unitDef in pairs(UnitDefs) do
    if not unitDef.cantBeTransported and unitDef.isImmobile and unitDef.leavesGhost then
      isTransportable[unitDefID] = true
    end
  end

  function gadget:UnitLoaded(unitID, unitDefID, unitTeam, transportID, transportTeam)
    if isTransportable[unitDefID] then
      Spring.Echo("Pick up", unitID)
      -- remove leavesGhost, and leave dead ghost behind (if no radar and had a live ghost)
      Spring.SetUnitLeavesGhost(unitID, false, true)
    end
  end
  function gadget:UnitUnloaded(unitID, unitDefID, unitTeam, transportID, transportTeam)
    if isTransportable[unitDefID] then
      Spring.Echo("Drop", unitID)
      -- reinstate leavesGhost
      Spring.SetUnitLeavesGhost(unitID, true)
    end
  end
end

