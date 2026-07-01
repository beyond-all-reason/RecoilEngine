function gadget:GetInfo()
  return {
    name      = "Dead Unit",
    desc      = "Remove behaviours from dead units",
    license   = "GNU GPL, v2 or later",
    layer     = -1999999,
    enabled   = true,
  }
end

if gadgetHandler:IsSyncedCode() then
  return
end

function gadget:UnitDestroyed(unitID, unitDefID, unitTeam, attackerID, attackerDefID, attackerTeam)
  Engine.Unsynced.SetUnitNoSelect(unitID, true)
  Engine.Unsynced.SetUnitNoGroup(unitID, true)
end
