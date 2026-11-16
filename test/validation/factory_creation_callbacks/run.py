#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
"""Run a factory callback case using an isolated overlay of an installed BAR package."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("engine", type=Path)
p.add_argument("output", type=Path)
p.add_argument("--assets", required=True, type=Path, help="development data containing maps/packages/pool")
p.add_argument("--base", required=True, type=Path, help="engine base archive directory")
p.add_argument("--game", default="Beyond All Reason test-31691-a617033")
p.add_argument("--case", choices=("normal", "lua-created", "alt-created"), required=True)
a = p.parse_args()
root = a.output.resolve()
root.mkdir(parents=True, exist_ok=False)
for name in ("maps", "packages", "pool"):
    (root / name).symlink_to(a.assets.resolve() / name)
(root / "base").symlink_to(a.base.resolve())
overlay = root / "games/factory-callback.sdd"
(overlay / "luarules/gadgets").mkdir(parents=True)
(overlay / "modinfo.lua").write_text('return {name="Factory Callback Test", version="1", modtype=1, depend={' + json.dumps(a.game) + '}}\n')
shutil.copyfile(Path(__file__).with_name("check.lua"), overlay / "luarules/gadgets/test_factory_callback.lua")
(overlay / "luarules/gadgets/unit_prevent_lab_hax2.lua").write_text("return false\n")
(root / "input.txt").write_text("""[GAME] {
MapName=Quicksilver Remake 1.24;
GameType=Factory Callback Test 1;
IsHost=1; HostIP=127.0.0.1; HostPort=18453; MyPlayerName=Validator;
FixedRNGSeed=123456789; StartPosType=3;
NumPlayers=1; NumUsers=1; NumTeams=2; NumAllyTeams=2;
[PLAYER0] {Name=Validator; Team=0; Spectator=0;}
[TEAM0] {TeamLeader=0; AllyTeam=0; Side=Cortex; StartPosX=512; StartPosZ=512;}
[TEAM1] {TeamLeader=0; AllyTeam=1; Side=Armada; StartPosX=4096; StartPosZ=4096;}
[ALLYTEAM0] {NumAllies=0;}
[ALLYTEAM1] {NumAllies=0;}
[MODOPTIONS] {deathmode=never; startpostype=2; maxspeed=100; minspeed=100; factorycallbackcase=""" + a.case + ";}\n}\n")
(root / "springsettings.cfg").write_text("LuaUI = 0\nSound = 0\nWorkerThreadCount = 2\nPathingThreadCount = 1\nLogFlush = 1\n")
command = [str(a.engine.resolve()), "--only-local", "--isolation", "--isolation-dir=" + str(root), "--write-dir=" + str(root), "--config=" + str(root / "springsettings.cfg"), str(root / "input.txt")]
try:
    with (root / "console.log").open("w") as log:
        result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, timeout=120)
    log = (root / "infolog.txt").read_text(errors="replace")
    passed = result.returncode == 0 and "[FactoryCallback] PASS " + a.case in log and "LuaRules::RunCallIn" not in log
    with a.engine.open("rb") as binary:
        digest = hashlib.file_digest(binary, "sha256").hexdigest()
    (root / "result.json").write_text(json.dumps({"case": a.case, "passed": passed, "exit_code": result.returncode, "engine": str(a.engine.resolve()), "engine_sha256": digest, "game": a.game}, indent=2) + "\n")
    print((root / "result.json").read_text())
    raise SystemExit(0 if passed else 1)
finally:
    shutil.rmtree(overlay)
