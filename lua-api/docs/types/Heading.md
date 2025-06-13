---
layout: default
title: Heading
parent: Lua API
permalink: lua-api/types/Heading
---

{% raw %}

# alias Heading
---



```lua
(alias) Heading = integer
```




Integer in range `[-32768, 32767]` that represents a 2D (xz plane) unit
orientation.

```
                  F(N=2) = H(-32768 / 32767)

                         ^
                         |
                         |
 F(W=3) = H(-16384)  <---o--->  F(E=1) = H(16384)
                         |
                         |
                         v

                  F(S=0) = H(0)
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3970-L3988" target="_blank">source</a>]


{% endraw %}