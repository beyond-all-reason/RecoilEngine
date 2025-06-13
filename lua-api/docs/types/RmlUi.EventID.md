---
layout: default
title: EventID
parent: Lua API
permalink: lua-api/types/EventID
---

{% raw %}

# alias EventID
---

- namespace: RmlUi



```lua
(alias) EventID = (0|1|2|3|4|5|6|7|8|9...)
    | 0 -- Invalid
    | 1 -- Mousedown
    | 2 -- Mousescroll
    | 3 -- Mouseover
    | 4 -- Mouseout
    | 5 -- Focus
    | 6 -- Blur
    | 7 -- Keydown
    | 8 -- Keyup
    | 9 -- Textinput
    | 10 -- Mouseup
    | 11 -- Click
    | 12 -- Dblclick
    | 13 -- Load
    | 14 -- Unload
    | 15 -- Show
    | 16 -- Hide
    | 17 -- Mousemove
    | 18 -- Dragmove
    | 19 -- Drag
    | 20 -- Dragstart
    | 21 -- Dragover
    | 22 -- Dragdrop
    | 23 -- Dragout
    | 24 -- Dragend
    | 25 -- Handledrag
    | 26 -- Resize
    | 27 -- Scroll
    | 28 -- Animationend
    | 29 -- Transitionend
    | 30 -- Change
    | 31 -- Submit
    | 32 -- Tabchange
    | 33 -- NumDefinedIDs
    | integer -- Custom ID

```




[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Rml/SolLua/bind/Global.cpp#L189-L226" target="_blank">source</a>]


{% endraw %}