+++
title = 'Referencing textures in LUA/Shaders/RmlUi'
date = 2025-07-02T21:17:05-00:00
draft = false
author = "mcukstorm"
+++

### Referencing textures in LUA/Shaders/RmlUi
In LUA, Shaders and the RmlUi texture element, textures are primarily referenced using a string in one of the formats described in this article. 

Many related to units or features and in these cases the UnitDefId is used, this id is generated at runtime and can vary if unit defs are added/removed so hard coding these numbers is not advisable and they should be looked up from the unitDef name. See [/docs/guides/getting-started/unit-types-basics](https://beyond-all-reason.github.io/RecoilEngine/docs/guides/getting-started/unit-types-basics/#the-unitdefs-table-inside-wupgets)

### Texture References
Textures in recoil are generally referenced by a string in one of the formats listed below.

`#<UnitDefId#int>` -- Build picture / the image that would be shown in a list of things a constructor could build e.g. `#101`
`^<UnitDefId#int>` -- Radar icon for unit e.g. `^101`

`%<UnitDefId#int>:<texNum#int[0-1]>` -- The unit texture for UnitDefId, each unit has two textures referenced here as 0 & 1 e.g. `%35:1`

The above reference for unit textures can also be used for features (e.g trees). Feature textures are effectively the same as units but always have negative UnitDefId e.g. `%-12:0`

`!<luaTextureId#int>` -- Lua created textures referenced by id provided at time of texture creation
`*<atlasId#int>` -- Atlas/sprite (added by LUA CreateTextureAtlas)

`$<textureName>` -- There are also named textures, some examples include `$minimap` `$groundfx` though there are many more, the names of which can be found in [LuaOpenGLUtils.cpp (GetLuaMatTextureType)](https://github.com/beyond-all-reason/RecoilEngine/blob/3206e7ea9b3a043b736bcda3d581f41737914c65/rts/Lua/LuaOpenGLUtils.cpp#L53). ssmf_splat_normals, extra and info also have subtextures beneath though there is no central reference for these.

### Notes
UnitID/UnitName - a unique string identifier given to a unit, e.g. armcom for Armada Commander in the BAR game content
UnitDefId - a numeric ID given to this unit by the spring/recoil engine at runtime
