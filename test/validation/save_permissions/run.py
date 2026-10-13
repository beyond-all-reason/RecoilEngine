#!/usr/bin/env python3
"""Check save commands through LuaUI in isolated single/multiplayer setups."""

import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tempfile


def run_case(args, name, multiplayer, cheats):
    root = Path(tempfile.mkdtemp(prefix=f"save-permissions-{name}-"))
    try:
        (root / "maps").symlink_to(args.assets / "maps", target_is_directory=True)
        fonts = args.engine.parent / "fonts"
        if not fonts.is_dir():
            fonts = args.engine.parent.parent / "cont/fonts"
        (root / "fonts").symlink_to(fonts, target_is_directory=True)
        games = root / "games"
        games.mkdir()
        (games / "bar.sdd").symlink_to(args.bar, target_is_directory=True)
        overlay = games / "validation.sdd"
        overlay.mkdir()
        (overlay / "modinfo.lua").write_text(
            'return {name="Save Permissions Validation",version="1",modtype=1,'
            'depend={"Beyond All Reason $VERSION"}}\n'
        )
        shutil.copyfile(Path(__file__).with_name("luaui.lua"), overlay / "luaui.lua")
        for directory in ("LuaRules", "LuaGaia"):
            (overlay / directory).mkdir()
            for filename in ("main.lua", "draw.lua"):
                (overlay / directory / filename).write_text("return true\n")
        script = root / "startscript.txt"
        extra_player = "[PLAYER1] { Name=OtherPlayer; Spectator=1; }" if multiplayer else ""
        script.write_text(f"""[GAME] {{
 MapName={args.map}; GameType=Save Permissions Validation 1;
 IsHost=1; HostIP=127.0.0.1; HostPort=0; MyPlayerName=Validation;
 GameStartDelay=0; StartPosType=0; RecordDemo=0; FixedRNGSeed=1;
 [MODOPTIONS] {{ save_validation_cheats={int(cheats)}; }}
 [PLAYER0] {{ Name=Validation; Team=0; }}
 {extra_player}
 [TEAM0] {{ TeamLeader=0; AllyTeam=0; }} [ALLYTEAM0] {{ }}
}}\n""")
        (root / "springsettings.cfg").write_text(
            "Windowed = 1\nXResolutionWindowed = 640\nYResolutionWindowed = 480\n"
            "Fullscreen = 0\nSound = 0\nShadows = 0\n"
            "FontFile = fonts/FreeSansBold.otf\nSmallFontFile = fonts/FreeSansBold.otf\n"
        )
        saves = root / "Saves"
        saves.mkdir()
        sentinel = b"save-permissions-overwrite-sentinel\n"
        for extension in ("ssf", "slsf"):
            (saves / f"overwrite.{extension}").write_bytes(sentinel)
        with (args.results / f"{name}-console.log").open("w") as output:
            result = subprocess.run(
                [str(args.engine), "--only-local", "--isolation", "--write-dir", str(root),
                 "--config", str(root / "springsettings.cfg"), str(script)],
                stdout=output, stderr=subprocess.STDOUT, timeout=180,
            )
        log = (root / "infolog.txt").read_text(errors="replace")
        (args.results / f"{name}-infolog.txt").write_text(log)
        assert result.returncode == 0, f"{name}: exit {result.returncode}"
        assert log.count("[SavePermissions] attempt ") == 4, f"{name}: incomplete commands"
        assert f"[SavePermissions] complete cheats={str(cheats).lower()}" in log, f"{name}: incomplete run"
        blocked = multiplayer and not cheats and not args.baseline
        if blocked:
            assert log.count("Saving multiplayer games requires cheats") == 4, f"{name}: missing refusals"
        for extension in ("ssf", "slsf"):
            fresh = saves / f"validation.{extension}"
            overwritten = saves / f"overwrite.{extension}"
            assert fresh.exists() != blocked, f"{name}: wrong fresh save result: {extension}"
            assert (overwritten.read_bytes() == sentinel) == blocked, f"{name}: wrong overwrite result: {extension}"
            if not blocked:
                assert fresh.stat().st_size > len(sentinel), f"{name}: empty save: {extension}"
        return {"case": name, "blocked": blocked, "commands": 4, "pass": True}
    finally:
        shutil.rmtree(root)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("--bar", type=Path, required=True)
    parser.add_argument("--assets", type=Path, required=True)
    parser.add_argument("--results", type=Path, required=True)
    parser.add_argument("--map", default="Quicksilver Remake 1.24")
    parser.add_argument("--baseline", action="store_true", help="Expect the unfixed multiplayer save leak")
    args = parser.parse_args()
    for field in ("engine", "bar", "assets", "results"):
        setattr(args, field, getattr(args, field).resolve())
    args.results.mkdir(parents=True, exist_ok=False)
    results = []
    for name, multiplayer, cheats in (
        ("singleplayer", False, False),
        ("singleplayer-cheats", False, True),
        ("multiplayer", True, False),
        ("multiplayer-cheats", True, True),
    ):
        result = run_case(args, name, multiplayer, cheats)
        results.append(result)
        print(json.dumps(result), flush=True)
    (args.results / "results.json").write_text(json.dumps(results, indent=2) + "\n")


if __name__ == "__main__":
    main()
