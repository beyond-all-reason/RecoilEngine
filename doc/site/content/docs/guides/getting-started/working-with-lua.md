+++
title = 'Working with Lua'
aliases = ['/docs/guides/getting-started/lua-language-server/']
date = 2025-05-10T01:45:27-07:00
draft = false
+++

Recoil provides a Lua library of type definitions, for type checking and intellisense in your IDE. It is written for [EmmyLua](https://github.com/EmmyLuaLs/emmylua-analyzer-rust).

### Adding the library to your project

The [recoil-lua-library](https://github.com/beyond-all-reason/recoil-lua-library) package provides type definitions for your editor. This can be added as a submodule to your project (following its usage guide).

### IDE support

When working with Lua, we recommend the EmmyLua language server. See its [editor setup](https://github.com/EmmyLuaLs/emmylua-analyzer-rust#editor-setup) for VS Code, Neovim and IntelliJ.
