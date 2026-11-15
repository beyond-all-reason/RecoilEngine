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
resolves to that same unit target outside the icon, with its frozen aimpoint supplied only for highlighting. Four screenshots in the output
`screenshots/` directory show live/dead ghosts before and during the translucent amber
highlight. Highlighting uses the stored aimpoint, never a hidden unit ID.
