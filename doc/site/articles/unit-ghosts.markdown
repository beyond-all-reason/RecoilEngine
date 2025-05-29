-- Immobile buildings by default have no radar wobble and leave ghosts even after losing los and/or radar (live ghosts).
-- When they die they also leave a "dead ghost" behind if they were not under los or radar.
-- Other units by default will also show a ghost when losing los of them but still under radar.
--
-- This behaviour is actually controlled through a unit and unitDef property: leavesGhost, and isn't really tied to buildings or other units specifically.
--
-- Its on by default for buildings (IsImmobileUnit() && !yardmap.empty()), and off for other units.
-- - a building is a unit with ud.isImmobile (equalling an unitdef with: `!yardmap.empty && pathType == -1U && !canfly && speed <= 0.0f`)
--
-- Live ghosts of 'leavesGhost' units will stay even after los and radar is lost.
--   - this is because they are assumed to be mostly immobile, so ghost stays to help the player remember its position
-- Live ghosts of 'not leavesGhost' units will dissapear when losing radar coverage.
--   - this is because they're assumed to be moving, so the player doesn't know where they are after they are gone
--
-- Immobile non-building units don't do this by default, but the behaviour can be enabled by setting leavesGhost at unitDef.

-- The problem with 'live ghosts' is they will follow the unit if it gets moved, so when moving, immobile units
-- `SetUnitLeavesGhost(leavesGhost, leaveDeadGhost)` needs to be used to toggle ghosting behaviour, otherwise you get a ghost moving and showing the unit's position!.
--
-- When setting leavesGhost from true to false:
--   If not under radar or los, the live ghost will dissapear, and optionally a dead ghost will be left behind.
--   If under radar, but no los, the radar position will start wobbling again.
--   If leaveDeadGhost is used, the unit will stop showing a live ghost for now, and leave a dead ghost behind. (maybe only when no los or radar????)


