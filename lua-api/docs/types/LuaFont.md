---
layout: default
title: LuaFont
parent: Lua API
permalink: lua-api/types/LuaFont
---

{% raw %}

# class LuaFont





Lua opengl font object.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaFonts.cpp#L21-L27" target="_blank">source</a>]







---

## methods
---

### LuaFont.Begin
---
```lua
function LuaFont.Begin(userDefinedBlending: boolean?)
```
@param `userDefinedBlending` - When `true` doesn't set the gl.BlendFunc automatically. Defaults to `false`.






Begin a block of font commands.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaFonts.cpp#L368-L381" target="_blank">source</a>]

Fonts can be printed without using Start/End, but when doing several operations it's more optimal
if done inside a block.

Also allows disabling automatic setting of the blend mode. Otherwise the font will always print
with `BlendFunc(GL.SRC_ALPHA, GL.ONE_MINUS_SRC_ALPHA)`.





@see gl.BlendFuncSeparate




### LuaFont.SubmitBuffered
---
```lua
function LuaFont.SubmitBuffered(
  noBillboarding: boolean?,
  userDefinedBlending: boolean?
)
```
@param `noBillboarding` - When `false` sets 3d billboard mode. Defaults to `true`.

@param `userDefinedBlending` - When `true` doesn't set the gl.BlendFunc automatically. Defaults to `false`.






Draws text printed with the `buffered` option.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaFonts.cpp#L401-L410" target="_blank">source</a>]





@see gl.BlendFuncSeparate








{% endraw %}