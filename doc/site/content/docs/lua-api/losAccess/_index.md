---
title: "losAccess"
permalink: "docs/lua-api/losAccess"
---
# class losAccess





Parameters for los access


If one condition is fulfilled all beneath it are too (e.g. if an unit is in
LOS it can read params with `inradar=true` even if the param has
`inlos=false`) All GameRulesParam are public, TeamRulesParams can just be
`private`,`allied` and/or `public` You can read RulesParams from any Lua
environments! With those losAccess policies you can limit their access.

All GameRulesParam are public, TeamRulesParams can just be `private`,`allied` and/or `public`
You can read RulesParams from any Lua environments! With those losAccess policies you can limit their access.







---



## fields
---

### losAccess.private
---
```lua
losAccess.private : boolean?
```



only readable by the ally (default)








### losAccess.allied
---
```lua
losAccess.allied : boolean?
```



readable by ally + ingame allied








### losAccess.inlos
---
```lua
losAccess.inlos : boolean?
```



readable if the unit is in LOS








### losAccess.inradar
---
```lua
losAccess.inradar : boolean?
```



readable if the unit is in AirLOS








### losAccess.public
---
```lua
losAccess.public : boolean?
```



readable by all








### losAccess.private
---
```lua
losAccess.private : boolean?
```



only readable by the ally (default)








### losAccess.allied
---
```lua
losAccess.allied : boolean?
```



readable by ally + ingame allied








### losAccess.inlos
---
```lua
losAccess.inlos : boolean?
```



readable if the unit is in LOS








### losAccess.inradar
---
```lua
losAccess.inradar : boolean?
```



readable if the unit is in AirLOS








### losAccess.public
---
```lua
losAccess.public : boolean?
```



readable by all









