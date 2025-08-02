+++
title = "Widgets and Gadgets"
weight = 1
+++

## Introduction
Before we get to Widgets and Gadgets we will start with an overview of some key areas of Recoil and the entry points into game code.

### Engine Concepts Refresh
- Synced mode is the environment that affects game simulation e.g ordering units around. It can include information about the entire simulation (all enemy positions etc) and should be considered privileged. Synced commands are distributed and run by all players in the game to ensure the simulation remains synced.
- Unsynced mode is the players own environment, functionality here could for example enhance controls, display helpful information. Information about units is limited to just what the play can 'see' i.e. LOS(line of sight) and Radar.

### Key Areas
- LuaRules - Generally home to lower level customisations that affect unit behavior or overall game operation
- LuaGaia - The controller of world objects not owned by a player (e.g wrecks, map features)
- LuaIntro - The handler for showing the game load screens.
- LuaMenu - The handler for showing the main menu, both present on game launch (if no script is specified) and can be switched back to at any time during the game.
- LuaUI - The handler for the in-game UI.

### Entrypoint into Lua code
Each of these handlers will provide a Lua environment with some predefined globals and start by executing a main.lua file within their respective folders in the game. (LuaGaia and LuaRules have two entry points main.lua for Synced and draw.lua for Unsynced) [more information on the environments and globals provided is available here.](https://springrts.com/wiki/Lua:Environments)

To allow you to control and extend the game/engine behavior predefined functions in your lua code will be called (if they are present) by the engine at certain hook points or on events in the engine. These are commonly referred to as call-ins (but could also be known as event handlers, callbacks). The list of available call-ins is extensive and can be retrieved in Lua using Script.GetCallInList()

## Widgets and Gadgets
Widgets and Gadgets are concepts that have been adopted by several games using Recoil as a modular way to extend the game and are not an engine level concept. Practically at a general functionality level both widgets and gadgets are the same and often abstracted to just the term addon. The different terms are mostly used to logically separate the types of addons.

Gadgets typically being lower level game logic, unit behaviour functionality that defines your game and should be present for all players, they are typically only found in LuaRules and LuaGaia and are often included using VFS.ZIP_ONLY so as not to be easily overridden by end users (your packaged version takes priority). 

Widgets usually involve improving UX or showing helpful UI interfaces, and are often considered things that users can turn on/off in the game to suit their needs, be it through settings or a widget manager UI.

To manage the invocation of Widgets & Gadgets a "**handler**" is setup in the Lua entrypoint. For environments with synced and unsyned entry points these typically use the same handler which calls the same widgets/gadgets but includes a flag they can check to determine if they are being run in synced mode or unsynced mode and behaving accordingly.

### Basic Addon Outline

Both widgets and gadgets usually follow a basic blueprint like this:

```lua
-- There is an injected global called addon/widget/gadget.
local addon = addon

function addon:GetInfo()
    return {
        name = "My addon", -- What it's called (Shows up in addon management menus, if there are any)
        description = "This is an addon", -- Description of your addon.
        author = "Me", -- Who wrote it
        date = "Present day, Present Time! Hahahahaha!", -- Nobody really cares about this.
        license = "GPL 3.0", -- License for other people who want to use your addon. Most code in Recoil games are GPL 3.0 or later.
        layer = 1, -- Affects execution order. Lower is earlier execution, and can go below 0. Useful if addons depend on eachother.
    }
end

-- Actual functional code

if addonHandler:isSynced() then
    -- Your synced code goes here.
    function addon:Initialize()
        Spring.Echo("We have liftoff!")
    end

    function addon:SomeSyncedCallin()
        Spring.Echo("Synced callin!")
    end

    function addon:Shutdown()
        Spring.Echo("They came from... behind...")
    end
else
    -- Your unsynced code egoes here.
    function addon:Initialize()
        Spring.Echo("My life for the horde!")
    end

    function addon:SomeUnsyncedCallin()
        Spring.Echo("Unsynced callin!")
    end

    function addon:Shutdown()
        Spring.Echo("Arrrgh!!!")
    end
end

```

## Handlers
Handlers are the code that runs in the entry point to load in addons/widgets/gadgets and distribute callins to them, and example implementation of a handler is includes in the [Recoil base content](https://github.com/beyond-all-reason/RecoilEngine/blob/master/cont/base/springcontent/LuaHandler/handler.lua), and will likely do well for a simple game, although many games eventually choose to make their own handlers.

Below is a very simplified pseudocode example of a handler is below to illustrate adding a gadget and forwarding callins to gadgets.
```lua
--- @class GadgetHandler
--- @field gadgets Gadget[]
GadgetHandler = {
    gadgets = {}
}

--- @param file string filepath to gadget
function GadgetHandler:add_gadget(self, file)
    local new_gadget = register_gadget(file)
    table.insert(self.gadgets, new_gadget)
end

--- @param delta number
function Update(self, delta)
    for _, gadget in self.gadgets do
        if gadget:Update then
            gadget:Update(delta)
        end
    end
end

Spring.UpdateCallin('Update')
```

This isn't how it's implemented exactly, but it illustrates the concept. The gadgets are loaded from a file and added to the handler's list of gadgets. When a callin is triggered, such as `Update` here, it calls `Update()` on all gadgets that have it overridden (be aware that the available callins vary by endpoint (Intro/Menu/Ui/Rules/Gaia) you can use Script.GetCallInList() to check what is available).

## Creating User Interfaces

Whether you decide to use a widget based approach or not you will inevitably need to create user interfaces in the game, there are two main options for doing this:-
- OpenGL drawing, this can be done with direct OpenGL commands using the GL global provided in Lua environment or using a framework like [FlowUi](https://github.com/beyond-all-reason/Beyond-All-Reason/blob/master/luaui/Widgets/flowui_gl4.lua) from Beyond All Reason or Chili [Chili](https://springrts.com/wiki/Chili) from the game lobby Chobby.
- Recoil has built in support for the [RmlUi framework](https://mikke89.github.io/RmlUiDoc/) which allows you to create user interfaces using HTML/CSS style markup, this was added in 2024. There is a recoil specific article on it here [article on it]({{% ref "rmlui" %}}).
