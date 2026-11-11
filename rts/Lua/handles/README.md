# The Lua library per handle

`doc/site`'s `lua_library` task writes `generated/{shared,synced,unsynced}.lua` here: the same docs as
`rts/Lua/library/generated`, routed by the `@env` tag each source file carries (lua-doc-extractor's `--env`), with
the `*.header` of each file in front so its functions are members of `Recoil.Shared`, `Recoil.Synced` or
`Recoil.Unsynced`. `Spring.lua` declares `Recoil`, the union, and types `Spring` with it.

A game that wants the checker to refuse the other handle's functions lists this directory as a library in place
of `rts/Lua/library/generated` and `rts/Lua/library/Spring.lua`, and casts `Spring` to the handle's view at a file's
import. The website reads `rts/Lua/library` alone and does not change.

The top-level globals land in the same files by the same tags, the standard library each handle keeps among them:
`LuaLibs.cpp` documents what both handles open and what the unsynced handle adds (`io`, `os`, `debug`), and
`LuaHandleSynced.cpp` what the synced handle removes. A game that loads files into environments of its own can read
the names per handle off these files (emmylua_doc_cli's JSON lists each class's members and each file's globals)
and allow a file only its handle's.
