---
layout: default
title: VFS
parent: Lua API
permalink: lua-api/globals/VFS
---

{% raw %}

# global VFS


The Virtual File System is an unified layer to access (read-only) the
different archives used at runtime. So you can access map, game & config
files via the same interface.

## Overview

Although Spring can access the filesystem directly (via os module) it is
more common that you would want to access files included with your game or
Spring. Trouble is, most of these files are compressed into archives
(`.sdz`/`.sd7`) so random access would generally be a difficult procedure.
Fortunately, the Spring Lua system automatically provides access to mod and
base files via the VFS module.

As an additional caveat, synced Lua cannot use the `os` and `io` modules,
so using VFS is mandatory there to have any file access at all.

The VFS module doesn't simply open archives though. What it does is map
your game files, game dependencies and Spring content onto a virtual file
tree. All archives start from the 'roots' of the tree and share the same
virtual space, meaning that if two or more archives contain the same
resource file name the resources overlap and only one of the files will be
retrieved. Overlapping directories on the other hand are merged so the
resulting virtual directory contains the contents of both. Here is an
example of how this works:

**Archive 1** (`games/mygame.sd7`)

```
textures
└── texture1.png
models
└── model1.mdl
```

**Archive 2** (`base/springcontent.sdz`)

```
textures
├── texture1.png
├── texture2.png
└── texture3.png
```

**VFS**

```
textures
├── texture1.png
├── texture2.png
└── texture3.png
models
└── model1.mdl
```

This raises the question: If both archives have a `texture1.png` then which
`texture1.png` is retreived via the VFS? The answer depends on the order the
archives are loaded and the VFS mode (more on modes below). Generally
however, each archive loaded overrides any archives loaded before it. The
standard order of loading (from first to last) is:

1. The automatic dependencies `springcontent.sdz` and `maphelper.sdz`.
2. Dependencies listed in your `modinfo.lua` (or `modinfo.tdf`), in the order listed.
Note that they are loaded fully and recursively, i.e. all the deeper dependencies of the 1st base-level dependency are
loaded before the 2nd base-level dependency. This breaks the usual "loaded later overrides loaded earlier" priority if
a dependency comes from multiple places, since only the first time an archive is loaded counts.
3. Your mod archive.

Loose files (not within any archive) in the engine dir are also visible
as if under the VFS root if loading under the `VFS.RAW` mode, though you
can also use full FS path (i.e. `C:/.../Spring/foo/bar.txt` is visible
both as that and as just `foo/bar.txt`). Note that `VFS.RAW` is only
accessible to unsynced Lua, all synced states are limited to loaded archives.

## Paths

Spring's VFS is **lowercase only**. Also it is **strongly** recommended to
use linux style path separators, e.g. `"foo/bar.txt"` and not `"foo\bar.txt"`.

## Engine read files

The engine access a few files directly, most of them are lua files which
access other files themselves. Here the list of files that must exist in the
VFS (some of them don't have to be in the game/map archive cause there are
fallback solutions in `springcontent.sdz` & `maphelper.sdz`):

- `./`
- anims/
- `cursornormal.bmp/png`
- gamedata/
- `defs.lua`
- `explosions.lua`
- `explosion_alias.lua`
- `icontypes.lua`
- `messages.lua`
- `modrules.lua`
- `resources.lua`
- `resources_map.lua`
- `sidedata.lua`
- `sounds.lua`
- `luagaia/`
- `main.lua`
- `draw.lua`
- `luarules/`
- `main.lua`
- `draw.lua`
- `luaui/`
- `main.lua`
- `shaders/`
- `?`
- `luaai.lua`
- `mapinfo.lua`
- `mapoptions.lua`
- `modinfo.lua`
- `modoptions.lua`
- `validmaps.lua`

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L27-L145" target="_blank">source</a>]







---

## methods
---

### VFS.GetMaps
---
```lua
function VFS.GetMaps() -> mapNames string[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaArchive.cpp#L52-L55" target="_blank">source</a>]








### VFS.GetGames
---
```lua
function VFS.GetGames() -> gameNames string[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaArchive.cpp#L63-L66" target="_blank">source</a>]








### VFS.GetAllArchives
---
```lua
function VFS.GetAllArchives() -> archiveNames string[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaArchive.cpp#L82-L85" target="_blank">source</a>]








### VFS.HasArchive
---
```lua
function VFS.HasArchive() -> hasArchive boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaArchive.cpp#L101-L104" target="_blank">source</a>]








### VFS.GetLoadedArchives
---
```lua
function VFS.GetLoadedArchives() -> archiveNames string[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaArchive.cpp#L115-L118" target="_blank">source</a>]








### VFS.GetArchivePath
---
```lua
function VFS.GetArchivePath(archiveName: string) -> archivePath string?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaArchive.cpp#L129-L133" target="_blank">source</a>]








### VFS.GetArchiveInfo
---
```lua
function VFS.GetArchiveInfo(archiveName: string) -> archiveInfo ArchiveInfo?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaArchive.cpp#L169-L173" target="_blank">source</a>]








### VFS.GetArchiveDependencies
---
```lua
function VFS.GetArchiveDependencies(archiveName: string) -> archiveNames string[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaArchive.cpp#L214-L218" target="_blank">source</a>]








### VFS.GetArchiveReplaces
---
```lua
function VFS.GetArchiveReplaces(archiveName: string) -> archiveNames string[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaArchive.cpp#L232-L236" target="_blank">source</a>]








### VFS.GetArchiveChecksum
---
```lua
function VFS.GetArchiveChecksum(archiveName: string)
 -> singleArchiveChecksum string
 -> completeArchiveChecksum string

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaArchive.cpp#L254-L259" target="_blank">source</a>]








### VFS.GetNameFromRapidTag
---
```lua
function VFS.GetNameFromRapidTag(rapidTag: string) -> archiveName string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaArchive.cpp#L279-L283" target="_blank">source</a>]








### VFS.GetAvailableAIs
---
```lua
function VFS.GetAvailableAIs(
  gameArchiveName: string?,
  mapArichiveName: string?
) -> ais AIInfo[]
```





Gets a list of all Spring AIs. The optional gameName and mapName parameters
can be used to include game/map specific LuaAIs in the list.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaArchive.cpp#L305-L313" target="_blank">source</a>]








### VFS.Include
---
```lua
function VFS.Include(
  filename: string,
  environment: table?,
  mode: string?
) -> module any
```
@param `filename` - Path to file, lowercase only. Use linux style path separators, e.g.
`"foo/bar.txt"`.

@param `environment` - (Default: the current function environment)

The environment arg sets the global environment (see generic lua refs). In
almost all cases, this should be left `nil` to preserve the current env.

If the provided, any non-local variables and functions defined in
`filename.lua` are then accessable via env. Vise-versa, any variables
defined in env prior to passing to `VFS.Include` are available to code in the
included file. Code running in `filename.lua` will see the contents of env in
place of the normal global environment.

@param `mode` - VFS modes are single char strings and can be concatenated;
doing specifies an order of preference for the mode (i.e. location) from
which to include files.


@return `module` - The return value of the included file.





Loads and runs lua code from a file in the VFS.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L276-L310" target="_blank">source</a>]

The path is relative to the main Spring directory, e.g.

```lua
VFS.Include('LuaUI/includes/filename.lua', nil, vfsmode)
```








### VFS.LoadFile
---
```lua
function VFS.LoadFile(
  filename: string,
  mode: string?
) -> data string?
```
@param `filename` - Path to file, lowercase only. Use linux style path separators, e.g.
`"foo/bar.txt"`.

@param `mode` - VFS modes are single char strings and can be concatenated;
doing specifies an order of preference for the mode (i.e. location) from
which to include files.


@return `data` - The contents of the file.





Load raw text data from the VFS.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L394-L414" target="_blank">source</a>]

Returns file contents as a string. Unlike `VFS.Include` the file will not be
executed. This lets you pre-process the code. Use `loadstring` afterwards.








### VFS.FileExists
---
```lua
function VFS.FileExists(
  filename: string,
  mode: string?
) -> exists boolean
```
@param `filename` - Path to file, lowercase only. Use linux style path separators, e.g.
`"foo/bar.txt"`.

@param `mode` - VFS modes are single char strings and can be concatenated;
doing specifies an order of preference for the mode (i.e. location) from
which to include files.


@return `exists` - `true` if the file exists, otherwise `false`.





Check if file exists in VFS.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L445-L470" target="_blank">source</a>]

Example usage:

```lua
if VFS.FileExists("mapconfig/custom_lava_config.lua", VFS.MAP) then
  # ...
end
```








### VFS.DirList
---
```lua
function VFS.DirList(
  directory: string,
  pattern: string?,
  mode: string?,
  recursive: boolean?
) -> filenames string[]
```
@param `directory` - Path to directory, lowercase only. Use linux style path separators, e.g.
`"foo/bar/"`.

@param `pattern` - (Default: `"*"`)

@param `mode` - VFS modes are single char strings and can be concatenated;
doing specifies an order of preference for the mode (i.e. location) from
which to include files.

@param `recursive` - (Default: `false`)






List files in a directory.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L491-L518" target="_blank">source</a>]

Example usage:

```lua
local luaFiles = VFS.DirList('units/', '*.lua', nil, true)
```








### VFS.SubDirs
---
```lua
function VFS.SubDirs(
  directory: string,
  pattern: string?,
  mode: string?,
  recursive: boolean?
) -> dirnames string[]
```
@param `directory` - Path to directory, lowercase only. Use linux style path separators, e.g.
`"foo/bar/"`.

@param `pattern` - (Default: `"*"`)

@param `mode` - VFS modes are single char strings and can be concatenated;
doing specifies an order of preference for the mode (i.e. location) from
which to include files.

@param `recursive` - (Default: `false`)






List sub-directories in a directory.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L550-L580" target="_blank">source</a>]

Example usage:

```lua
local files = VFS.SubDirs('sounds/voice/' .. language, '*')
for _, file in ipairs(files) do
    # ...
end
```








### VFS.GetFileAbsolutePath
---
```lua
function VFS.GetFileAbsolutePath(
  filename: string,
  mode: string?
) -> absolutePath string?
```
@param `filename` - Path to file, lowercase only. Use linux style path separators, e.g.
`"foo/bar.txt"`.

@param `mode` - VFS modes are single char strings and can be concatenated;
doing specifies an order of preference for the mode (i.e. location) from
which to include files.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L608-L623" target="_blank">source</a>]








### VFS.GetArchiveContainingFile
---
```lua
function VFS.GetArchiveContainingFile(
  filename: string,
  mode: string?
) -> archiveName string?
```
@param `filename` - Path to file, lowercase only. Use linux style path separators, e.g.
`"foo/bar.txt"`.

@param `mode` - VFS modes are single char strings and can be concatenated;
doing specifies an order of preference for the mode (i.e. location) from
which to include files.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L647-L662" target="_blank">source</a>]








### VFS.UseArchive
---
```lua
function VFS.UseArchive(
  archiveName: string,
  fun: unknown
) -> Results any ...
```

@return `Results` - of of the given function





Temporarily load an archive from the VFS and run the given function,
which can make usage of the files in the archive.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L687-L695" target="_blank">source</a>]








### VFS.CompressFolder
---
```lua
function VFS.CompressFolder(
  folderPath: string,
  archiveType: string?,
  compressedFilePath: string?,
  includeFolder: boolean?,
  mode: string?
)
```
@param `archiveType` - (Default: `"zip"`)The compression type (can
currently be only `"zip"`).

@param `compressedFilePath` - (Default: `folderPath .. ".sdz"`)

@param `includeFolder` - (Default: `false`) Whether the archive should
have the specified folder as root.






Compresses the specified folder.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L815-L825" target="_blank">source</a>]








### VFS.ZlibCompress
---
```lua
function VFS.ZlibCompress(uncompressed: string) -> compressed string?
```
@param `uncompressed` - Data to compress.


@return `compressed` - Compressed data, or `nil` on error.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L861-L865" target="_blank">source</a>]








### VFS.ZlibDecompress
---
```lua
function VFS.ZlibDecompress(compressed: string) -> uncompressed string?
```
@param `compressed` - Data to decompress.


@return `uncompressed` - Uncompressed data, or `nil` on error.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L881-L885" target="_blank">source</a>]








### VFS.CalculateHash
---
```lua
function VFS.CalculateHash(
  input: string,
  hashType: HashType
) -> hash string?
```
@param `hashType` - Hash type.






Calculates hash of a given string.

- MD5 gets base64 encoded.
- SHA512 gets hex encoded.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L908-L918" target="_blank">source</a>]








### VFS.PackU8
---
```lua
function VFS.PackU8(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert unsigned 8-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1000-L1005" target="_blank">source</a>]








### VFS.PackU8
---
```lua
function VFS.PackU8(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert unsigned 8-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1006-L1011" target="_blank">source</a>]








### VFS.PackU16
---
```lua
function VFS.PackU16(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert unsigned 16-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1014-L1019" target="_blank">source</a>]








### VFS.PackU16
---
```lua
function VFS.PackU16(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert unsigned 16-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1020-L1025" target="_blank">source</a>]








### VFS.PackU32
---
```lua
function VFS.PackU32(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert unsigned 32-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1028-L1033" target="_blank">source</a>]








### VFS.PackU32
---
```lua
function VFS.PackU32(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert unsigned 32-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1034-L1039" target="_blank">source</a>]








### VFS.PackS8
---
```lua
function VFS.PackS8(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert signed 8-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1042-L1047" target="_blank">source</a>]








### VFS.PackS8
---
```lua
function VFS.PackS8(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert signed 8-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1048-L1053" target="_blank">source</a>]








### VFS.PackS16
---
```lua
function VFS.PackS16(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert signed 16-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1056-L1061" target="_blank">source</a>]








### VFS.PackS16
---
```lua
function VFS.PackS16(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert signed 16-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1062-L1067" target="_blank">source</a>]








### VFS.PackS32
---
```lua
function VFS.PackS32(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert signed 32-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1070-L1075" target="_blank">source</a>]








### VFS.PackS32
---
```lua
function VFS.PackS32(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert signed 32-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1076-L1081" target="_blank">source</a>]








### VFS.PackS32
---
```lua
function VFS.PackS32(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert signed 32-bit float(s) to binary string.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1084-L1089" target="_blank">source</a>]








### VFS.PackS32
---
```lua
function VFS.PackS32(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert signed 32-bit float(s) to binary string.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1090-L1095" target="_blank">source</a>]








### VFS.UnpackU8
---
```lua
function VFS.UnpackU8(
  str: string,
  pos: integer?
) ->  integer
```
@param `str` - Binary string.

@param `pos` - Byte offset.






Convert a binary string to an unsigned 8-bit integer.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1143-L1149" target="_blank">source</a>]








### VFS.UnpackU16
---
```lua
function VFS.UnpackU16(
  str: string,
  pos: integer?
) ->  integer
```
@param `str` - Binary string.

@param `pos` - Byte offset.






Convert a binary string to an unsigned 16-bit integer.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1152-L1158" target="_blank">source</a>]








### VFS.UnpackU32
---
```lua
function VFS.UnpackU32(
  str: string,
  pos: integer?
) ->  integer
```
@param `str` - Binary string.

@param `pos` - Byte offset.






Convert a binary string to an unsigned 32-bit integer.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1161-L1167" target="_blank">source</a>]








### VFS.UnpackS8
---
```lua
function VFS.UnpackS8(
  str: string,
  pos: integer?
) ->  integer
```
@param `str` - Binary string.

@param `pos` - Byte offset.






Convert a binary string to a signed 8-bit integer.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1170-L1176" target="_blank">source</a>]








### VFS.UnpackS16
---
```lua
function VFS.UnpackS16(
  str: string,
  pos: integer?
) ->  integer
```
@param `str` - Binary string.

@param `pos` - Byte offset.






Convert a binary string to a signed 16-bit integer.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1179-L1185" target="_blank">source</a>]








### VFS.UnpackS32
---
```lua
function VFS.UnpackS32(
  str: string,
  pos: integer?
) ->  integer
```
@param `str` - Binary string.

@param `pos` - Byte offset.






Convert a binary string to a signed 32-bit integer.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1188-L1194" target="_blank">source</a>]








### VFS.UnpackF32
---
```lua
function VFS.UnpackF32(
  str: string,
  pos: integer?
) ->  integer
```
@param `str` - Binary string.

@param `pos` - Byte offset.






Convert a binary string to a signed 32-bit float.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L1197-L1203" target="_blank">source</a>]











## fields
---

### VFS.RAW
---
```lua
VFS.RAW: string = "r"
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L150-L150" target="_blank">source</a>]
Only select uncompressed files.








### VFS.GAME
---
```lua
VFS.GAME: string = "M"
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L152-L152" target="_blank">source</a>]








### VFS.MAP
---
```lua
VFS.MAP: string = "m"
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L154-L154" target="_blank">source</a>]








### VFS.BASE
---
```lua
VFS.BASE: string = "b"
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L156-L156" target="_blank">source</a>]








### VFS.MENU
---
```lua
VFS.MENU: string = "e"
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L158-L158" target="_blank">source</a>]








### VFS.ZIP
---
```lua
VFS.ZIP: string = "Mmeb"
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L160-L160" target="_blank">source</a>]
Only select compressed files (`.sdz`, `.sd7`).








### VFS.RAW_FIRST
---
```lua
VFS.RAW_FIRST: string = "rMmeb"
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L162-L162" target="_blank">source</a>]
Try uncompressed files first, then compressed.








### VFS.ZIP_FIRST
---
```lua
VFS.ZIP_FIRST: string = "Mmebr"
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L164-L164" target="_blank">source</a>]
Try compressed files first, then uncompressed.








### VFS.MOD
---
```lua
VFS.MOD: string = "M"
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L167-L170" target="_blank">source</a>]



@deprecated Older spelling for `VFS.GAME`






### VFS.RAW_ONLY
---
```lua
VFS.RAW_ONLY: string = "r"
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L172-L175" target="_blank">source</a>]



@deprecated Deprecated






### VFS.ZIP_ONLY
---
```lua
VFS.ZIP_ONLY: string = "Mmeb"
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVFS.cpp#L177-L180" target="_blank">source</a>]



@deprecated Deprecated








{% endraw %}