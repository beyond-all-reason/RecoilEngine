---
title: "UniformParam"
permalink: "docs/lua-api/UniformParam"
---
# class UniformParam


- inherits:
{ , [string]: (T|T[]) }, { , [string]: (T|T[]) }




A table of uniform name to value.

The Uniforms are the values you send along with the shader-program. To use
them in the shader-program declare them like this: `uniform float frame;`

Specify a Lua array to initialize GLSL arrays.

The engine will automatically fill in an appropriately named uniform for team
colour if it is declared;

```glsl
uniform vec4 teamColor;
```







---



