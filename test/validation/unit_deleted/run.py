#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
"""Compare pre-deletion finalization with PR #3454's post-deletion baseline.

Manual check: destroy a target with a native Attack queue. UnitDestroyed still
sees a valid unit. UnitPreDelete sees a valid, readable ID and the next native
Attack. UnitDeleted on the baseline sees an invalid ID instead.
Neutralization must notify only affected units after native queue pruning, including
virtual targets without a materialized order. A callback must be able to immediately
delete another listener without invalidating the remaining listener snapshot.
Returned tags must be erased without UnitCmdDone or immediate successor execution.
Finalization must reject new native target commands, inserted commands, orders to
the finalizing unit, weapon targets and recursive self-deletion. Nested deletion
and ID recycling must preserve the outer callback's unit. Neither callin may run
unsynced. --compare checks per-frame native motion, queues and completion timing.
AI assistance: OpenAI Codex authored the fixture and runner.
"""
import argparse
from pathlib import Path
import subprocess
import json
import shutil

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--engine', type=Path, required=True)
p.add_argument('--game', type=Path, required=True)
p.add_argument('--maps', type=Path, required=True)
p.add_argument('--map', default='Supreme Isthmus v2.1')
p.add_argument('--output', type=Path, required=True)
p.add_argument('--callin', choices=('UnitDeleted', 'UnitPreDelete'), default='UnitPreDelete')
p.add_argument('--compare', type=Path)
p.add_argument('--fire-at-killed', action='store_true')
p.add_argument('--ground', action='store_true')
p.add_argument('--with-ui', action='store_true')
a = p.parse_args()
root = a.output.resolve()
root.mkdir(parents=True, exist_ok=False)
overlay = root/'games/validation.sdd'
(overlay/'luarules/gadgets').mkdir(parents=True)
(root/'games/game.sdd').symlink_to(a.game.resolve())
(root/'maps').symlink_to(a.maps.resolve())
(overlay/'modinfo.lua').write_text('return {name="Unit Deletion Validation",version="1",modtype=1,depend={"Beyond All Reason $VERSION"}}\n')
if not a.with_ui:
 (overlay/'luaui.lua').write_text('Spring.Echo("LIFECYCLE_UI_DISABLED")\n')
if a.fire_at_killed:
 (overlay/'gamedata').mkdir()
 modrules = (a.game/'gamedata/modrules.lua').read_text()
 assert 'fireAtKilled = false' in modrules
 (overlay/'gamedata/modrules.lua').write_text(modrules.replace('fireAtKilled = false', 'fireAtKilled = true', 1))
handler = (a.game/'luarules/gadgets.lua').read_text()
assert '"UnitDeleted"' not in handler, 'Use an unmodified BAR game for this engine fixture'
handler = handler.replace('"UnitDestroyed",', '"UnitDestroyed",\n\t"UnitDeleted",\n\t"UnitAttackTargetRemoved",', 1)
handler = handler.replace('function gadgetHandler:UnitDestroyed(', 'function gadgetHandler:UnitDeleted(...)\n for _,g in ipairs(self.UnitDeletedList) do g:UnitDeleted(...) end\nend\n\nfunction gadgetHandler:UnitDestroyed(', 1)
handler = handler.replace('function gadgetHandler:UnitDestroyed(', 'function gadgetHandler:UnitAttackTargetRemoved(...)\n local tags = {}\n for _,g in ipairs(self.UnitAttackTargetRemovedList) do\n local removed = g:UnitAttackTargetRemoved(...)\n for _,tag in ipairs(removed or {}) do tags[#tags+1] = tag end\n end\n return tags\nend\n\nfunction gadgetHandler:UnitDestroyed(', 1)
(overlay/'luarules/gadgets.lua').write_text(handler.replace('UnitDeleted', a.callin))
observer = Path(__file__).with_name('observer.lua').read_text().replace('UnitDeleted', a.callin)
(overlay/'luarules/gadgets/dbg_unit_deleted.lua').write_text(f'local preDelete = {str(a.callin == "UnitPreDelete").lower()}\nlocal groundFixture = {str(a.ground).lower()}\n' + observer)
(root/'springsettings.cfg').write_text('HostPortDefault=51320\nNoSound=1\nWorkerThreadCount=2\nHardwareThreadCount=1\n')
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
 try:
  r = subprocess.run([str(a.engine.resolve()), '--isolation', '--write-dir', str(root), str(root/'startscript.txt')], cwd=root, stdout=log, stderr=subprocess.STDOUT, timeout=180)
 finally:
  shutil.rmtree(root/'games')
  (root/'maps').unlink()
text = (root/'console.log').read_text(errors='replace')
trace = [line.split('LIFECYCLE_TRACE|', 1)[1] for line in text.splitlines() if 'LIFECYCLE_TRACE|' in line]
(root/'trace.json').write_text(json.dumps(trace, indent=2) + '\n')
lifecycle_passed = r.returncode == 0 and 'UNIT_DELETED_COMPLETE' in text and not any(s in text for s in ('LUA_ERR', 'RunCallInTraceback', 'Fatal:'))
if not a.with_ui:
 lifecycle_passed = lifecycle_passed and 'LIFECYCLE_UI_DISABLED' in text and 'Loading widget from mod:' not in text
parity_passed = trace == json.loads(a.compare.read_text()) if a.compare else None
passed = lifecycle_passed and parity_passed is not False
(root/'result.json').write_text(json.dumps(dict(passed=passed, lifecycle_passed=lifecycle_passed, parity_passed=parity_passed, callin=a.callin, fire_at_killed=a.fire_at_killed, ground=a.ground, with_ui=a.with_ui, trace_entries=len(trace), exit=r.returncode), indent=2)+'\n')
assert passed, root
print(a.callin, 'lifecycle validation passed:', root)
