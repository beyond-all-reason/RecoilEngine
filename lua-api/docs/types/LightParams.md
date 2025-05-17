---
layout: default
title: LightParams
parent: Lua API
permalink: lua-api/types/LightParams
---

{% raw %}

# class LightParams





Parameters for lighting

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/197053673559ebf8a6285a26a97956a0b85365a2/rts/Lua/LuaUnsyncedCtrl.cpp#L1590-L1616" target="_blank">source</a>]







---



## fields
---

### LightParams.position
---
```lua
LightParams.position : { pz: number,px: number,py: number }
```










### LightParams.direction
---
```lua
LightParams.direction : { dz: number,dx: number,dy: number }
```










### LightParams.ambientColor
---
```lua
LightParams.ambientColor : { red: number,green: number,blue: number }
```










### LightParams.diffuseColor
---
```lua
LightParams.diffuseColor : { blue: number,red: number,green: number }
```










### LightParams.specularColor
---
```lua
LightParams.specularColor : { green: number,blue: number,red: number }
```










### LightParams.intensityWeight
---
```lua
LightParams.intensityWeight : { ambientWeight: number,diffuseWeight: number,specularWeight: number }
```










### LightParams.ambientDecayRate
---
```lua
LightParams.ambientDecayRate : { ambientBlueDecay: number,ambientRedDecay: number,ambientGreenDecay: number }
```



Per-frame decay of `ambientColor` (spread over TTL frames)








### LightParams.diffuseDecayRate
---
```lua
LightParams.diffuseDecayRate : { diffuseBlueDecay: number,diffuseGreenDecay: number,diffuseRedDecay: number }
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
LightParams.decayFunctionType : { diffuseDecayType: number,specularDecayType: number,ambientDecayType: number }
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