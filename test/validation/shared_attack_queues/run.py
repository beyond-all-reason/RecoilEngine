#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
"""Prepare one isolated BAR game and write its queue/event trace for comparison."""
import argparse
import errno
import difflib
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import re
import threading

p = argparse.ArgumentParser()
p.add_argument('engine', type=Path)
p.add_argument('root', type=Path)
p.add_argument('--bar', type=Path, required=True)
p.add_argument('--assets', type=Path, required=True)
p.add_argument('--base', type=Path, required=True)
p.add_argument('--scenario', choices=['check', 'benchmark', 'repeat', 'save', 'death'], default='check')
p.add_argument('--resume', type=Path, help='Load an existing save instead of the start script')
p.add_argument('--attackers', type=int, default=1000)
p.add_argument('--targets', type=int, default=200)
p.add_argument('--compare', type=Path, help='Require the same trace as this completed baseline run')
a = p.parse_args()
root = a.root.resolve()
root.mkdir(parents=True, exist_ok=False)
(root / 'maps').symlink_to(a.assets.resolve() / 'maps', target_is_directory=True)
engine_base = a.engine.resolve().parent / 'base'
if engine_base.is_dir():
    assert engine_base.resolve() == a.base.resolve(), 'engine already has different base assets'
else:
    (root / 'base').symlink_to(a.base.resolve(), target_is_directory=True)
game = root / 'games/shared-queues.sdd'
def copy_asset(src, dst):
    try:
        os.link(src, dst)
    except OSError as error:
        if error.errno != errno.EXDEV:
            raise
        shutil.copy2(src, dst)
shutil.copytree(a.bar, game, copy_function=copy_asset, ignore=shutil.ignore_patterns('.git', '.lux'))
for name, contents in [('modinfo.lua', (a.bar / 'modinfo.lua').read_text().replace('$VERSION', 'shared-queues')), ('luaui.lua', Path(__file__).with_name('network.lua').read_text() if a.scenario == 'check' else '-- Benchmark has no UI orders.\n')]:
    (game / name).unlink()
    (game / name).write_text(contents)
# The baseline engine has this call-in; expose it in the isolated BAR handler.
handler = game / 'luarules/gadgets.lua'
text = handler.read_text()
if 'function gadgetHandler:UnitCommandEnded(' not in text:
    text = text.replace('"UnitCmdDone",', '"UnitCmdDone",\n\t"UnitCommandEnded",', 1)
    marker = '\ngadgetHandler:Initialize()'
    assert marker in text
    text = text.replace(marker, """
function gadgetHandler:UnitCommandEnded(id, cmd, tag, reason)
    for _, gadget in ipairs(self.UnitCommandEndedList) do
        gadget:UnitCommandEnded(id, cmd, tag, reason)
    end
end

gadgetHandler:Initialize()""")
    handler.unlink()
    handler.write_text(text)
if a.scenario == 'repeat':
    text = handler.read_text()
    assert 'function gadgetHandler:AttackCommandMovement(' not in text
    text = text.replace('"UnitCmdDone",', '"UnitCmdDone",\n\t"AttackCommandMovement",', 1)
    text = text.replace('\ngadgetHandler:Initialize()', """
function gadgetHandler:AttackCommandMovement(id, ...)
    for _, gadget in ipairs(self.AttackCommandMovementList) do
        if gadget:AttackCommandMovement(id, ...) then return true end
    end
    return false
end

gadgetHandler:Initialize()""")
    handler.unlink()
    handler.write_text(text)
shutil.copyfile(Path(__file__).with_name(a.scenario + '.lua'), game / 'luarules/gadgets/test_shared_attack_queues.lua')
script = (Path(__file__).parent.parent / 'attack_movement/branches.txt').read_text()
script = script.replace('Beyond All Reason attack-movement', 'Beyond All Reason shared-queues')
script = script.replace('attackmovementbranches=1;', 'attackmovementbranches=0;')
script = script.replace('[MODOPTIONS] {', f'[MODOPTIONS] {{ sharedqueueattackers={a.attackers}; sharedqueuetargets={a.targets};')
(root / 'input.txt').write_text(script)
(root / 'springsettings.cfg').write_text('LuaUI = 0\nSound = 0\nWorkerThreadCount = 2\nPathingThreadCount = 1\nLogFlush = 1\n')
rss = {}
engine_input = root / 'input.txt'
if a.resume:
    assert a.scenario == 'save', '--resume requires --scenario save'
    engine_input = root / 'resume.ssf'
    shutil.copyfile(a.resume, engine_input)
with (root / 'console.log').open('w') as console:
    process = subprocess.Popen([str(a.engine.resolve()), '--only-local', '--isolation', '--write-dir', str(root), str(engine_input)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors='replace')
    timer = threading.Timer(180, process.kill)
    timer.daemon = True
    timer.start()
    try:
        for line in process.stdout:
            console.write(line)
            if '[SharedQueueBench] ' in line:
                label = line.split('[SharedQueueBench] ', 1)[1].strip()
                status = Path(f'/proc/{process.pid}/status').read_text()
                match = re.search(r'^VmRSS:\s+(\d+) kB', status, re.M)
                if match:
                    rss[label] = int(match[1])
        returncode = process.wait()
    finally:
        timer.cancel()
log = (root / 'infolog.txt').read_text(errors='replace')
trace = [line.split('[SharedQueue] ', 1)[1] for line in log.splitlines() if '[SharedQueue] ' in line]
(root / 'trace.txt').write_text('\n'.join(trace) + '\n')
assert returncode == 0, returncode
assert trace and trace[-1] == 'PASS', 'missing PASS; inspect ' + str(root / 'infolog.txt')
assert 'LuaRules::RunCallIn' not in log and 'LuaRules::RunCallInTraceback' not in log, 'Lua errors'
if a.scenario == 'save' and not a.resume:
    assert (root / 'Saves/shared-queues.ssf').is_file(), 'save was not created'
if a.scenario == 'benchmark':
    markers = {}
    for line in log.splitlines():
        match = re.search(r'\[t=(\d+):(\d+):([\d.]+)\].*\[SharedQueueBench\] (\w+)', line)
        if match:
            hours, minutes, seconds, label = match.groups()
            markers[label] = int(hours) * 3600 + int(minutes) * 60 + float(seconds)
    measurements = {'attackers': a.attackers, 'targets': a.targets, 'rss_kib': rss}
    for action in ('issue', 'read', 'stop'):
        measurements[action + '_ms'] = 1000 * (markers['post_' + action] - markers['pre_' + action])
    measurements['issue_rss_delta_kib'] = rss['post_issue'] - rss['pre_issue']
    (root / 'measurements.json').write_text(json.dumps(measurements, indent=2) + '\n')
    print(json.dumps(measurements))
if a.compare:
    expected = a.compare.read_text().splitlines()
    assert trace == expected, '\n'.join(difflib.unified_diff(expected, trace, fromfile=str(a.compare), tofile=str(root / 'trace.txt')))
with a.engine.open('rb') as binary:
    digest = hashlib.file_digest(binary, 'sha256').hexdigest()
(root / 'manifest.json').write_text(json.dumps({
    'engine': str(a.engine.resolve()), 'engine_sha256': digest,
    'bar_commit': subprocess.check_output(['git', '-C', str(a.bar), 'rev-parse', 'HEAD'], text=True).strip(),
    'trace_lines': len(trace), 'comparison': str(a.compare) if a.compare else None,
    'scenario': a.scenario, 'resume': str(a.resume) if a.resume else None,
}, indent=2) + '\n')
print(f'{len(trace)} trace lines: {root / "trace.txt"}')
