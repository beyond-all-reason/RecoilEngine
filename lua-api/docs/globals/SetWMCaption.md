---
layout: default
title: SetWMCaption
parent: Lua API
permalink: lua-api/globals/SetWMCaption
---

{% raw %}

# global SetWMCaption

---

```lua
function SetWMCaption(
  title: string,
  titleShort: string?
) ->  nil
```
@param `titleShort` - (Default: title)







Sets the window title for the process (default: "Spring <version>").

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/197053673559ebf8a6285a26a97956a0b85365a2/rts/Lua/LuaUnsyncedCtrl.cpp#L5249-L5260" target="_blank">source</a>]

The shortTitle is displayed in the OS task-bar (default: "Spring <version>").

NOTE: shortTitle is only ever possibly used under X11 (Linux & OS X), but not with QT (KDE) and never under Windows.


{% endraw %}