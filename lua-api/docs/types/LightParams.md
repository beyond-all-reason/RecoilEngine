---
layout: default
title: LightParams
parent: Lua API
permalink: lua-api/types/LightParams
---

{% raw %}

# class LightParams





Parameters for lighting

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/af780b403a99ed35936c8ebab26c781d70cfeaeb/rts/Lua/LuaUnsyncedCtrl.cpp#L1590-L1616" target="_blank">source</a>]







---



## fields
---

### LightParams.position
---
```lua
LightParams.position : { pz: number,py: number,px: number }
```










### LightParams.direction
---
```lua
LightParams.direction : { dx: number,dy: number,dz: number }
```










### LightParams.ambientColor
---
```lua
LightParams.ambientColor : { red: number,green: number,blue: number }
```










### LightParams.diffuseColor
---
```lua
LightParams.diffuseColor : { green: number,red: number,blue: number }
```










### LightParams.specularColor
---
```lua
LightParams.specularColor : { green: number,blue: number,red: number }
```










### LightParams.intensityWeight
---
```lua
LightParams.intensityWeight : { specularWeight: number,ambientWeight: number,diffuseWeight: number }
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
LightParams.diffuseDecayRate : { diffuseBlueDecay: number,diffuseGreenDecay: number,diffuseRedDecay: number }
```



Per-frame decay of `diffuseColor` (spread over TTL frames)








### LightParams.specularDecayRate
---
```lua
LightParams.specularDecayRate : { specularGreenDecay: number,specularBlueDecay: number,specularRedDecay: number }
```



Per-frame decay of `specularColor` (spread over TTL frames)








### LightParams.decayFunctionType
---
```lua
LightParams.decayFunctionType : { ambientDecayType: number,diffuseDecayType: number,specularDecayType: number }
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