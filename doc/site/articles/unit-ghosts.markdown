## Unit Ghosts

Immobile buildings by default have no radar wobble once spotted, and leave `live ghosts` even after losing los and/or radar.

When they die they also leave a `dead ghost` behind, if previously spotted and not under los or radar at that moment.

Other units by default will also show a ghost when losing los but still under radar.

### The leavesGhost property

This behaviour is actually controlled through a unit and unitDef property: leavesGhost, and isn't really tied to buildings or other units specifically.

It controls the following behaviour:

- leavesGhost: on
   - Default for buildings.
   - Ghosts will stay even after los and radar is lost, if previously spotted.
      - This is because they are assumed to be mostly immobile, so ghost stays to help the player remember its position.
   - No radar wobble once spotted.
- leavesGhost: off
   - Default for non building units (even if immobile!).
   - Ghosts will dissapear when losing radar coverage.
      - This is because they're assumed to be moving, so the player doesn't know where they are after they are gone.
   - Standard radar wobble behaviour.

So, immobile non-building units don't do this by default, but the behaviour can be enabled by setting leavesGhost at unitDef.

### leavesGhost units who move

Even when leavesGhost is enabled, and unit is marked as immobile at its unitDef, it can still be moved around by transports and lua.

The problem with 'live ghosts' is by default (c++ implementation) they will follow the unit if it gets moved, so when moving you will see the ghost following the unit around with no wobble, even when out of radar!!

Recoil provides an example gadget [mobile_ghost_transport.lua](cont/base/springcontent/LuaGadgets/Gadgets/mobile_ghost_transport.lua) to make default behaviour reasonable. That gadget takes care to enable and disable `leavesGhost` for the unit when it hops on and off transports.

If you want even more control, because you implemented other methods to move immobile units (like teleportation for example), you will need to tweak it or use `SetUnitLeavesGhost` in your own gadgets.

This requires switching leavesGhost `off` before moving, and back `on` again after having moved.

### Internal details of switching leavesGhost on and off

It's all you need to know, but here are the internal details of how it works.

When setting leavesGhost `off`:

- If not under radar or los, the `live ghost` will dissapear, and optionally a `dead ghost` will be left behind.
   - If `leaveDeadGhost` at `SetUnitLeavesGhost` is used, the unit will stop showing a live ghost and leave a dead ghost behind when not under los or radar.
- If under radar, but no los, the radar position will start wobbling again.

When setting leavesGhost back `on`:

- PREVLOS for the unit will be removed unless in LOS or CONTRADAR.


## Further details

Units generating a dead ghost for an ally team will lose their `PREVLOS` for that ally team.

## ModOptions

Ghost behaviour can be disabled altogether throught the `GAME\\ModOptions\\GhostedBuildings` property.

## References

- Spring.SetUnitLeavesGhost
- Spring.GetUnitLeavesGhost
- [mobile_ghost_transport.lua](cont/base/springcontent/LuaGadgets/Gadgets/mobile_ghost_transport.lua)

## Glossary

- Live ghost icon: A ghost icon for a unit who was previously seen an in continuous radar coverage.
- Dead ghost icon: An independent unit ghost, usually for dead units, but also for units who sneaked away through SetUnitLeavesGhost.
- Building: A unit having and empty yardmap, and also Immobile. Usually identified through unitDef.isBuilding.
- Immobile unit: A unit having `pathType == -1U && !canfly && speed <= 0.0f`. Usually identified through unitDef.isImmobile.
