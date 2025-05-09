---
layout: default
title: LightParams
parent: Lua API
permalink: lua-api/types/LightParams
---

{% raw %}

# class LightParams





Parameters for lighting

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaUnsyncedCtrl.cpp#L1590-L1616" target="_blank">source</a>]







---



## fields
---

### LightParams.position
---
```lua
LightParams.position : { py: number,pz: number,px: number }
```










### LightParams.direction
---
```lua
LightParams.direction : { dy: number,dz: number,dx: number }
```










### LightParams.ambientColor
---
```lua
LightParams.ambientColor : { green: number,blue: number,red: number }
```










### LightParams.diffuseColor
---
```lua
LightParams.diffuseColor : { green: number,blue: number,red: number }
```










### LightParams.specularColor
---
```lua
LightParams.specularColor : { green: number,blue: number,red: number }
```










### LightParams.intensityWeight
---
```lua
LightParams.intensityWeight : { specularWeight: number,diffuseWeight: number,ambientWeight: number }
```










### LightParams.ambientDecayRate
---
```lua
LightParams.ambientDecayRate : { ambientRedDecay: number,ambientGreenDecay: number,ambientBlueDecay: number }
```



Per-frame decay of `ambientColor` (spread over TTL frames)








### LightParams.diffuseDecayRate
---
```lua
LightParams.diffuseDecayRate : { diffuseRedDecay: number,diffuseBlueDecay: number,diffuseGreenDecay: number }
```



Per-frame decay of `diffuseColor` (spread over TTL frames)








### LightParams.specularDecayRate
---
```lua
LightParams.specularDecayRate : { specularRedDecay: number,specularGreenDecay: number,specularBlueDecay: number }
```



Per-frame decay of `specularColor` (spread over TTL frames)








### LightParams.decayFunctionType
---
```lua
LightParams.decayFunctionType : { ambientDecayType: number,specularDecayType: number,diffuseDecayType: number }
```



If value is `0.0` then the `*DecayRate` values will be interpreted as linear, otherwise exponential.








### LightParams.radius
---
```lua
LightParams.radius : number
```










### LightParams.fov
---
```lua
LightParams.fov : number
```










### LightParams.ttl
---
```lua
LightParams.ttl : number
```










### LightParams.priority
---
```lua
LightParams.priority : number
```










### LightParams.ignoreLOS
---
```lua
LightParams.ignoreLOS : boolean
```












{% endraw %}