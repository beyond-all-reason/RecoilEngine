+++
title = 'Basecontent'
date = 2025-05-09T00:12:22-07:00
draft = false
+++

*This article is a  stub. You can help by expanding it.*

Recoil's focus as an engine is unit management and rendering. Pretty much everything else is up to you. This can be... intimidating, like being asked to build a car that has just an engine. So, over the years, the community has accrewed a suite of Lua scripts that cover much of the low-level stuff. This is where the idea of gadgets and widgets comes from; they are not actually part of the engine, but instead an abstraction over the bare bones of Recoil that the community developed.

Eventually, it was rolled into the engine, and the [VFS]({{% ref "/docs/guides/getting-started/vfs-basics" %}}) will fall back on these scripts if you do not override them. You can, of course, copy the scripts and modify them to suit your needs, as most games eventually do. You can find them [here](https://github.com/beyond-all-reason/RecoilEngine/tree/master/cont/base/springcontent).

> [!WARNING]
> Basecontent is due to have a major reorganization at some point in the near future. Also, the basecontent for LuaUI is [here](https://github.com/beyond-all-reason/RecoilEngine/tree/master/cont/LuaUI) for some reason. See [This issue](https://github.com/beyond-all-reason/RecoilEngine/issues/1483#issuecomment-2863792961).
