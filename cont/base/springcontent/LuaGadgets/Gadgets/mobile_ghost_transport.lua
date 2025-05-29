function gadget:GetInfo()
  return {
    name      = "LeavesGhost Updater",
    desc      = "Removes radar icon wobble for mobile units with leavesGhost",
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
      Spring.SetUnitLeavesGhost(unitID, false, true)
    end
  end
  function gadget:UnitUnloaded(unitID, unitDefID, unitTeam, transportID, transportTeam)
    if isTransportable[unitDefID] then
      Spring.Echo("Drop", unitID)
      Spring.SetUnitLeavesGhost(unitID, true)
    end
  end
end

