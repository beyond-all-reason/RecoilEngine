#!/usr/bin/env python3
"""Validate timer handles in isolated unsynced Lua states."""

import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("--bar", type=Path, required=True)
    parser.add_argument("--assets", type=Path, required=True)
    parser.add_argument("--results", type=Path, required=True)
    parser.add_argument("--baseline", action="store_true")
    args = parser.parse_args()
    for field in ("engine", "bar", "assets", "results"):
        setattr(args, field, getattr(args, field).resolve())
    args.results.mkdir(parents=True, exist_ok=False)
    root = Path(tempfile.mkdtemp(prefix="timer-handles-"))
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
            'return {name="Timer Handle Validation",version="1",modtype=1,'
            'depend={"Beyond All Reason $VERSION"}}\n'
        )
        shutil.copyfile(Path(__file__).with_name("check.lua"), overlay / "timer_validation.lua")
        for directory in ("LuaRules", "LuaGaia"):
            (overlay / directory).mkdir()
            (overlay / directory / "main.lua").write_text("return true\n")
            (overlay / directory / "draw.lua").write_text(
                f'VFS.Include("timer_validation.lua")("{directory}")\n'
            )
        (overlay / "luaui.lua").write_text('VFS.Include("timer_validation.lua")("LuaUI")\n')
        script = root / "startscript.txt"
        script.write_text(f"""[GAME] {{
 MapName=Quicksilver Remake 1.24; GameType=Timer Handle Validation 1;
 IsHost=1; HostIP=127.0.0.1; HostPort=0; MyPlayerName=Validation;
 GameStartDelay=0; StartPosType=0; RecordDemo=0; FixedRNGSeed=1;
 [MODOPTIONS] {{ timer_validation_baseline={int(args.baseline)}; }}
 [PLAYER0] {{ Name=Validation; Team=0; }}
 [TEAM0] {{ TeamLeader=0; AllyTeam=0; }} [ALLYTEAM0] {{ }}
}}\n""")
        config = root / "springsettings.cfg"
        config.write_text(
            "Windowed = 1\nXResolutionWindowed = 640\nYResolutionWindowed = 480\n"
            "Fullscreen = 0\nSound = 0\nShadows = 0\n"
            "FontFile = fonts/FreeSansBold.otf\nSmallFontFile = fonts/FreeSansBold.otf\n"
        )
        with (args.results / "console.log").open("w") as output:
            result = subprocess.run(
                [str(args.engine), "--only-local", "--isolation", "--write-dir", str(root),
                 "--config", str(config), str(script)],
                stdout=output, stderr=subprocess.STDOUT, timeout=120,
            )
        log = (root / "infolog.txt").read_text(errors="replace")
        (args.results / "infolog.txt").write_text(log)
        assert result.returncode == 0, f"engine exit {result.returncode}"
        assert "[TimerHandles] FAIL" not in log, "Lua validation failed"
        for scope in ("LuaUI", "LuaRules", "LuaGaia"):
            marker = f"[TimerHandles] {scope} PASS baseline={str(args.baseline).lower()}"
            assert marker in log, f"missing marker: {marker}"
            print(marker, flush=True)
    finally:
        shutil.rmtree(root)


if __name__ == "__main__":
    main()
