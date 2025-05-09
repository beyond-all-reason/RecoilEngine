---
title: "LightParams"
permalink: "docs/lua-api/LightParams"
---
# class LightParams





Parameters for lighting







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
LightParams.direction : { dx: number,dy: number,dz: number }
```










### LightParams.ambientColor
---
```lua
LightParams.ambientColor : { blue: number,red: number,green: number }
```










### LightParams.diffuseColor
---
```lua
LightParams.diffuseColor : { green: number,blue: number,red: number }
```










### LightParams.specularColor
---
```lua
LightParams.specularColor : { red: number,green: number,blue: number }
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
LightParams.diffuseDecayRate : { diffuseRedDecay: number,diffuseGreenDecay: number,diffuseBlueDecay: number }
```



Per-frame decay of `diffuseColor` (spread over TTL frames)








### LightParams.specularDecayRate
---
```lua
LightParams.specularDecayRate : { specularBlueDecay: number,specularGreenDecay: number,specularRedDecay: number }
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










### LightParams.position
---
```lua
LightParams.position : { pz: number,px: number,py: number }
```










### LightParams.direction
---
```lua
LightParams.direction : { dx: number,dy: number,dz: number }
```










### LightParams.ambientColor
---
```lua
LightParams.ambientColor : { green: number,red: number,blue: number }
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
LightParams.intensityWeight : { diffuseWeight: number,specularWeight: number,ambientWeight: number }
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
LightParams.diffuseDecayRate : { diffuseGreenDecay: number,diffuseBlueDecay: number,diffuseRedDecay: number }
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











