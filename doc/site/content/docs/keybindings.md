+++
title = 'Keybindings Guide'
date = 2025-09-09T14:11:37-07:00
draft = true
+++

# Keybindings Guide

## Overview

Bindings are done using actions and keysets. A binding looks like this: `bind <keyset> <action>`, e.g.: `bind ctrl+f12 say hi from ctrl+f12`.

Somethingsomething uikeys.txt and

## Keysets

The key combination from keyboard input the action will be bound to.

You can bind to keycodes (the character you see labeled on your keyboard) or scancodes (the position of the key on the keyboard, qwerty as reference).

TLDR: Use keycodes when writing bindings to yourself, use scancodes if you want to share your bindings to players using different keyboard layouts.

### Keycodes

Keycodes change when the keyboard layout changes, for example:

```
bind z say hi from z
bind y say hi from y
```

In both [qwerty] and [qwertz] keyboards pressing z will say `hi from z`, but the position of the key is completely different.

Pressing the same position of `z` in a qwertz keyboard will say `hi from y` (`y` is the key in that position on qwertz).

Use when you don't care about portability between keyboard layouts or you need the exact key label to be bound.

All ascii characters can be used as keycodes, e.g. `a`, `b`, etc.

Use `/keysyms` in chat to see a list of all available keycodes or see [here](https://github.com/beyond-all-reason/RecoilEngine/blob/master/rts/Game/UI/KeyCodes.cpp#L64-L146).

### Scancodes

Keycodes don't change when the keyboard layout changes, for example:

```
bind sc_z say hi from sc_z
bind sc_y say hi from sc_y
```

Pressing `z` in a [qwerty] keyboard will say `hi from sc_z`. Pressing `y` on a [qwertz] keyboard will say `hi from sc_z` (notice the position on the keyboard didn't change).

Scancodes are mostly similar to keycodes, with the `sc_` prefix, e.g. `sc_a`, `sc_b`, etc. See [here](https://github.com/beyond-all-reason/RecoilEngine/blob/master/rts/Game/UI/ScanCodes.cpp#L60-L224) for a list of all available scancodes.

[qwerty]: https://www.kbdlayout.info/KBDUS/
[qwertz]: https://www.kbdlayout.info/kbdgr/

### Modifiers

The following modifiers are available: `alt`, `ctrl`, `shift`, `meta`, `any`.

Keysets with modifiers look like this: `<modifier>+<key>`, e.g. `alt+f1`.

Meta is the "windows key" originally, but often is rebound to `space` by games (since the windows key in windows can't be configured).

Any means "any modifiers", useful for when an action should trigger regardless of modifiers pressed, e.g. `any+f1` will be triggered for `f1`, `alt+f1`, `shift+alt+f1`, etc.

Modifiers are the same for keycodes and scancodes.

### Keychains

Describe keychains...

## Actions

The operation the game or engine will execute when triggered, e.g.: `group select 1` selects all units in group 1.

Some actions are done by the game and some by the engine, in [this file](https://github.com/resopmok/BAR_uikeys_collections/blob/main/keybinds-explained) we describe
the most common actions but its not a complete list (game might have changed or added actions over time).

Some of the engine actions are described [here](https://beyond-all-reason.github.io/RecoilEngine/docs/unsynced-commands/).

### Action fallback

Describe action fallback mechanism...

## Defining bindings

Below are all available commands that can be issued.

### Bind

Binds a keyset to an action:

```
bind <keyset> 
```
