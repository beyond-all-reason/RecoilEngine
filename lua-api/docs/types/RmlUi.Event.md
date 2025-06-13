---
layout: default
title: Event
parent: Lua API
permalink: lua-api/types/Event
---

{% raw %}

# class Event



- namespace: RmlUi



An event that happens to an element.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Event.cpp#L108-L111" target="_blank">source</a>]







---



## fields
---

### Event.current_element
---
```lua
Event.current_element : RmlUi.Element {
    class_name: string,
    id: string,
    inner_rml: string,
    scroll_left: integer,
    scroll_top: integer,
    attributes: RmlUi.ElementAttributesProxy,
    child_nodes: RmlUi.ElementChildNodesProxy,
    client_left: integer,
    client_height: integer,
    client_top: integer,
    client_width: integer,
    first_child: RmlUi.Element?,
    ...
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Event.cpp#L128-L128" target="_blank">source</a>]








### Event.type
---
```lua
Event.type : string
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Event.cpp#L130-L130" target="_blank">source</a>]








### Event.target_element
---
```lua
Event.target_element : RmlUi.Element {
    class_name: string,
    id: string,
    inner_rml: string,
    scroll_left: integer,
    scroll_top: integer,
    attributes: RmlUi.ElementAttributesProxy,
    child_nodes: RmlUi.ElementChildNodesProxy,
    client_left: integer,
    client_height: integer,
    client_top: integer,
    client_width: integer,
    first_child: RmlUi.Element?,
    ...
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Event.cpp#L132-L132" target="_blank">source</a>]
What element it happened to








### Event.parameters
---
```lua
Event.parameters : RmlUi.EventParametersProxy {
    button: RmlUi.EventParametersProxy.MouseButton,
    mouse_x: integer,
    mouse_y: integer,
    scroll_lock_key: RmlUi.EventParametersProxy.TrueFalse,
    meta_key: RmlUi.EventParametersProxy.TrueFalse,
    ctrl_key: RmlUi.EventParametersProxy.TrueFalse,
    shift_key: RmlUi.EventParametersProxy.TrueFalse,
    caps_lock_key: RmlUi.EventParametersProxy.TrueFalse,
    alt_key: RmlUi.EventParametersProxy.TrueFalse,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Event.cpp#L134-L134" target="_blank">source</a>]








### Event.event_phase
---
```lua
Event.event_phase : RmlUi.RmlEventPhase {
    None: integer,
    Capture: integer,
    Target: integer,
    Bubble: integer,
}
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Event.cpp#L137-L137" target="_blank">source</a>]








### Event.interruptible
---
```lua
Event.interruptible : boolean
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Event.cpp#L139-L139" target="_blank">source</a>]








### Event.propagating
---
```lua
Event.propagating : boolean
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Event.cpp#L141-L141" target="_blank">source</a>]








### Event.immediate_propagating
---
```lua
Event.immediate_propagating : boolean
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Event.cpp#L143-L143" target="_blank">source</a>]










{% endraw %}