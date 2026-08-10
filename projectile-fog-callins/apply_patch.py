#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(relpath: str, old: str, new: str) -> None:
    path = ROOT / relpath
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"Expected exactly one match in {relpath}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8", newline="\n")
    print(f"Patched {relpath}")


# ---------------------------------------------------------------------------
# 1) Register the two new managed unsynced draw events.
# ---------------------------------------------------------------------------
replace_once(
    "rts/System/Events.def",
    "\tSETUP_EVENT(DrawWorld,                 MANAGED_BIT | UNSYNCED_BIT)\n"
    "\tSETUP_EVENT(DrawWorldPreUnit,          MANAGED_BIT | UNSYNCED_BIT)\n",
    "\tSETUP_EVENT(DrawWorld,                 MANAGED_BIT | UNSYNCED_BIT)\n"
    "\tSETUP_EVENT(DrawWorldPreUnit,          MANAGED_BIT | UNSYNCED_BIT)\n"
    "\tSETUP_EVENT(DrawWorldPreProjectiles,   MANAGED_BIT | UNSYNCED_BIT)\n"
    "\tSETUP_EVENT(DrawWorldPreAlpha,         MANAGED_BIT | UNSYNCED_BIT)\n",
)

replace_once(
    "rts/System/EventClient.h",
    "\t\tvirtual void DrawWorld() {}\n"
    "\t\tvirtual void DrawWorldPreUnit() {}\n"
    "\t\tvirtual void DrawPreDecals() {}\n",
    "\t\tvirtual void DrawWorld() {}\n"
    "\t\tvirtual void DrawWorldPreUnit() {}\n"
    "\t\tvirtual void DrawWorldPreProjectiles() {}\n"
    "\t\tvirtual void DrawWorldPreAlpha() {}\n"
    "\t\tvirtual void DrawPreDecals() {}\n",
)

replace_once(
    "rts/System/EventHandler.h",
    "\t\tvoid DrawWorld();\n"
    "\t\tvoid DrawWorldPreUnit();\n"
    "\t\tvoid DrawPreDecals();\n",
    "\t\tvoid DrawWorld();\n"
    "\t\tvoid DrawWorldPreUnit();\n"
    "\t\tvoid DrawWorldPreProjectiles();\n"
    "\t\tvoid DrawWorldPreAlpha();\n"
    "\t\tvoid DrawPreDecals();\n",
)

replace_once(
    "rts/System/EventHandler.cpp",
    "DRAW_CALLIN(World)\n"
    "DRAW_CALLIN(WorldPreUnit)\n"
    "DRAW_CALLIN(PreDecals)\n",
    "DRAW_CALLIN(World)\n"
    "DRAW_CALLIN(WorldPreUnit)\n"
    "DRAW_CALLIN(WorldPreProjectiles)\n"
    "DRAW_CALLIN(WorldPreAlpha)\n"
    "DRAW_CALLIN(PreDecals)\n",
)

# ---------------------------------------------------------------------------
# 2) Expose the call-ins through CLuaHandle.
# ---------------------------------------------------------------------------
replace_once(
    "rts/Lua/LuaHandle.h",
    "\t\tvoid DrawWorld() override;\n"
    "\t\tvoid DrawWorldPreUnit() override;\n"
    "\t\tvoid DrawPreDecals() override;\n",
    "\t\tvoid DrawWorld() override;\n"
    "\t\tvoid DrawWorldPreUnit() override;\n"
    "\t\tvoid DrawWorldPreProjectiles() override;\n"
    "\t\tvoid DrawWorldPreAlpha() override;\n"
    "\t\tvoid DrawPreDecals() override;\n",
)

replace_once(
    "rts/Lua/LuaHandle.cpp",
    "/*** Spring draws units, features, some water types, cloaked units, and the sun.\n"
    " *\n"
    " * @function Callins:DrawWorldPreUnit\n"
    " */\n"
    "DRAW_CALLIN(DrawWorldPreUnit)\n\n"
    "/*** Called before decals are drawn\n",
    "/*** Spring draws units, features, some water types, cloaked units, and the sun.\n"
    " *\n"
    " * @function Callins:DrawWorldPreUnit\n"
    " */\n"
    "DRAW_CALLIN(DrawWorldPreUnit)\n\n"
    "/*** Called after opaque units and features, immediately before opaque projectiles.\n"
    " *\n"
    " * Useful for world-space compositing effects that should remain behind projectiles.\n"
    " *\n"
    " * @function Callins:DrawWorldPreProjectiles\n"
    " */\n"
    "DRAW_CALLIN(DrawWorldPreProjectiles)\n\n"
    "/*** Called after all opaque world objects and immediately before the alpha pass.\n"
    " *\n"
    " * @function Callins:DrawWorldPreAlpha\n"
    " */\n"
    "DRAW_CALLIN(DrawWorldPreAlpha)\n\n"
    "/*** Called before decals are drawn\n",
)

# New call-ins use the same world drawing state/matrices as DrawWorld.
replace_once(
    "rts/Lua/LuaOpenGL.h",
    "\t\tstatic void EnableDrawWorldPreUnit();\n"
    "\t\tstatic void ResetDrawWorldPreUnit();\n"
    "\t\tstatic void DisableDrawWorldPreUnit();\n\n"
    "\t\tNOOP_STATE_FUNCS(DrawPreDecals)\n",
    "\t\tstatic void EnableDrawWorldPreUnit();\n"
    "\t\tstatic void ResetDrawWorldPreUnit();\n"
    "\t\tstatic void DisableDrawWorldPreUnit();\n\n"
    "\t\tinline static void EnableDrawWorldPreProjectiles() { EnableDrawWorld(); }\n"
    "\t\tinline static void ResetDrawWorldPreProjectiles() { ResetDrawWorld(); }\n"
    "\t\tinline static void DisableDrawWorldPreProjectiles() { DisableDrawWorld(); }\n\n"
    "\t\tinline static void EnableDrawWorldPreAlpha() { EnableDrawWorld(); }\n"
    "\t\tinline static void ResetDrawWorldPreAlpha() { ResetDrawWorld(); }\n"
    "\t\tinline static void DisableDrawWorldPreAlpha() { DisableDrawWorld(); }\n\n"
    "\t\tNOOP_STATE_FUNCS(DrawPreDecals)\n",
)

# ---------------------------------------------------------------------------
# 3) Emit the call-ins at stable render boundaries without moving DrawWorld.
# ---------------------------------------------------------------------------
replace_once(
    "rts/Rendering/WorldDrawer.cpp",
    "\tDrawOpaqueObjects();\n"
    "\tDrawAlphaObjects();\n",
    "\tDrawOpaqueObjects();\n"
    "\teventHandler.DrawWorldPreAlpha();\n"
    "\tDrawAlphaObjects();\n",
)

replace_once(
    "rts/Rendering/WorldDrawer.cpp",
    "\t{\n"
    "\t\tSCOPED_TIMER(\"Draw::World::Models::Opaque\");\n"
    "\t\tSCOPED_GL_DEBUGGROUP(\"Draw::World::Models::Opaque\");\n"
    "\t\tunitDrawer->Draw(false);\n"
    "\t\tfeatureDrawer->Draw(false);\n"
    "\t}\n"
    "\t{\n"
    "\t\tSCOPED_TIMER(\"Draw::World::Models::Projectiles\");\n",
    "\t{\n"
    "\t\tSCOPED_TIMER(\"Draw::World::Models::Opaque\");\n"
    "\t\tSCOPED_GL_DEBUGGROUP(\"Draw::World::Models::Opaque\");\n"
    "\t\tunitDrawer->Draw(false);\n"
    "\t\tfeatureDrawer->Draw(false);\n"
    "\t}\n"
    "\n"
    "\teventHandler.DrawWorldPreProjectiles();\n"
    "\n"
    "\t{\n"
    "\t\tSCOPED_TIMER(\"Draw::World::Models::Projectiles\");\n",
)

# ---------------------------------------------------------------------------
# 4) Route the call-ins through the stock LuaUI widget handler.
# ---------------------------------------------------------------------------
replace_once(
    "cont/LuaUI/callins.lua",
    "\t\"DrawWorld\",\n"
    "\t\"DrawWorldPreUnit\",\n"
    "\t\"DrawWorldPreParticles\",\n",
    "\t\"DrawWorld\",\n"
    "\t\"DrawWorldPreUnit\",\n"
    "\t\"DrawWorldPreProjectiles\",\n"
    "\t\"DrawWorldPreAlpha\",\n"
    "\t\"DrawWorldPreParticles\",\n",
)

replace_once(
    "cont/LuaUI/widgets.lua",
    "  'DrawWorld',\n"
    "  'DrawWorldPreUnit',\n"
    "  'DrawWorldPreParticles',\n",
    "  'DrawWorld',\n"
    "  'DrawWorldPreUnit',\n"
    "  'DrawWorldPreProjectiles',\n"
    "  'DrawWorldPreAlpha',\n"
    "  'DrawWorldPreParticles',\n",
)

print("Projectile fog call-ins applied successfully.")
