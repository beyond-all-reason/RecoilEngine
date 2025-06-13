---
layout: default
title: EventListener
parent: Lua API
permalink: lua-api/types/EventListener
---

{% raw %}

# class EventListener



- namespace: RmlUi



Event listener interface

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L198-L201" target="_blank">source</a>]







---

## methods
---

### EventListener.OnAttach
---
```lua
function EventListener.OnAttach(element: RmlUi.Element)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L204-L207" target="_blank">source</a>]








### EventListener.OnDetach
---
```lua
function EventListener.OnDetach(element: RmlUi.Element)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L209-L212" target="_blank">source</a>]








### EventListener.ProcessEvent
---
```lua
function EventListener.ProcessEvent(event: RmlUi.Event)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Element.cpp#L214-L217" target="_blank">source</a>]












{% endraw %}