---
title: "Menu"
permalink: "docs/lua-api/Menu"
---
# class Menu


- inherits:
Callins, Callins










---

## methods
---

### Menu.ActivateMenu
---
```lua
function Menu.ActivateMenu()
```





Called whenever LuaMenu is on with no game loaded.








### Menu.ActivateGame
---
```lua
function Menu.ActivateGame()
```





Called whenever LuaMenu is on with a game loaded.








### Menu.AllowDraw
---
```lua
function Menu.AllowDraw() -> allowDraw boolean
```





Enables Draw{Genesis,Screen,ScreenPost} callins if true is returned,
otherwise they are called once every 30 seconds. Only active when a game
isn't running.








### Menu.ActivateMenu
---
```lua
function Menu.ActivateMenu()
```





Called whenever LuaMenu is on with no game loaded.








### Menu.ActivateGame
---
```lua
function Menu.ActivateGame()
```





Called whenever LuaMenu is on with a game loaded.








### Menu.AllowDraw
---
```lua
function Menu.AllowDraw() -> allowDraw boolean
```





Enables Draw{Genesis,Screen,ScreenPost} callins if true is returned,
otherwise they are called once every 30 seconds. Only active when a game
isn't running.











