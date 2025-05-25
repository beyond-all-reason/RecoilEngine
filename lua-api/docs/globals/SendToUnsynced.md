---
layout: default
title: SendToUnsynced
parent: Lua API
permalink: lua-api/globals/SendToUnsynced
---

{% raw %}

# global SendToUnsynced

---

```lua
function SendToUnsynced(...: (boolean|number|string|table)?)
```
@param `...` - Arguments. Typically the first argument is the name of a function to call.

Argument tables will be recursively copied and stripped of unsupported types and metatables.







Invoke `UnsyncedCallins:RecvFromSynced` callin with the given arguments.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/af780b403a99ed35936c8ebab26c781d70cfeaeb/rts/Lua/LuaHandleSynced.cpp#L1983-L1993" target="_blank">source</a>]


{% endraw %}