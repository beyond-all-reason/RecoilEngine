---
layout: default
title: Menu
parent: Lua API
permalink: lua-api/types/Menu
---

{% raw %}

# class Menu


- supers: Callins




[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaMenu.cpp#L34-L37" target="_blank">source</a>]







---

## methods
---

### Menu.ActivateMenu
---
```lua
function Menu.ActivateMenu()
```





Called whenever LuaMenu is on with no game loaded.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaMenu.cpp#L373-L376" target="_blank">source</a>]








### Menu.ActivateGame
---
```lua
function Menu.ActivateGame()
```





Called whenever LuaMenu is on with a game loaded.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaMenu.cpp#L394-L397" target="_blank">source</a>]








### Menu.AllowDraw
---
```lua
function Menu.AllowDraw() -> allowDraw boolean
```





Enables Draw{Genesis,Screen,ScreenPost} callins if true is returned,
otherwise they are called once every 30 seconds. Only active when a game
isn't running.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaMenu.cpp#L414-L421" target="_blank">source</a>]












{% endraw %}