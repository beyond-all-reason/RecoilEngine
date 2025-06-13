---
layout: default
title: ExplosionParams
parent: Lua API
permalink: lua-api/types/ExplosionParams
---

{% raw %}

# class ExplosionParams





Parameters for explosion.

Please note the explosion defaults to 1 damage regardless of what it's defined in the weaponDef.
The weapondefID is only used for visuals and for passing into callins like UnitDamaged.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L7111-L7130" target="_blank">source</a>]







---



## fields
---

### ExplosionParams.weaponDef
---
```lua
ExplosionParams.weaponDef : number
```










### ExplosionParams.owner
---
```lua
ExplosionParams.owner : number
```










### ExplosionParams.hitUnit
---
```lua
ExplosionParams.hitUnit : number
```










### ExplosionParams.hitFeature
---
```lua
ExplosionParams.hitFeature : number
```










### ExplosionParams.craterAreaOfEffect
---
```lua
ExplosionParams.craterAreaOfEffect : number
```










### ExplosionParams.damageAreaOfEffect
---
```lua
ExplosionParams.damageAreaOfEffect : number
```










### ExplosionParams.edgeEffectiveness
---
```lua
ExplosionParams.edgeEffectiveness : number
```










### ExplosionParams.explosionSpeed
---
```lua
ExplosionParams.explosionSpeed : number
```










### ExplosionParams.gfxMod
---
```lua
ExplosionParams.gfxMod : number
```










### ExplosionParams.impactOnly
---
```lua
ExplosionParams.impactOnly : boolean
```










### ExplosionParams.ignoreOwner
---
```lua
ExplosionParams.ignoreOwner : boolean
```










### ExplosionParams.damageGround
---
```lua
ExplosionParams.damageGround : boolean
```












{% endraw %}