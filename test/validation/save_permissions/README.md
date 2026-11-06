# Save permissions regression

Run the interactive engine against an isolated LuaUI driver. A graphical session
is required. The driver dispatches both `save` and `luasave` through
`Spring.SendCommands`, using both string and table arguments. It checks new files
and `-y` overwrites separately.

```sh
python3 test/validation/save_permissions/run.py build-amd64-linux/spring \
  --bar /path/to/Beyond-All-Reason --assets /path/to/development-data \
  --results /path/to/new-results-directory
```

Use `--baseline` with an unmodified engine to require that the multiplayer leak
is reproduced. Run the same command without that flag against the fixed engine.

The four setups cover a local single-player host and a host with an additional
player slot, with cheats disabled and enabled. The extra slot is deliberately
not connected: multiplayer saves must already be refused before another player
joins. Logs and a JSON summary are retained in the results directory. Temporary
game overlays and save files are removed even when a check fails. Unchanged BAR,
map and engine base assets are linked, never modified.

The policy in `CGame::Save` also refuses remote clients without cheats, even if
their player list contains only one entry. Neither spectator status nor a replay
provides an exception. Local one-player games remain saveable without cheats;
multiplayer replays require cheats like other multiplayer sessions.

The console commands and the quit-dialog save button use the same guarded
`CGame::Save` entry point. The driver covers command dispatch from LuaUI, not
physical keyboard input or clicking the quit dialog.
