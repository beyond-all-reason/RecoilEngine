---
layout: default
title: ElementTabSet
parent: Lua API
permalink: lua-api/types/ElementTabSet
---

{% raw %}

# class ElementTabSet


- supers: RmlUi.Element


- namespace: RmlUi



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/ElementDerived.cpp#L57-L59" target="_blank">source</a>]







---

## methods
---

### ElementTabSet.SetPanel
---
```lua
function ElementTabSet.SetPanel(
  index: integer,
  rml: string
)
```





Sets the contents of a panel to the RML content rml. If index is out-of-bounds, a new panel will be added at the end.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/ElementDerived.cpp#L63-L68" target="_blank">source</a>]








### ElementTabSet.SetTab
---
```lua
function ElementTabSet.SetTab(
  index: integer,
  rml: string
)
```





Sets the contents of a tab to the RML content rml. If index is out-of-bounds, a new tab will be added at the end.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/ElementDerived.cpp#L70-L75" target="_blank">source</a>]








### ElementTabSet.RemoveTab
---
```lua
function ElementTabSet.RemoveTab(index: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/ElementDerived.cpp#L78-L81" target="_blank">source</a>]











## fields
---

### ElementTabSet.active_tab
---
```lua
ElementTabSet.active_tab : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/ElementDerived.cpp#L85-L85" target="_blank">source</a>]








### ElementTabSet.num_tabs
---
```lua
ElementTabSet.num_tabs : integer
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/ElementDerived.cpp#L89-L89" target="_blank">source</a>]










{% endraw %}