---
layout: default
title: DrawFlag
parent: Lua API
permalink: lua-api/types/DrawFlag
---

{% raw %}

# alias DrawFlag
---



```lua
(alias) DrawFlag = (0|1|2|4|8|16|32|128)
    | 0 -- No Draw
    | 1 -- Opaque Pass
    | 2 -- Alpha Pass
    | 4 -- Reflection Pass
    | 8 -- Refraction Pass
    | 16 -- Shadow pass - Opaque Objects
    | 32 -- Shadow pass - Transparent Objects
    | 128 -- Icon - Possibly Radar Icons

```




Drawing Flags

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2110-L2122" target="_blank">source</a>]


{% endraw %}