#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
"""Run the BAR ghost targeting fixture in an isolated graphical Linux engine."""
import argparse
from pathlib import Path
import resource
import shutil
import socket
import subprocess
import tempfile
import time

START_SCRIPT = """[GAME] {
 MapName=Quicksilver Remake 1.24; GameType=Ghost Target Validation 1;
 GameStartDelay=0; StartPosType=0; RecordDemo=0; MyPlayerName=Test;
 IsHost=1; HostIP=127.0.0.1; HostPort=0; FixedRNGSeed=1;
 [MODOPTIONS] { deathmode=neverend; }
 [ALLYTEAM0] { NumAllies=0; } [ALLYTEAM1] { NumAllies=0; }
 [TEAM0] { AllyTeam=0; TeamLeader=0; }
 [TEAM1] { AllyTeam=1; TeamLeader=0; }
 [PLAYER0] { Name=Test; Team=0; Spectator=0; }
}"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--bar", type=Path, required=True)
    parser.add_argument("--assets", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--baseline", action="store_true")
    parser.add_argument("--colors", action="store_true", help="exercise Lua RGBA overrides and reset")
    parser.add_argument("--glsl", action="store_true", help="force the legacy GLSL model renderer")
    args = parser.parse_args()
    if args.colors and args.baseline:
        parser.error("--colors requires the candidate engine")
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    exe, bar, assets, out = (p.resolve() for p in (args.engine, args.bar, args.assets, args.output))
    out.mkdir(parents=True, exist_ok=True)
    fixture = Path(__file__).resolve().parent
    candidate = not args.baseline
    with tempfile.TemporaryDirectory(prefix="data-", dir=out) as temporary, socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        data = Path(temporary)
        fonts = exe.parent / "fonts"
        if not fonts.exists():
            fonts = exe.parent / "install/fonts"
        for name, source in (("maps", assets / "maps"), ("base", exe.parent / "base"), ("fonts", fonts)):
            (data / name).symlink_to(source)
        overlay = data / "games/test.sdd"
        (overlay / "luarules/gadgets").mkdir(parents=True)
        (data / "games/bar.sdd").symlink_to(bar)
        (overlay / "modinfo.lua").write_text(
            'return {name="Ghost Target Validation",version="1",modtype=1,depend={"Beyond All Reason $VERSION"}}'
        )
        shutil.copyfile(fixture / "scenario.lua", overlay / "luarules/gadgets/test_ghost_target.lua")
        shutil.copyfile(fixture / "ui.lua", overlay / "luaui.lua")
        shutil.copyfile(fixture / "ghost_colors.lua", overlay / "ghost_colors.lua")
        (out / "input.txt").write_text(START_SCRIPT)
        sock.bind(("127.0.0.1", 0))
        sock.settimeout(0.2)
        config = out / "settings.cfg"
        config.write_text(
            "LuaUI = 1\nSound = 0\nWorkerThreadCount = 4\nPathingThreadCount = 1\n"
            "GhostedBuildings = 1\nUnitIconsAsUI = 1\nFullscreen = 0\nXResolutionWindowed = 1280\nYResolutionWindowed = 720\nVSync = 0\n"
            f"GhostTestColors = {int(args.colors)}\nForceDisableGL4 = {int(args.glsl)}\n"
            f"GhostTestCandidate = {int(candidate)}\nAutohostIP = 127.0.0.1\nAutohostPort = {sock.getsockname()[1]}\n"
        )
        command = [str(exe), "--isolation", f"--isolation-dir={data}", f"--write-dir={data}", f"--config={config}", "--only-local", str(out / "input.txt")]
        process = None
        try:
            with (out / "engine.log").open("w") as log:
                process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
                started, sent, peer = time.monotonic(), set(), None
                while process.poll() is None:
                    try:
                        _, peer = sock.recvfrom(65535)
                    except socket.timeout:
                        pass
                    elapsed = time.monotonic() - started
                    for delay, message in ((5, "/forcestart"), (7, "/setmaxspeed 1"), (8, "/setminspeed 1")):
                        if peer and elapsed > delay and message not in sent:
                            sock.sendto(message.encode(), peer)
                            sent.add(message)
                    if elapsed > 120:
                        raise TimeoutError("ghost targeting fixture did not finish")
                assert process.returncode == 0, f"engine exited with {process.returncode}"
            content = (out / "engine.log").read_text(errors="replace")
            events = [line for line in content.splitlines() if "GHOST_TEST" in line]
            (out / "events.txt").write_text("\n".join(events) + "\n")
            print("\n".join(events))
            assert "GHOST_TEST DONE" in content, "missing completion marker; see engine.log"
            assert "GHOST_TEST FAIL" not in content, "fixture failed; see engine.log"
            assert "LUA_ERRRUN" not in content, "Lua error; see engine.log"
            assert "[Shader] Error:" not in content, "shader error; see engine.log"
            if args.colors:
                assert "GHOST_TEST PASS ghost color API validation" in content
                assert "GHOST_TEST PASS ghost colors reset" in content
            expected = 280 if candidate else 200
            for event in ("attack_y", "settarget_y"):
                assert f"GHOST_TEST {event} {expected}" in content, event
            for event in ("ground_attack_y", "ground_settarget_y", "nuke_y"):
                assert f"GHOST_TEST {event} 200" in content, event
            if candidate:
                for event in ("click_attack_y", "click_settarget_y"):
                    assert f"GHOST_TEST {event} 280" in content, event
        finally:
            if process is not None and process.poll() is None:
                process.kill()
                process.wait()
            if (data / "screenshots").exists():
                shots = out / "screenshots"
                shots.mkdir(exist_ok=True)
                for shot in (data / "screenshots").iterdir():
                    shot.replace(shots / shot.name)
            if (data / "infolog.txt").exists():
                shutil.copyfile(data / "infolog.txt", out / "infolog.txt")


if __name__ == "__main__":
    main()
