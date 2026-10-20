#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
"""Self-contained GPU regression using bundled assets and a small native map."""

import argparse
from contextlib import ExitStack
import json
import os
import re
import signal
import socket
import time
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent


def make_map(target):
    """Native flat 10x8 SMF, one solid DXT1 tile. Test patterns are drawn in Lua."""
    target.mkdir(parents=True)
    maps = target / "maps"
    maps.mkdir()
    block = struct.pack("<HHI", 0x4BE8, 0x4BE8, 0)
    tile = b"".join(block * max(1, size // 4) ** 2 for size in (32, 16, 8, 4))
    (maps / "flat.smt").write_bytes(struct.pack("<16s4i", b"spring tilefile\0", 1, 1, 32, 1) + tile)
    data = bytearray(80)
    header = [b"spring map file\0", 1, 876543, 640, 512, 8, 8, 32, 0.0, 256.0, 0, 0, 0, 0, 0, 0, 0]
    header[10] = len(data)
    data.extend(struct.pack("<H", 24576) * (641 * 513))
    for index in (11, 14):
        header[index] = len(data)
        data.extend(bytes(320 * 256))
    header[12] = len(data)
    data.extend(struct.pack("<iii", 1, 1, 1) + b"flat.smt\0" + bytes(160 * 128 * 4))
    header[13] = len(data)
    data.extend(block * (699048 // 8))
    header[15] = len(data)
    data.extend(struct.pack("<ii", 0, 0))
    struct.pack_into("<16s7i2f7i", data, 0, *header)
    (maps / "flat.smf").write_bytes(data)
    (target / "mapinfo.lua").write_text(
        "return {name='Region View Fixture',version='1',mapfile='maps/flat.smf',"
        "lighting={sundir={1,1,1}},teams={[0]={startpos={x=1500,z=1250}}}}\n"
    )


def run_clients(command, env, data, output, clients):
    commands = [command]
    logs = [output / "console.log"]
    if clients == 2:
        peer = data / "peer"
        peer.mkdir()
        for name in ("games", "maps", "base", "fonts", "AI"):
            if (data / name).exists():
                (peer / name).symlink_to(data / name)
        script = output / "peer-start.txt"
        host_script = (output / "start.txt").read_text()
        port = re.search(r"HostPort=(\d+);", host_script).group(1)
        script.write_text("[GAME]{IsHost=0;HostIP=127.0.0.1;HostPort=" + port + ";MyPlayerName=RegionPeer;}")
        peer_command = list(command)
        peer_command[peer_command.index("--isolation-dir") + 1] = str(peer)
        peer_command[peer_command.index("--write-dir") + 1] = str(output / "peer-write")
        peer_command[-1] = str(script)
        commands.append(peer_command)
        logs.append(output / "peer-console.log")

    processes = []
    deadline = time.monotonic() + 90
    with ExitStack() as stack:
        try:
            for cmd, log in zip(commands, logs):
                handle = stack.enter_context(log.open("w"))
                processes.append(
                    subprocess.Popen(
                        cmd,
                        env=env,
                        cwd=ROOT,
                        stdout=handle,
                        stderr=subprocess.STDOUT,
                        start_new_session=True,
                    )
                )
            codes = [process.wait(timeout=max(1, deadline - time.monotonic())) for process in processes]
        except subprocess.TimeoutExpired:
            codes = [-9] * len(commands)
        finally:
            for process in processes:
                if process.poll() is None:
                    os.killpg(process.pid, signal.SIGTERM)
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        os.killpg(process.pid, signal.SIGKILL)
                        process.wait()
    return codes, logs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine-dir", type=Path, default=ROOT / "build-amd64-linux/install")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--case", choices=["camera", "draw", "flame", "layout", "nano", "sensor", "crater"], default="camera"
    )
    parser.add_argument(
        "--render-fps", type=float, default=30, help="Cap offscreen drawing; zero disables the limit"
    )
    parser.add_argument(
        "--save-load", action="store_true", help="Save and reload the region configuration in a fresh process"
    )
    parser.add_argument("--clients", type=int, choices=[1, 2], default=1)
    parser.add_argument("--sensors", "--regions", choices=["isolated", "whole-map"], default="isolated")
    parser.add_argument("--direct", action="store_true", help="Uncached minimap path")
    parser.add_argument("--drawer", type=int, choices=[0, 1], default=1)
    parser.add_argument("--nano", choices=["legacy", "geometry", "instanced"], default="legacy")
    options = parser.parse_args()
    if options.save_load and (options.case not in ("sensor", "crater") or options.clients != 1):
        parser.error("Save/load regression requires one sensor/crater client")
    if options.clients == 2 and options.case not in ("sensor", "crater"):
        parser.error("Two-client regression requires --case sensor or crater")
    output = options.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    engine = options.engine_dir.resolve()
    with tempfile.TemporaryDirectory(prefix="region-view-") as folder:
        data = Path(folder)
        game = data / "games/region_views.sdd"
        shutil.copytree(HERE / "game", game)
        (game / "LuaRules/Gadgets").mkdir()
        shutil.copyfile(
            ROOT / "cont/base/springcontent/LuaGadgets/Gadgets/unit_script.lua",
            game / "LuaRules/Gadgets/unit_script.lua",
        )
        shutil.copyfile(
            HERE / (("camera" if options.case == "layout" else options.case) + "_setup.lua"),
            game / "LuaRules/Gadgets/region_views.lua",
        )
        (game / "LuaUI").mkdir()
        ui = (HERE / (options.case + "_checks.lua")).read_text()
        ui += f"\nSpring.SendCommands('unitdrawer {options.drawer}', 'featuredrawer {options.drawer}')\n"
        (game / "LuaUI/main.lua").write_text(ui)
        for source, dest in [
            ("resources.lua", "gamedata/resources.lua"),
            ("explosions.lua", "gamedata/explosions.lua"),
            ("projectile.lua", "weapons/draw_bounds.lua"),
        ]:
            target = game / dest
            target.parent.mkdir(exist_ok=True)
            shutil.copyfile(HERE / source, target)
        if options.case == "sensor":
            shutil.copyfile(HERE / "sensor_movedefs.lua", game / "gamedata/movedefs.lua")
            shutil.copyfile(HERE / "sensor_unit.lua", game / "units/region_sensor_unit.lua")
            modrules = game / "gamedata/modrules.lua"
            modrules.write_text(
                modrules.read_text().replace("return {", "return {sensors={separateJammers=false},", 1)
            )
        make_map(data / "maps/region_view_map.sdd")
        # Use this checkout's content, and only the selected build's AI/fonts/base.
        for name in ["base", "fonts", "AI"]:
            if (engine / name).exists():
                (data / name).symlink_to(engine / name)
        for name in ["springcontent", "maphelper", "bitmaps", "cursors"]:
            (data / "games" / (name + ".sdd")).symlink_to(ROOT / "cont/base" / name)
        script = output / "start.txt"
        script.write_text("""[GAME]{IsHost=1;MyPlayerName=RegionViews;MyPlayerNum=0;
GameType=Region View Regression 1;MapName=Region View Fixture 1;OnlyLocal=1;
HostIP=localhost;HostPort=0;StartPosType=0;GameStartDelay=0;RecordDemo=0;
[MODOPTIONS]{LuaRules=1;LuaGaia=1;}
[PLAYER0]{Name=RegionViews;Team=0;Spectator=0;}
[TEAM0]{TeamLeader=0;AllyTeam=0;RGBColor=1 1 1;Side=Arm;}
[ALLYTEAM0]{NumAllies=0;}}
""")
        text = script.read_text().replace(
            "[MODOPTIONS]{",
            "[MODOPTIONS]{sensor_test="
            + options.sensors
            + ";fixture_clients="
            + str(options.clients)
            + ";fixture_save_load="
            + str(int(options.save_load))
            + ";",
        )
        if options.clients == 2:
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
                probe.bind(("127.0.0.1", 0))
                port = probe.getsockname()[1]
            text = text.replace("OnlyLocal=1;", "OnlyLocal=0;").replace(
                "HostIP=localhost;HostPort=0;", "HostIP=127.0.0.1;HostPort=" + str(port) + ";"
            )
            text = text[:-2] + "[PLAYER1]{Name=RegionPeer;Team=0;Spectator=0;}\n}\n"
        script.write_text(text)
        settings = output / "spring.conf"
        settings.write_text(
            "Fullscreen=0\nXResolutionWindowed=1280\nYResolutionWindowed=720\n"
            "VSync=0\nSound=0\nWorkerThreadCount=2\nShadows=1\nShadowMapSize=1024\n"
            "MiniMapCanFlip=0\nMiniMapRefreshRate=60\nUnitIconsAsUI=0\nUnitIconDist=100000\n"
            "FeatureDrawDistance=10000\nFeatureFadeDistance=10000\nSoftParticles=0\n"
            f"MiniMapRenderToTexture={int(not options.direct)}\n"
            f"NanoParticlesGL4={int(options.nano != 'legacy')}\n"
            f"NanoParticlesNoGeometryShader={int(options.nano == 'instanced')}\n"
            "NanoParticlesUpdateLuaUI=1\nNanoParticlesUpdateLuaUISampleRate=1\nNanoParticlesRate=1\nTeamNanoSpray=0\n"
        )
        env = os.environ.copy()
        env.pop("DISPLAY", None)
        env.pop("WAYLAND_DISPLAY", None)
        env["SDL_VIDEODRIVER"] = "offscreen"
        if options.render_fps > 0:
            limiter = data / "limit-swap.so"
            subprocess.run(
                ["cc", "-O2", "-shared", "-fPIC", str(HERE / "limit_swap.c"), "-ldl", "-o", str(limiter)],
                check=True,
            )
            env["LD_PRELOAD"] = str(limiter) + (":" + env["LD_PRELOAD"] if env.get("LD_PRELOAD") else "")
            env["SPRING_TEST_FPS"] = str(options.render_fps)
        fontconfig = output / "fonts.conf"
        fontconfig.write_text(
            f"<fontconfig><dir>{engine / 'fonts'}</dir><cachedir>{output / 'fontcache'}</cachedir></fontconfig>"
        )
        env["FONTCONFIG_FILE"] = str(fontconfig)
        command = [
            str(engine / "spring"),
            "--isolation",
            "--isolation-dir",
            str(data),
            "--write-dir",
            str(output / "write"),
            "--config",
            str(settings),
            "--nocolor",
            str(script),
        ]
        codes, logs = run_clients(command, env, data, output, options.clients)
        loaded_result = None
        if options.save_load and codes == [0]:
            saved = output / ("write/Saves/" + options.case + "-regions.ssf")
            if not saved.exists():
                raise SystemExit("Save fixture did not produce " + str(saved))
            loaded = output / "loaded"
            loaded.mkdir()
            config = loaded / "spring.conf"
            config.write_text(settings.read_text() + "\nRegionFixtureReloaded=1\n")
            load_command = list(command)
            load_command[load_command.index("--config") + 1] = str(config)
            load_command[load_command.index("--write-dir") + 1] = str(loaded / "write")
            load_command[-1] = str(saved)
            load_codes, load_logs = run_clients(load_command, env, data, loaded, 1)
            load_text = load_logs[0].read_text(errors="replace")
            loaded_result = (
                load_codes == [0]
                and (options.case.upper() + "_SAVELOAD_PASS") in load_text
                and (options.case.upper() + "_SAVELOAD_FAIL") not in load_text
                and "LUA_ERRRUN" not in load_text
            )
    texts = [path.read_text(errors="replace") for path in logs]
    prefix = {
        "camera": "VIEW_RECTANGLES",
        "draw": "DRAW_BOUNDS",
        "flame": "FLAME_BOUNDS",
        "layout": "LAYOUT_STACK",
        "nano": "NANO_BOUNDS",
        "sensor": "SENSOR_REGIONS",
        "crater": "CRATER_REGIONS",
    }[options.case]
    passed = all(
        code == 0
        and prefix + "_PASS" in log
        and prefix + "_FAIL" not in log
        and "LUA_ERRRUN" not in log
        and "LayoutButtons() bad" not in log
        and not re.search(r"sync error|desync", log, re.I)
        for code, log in zip(codes, texts)
    )
    witnesses = [re.findall(r"\[" + options.case.upper() + r"_WITNESS\] ([^\n]+)", text) for text in texts]
    views = [re.findall(r"\[" + options.case.upper() + r"_LOCAL_VIEW\] ([^\n]+)", text) for text in texts]
    if options.clients == 2:
        passed = passed and len(witnesses[0]) >= 5 and witnesses[0] == witnesses[1] and views[0] != views[1]
    if options.save_load:
        passed = passed and loaded_result is True
    (output / "result.json").write_text(
        json.dumps(
            {
                "passed": passed,
                "exit_code": codes[0],
                "exit_codes": codes,
                "save_load_passed": loaded_result,
                "witnesses_match": witnesses[0] == witnesses[-1],
                "different_views": views[0] != views[-1],
                "options": vars(options),
            },
            indent=2,
            default=str,
        )
    )
    if not passed:
        raise SystemExit("FAIL: see " + str(output / "console.log"))
    print("PASS:", options.case, "drawer", options.drawer, "direct", options.direct, "nano", options.nano)


if __name__ == "__main__":
    main()
