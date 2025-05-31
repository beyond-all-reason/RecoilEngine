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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L1998-L2008" target="_blank">source</a>]


{% endraw %}