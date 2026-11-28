# Ghost aimpoint integration fixture

Requires a graphical Linux engine build, a BAR checkout with the companion ghost-target
widget, and development assets containing `Quicksilver Remake 1.24`. Uses an isolated
data directory, removed after each run; logs remain in the output directory. No production
installation is modified. The script starts and exits the engine automatically.

```sh
python3 test/validation/ghost_target/run.py \
  --engine /path/to/build-amd64-linux/spring \
  --bar /path/to/Beyond-All-Reason \
  --assets /path/to/development/data \
  --output /tmp/ghost-target-candidate
```

Pass `--baseline` with an unmodified engine to reproduce the original ground-height
behavior. Both builds clamp a raw positional attack to Y=200, including a supplied height flag.
BAR resolves validated sightings to their saved aim height, including Y=280.
The nuclear weapon exception stays at Y=200. The candidate also exercises BAR's widget through its click/command callins and the real Ignore Self ray wrapper.

The graphical checks cover ordinary unit selection, live ghosts, hidden aimpoint changes,
hidden destruction, clearing a dead ghost by restoring vision, and default ray behavior.
The fixture moves the building out of view before a hover and again before hidden destruction. The full XYZ aimpoint is checked, including a horizontal offset. These checks simulate
input through Lua callins; they do not replace a human playtest with physical mouse input.

Manual review: reveal an enemy building, lose both LOS and radar, and issue A-click and
Set Target-click with a direct-fire unit. Check the command endpoint and fire above low
walls. Repeat with Shift and queue insertion, and confirm area drags and empty-ground
clicks keep their prior behavior. Minimap clicks keep their existing handling.

Radar coverage checks the two results for both picks: the icon returns `"unit", unitID`
and the remembered model returns `"ghost", unitID` on radar. Both clicks issue a
unit-targeted order. After radar loss, fresh Attack, inserted Attack and Set
Target orders addressed to that unit ID must use each allyteam's frozen XYZ.
The fixture checks the inserted command's queue position and options.

## Lua hover and engine fallback

Use `--lua-render` to load BAR's Ghost Hover widget and shader. The fixture
positions a simulated cursor on the model during Attack and checks the widget
draws the matching live, radar and destroyed sightings. Between hovers it checks
that the widget returns false so the engine draws the ghost. The nine screenshots
in `screenshots/` capture the ordinary appearance and the Lua hover phases.
Repeat with `--glsl` for the legacy renderer, and omit `--lua-render` to see the
engine appearance at the same camera position throughout the scenario.

## Synced sightings for two observers

The target belongs to team 1. Observers 0 and 2 lose LOS on different frames
and remember aimpoints (2060, 280, 2056) and (2100, 400, 2080). The gadget
queries both allyteams before and after hidden destruction. Revealing the site
to observer 0 clears only its sighting. A second unit leaves two sightings
at the same XZ, and regaining LOS clears only the attached one.
Two mobile probes leave ghosts at neighbouring sites. One moves away under fog,
the other changes owner under fog and is destroyed. When a scout sees both empty sites, both IDs must
expire even though the surviving probe remains outside the scout's LOS.

Repeat the command above with `--observer 2 --glsl` to compare the second
observer's mouse trace and `DrawGhost` callback against its getter. This run also compares the
minimap icon pixels before and after the hidden transfer, after destruction, and after the
site is revealed. It exercises LuaUI's allyteam access
restriction. To run the synced assertions alone, use `--headless` and pass the
`spring-headless` executable. Add `--save-load` to restore the game from a save
made after hidden destruction and check both observers again.


## Command identity and contact transitions

The synced fixture sends direct and inserted requests, including foreign, fractional,
expired and malformed IDs and an embedded height flag. It reads the actual attack
queue and BAR's Set Target list. A tag-relative insertion between two moves must
retain both neighbouring tags and the shift option while the middle command changes
from a three-coordinate attack to a unit attack on radar, back on radar loss, and
again on LOS. If a second gadget vetoes the inserted unit attack during radar
recontact, the original XYZ order keeps its tag until a later contact succeeds.
A subsequent sighting of that unit must have a new ID.
Two sightings with identical XYZ and the same historical unit ID verify that only
the attached sighting converts. The graphical fixture also sends an elevated
insert and Set Target directly from LuaUI and reads back their clamped heights.

## Default command height policy

Use the headless command with `--height-policy default` to exercise the stock
LuaRules handler and the compatibility gadget from the built base archive.
Repeat with `--height-policy override` to load BAR's file override in the same
fixture. Both runs inspect queued Attack and Manual Fire orders, insert by
position and by tag, and check that the other option bits survive. The fixture
then sends direct and inserted attacks from player 0 via the local server. It
checks the single validation and acceptance of each order, the issuer and source
flags, and observes the mobile units dropping their targets after the two distinct
absolute timeouts. The ordinary
BAR scenario above covers its own synced height validation and ghost targeting.
