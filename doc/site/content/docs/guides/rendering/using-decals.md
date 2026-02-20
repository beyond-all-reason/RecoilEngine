+++
title = "Using Decals"
+++

## Overview

Recoil provides the ability for games to efficiently draw textures on the map,
called decals here.

This guide aims to orient game developers on how to make use of them, the
procedure is split in two phases: registering decal textures, using them in
game code.

### Summary

Register your decal textures on `gamedata/resources.lua`, example:

```lua
{
  graphics = {
    decals = {
      -- bitmaps/decals/01_somedecal.dds, bitmaps/decals/01_somedecal_norm.dds
      "decals/01_somedecal.dds", -- maindecal_1, normdecal_1
      -- bitmaps/decals/02_somedecal.dds, bitmaps/decals/02_somedecal_norm.dds
      "decals/02_anotherdecal.dds", -- maindecal_2, normdecal_2
    }
  }
}
```

Reference decal names as `maindecal_n` and `normdecal_n`, where `n` is the order
they were registered as exemplified above.

Manipulate decals via:

- `Spring.CreateGroundDecal()`
- `Spring.DestroyGroundDecal`
- `Spring.SetGroundDecal*`
- `Spring.GetGroundDecal*`

## Registering Decal textures

In order to use decals, first you must register their textures with the engine.
What this means is that "decals" has its own defined atlas (like "scars").

In practice, the engine looks for a list of strings in the table returned
by `gamedata/resources.lua`, indexed by `.graphics.decals`.

Each entry should refer to a texture located in `bitmaps/`, for the path
_inside that directory_. It also **MUST** contain a similarly name texture for
the normal, see example below:

```lua
-- gamedata/resources.lua
return {
  graphics = {
    decals = {
      -- looks for bitmaps/somedecal.dds and bitmaps/somedecal_normal.dds
      -- registers as maindecal_1 and normdecal_1
      "somedecal.dds",
      -- looks for bitmaps/decals/otherdecal.dds and
      -- bitmaps/decals/otherdecal_normal.dds
      -- registers as maindecal_2 and normdecal_2
      "decals/otherdecal.dds"
    }
  }
}
```

If you don't have such a file in your game, the
[one in basecontent](https://github.com/beyond-all-reason/RecoilEngine/blob/e3c12f5/cont/base/springcontent/gamedata/resources.lua)
is used instead. See [VFS Basics]({{% ref "docs/guides/getting-started/vfs-basics" %}})
for how the engine loads game files.

An example for programmatically adding decals can be found in
[SplinterFaction/Gamedata/resources.lua at master · SplinterFaction/SplinterFaction · GitHub](https://github.com/SplinterFaction/SplinterFaction/blob/57d61e3/Gamedata/resources.lua)

Once you have your decal textures registered with the engine you may use them.

### Formats

I recommend using DDS, you can use PNG if you must (DDS will look much better
from a distance). The engine will even take TGA.

### Naming

Notice the registered names for the decals textures are not the same as the
ones we passed on `.graphics.decals`. The engine registers using its own
nomenclature and indexing. This can make it difficult to predict what
name was registered for a particular texture we want to use.

To make it easier, especially when adding decals programmatically, we recommend
indexing your decal files numerically inside a `decals` folder, so you can
always easily refer to the registered texture name using the filename.

For example:

```lua
-- gamedata/resources.lua
return {
  graphics = {
    decals = {
      -- looks for <index>_<name>.dds in bitmaps/decals/
      "decals/01_somedecal.dds", -- maindecal_1, normdecal_1
      "decals/02_anotherdecal.dds", -- maindecal_2, normdecal_2
      "decals/03_evenanother.dds", -- maindecal_3, normdecal_3
    }
  }
}
```

### Troubleshooting

You might get a red square where your decal should be, this means something
went wrong registering or loading the decal texture.

Here's a snippet to check which names your decal textures were registered to:

```lua
-- This is debug code so that we can see what decals are in the index
for i, v in ipairs(Spring.GetGroundDecalTextures(true)) do
  Spring.Echo("[DECALS] > maindecal_" .. i, v)
end

for i, v in ipairs(Spring.GetGroundDecalTextures(false)) do
  Spring.Echo("[DECALS] > normdecal_" .. i, v)
end
```

## Using Decals in your game

This is probably the easiest part. Make sure you store state for your created
decals ids, so they can be cleaned up or further manipulated later.

### Simple example

```lua
local decalID = Spring.CreateGroundDecal() -- create a new decal

if decalID then
  -- posX, posZ refer to a point in world space
  Spring.SetGroundDecalPosAndDims(decalID, posX, posZ, decalWidth, decalHeight)
  Spring.SetGroundDecalTexture(decalID, "maindecal_1")
  -- Comment below if you don't wish to use a normal map
  Spring.SetGroundDecalTexture(decalID, "normdecal_1", false)
end

-- When your decal is not needed anymore
-- Spring.DestroyGroundDecal(decalID)
```

You can perform some additional operations on you decal:

```lua
-- Applying some random rotation to decal
local angle = math.random() * 2 * math.pi
Spring.SetGroundDecalRotation(decalID, angle)

-- newly created decals have alpha = 1.0 and alphaFallOff = 0
Spring.SetGroundDecalAlpha(decalID, alpha, alphaFallOff)

-- newly created decals have tint values 0.5
-- Note that the actual color applied is `2 * textureColor * tintColor` so 0.5 is effectively no color changes
Spring.SetGroundDecalTint(decalID, tintRed, tintGreen, tintBlue, tintAlpha)
```

### Example use case, metal spots decals

> [!NOTE]
> The example below is extracted from the common necessity to draw metal spots.
>
> If you're looking for the same functionality, see
> [Drawer](https://github.com/SplinterFaction/SplinterFaction/blob/1a0f61972481365b9d95188bbe3d473b22eb5bf9/LuaUI/Widgets/game_metal_spot_drawer.lua)
> , [Metal Spot Generator](https://github.com/SplinterFaction/SplinterFaction/blob/1a0f61972481365b9d95188bbe3d473b22eb5bf9/LuaUI/Widgets/game_metal_spot_drawer.lua)
> , [Metal Spot Loader](https://github.com/SplinterFaction/SplinterFaction/blob/1a0f61972481365b9d95188bbe3d473b22eb5bf9/LuaUI/Widgets/game_metal_spot_loader.lua)

```lua
local decalTextureIndex = "1" -- Uses `maindecal_1` and `normdecal_1`
local decalSize = 85
local offset = { x = 7, z = 7 }

local decalName = "maindecal_" .. decalTextureIndex
local decalNormalName = "normdecal_" .. decalTextureIndex

-- Decal IDs to clean up or operate on later
local decalIDs = {} ---@type integer[]

function widget:Initialize()
  -- A list of tables with fields z and x
  local spots = {} ---@type ({ x: number, z: number })[]

  for _, spot in ipairs(spots) do
    local x, z = spot.x + offset.x, spot.z + offset.z

    local decalID = Spring.CreateGroundDecal()

    if decalID then
      Spring.SetGroundDecalPosAndDims(decalID, x, z, decalSize, decalSize)

      Spring.SetGroundDecalTexture(decalID, decalName, true)
      Spring.SetGroundDecalTexture(decalID, decalNormalName, false)

      -- store the decalID for cleanup on shutdown
      table.insert(decalIDs, decalID)
    end
  end
end

function widget:Shutdown()
  for _, id in ipairs(decalIDs) do
    Spring.DestroyGroundDecal(id)
  end
end
```

## Keeping it Clean

It is a good practice to clean up your decals when the widget exits. You may or
may not want this behavior though. If you do, you can simply call
`Spring.DestroyGroundDecal()` in `widget:Shutdown()`

Like this:

```lua
function widget:Shutdown()
  for _, id in ipairs(decalIDs) do
    Spring.DestroyGroundDecal(id)
  end
end
```
