---
layout: post
title: Lua environments
parent: Articles
permalink: articles/lua-environments
author: sprunk
---

# Widget vs gadget basics

The naive description of how things work in Recoil is that there's three kinds of active Lua code: **widgets and gadgets** (sometimes collectively referred to as wupgets),
with **gadgets split into synced and unsynced**. From the outside they look very similar to each other in that they are **little Lua programs that work by hooking into events**,
but they serve different roles.

## Widgets (aka LuaUI)

Widgets are, in a way, an **extension of the player**.
They can **see and do what a player can see and do**, and as such are generally used to implement **pieces of UI that help the player** read the battlefield and/or control his units.

Examples of widget tasks:
 * displays such as resource bars, scoreboards, or healthbars.
 * menus such as unit control panel, or settings (hotkeys, video/audio etc).
 * implementing default rightclick or mouse-drag behaviours.
 * render visuals such as rain, burning units, or the graphical part of day/night cycle (but not e.g. the income of solar panels).
 * giving orders units when they're idle or near an enemy.
 * collecting stats from replays.

By default, **players can run their own widgets** and have access to the full interface, so as an extreme case this lets players do things like implement some sort of AI that plays for them,
run a shader that makes subtle visuals much more visible, or remove various graphics. Games that don't want this to happen can remove specific interfaces or just disallow loading local code.

Another caveat is that since widgets work on behalf of a player, they are subject to network lag. Many widget requests (e.g. giving orders) take a roundtrip via the server,
so gameplay mechanics should provide information to widgets unprompted so that it's available immediately.

## Gadgets (aka LuaRules)

Both synced and unsynced gadgets have **full read access to the gamestate** (for example, callouts such as `Spring.GetUnitHealth` are not subject to line of sight checks) and,
by default, have to be included with a game (i.e. **players can't write their own gadgets**), though they offer controlled ways to read data passed from outside.
A caveat is that typically, **each gadget file (i.e. `LuaRules/Gadgets/foo.lua`) runs twice**, both as a synced and unsynced gadget, and the way this works is that
each "half" is under a **different branch of an `if synced` check**. This convention is a bit confusing and can lead to errors, but gadgetry often wants to do something
both in synced and unsynced so it's convenient to have both in the same file.

Another caveat is that since widgets work on behalf of a player, they are subject to network lag. Any order the widget sends is subject to the server network roundtrip.
Any sort of information published by a widget (e.g. "my controlling player's cursor is at XY and the camera is looking at XYZ") can be outdated by the time it reaches the server and other players.

### Synced gadgets (aka synced LuaRules)

Synced gadgets implement **game mechanics**.
They can **control the simulation gamestate** by virtue of having access to more callouts (such as `Spring.SetUnitHealth`), and are also partially responsible for **information flow**.

Examples of synced gadget tasks:
 * implement abilities (e.g. blink or stimpaks), including things like global support powers.
 * implement events, e.g. meteor showers or rising tides.
 * implement status effects, either for units (de/buffs, mana), teams (research and upgrades), or the world (e.g. day/night cycle that affects mechanics such as solar income).
 * implement opponent AI, either to play a "normal" faction or custom PvE.
 * implement custom resources.
 * calculate and publicly expose data based on infromation not otherwise available (think FFA scoreboard in age of empires).
 * send events to other Lua states, e.g. "spawn some particles at XYZ, there's a hidden unit there you can't otherwise see".

Synced gadgets cannot do anything related to rendering or that requires the concept of a "local player", and the exact same code has to run for every player in the game. For example:
 * you cannot do "heal _my_ units", even in singleplayer. You have to do "heal units _of player X_" and figure out who the X should be via other clues.
 * you cannot render things. You have to either tell widgets how to do it (keeping in mind they may not necessarily know the details that the synced gadget does, e.g. the exact type of a unit that could be in the fog from player PoV) or use an unsynced gadget.
 * you cannot do things like "do X if camera is over Y", because camera location is different per-player. Widgets could send their camera locations to synced, but this is subject to network roundtrip as mentioned in the widget section, and the synced gadget would have to process every player's info.
 * you can have per-player settings, but not the GUI to control them (widget has to handle GUI, and then send it to synced) or things like directly saving to a local file (also needs to be laundered via widget).

### Unsynced gadgets (aka unsynced LuaRules)

Unsynced gadgets are a mix of widgets synced gadgets and widgets - they have **full read access like a synced gadget, but can do rendering and know who the local player is**.
In theory, **everything can be done via widgets and synced gadgets** so unsynced gadgets are a bit redundant, but they make things **much more convenient to work with with a bit of performance savings**.

For example:
 * an enemy unit does something that causes it to spawn some gfx, even in the fog. A widget cannot know that unit is doing it or even that it is there at all, while synced gadgets can't do rendering. They could talk to each other, or an unsynced gadget can just check the unit itself (since it has full read access) while avoiding having to recalculate it for every player (since synced has to run the same code for everybody), and then do the rendering all in one go.
 * you could do the rendering part of a day/night cycle in an unsynced gadget and then completely remove the relevant interfaces from widgets, to prevent players from screwing things up. With widgets, you'd have to go a level higher and prevent most widgets from accessing the interface while still keeping it available for that one dedicated widget supposed to handle it.
 * for performance-heavy things like particles for large amounts of projectiles, the performance savings of not having to calculate it for every team (think 16-way FFA) and avoiding the overhead of cross-environment communication is a value of its own.

# Lua environments - advanced

The above is mostly useful from the PoV of individual contributors to a large game, or a beginner game developer.
But a mature game might want to delve deeper and deal with internals rather than just individual wupgets.

### Handlers
The **engine does not have the concept of individual wupgets**. It just sees a bunch of **monolith environments** like "synced LuaRules", and sends events there.
On the Lua side, there is a **widget/gadget handler** respectively that hooks into the events received directly by the engine, and **distributes events to individual widgets**,
which hook into the handler.

The handler's hook functions tend to be trivial and do nothing more than just loop over known wupgets and forward calls, but you can also do things like:
 * adjust args (add extras for convenience, or remove args for information control)
 * wrap calls for safety (pcall) or logging/tracing
 * add or remove problematic interfaces (via `Spring.Foo = nil`) before wupget even run

### Other envs

TODO: say something about luagaia, luaintro, luamenu, and the dozen or so luaparsers

### Synopsis

TODO: which env has access to which `spring.foo`/`math.foo` etc callouts, which `wupget:foo` callins, what other metadata like readTeam etc.

### MAYBE: Other event clients

TODO: consider whether to mention skirmish AI, and/or address engine philosophy on gameside C++
