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
behavior. The same fixture expects positional attacks at Y=200 on baseline and Y=280
with the change. Explicit ground commands and the nuclear weapon exception stay at
Y=200. The candidate also exercises BAR's widget through its click/command callins and the real Ignore Self ray wrapper.

The graphical checks cover ordinary unit selection, live ghosts, hidden aimpoint changes,
hidden destruction, clearing a dead ghost by restoring vision, and default ray behavior.
The full XYZ aimpoint is checked, including a horizontal offset. These checks simulate
input through Lua callins; they do not replace a human playtest with physical mouse input.

Manual review: reveal an enemy building, lose both LOS and radar, and issue A-click and
Set Target-click with a direct-fire unit. Check the command endpoint and fire above low
walls. Repeat with Shift and queue insertion, and confirm area drags and empty-ground
clicks keep their prior behavior. Minimap targeting and automatic target persistence are
outside this patch.

Radar coverage checks that the icon remains a unit target while the remembered model
resolves to that same unit target outside the icon, with its frozen aimpoint supplied only for highlighting. Seven screenshots in the output
`screenshots/` directory show live, radar and dead ghosts before and during the
highlight, followed by the reset appearance. Highlighting uses the stored aimpoint, never a hidden unit ID.

## Ghost RGBA colors

Add `--colors` to run the `ghost_colors.lua` test widget. It checks all four
state names, component boundaries, invalid input, and reset calls. The screenshots
use green normal ghosts, red highlighted ghosts, blue radar ghosts and magenta
highlighted radar ghosts, with explicit alpha values. At logical frame 230 it resets all
states; the final screenshot must match the default ghost appearance. Run again
without `--colors` for comparison, and with `--glsl` to exercise the GLSL path.

Manual procedure: reveal an enemy building, lose LOS, then compare its model with
and without continuous radar contact and hover. Check all four colors in both
renderers. Change `UnitTransparency`: default normal/radar opacity must respond,
while explicit Lua alpha must remain unchanged. Reset each state with
`Spring.SetGhostColor(state)` and verify the default appearance returns.
The widget can also be installed temporarily in a development game's LuaUI Widgets
folder and enabled manually; shutdown restores the engine defaults.

Scenario phases use six simulation frames per logical frame at speed 1, leaving
time for synchronous screenshot capture and ray sweeps between radar transitions.
