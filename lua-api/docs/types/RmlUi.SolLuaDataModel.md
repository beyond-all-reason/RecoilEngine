---
layout: default
title: SolLuaDataModel
parent: Lua API
permalink: lua-api/types/SolLuaDataModel
---

{% raw %}

# class SolLuaDataModel



- namespace: RmlUi



Handle for your data model. It is a wrapper around the model table, marked as type T.
You can access fields by using normal indexing, but for the moment, the only keys that work are strings. Any index done this way will automatically trigger a rerender.
If you need to index any tables or subtables by anything not a string, you will need to use the underlying table, gotten with `__GetTable`. This will not trigger a rerender.
To trigger a rerender manually, use `_SetDirty`, passing in the name of the top-level entry in your model table that you edited.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/DataModel.cpp#L60-L66" target="_blank">source</a>]







---

## methods
---

### SolLuaDataModel.__SetDirty
---
```lua
function SolLuaDataModel.__SetDirty(property: string)
```





Set a table property dirty to trigger a rerender

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/DataModel.cpp#L71-L75" target="_blank">source</a>]












{% endraw %}