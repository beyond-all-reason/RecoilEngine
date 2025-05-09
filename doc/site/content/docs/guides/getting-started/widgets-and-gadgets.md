---
title: "Widgets and Gadgets"
weight: 1
---

Strictly speaking, Recoil does not have a concept of UI and game rules. It simply has a `draw` command, and a number of so-called "callins" that lua scripts can respond to. Most games, however, have abstractions on top of this to make these much simpler. By far the most pervasive of these are the "Widgets" and "Gadgets", collectively known as "Addons". Pretty much everything you will be doing during the course of developing your game will concern addons, unit defs, and feature defs. Let's take a look at some of the concepts here.

## Addons

Addons are files that override callins. For a refresher, callins (or callbacks) are functions that are invoked by the engine to define specific behavior. For those that are familiar with other engines, this is analagous to the virtual script functions in others like `func _process(delta: float)` in Godot, or `void Update(float delta)` in Unity. This isn't exact, however; for more information, see the "Handlers" section, but at the high level this is the general concept.

### Synced and Unsynced

This is an incredibly importand concept in regards to addons. When you write an addon, you are really writing *two* addons: one that runs in a synced context, and one in an unsynced context. But, what does this mean?
- Synced code is anything that affects the game simulation: ordering units around, etc. since it runs on a fixed tick rate.
- Unsynced code is anything that doesn't care about the tick rate of the simulation state, like reading unit position, etc.

### Basic Outline

Both widgets and gadgets follow this basic blueprint:

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

## Widgets - UI

Widgets are addons that specialize in adding UI to your game. Synced code for these are buttons that affect the world, such as issuing commands. These lie in the "LuaUI/Widgets" folder. There are a few UI frameworks available to you if you'd rather not make your own. The major ones are:
- Chili: A legacy UI framework using a retained-mode model. It's largely unmaintained and should be considered EOL, but it still functions.
- RmlUi: A brand-new HTML/CSS style UI framework. If you have experience with web dev, you will feel at home here. It's very new, so the documentation and implementation has some rough edges. See the [article on it](docs/guides/ui/getting started with rmlui.md).

TODO: Example
TODO: Link to docs

## Gadgets - Game logic

Gadgets are anything that affects how the game works: unit commands, behaviors, that sort of thing. This is where the vast majority of your game code and logic will lie. These lie in the "LuaRules/Gadgets" folder.

TODO: An example of a unit action
TODO: Link to docs

## Handlers

Let's talk about *how* addons are implemented. This is by using "**handlers**". Example implementations of these handlers are found in [basecontent]({{% ref "/docs/guides/getting-started/basecontent" %}}), and do well for a simple game, although many games eventually choose to make their own handlers.

What *are* handlers, though? They keep track of registered addons, and forward callins to them. They can be thought of as sort of like an API endpoint, for the web devs out there. The handler receives callins from the engine, and then forwards them to the addons. Let's take a look at a pseudocode handler:

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
function callins:Update(self, delta)
    for _, gadget in self.gadgets do
        if gadget:Update then
            gadget:Update(delta)
        end
    end
end
```

This isn't how it's implemented exactly, but it illustrates the concept. The gadgets are loaded from a file and added to the handler's list of gadgets. When a callin is triggered, such as `Update` here, it calls `Update()` on all gadgets that have it overridden.
