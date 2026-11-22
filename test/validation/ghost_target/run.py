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
 [ALLYTEAM2] { NumAllies=0; }
 [TEAM0] { AllyTeam=0; TeamLeader=0; }
 [TEAM1] { AllyTeam=1; TeamLeader=0; }
 [TEAM2] { AllyTeam=2; TeamLeader=0; }
 [PLAYER0] { Name=Test; Team=OBSERVER; Spectator=0; }
}"""


def run_engine(command, sock, log_path, headless, resume=False):
    speed = 20 if headless else 1
    messages = [(7, f"/setmaxspeed {speed}"), (8, f"/setminspeed {speed}")]
    if resume:
        messages.append((12, "/pause 0"))
    else:
        messages.insert(0, (5, "/forcestart"))
    with log_path.open("w") as log, subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT) as process:
        try:
            started, sent, peer = time.monotonic(), set(), None
            while process.poll() is None:
                try:
                    _, peer = sock.recvfrom(65535)
                except socket.timeout:
                    pass
                elapsed = time.monotonic() - started
                # PostLoad starts a saved game and pauses it at the restored frame.
                # Starting it earlier can queue server frames from frame zero.
                ready = not resume or sent or "paused the game" in log_path.read_text(errors="replace")
                for delay, message in messages:
                    if ready and peer and elapsed > delay and message not in sent:
                        sock.sendto(message.encode(), peer)
                        sent.add(message)
                if elapsed > 120:
                    raise TimeoutError("ghost targeting fixture did not finish")
            assert process.returncode == 0, f"engine exited with {process.returncode}"
        finally:
            if process.poll() is None:
                process.kill()
    return log_path.read_text(errors="replace")


def setup_height_policy(overlay, bar, fixture, policy):
    # The test game has no gadgets or modrules of its own. Both the model and
    # the standard handler come from the built engine archive.
    rules = overlay / "luarules"
    (overlay / "gamedata").mkdir()
    (overlay / "gamedata/defs.lua").write_text("""
return {
    unitdefs = { probe = { name = "Probe", objectname = "fir_tree_large.s3o",
        canattack = true, canmanualfire = true, explodeas = "laser", selfdestructas = "laser",
        weapons = {{name = "laser"}} } },
    weapondefs = { laser = { weapontype = "LaserCannon", range = 500, damage = {default = 1} } },
    featuredefs = {}, armordefs = {}, movedefs = {},
}
""")
    (overlay / "modinfo.lua").write_text('return {name="Ghost Target Validation",version="1",modtype=1}')
    (rules / "main.lua").write_text('VFS.Include("LuaGadgets/gadgets.lua", nil, VFS.BASE)')
    (rules / "draw.lua").write_text(
        'function RecvFromSynced(tag) if tag == "height_done" then Spring.SendCommands("quitforce") end end'
    )
    shutil.copyfile(fixture / "height_policy.lua", rules / "gadgets/test_height.lua")
    if policy == "override":
        shutil.copyfile(bar / "luarules/gadgets/cmd_keep_height_compat.lua", rules / "gadgets/cmd_keep_height_compat.lua")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--bar", type=Path, required=True)
    parser.add_argument("--assets", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--headless", action="store_true", help="run only the synced checks with spring-headless")
    parser.add_argument("--save-load", action="store_true", help="reload the headless scenario after hidden destruction")
    parser.add_argument("--observer", type=int, choices=(0, 2), default=0, help="local observing allyTeam")
    parser.add_argument("--baseline", action="store_true")
    parser.add_argument("--lua-render", action="store_true", help="exercise BAR DrawGhost and its shared shimmer shader")
    parser.add_argument("--glsl", action="store_true", help="force the legacy GLSL model renderer")
    parser.add_argument("--height-policy", choices=("default", "override"),
                        help="exercise keepheight with the stock LuaRules handler, with or without the base gadget")
    args = parser.parse_args()
    if args.height_policy and (not args.headless or args.save_load or args.baseline or args.observer != 0):
        parser.error("--height-policy requires --headless and runs a standalone command-queue scenario")
    if args.lua_render and args.baseline:
        parser.error("--lua-render requires the candidate engine")
    if (args.headless or args.observer == 2) and (args.baseline or args.lua_render):
        parser.error("use the default graphical observer for baseline and Lua renderer checks")
    if args.save_load and not args.headless:
        parser.error("--save-load requires --headless")
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
        script = START_SCRIPT.replace("OBSERVER", str(args.observer))
        if args.height_policy:
            setup_height_policy(overlay, bar, fixture, args.height_policy)
            script = script.replace("deathmode=neverend;", f"LuaGaia=0; heightpolicy={args.height_policy};")
        else:
            (data / "games/bar.sdd").symlink_to(bar)
            (overlay / "modinfo.lua").write_text(
                'return {name="Ghost Target Validation",version="1",modtype=1,depend={"Beyond All Reason $VERSION"}}'
            )
            shutil.copyfile(fixture / "scenario.lua", overlay / "luarules/gadgets/test_ghost_target.lua")
        if args.headless:
            (overlay / "luaui.lua").write_text("-- Synced fixture only.\n")
        else:
            shutil.copyfile(fixture / "ui.lua", overlay / "luaui.lua")
        (out / "input.txt").write_text(script)
        sock.bind(("127.0.0.1", 0))
        sock.settimeout(0.2)
        config = out / "settings.cfg"
        config.write_text(
            f"GhostTestSave = {int(args.save_load)}\nGhostTestHeadless = {int(args.headless)}\nLuaUI = {int(not args.headless)}\nSound = 0\nWorkerThreadCount = 4\nPathingThreadCount = 1\n"
            "GhostedBuildings = 1\nUnitIconsAsUI = 1\nFullscreen = 0\nXResolutionWindowed = 1280\nYResolutionWindowed = 720\nVSync = 0\n"
            f"GhostTestLuaRender = {int(args.lua_render)}\nForceDisableGL4 = {int(args.glsl)}\n"
            f"GhostTestCandidate = {int(candidate)}\nAutohostIP = 127.0.0.1\nAutohostPort = {sock.getsockname()[1]}\n"
        )
        command = [str(exe), "--isolation", f"--isolation-dir={data}", f"--write-dir={data}", f"--config={config}", "--only-local", str(out / "input.txt")]
        try:
            content = run_engine(command, sock, out / "engine.log", args.headless)
            if args.height_policy:
                assert "HEIGHT_TEST PASS" in content and "HEIGHT_TEST FAIL" not in content, "see engine.log"
                assert "LUA_ERRRUN" not in content, "Lua error; see engine.log"
                print(f"PASS {args.height_policy} command height policy")
                return
            events = [line for line in content.splitlines() if "GHOST_TEST" in line]
            (out / "events.txt").write_text("\n".join(events) + "\n")
            print("\n".join(events))
            assert "GHOST_TEST DONE" in content, "missing completion marker; see engine.log"
            assert "GHOST_TEST FAIL" not in content, "fixture failed; see engine.log"
            assert "LUA_ERRRUN" not in content, "Lua error; see engine.log"
            assert "[Shader] Error:" not in content, "shader error; see engine.log"
            if args.lua_render:
                for marker in ("live", "radar", "dead", "fallback"):
                    assert f"GHOST_TEST PASS Lua ghost renderer {marker}" in content, marker
            expected = 200
            for event in ("attack_y", "settarget_y"):
                assert f"GHOST_TEST {event} {expected}" in content, event
            for event in ("ground_attack_y", "ground_settarget_y", "nuke_y"):
                assert f"GHOST_TEST {event} 200" in content, event
            if candidate:
                if not args.headless:
                    assert "GHOST_TEST PASS unsynced gadget DrawGhost" in content
                assert "GHOST_TEST empty_sites true" in content
                assert "GHOST_TEST synced_observers true" in content
                assert "GHOST_TEST separate_sightings true" in content
                assert "GHOST_TEST orders_recontact true" in content
                if args.observer == 2 and not args.headless:
                    for phase in ("live ghost", "dead ghost", "only remaining ghost", "DrawGhost live", "DrawGhost dead", "minimap before transfer", "minimap after transfer", "minimap after destruction", "minimap after reveal"):
                        assert f"GHOST_TEST PASS second observer {phase}" in content, phase
                if args.observer == 0 and not args.headless:
                    for phase in ("continuous", "reacquired"):
                        assert f"GHOST_TEST PASS remembered radar minimap {phase}" in content, phase
                    for case in ("inserted", "set-target"):
                        assert f"GHOST_TEST PASS LuaUI {case} height clamped" in content
                    for event in ("click_attack_y", "click_settarget_y"):
                        assert f"GHOST_TEST {event} 280" in content, event
            if args.save_load:
                save = data / "Saves/ghost_observers.ssf"
                assert save.is_file(), "missing saved game"
                resume_script = out / "resume.txt"
                resume_script.write_text("[GAME] { SaveFile=Saves/ghost_observers.ssf; IsHost=1; OnlyLocal=1; MyPlayerName=Test; }")
                resumed = run_engine([*command[:-1], str(resume_script)], sock, out / "resume.log", headless=True, resume=True)
                for marker in ("synced_observers true", "separate_sightings true", "orders_recontact true", "DONE"):
                    assert f"GHOST_TEST {marker}" in resumed, marker
                assert "LUA_ERRRUN" not in resumed
                assert "GHOST_TEST attack_y" not in resumed, "scenario restarted from frame zero"
        finally:
            if (data / "screenshots").exists():
                shots = out / "screenshots"
                shots.mkdir(exist_ok=True)
                for shot in (data / "screenshots").iterdir():
                    shot.replace(shots / shot.name)
            if (data / "infolog.txt").exists():
                shutil.copyfile(data / "infolog.txt", out / "infolog.txt")


if __name__ == "__main__":
    main()
