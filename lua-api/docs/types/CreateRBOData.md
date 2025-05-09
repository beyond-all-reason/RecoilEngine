---
layout: default
title: CreateRBOData
parent: Lua API
permalink: lua-api/types/CreateRBOData
---

{% raw %}

# class CreateRBOData





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaRBOs.cpp#L150-L155" target="_blank">source</a>]







---



## fields
---

### CreateRBOData.target
---
```lua
CreateRBOData.target : GL {
    POINTS: integer,
    LINES: integer,
    LINE_LOOP: integer,
    LINE_STRIP: integer,
    TRIANGLES: integer,
    TRIANGLE_STRIP: integer,
    TRIANGLE_FAN: integer,
    QUADS: integer,
    QUAD_STRIP: integer,
    POLYGON: integer,
    LINE_STRIP_ADJACENCY: integer,
    LINES_ADJACENCY: integer,
    ...
}
```










### CreateRBOData.format
---
```lua
CreateRBOData.format : GL {
    POINTS: integer,
    LINES: integer,
    LINE_LOOP: integer,
    LINE_STRIP: integer,
    TRIANGLES: integer,
    TRIANGLE_STRIP: integer,
    TRIANGLE_FAN: integer,
    QUADS: integer,
    QUAD_STRIP: integer,
    POLYGON: integer,
    LINE_STRIP_ADJACENCY: integer,
    LINES_ADJACENCY: integer,
    ...
}
```










### CreateRBOData.samples
---
```lua
CreateRBOData.samples : number?
```



any number here will result in creation of multisampled RBO










{% endraw %}