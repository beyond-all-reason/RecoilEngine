---
title: "CreateCommandOptions"
url: "docs/lua-api/CreateCommandOptions"
---
# alias CreateCommandOptions
---



```lua
(alias) CreateCommandOptions = (CommandOptionName[]|table<CommandOptionName,boolean>|CommandOptionBit|integer)
    | CommandOptionName[] -- An array of option names.
    | table<...> -- A map of command names to booleans, considered held when `true`.
    | CommandOptionBit -- A specific integer value for a command option.
    | integer -- A bit mask combination of `CommandOptionBit` values. Pass `0` for no options.

```




