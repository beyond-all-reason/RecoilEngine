#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
"""Compare native and custom movement controllers in isolated headless BAR runs.

Example (use development assets, never the production write directory):
  python3 test/validation/custom_move_commands/run.py \
    --engine build-amd64-linux/spring-headless --game /path/to/BAR \
    --maps /path/to/development/maps --output /tmp/custom-move-validation

The default map is Red Comet Remake 1.8. Requires BAR's armfav unit. Checks exact
per-frame position, velocity and heading for two units, plus their preceding Move
finish frames. The unregistered case is a required negative control. Registration
must work globally without descriptions, survive description removal/ID changes,
be idempotent, and leave other command IDs unclassified. All cases run serially. With --attack-controller, also checks the real BAR
ATTACK_TARGETS controller against native attacks (requires a game with that gadget).

Manual reproduction: call Spring.RegisterMoveCommand(customID) from synced gadget
initialization, then queue Move followed by that custom controller on any unit.
The preceding Move should finish like Move followed by a native Move. The gadget
still implements its controller in CommandFallback; no command description needed.
"""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--game", type=Path, required=True)
    parser.add_argument("--maps", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--map", default="Red Comet Remake 1.8")
    parser.add_argument("--game-name", default="Beyond All Reason $VERSION")
    parser.add_argument("--port", type=int, default=50900)
    parser.add_argument("--attack-controller", action="store_true", help="Also compare BAR ATTACK_TARGETS with native attacks")
    args = parser.parse_args()
    engine, game, maps, output = [p.resolve() for p in (args.engine, args.game, args.maps, args.output)]
    assert engine.is_file() and game.is_dir() and maps.is_dir()
    output.mkdir(parents=True, exist_ok=False)
    with engine.open("rb") as handle:
        engine_sha = hashlib.file_digest(handle, "sha256").hexdigest()
    result = dict(passed=False, engine_sha256=engine_sha, game=str(game), map=args.map, cases={})
    traces = {}
    observer = (HERE / "observer.lua").read_bytes()
    result["observer_sha256"] = hashlib.sha256(observer).hexdigest()
    modes = ["native", "unregistered", "registered", "duplicate", "removed", "renamed", "other_id"]
    if args.attack_controller:
        modes += ["attack_native", "attack_controller"]
    try:
        for index, mode in enumerate(modes):
            root = output / mode
            overlay = root / "games/validation.sdd"
            (overlay / "luarules/gadgets").mkdir(parents=True)
            (root / "games/game.sdd").symlink_to(game)
            (root / "maps").symlink_to(maps)
            (overlay / "modinfo.lua").write_text(
                'return {name="Custom Move Validation",version="1",modtype=1,depend={' + json.dumps(args.game_name) + '}}\n')
            (overlay / "custom_move_config.lua").write_text(f'return {{mode="{mode}",finish=180}}\n')
            (overlay / "luarules/gadgets/dbg_custom_move.lua").write_bytes(observer)
            (root / "springsettings.cfg").write_text(
                f"HostPortDefault={args.port + index}\nNoSound=1\nLuaUI=0\nWorkerThreadCount=1\nHardwareThreadCount=1\n")
            (root / "startscript.txt").write_text(f"""[GAME]
{{
 MapName={args.map}; GameType=Custom Move Validation 1;
 IsHost=1; HostIP=127.0.0.1; MyPlayerName=Validation; GameStartDelay=0; StartPosType=0;
 FixedRNGSeed=1; RecordDemo=0; MinSpeed=20; MaxSpeed=20;
 [MODOPTIONS] {{ deathmode=neverend; }}
 [PLAYER0] {{ Name=Validation; Team=0; }}
 [TEAM0] {{ TeamLeader=0; AllyTeam=0; }}
 [ALLYTEAM0] {{ }}
 [TEAM1] {{ TeamLeader=0; AllyTeam=1; }}
 [ALLYTEAM1] {{ }}
}}
""")
            command = [str(engine), "--isolation", "--write-dir", str(root), str(root / "startscript.txt")]
            print("START", mode, flush=True)
            started = time.monotonic()
            with (root / "console.log").open("w") as log:
                completed = subprocess.run(command, cwd=root, stdout=log, stderr=subprocess.STDOUT, timeout=180)
            text = (root / "console.log").read_text(errors="replace")
            assert completed.returncode == 0 and "CUSTOM_MOVE_COMPLETE, 180" in text, root
            assert not any(s in text for s in ("RunCallInTraceback", "LUA_ERRRUN")), root
            trace = (root / "movement.tsv").read_bytes()
            rows = trace.decode().splitlines()
            assert [int(row.split("\t")[0]) for row in rows] == list(range(5, 181)), "Incomplete trace"
            traces[mode] = trace
            finish_frames = dict(map(int, row.split()) for row in (root / "first-move.txt").read_text().splitlines())
            assert set(finish_frames) == {1, 2}, "Missing unit completion"
            result["cases"][mode] = dict(first_move_frames=finish_frames,
                                         trace_sha256=hashlib.sha256(trace).hexdigest(), seconds=time.monotonic() - started)
            print("DONE", mode, result["cases"][mode], flush=True)
            if mode == "unregistered":
                assert traces[mode] != traces["native"], "Negative control did not reproduce the distinction"
                native_frames = result["cases"]["native"]["first_move_frames"]
                assert all(finish_frames[unit] >= native_frames[unit] for unit in (1, 2))
                assert any(finish_frames[unit] > native_frames[unit] for unit in (1, 2))
        for mode in ("registered", "duplicate", "removed", "renamed"):
            assert traces[mode] == traces["native"], f"Movement differs: {mode}"
            assert result["cases"][mode]["first_move_frames"] == result["cases"]["native"]["first_move_frames"]
        assert traces["other_id"] == traces["unregistered"], "Registration affected another command ID"
        if args.attack_controller:
            assert traces["attack_controller"] == traces["attack_native"], "BAR attack controller differs from native attacks"
            assert result["cases"]["attack_controller"]["first_move_frames"] == result["cases"]["attack_native"]["first_move_frames"]
        result["passed"] = True
    except BaseException as error:
        result["error"] = repr(error)
        raise
    finally:
        (output / "summary.json").write_text(json.dumps(result, indent=2) + "\n")
        print("Results:", output / "summary.json", flush=True)


if __name__ == "__main__":
    main()
