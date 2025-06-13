---
layout: default
title: LightParams
parent: Lua API
permalink: lua-api/types/LightParams
---

{% raw %}

# class LightParams





Parameters for lighting

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1590-L1616" target="_blank">source</a>]







---



## fields
---

### LightParams.position
---
```lua
LightParams.position : { px: number,py: number,pz: number }
```










### LightParams.direction
---
```lua
LightParams.direction : { dy: number,dx: number,dz: number }
```










### LightParams.ambientColor
---
```lua
LightParams.ambientColor : { blue: number,red: number,green: number }
```










### LightParams.diffuseColor
---
```lua
LightParams.diffuseColor : { red: number,green: number,blue: number }
```










### LightParams.specularColor
---
```lua
LightParams.specularColor : { red: number,green: number,blue: number }
```










### LightParams.intensityWeight
---
```lua
LightParams.intensityWeight : { ambientWeight: number,specularWeight: number,diffuseWeight: number }
```










### LightParams.ambientDecayRate
---
```lua
LightParams.ambientDecayRate : { ambientGreenDecay: number,ambientRedDecay: number,ambientBlueDecay: number }
```



Per-frame decay of `ambientColor` (spread over TTL frames)








### LightParams.diffuseDecayRate
---
```lua
LightParams.diffuseDecayRate : { diffuseRedDecay: number,diffuseGreenDecay: number,diffuseBlueDecay: number }
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
LightParams.decayFunctionType : { specularDecayType: number,diffuseDecayType: number,ambientDecayType: number }
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