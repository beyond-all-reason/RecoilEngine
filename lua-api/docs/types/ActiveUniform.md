---
layout: default
title: ActiveUniform
parent: Lua API
permalink: lua-api/types/ActiveUniform
---

{% raw %}

# class ActiveUniform





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L922-L929" target="_blank">source</a>]







---



## fields
---

### ActiveUniform.name
---
```lua
ActiveUniform.name : string
```










### ActiveUniform.type
---
```lua
ActiveUniform.type : string
```



String name of `GL_*` constant.








### ActiveUniform.length
---
```lua
ActiveUniform.length : integer
```



The character length of `name`.








### ActiveUniform.size
---
```lua
ActiveUniform.size : integer
```










### ActiveUniform.location
---
```lua
ActiveUniform.location : GL {
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












{% endraw %}