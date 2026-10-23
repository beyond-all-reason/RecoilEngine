#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
"""Run the BAR command-repeat control in an isolated write directory."""
import argparse
import csv
import os
from pathlib import Path
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--engine", type=Path, required=True)
parser.add_argument("--game", type=Path, required=True, help="Disposable BAR checkout/copy")
parser.add_argument("--map", type=Path, required=True, help="Supreme Isthmus v2.1 archive")
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--expect-baseline", action="store_true")
args = parser.parse_args()
engine, game, mapfile, output = (p.resolve() for p in (args.engine, args.game, args.map, args.output))
output.mkdir(parents=True, exist_ok=False)
for name in ("games", "maps"):
    (output / name).mkdir()
(output / "games" / "BAR.sdd").symlink_to(game)
(output / "maps" / mapfile.name).symlink_to(mapfile)
shutil.copyfile(Path(__file__).with_name("check.lua"), game / "luarules/gadgets/dbg_command_repeat.lua")
# BAR disables repeat for turrets; enable it in this disposable fixture to
# exercise CCommandAI as well as MobileCAI, AirCAI and BuilderCAI.
turret = game / "units/ArmBuildings/LandDefenceOffence/armllt.lua"
turret.write_text(turret.read_text().replace("canrepeat = false", "canrepeat = true"))
(output / "springsettings.cfg").write_text("LuaUI=0\nLuaMenu=0\nNoSound=1\nWorkerThreadCount=2\n")
script = output / "startscript.txt"
script.write_text("""[GAME]
{
    MapName=Supreme Isthmus v2.1;
    GameType=Beyond All Reason $VERSION;
    GameStartDelay=0;
    StartPosType=0;
    RecordDemo=0;
    MyPlayerName=Validation;
    IsHost=1;
    HostIP=127.0.0.1;
    HostPort=0;
    FixedRNGSeed=1;
    [MODOPTIONS] { debugcommands=1:cheat|2:godmode; deathmode=neverend; }
    [ALLYTEAM0] {}
    [ALLYTEAM1] {}
    [TEAM0] { allyteam=0; teamleader=0; }
    [TEAM1] { allyteam=1; teamleader=0; }
    [PLAYER0] { name=Validation; team=0; }
}
""")
env = {k: v for k, v in os.environ.items() if k not in ("SPRING_DATADIR", "SPRING_WRITEDIR")}
with (output / "console.log").open("w") as log:
    result = subprocess.run([str(engine), "--isolation", "--write-dir", str(output), str(script)],
                            cwd=output, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=180)
assert result.returncode == 0, f"Engine failed: {output / 'console.log'}"
assert "RunCallInTraceback" not in (output / "console.log").read_text(errors="replace")
with (output / "command-repeat.csv").open() as source:
    rows = {row[0]: list(map(int, row[1:])) for row in csv.reader(source)}
assert rows.pop("finished") == [300]
assert len(rows) == 14, rows
for name, (done, queued, target) in sorted(rows.items()):
    if name.startswith("self_") or name == "crashing":
        assert (done > 1 and queued == 1) if args.expect_baseline else (done == 1 and queued == 0), (name, done, queued)
    elif name.startswith("lost_") or name in ("transported", "area_repeat"):
        assert done > 1 and queued == 1, (name, done, queued)
    elif name == "area_persistent":
        assert done == 0 and queued == 1, (name, done, queued)
    elif name == "successor":
        assert done == 1 and queued == (2 if args.expect_baseline else 1), (name, done, queued)
    elif name == "repeat_off":
        assert done == 1 and queued == 0, (name, done, queued)
    print(f"PASS {name}: completions={done}, queued={queued}")
print(f"PASS: {output}")
