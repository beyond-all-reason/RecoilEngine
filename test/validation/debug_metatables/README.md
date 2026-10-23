# Native userdata metatable access

The unsynced Lua library no longer exposes `debug.getregistry`,
`debug.getmetatable` or `debug.setmetatable`. These functions bypass protected
engine userdata metatables or expose the registry used by native type checks.
Diagnostic functions such as `debug.traceback` and `debug.getinfo` remain intact,
as do ordinary table metatables and engine-specific debug entries.

```sh
python3 test/validation/debug_metatables/run.py build-amd64-linux/spring \
  --bar /path/to/Beyond-All-Reason --assets /path/to/development-data \
  --results /path/to/new-results-directory
```

A graphical session is required. With `--baseline`, the driver requires the old
unsafe functions to be available. It demonstrates retagging a real engine Font
userdata without dereferencing it as another native type, then restores the
original metatable before deletion. The fixed-engine run requires those entry
points to be absent and checks normal font use, table metatables and tracebacks.
Both runs check LuaUI and the unsynced LuaRules and LuaGaia handles.

Logs are retained in the results directory. The temporary overlay is removed
after each run; unchanged development game, map and font assets are linked.

The common library change also affects LuaIntro and LuaMenu, which are not
exercised by this driver. Synced developer-mode debug loading is unchanged.
This is a focused fix for native userdata retagging, not a complete audit of
Lua debug introspection, widget environments or native userdata lifetimes.
