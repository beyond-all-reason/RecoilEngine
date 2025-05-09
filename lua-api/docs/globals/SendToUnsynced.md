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
function SendToUnsynced(...: (boolean|number|string...)?)
```
@param `...` - Arguments. Typically the first argument is the name of a function to call.







Invoke `UnsyncedCallins:RecvFromSynced` callin with the given arguments.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaHandleSynced.cpp#L1966-L1972" target="_blank">source</a>]


{% endraw %}