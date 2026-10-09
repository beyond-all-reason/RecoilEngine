#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
"""Validate UnitDeleted ordering in an isolated BAR game, using development assets.

Manual check: destroy a target with a native Attack queue. UnitDestroyed still
sees a valid unit. UnitDeleted sees an invalid ID and the next native Attack,
with the original unit definition/team; the unsynced callin must never run.
AI assistance: OpenAI Codex authored the fixture and runner.
"""
import argparse
from pathlib import Path
import subprocess
import json

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--engine', type=Path, required=True)
p.add_argument('--game', type=Path, required=True)
p.add_argument('--maps', type=Path, required=True)
p.add_argument('--map', default='Supreme Isthmus v2.1')
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = a.output.resolve()
root.mkdir(parents=True, exist_ok=False)
overlay = root/'games/validation.sdd'
(overlay/'luarules/gadgets').mkdir(parents=True)
(root/'games/game.sdd').symlink_to(a.game.resolve())
(root/'maps').symlink_to(a.maps.resolve())
(overlay/'modinfo.lua').write_text('return {name="Unit Deletion Validation",version="1",modtype=1,depend={"Beyond All Reason $VERSION"}}\n')
handler = (a.game/'luarules/gadgets.lua').read_text()
assert '"UnitDeleted"' not in handler, 'Use an unmodified BAR game for this engine fixture'
handler = handler.replace('"UnitDestroyed",', '"UnitDestroyed",\n\t"UnitDeleted",', 1)
handler = handler.replace('function gadgetHandler:UnitDestroyed(', 'function gadgetHandler:UnitDeleted(...)\n for _,g in ipairs(self.UnitDeletedList) do g:UnitDeleted(...) end\nend\n\nfunction gadgetHandler:UnitDestroyed(', 1)
(overlay/'luarules/gadgets.lua').write_text(handler)
(overlay/'luarules/gadgets/dbg_unit_deleted.lua').write_bytes(Path(__file__).with_name('observer.lua').read_bytes())
(root/'springsettings.cfg').write_text('HostPortDefault=51320\nNoSound=1\nLuaUI=0\nWorkerThreadCount=1\nHardwareThreadCount=1\n')
(root/'startscript.txt').write_text(f'''[GAME] {{
 MapName={a.map}; GameType=Unit Deletion Validation 1;
 IsHost=1; HostIP=127.0.0.1; MyPlayerName=Validation; GameStartDelay=0; StartPosType=0;
 FixedRNGSeed=1; RecordDemo=0; MinSpeed=20; MaxSpeed=20;
 [MODOPTIONS] {{ deathmode=neverend; }}
 [PLAYER0] {{ Name=Validation; Team=0; }}
 [TEAM0] {{ TeamLeader=0; AllyTeam=0; }} [ALLYTEAM0] {{ }}
 [TEAM1] {{ TeamLeader=0; AllyTeam=1; }} [ALLYTEAM1] {{ }}
}}\n''')
with (root/'console.log').open('w') as log:
 r = subprocess.run([str(a.engine.resolve()), '--isolation', '--write-dir', str(root), str(root/'startscript.txt')], cwd=root, stdout=log, stderr=subprocess.STDOUT, timeout=180)
text = (root/'console.log').read_text(errors='replace')
passed = r.returncode == 0 and 'UNIT_DELETED_COMPLETE' in text and not any(s in text for s in ('LUA_ERR', 'RunCallInTraceback', 'Fatal:'))
(root/'result.json').write_text(json.dumps(dict(passed=passed, exit=r.returncode), indent=2)+'\n')
assert passed, root
print('UnitDeleted lifecycle validation passed:', root)
