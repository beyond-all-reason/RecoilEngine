---
title: "CallAsTeam"
permalink: "docs/lua-api/CallAsTeam"
---

# global CallAsTeam

---

```lua
function CallAsTeam(
  teamID: integer,
  func: fun(...),
  ...: any
) -> The any ...
```
@param `teamID` - Team ID.

@param `func` - The function to call.

@param `...` - Arguments to pass to the function.


@return `The` - return values of the function.






Calls a function from given team's PoV. In particular this makes callouts obey that team's visibility rules.

