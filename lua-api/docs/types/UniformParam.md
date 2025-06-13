---
layout: default
title: UniformParam
parent: Lua API
permalink: lua-api/types/UniformParam
---

{% raw %}

# class UniformParam


- supers: { , [string]: (T|T[]) }




A table of uniform name to value.

The Uniforms are the values you send along with the shader-program. To use
them in the shader-program declare them like this: `uniform float frame;`

Specify a Lua array to initialize GLSL arrays.

The engine will automatically fill in an appropriately named uniform for team
colour if it is declared;

```glsl
uniform vec4 teamColor;
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L552-L568" target="_blank">source</a>]







---




{% endraw %}