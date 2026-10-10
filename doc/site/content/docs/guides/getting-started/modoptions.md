+++
title = 'Using ModOptions'
date = 2025-09-07T01:44:46-07:00
draft = false
+++

## Overview

Recoil allows games to configure their behavior before game starts via the use
of modoptions, a key-value store with string names as keys.

## Using ModOptions

The list of modoptions can be retrieved via `Spring.GetModOptions()` for all
environments.

Points of interest are:

### Changing game definitions

Inside `gamedata/defs.lua` and any of the files it requires, one can use
modoptions to configure changes to game definitions. For example:

```lua
local unitDefs = loadUnitDefs()
local modOptions = Spring.GetModOptions()

if modOptions["units_are_cheaper"] then
  for _, unitDef in pairs(unitDefs) do
    unitDef.metalcost = math.ceil(unitDef.metalcost * 0.5)
  end
end

if modOptions["wacky_setting"] == "units_cant_build" then
  for _, unitDef in pairs(unitDefs) do
    if unitDef.buildoptions then
      unitDef.buildoptions = {}
    end
  end
end

-- unitDefs is returned with the `UnitDefs` key
```

### Changing game behavior

In any of the `LuaRules`, `LuaUI` or `LuaGaia` environments modoptions can
configure behavior. For example:

```lua
-- LuaRules/gadgets/unit_stun_control.lua
local stunnedUnitsCanBeTransported = Spring.GetModOptions()["stunned_units_transport"] == "enabled"

function gadget:AllowCommand(unitID, unitDefID, teamID, cmdID, cmdParams, cmdOptions, cmdTag, playerID, fromSynced, fromLua)
  if cmdID ~= CMD.LOAD_UNITS then
    return true
  end

  local unitIsStunned = Spring.GetUnitIsStunned(unitID)

  -- nil when unit is invalid
  if unitIsStunned == nil then
    return false
  end

  return not(unitIsStunned) or stunnedUnitsCanBeTransported
end
```

## ModOption interface

### Passing modoptions

Modoptions are passed through the
[startscript]({{% ref "articles/start-script-format" %}}).

### Game clients

For game clients that use the unitsync library for fetching game option metadata
there's an established format for informing game and engine modoptions via the
`ModOptions.lua` and `EngineOptions.lua` file.

See `ModOption` in the lua api reference for more information.

In some cases the unitsync library might not necessarily be used but these
files still inform the client, with possibly some extended format, e.g. Chobby.

### Engine behavior

The engine uses a few hardcoded modoptions to configure behavior:

| Name               | Description                                                                                                           | Default |
| ------------------ | --------------------------------------------------------------------------------------------------------------------- | ------- |
| `maxUnitsPerTeam`  | The maximum number of units a team can have                                                                           | `32000` |
| `fixedAllies`      | Team alliances cannot be changed                                                                                      | `true`  |
| `maxSpeed`         | Maximum wanted simulation speed players can issue during a game                                                       | `20.0`  |
| `minSpeed`         | Minimum wanted simulation speed players can issue during a game                                                       | `0.1`   |
| `ghostedBuildings` | Enemy buildings are drawn as "ghosts" after line of sight is lost                                                     | `true`  |
| `disableMapDamage` | Map can't be deformed by weapon damage (e.g. craters)                                                                 | `false` |
| `noHelperAIs`      | Disable ability to send unit orders by unsynced lua environments (LuaRules, LuaUI)                                    | `false` |
| `useLuaGaia`       | Add a "gaia" team and enable the LuaGaia lua environment                                                              | `true`  |
| `luaDevMode`       | Includes the "debug" lua library and allows certain game files to be loaded from the local filesystem (VFS.RAW_FIRST) | `false` |
