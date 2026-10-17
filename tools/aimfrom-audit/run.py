import argparse
import collections
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parent


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def summarize(log, output):
    records = [line.split("AIMFROM_AUDIT\t", 1)[1].split("\t")
               for line in log.splitlines() if "AIMFROM_AUDIT\t" in line]
    assert sum(row[0] == "begin" for row in records) == 1, "missing/duplicate begin"
    assert sum(row[0] == "complete" for row in records) == 1, "audit incomplete"
    units = {row[1]: row[2] for row in records if row[0] == "unit"}
    expected = int(next(row[1] for row in records if row[0] == "begin"))
    assert len(units) == expected, "unit definition coverage incomplete"
    weapons = collections.defaultdict(list)
    for row in records:
        if row[0] == "weapon":
            assert len(row) == 10, row
            weapons[(row[1], int(row[2]))].append(row)
    results = []
    for (name, slot), samples in sorted(weapons.items()):
        categories = {row[7] for row in samples}
        category = "different" if "different" in categories else "unknown" if "unknown" in categories else "same"
        results.append({"unit": name, "slot": slot, "weapon": samples[0][3],
                        "type": samples[0][4], "script": samples[0][5],
                        "category": category, "samples": [
                            {"age": int(row[6]), "category": row[7], "query": row[8], "aimfrom": row[9]}
                            for row in samples]})
    for result in results:
        assert sorted(sample["age"] for sample in result["samples"]) == [1, 30], result
    assert all(status in ("candidate", "unaffected-type") for status in units.values()), "spawn/lifecycle coverage gap"
    assert {name for name, status in units.items() if status == "candidate"} == {result["unit"] for result in results}
    unit_results = {}
    for name, status in units.items():
        categories = {result["category"] for result in results if result["unit"] == name}
        unit_results[name] = ("unaffected-type" if status == "unaffected-type" else
                              "different" if "different" in categories else
                              "unknown" if "unknown" in categories else "same")
    summary = {"unit_definitions": expected, "unit_categories": dict(collections.Counter(units.values())),
               "unit_sample_categories": dict(collections.Counter(unit_results.values())),
               "unit_scripts": {row[1]: row[3] for row in records if row[0] == "unit" and len(row) > 3},
               "weapon_categories": dict(collections.Counter(row["category"] for row in results)),
               "weapon_types": dict(collections.Counter(row["type"] for row in results)),
               "script_types": dict(collections.Counter(row["script"] for row in results)),
               "weapons": results}
    (output / "samples.tsv").write_text("\n".join("\t".join(row) for row in records) + "\n")
    (output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps({key: value for key, value in summary.items() if key not in ("weapons", "unit_scripts")}, indent=2))
    return summary


def main():
    parser = argparse.ArgumentParser(description="Run the opt-in AimFrom audit in an isolated BAR overlay")
    parser.add_argument("--source", type=Path, required=True, help="BAR source checkout")
    parser.add_argument("--engine", type=Path, required=True, help="spring-headless executable")
    parser.add_argument("--maps", type=Path, required=True, help="development maps directory")
    parser.add_argument("--output", type=Path, required=True, help="new isolated write directory")
    arguments = parser.parse_args()
    source, engine, output = arguments.source.resolve(), arguments.engine.resolve(), arguments.output.resolve()
    assert not output.exists(), "choose a new output directory to preserve existing evidence"
    output.mkdir(parents=True)
    (output / "maps").symlink_to(arguments.maps.resolve())
    (output / "games").mkdir()
    game_link = output / "games/audit.sdd"
    before = shutil.disk_usage(output).free
    manifest = {"source": str(source), "source_head": subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=source, text=True).strip(),
        "source_status": subprocess.check_output(["git", "status", "--porcelain"], cwd=source, text=True),
        "engine": str(engine), "engine_sha256": digest(engine),
        "tool_sha256": digest(ROOT / "dbg_aimfrom_audit.lua"), "harness_sha256": digest(ROOT / "isolate.lua")}
    try:
        with tempfile.TemporaryDirectory(prefix="aimfrom-game-", dir="/tmp") as temporary:
            game = Path(temporary)
            for current, directories, filenames in os.walk(source):
                directories[:] = [name for name in directories if name not in
                                   (".git", ".lux", "spec", "recoil-lua-library")]
                relative = Path(current).relative_to(source)
                destination = game / relative
                destination.mkdir(parents=True, exist_ok=True)
                for filename in filenames:
                    if filename != ".git":
                        (destination / filename).symlink_to(source / relative / filename)
            for filename, target in (("dbg_aimfrom_audit.lua", "dbg_aimfrom_audit.lua"),
                                     ("dbg_aimfrom_isolate.lua", "isolate.lua")):
                (game / "luarules/gadgets" / filename).symlink_to(ROOT / target)
            game_link.symlink_to(game)
            (output / "startscript.txt").write_text("""[GAME]
{
 MapName=Quicksilver Remake 1.24;
 GameType=Beyond All Reason $VERSION;
 GameStartDelay=0;
 StartPosType=0;
 RecordDemo=0;
 MyPlayerName=Audit;
 IsHost=1;
 FixedRNGSeed=1;
 MaxUnits=2000;
 HostPort=49417;
 [MODOPTIONS] { deathmode=neverend; aimfromaudit=1; forceallunits=1; maxspeed=100; }
 [ALLYTEAM0] { }
 [TEAM0] { allyteam=0; teamleader=0; }
 [PLAYER0] { name=Audit; team=0; }
}
""")
            (output / "springsettings.cfg").write_text(
                "LuaSocketEnabled=0\nNoSound=1\nLuaUI=0\nMaxSpeed=100\nMinSpeed=100\nServerSleepTime=1\n")
            command = [str(engine), "--isolation", "--write-dir", str(output), "--config",
                       str(output / "springsettings.cfg"), str(output / "startscript.txt")]
            manifest["command"] = command
            with (output / "engine.log").open("w") as log:
                result = subprocess.run(command, cwd=output, stdout=log, stderr=subprocess.STDOUT, timeout=240)
            manifest["returncode"] = result.returncode
            assert result.returncode == 0, "engine failed; inspect engine.log"
            log = (output / "engine.log").read_text(errors="replace")
            assert not re.search(r"LUA_ERRRUN|RunCallInTraceback", log), "Lua error; inspect engine.log"
            summarize(log, output)
    finally:
        if game_link.is_symlink():
            game_link.unlink()
        manifest["output_filesystem_free_bytes_before"] = before
        manifest["output_filesystem_free_bytes_after"] = shutil.disk_usage(output).free
        (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main()
