#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
"""Compare already-prepared parent/native/fallback/Lua replay runs.

Preparation uses prepare.py and identical historical game files with parity.lua
installed as a gadget. Supply separate engine directories (including base/).
Run performance trials separately with attackmovementparity=0 in the setup.
"""
import argparse
import hashlib
import json
import re
import statistics
import subprocess
import time
from pathlib import Path


def read_result(root, expected_frames, parity):
    log = (root / 'infolog.txt').read_text(errors='replace')
    if '[AttackValidation] PASS replay' not in log:
        raise RuntimeError(f'{root}: missing completion')
    if '[AttackValidation] FAIL' in log or re.search(r'Fatal:|LUA_ERR|Error: \[Lua(?:Rules|Gaia)', log):
        raise RuntimeError(f'{root}: Lua/check failure; inspect infolog.txt')
    snapshots = [(int(f), u, p) for f, u, p in re.findall(r'\[AttackParity\] (\d+) (\S+) (\S+)', log)]
    if parity and [s[0] for s in snapshots] != list(range(1, expected_frames + 1)):
        raise RuntimeError(f'{root}: incomplete/duplicate frame coverage ({len(snapshots)})')
    ticks = [(int(f), float(t)) for f, t in re.findall(r'\[AttackTiming\] (\d+) ([\d.]+)', log)]
    if [f for f, _ in ticks] != list(range(2, expected_frames + 1)):
        raise RuntimeError(f'{root}: incomplete/duplicate timing coverage ({len(ticks)})')
    timing = [t for f, t in ticks if f > 300]
    elapsed = re.findall(r'\[AttackElapsed\] ([\d.]+)', log)
    if len(elapsed) != 1:
        raise RuntimeError(f'{root}: missing/duplicate elapsed time')
    return {
        'root': str(root), 'frames': expected_frames, 'snapshots': snapshots,
        'elapsed_ms': float(elapsed[-1]) if elapsed else None,
        'mean_frame_ms': statistics.mean(timing) if timing else None,
        'p95_frame_ms': sorted(timing)[int(len(timing) * .95)] if timing else None,
    }


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('manifest', type=Path, help='JSON: frames, parity, runs [{name,engine,root}]')
    p.add_argument('--read-only', action='store_true', help='Compare existing logs without launching engines')
    a = p.parse_args()
    spec = json.loads(a.manifest.read_text())
    records, baseline = [], None
    for run in spec['runs']:
        root, engine = Path(run['root']).resolve(), Path(run['engine']).resolve()
        if not a.read_only:
            print(f"Running {run['name']} in {root}", flush=True)
            started = time.monotonic()
            with (root / 'console.log').open('w') as log:
                proc = subprocess.run([str(engine), '--only-local', '--isolation', '--write-dir', str(root), str(root / run.get('input', 'input.sdfz'))],
                                      stdout=log, stderr=subprocess.STDOUT, timeout=3600)
            if proc.returncode:
                raise RuntimeError(f"{run['name']}: exit {proc.returncode}")
            print(f'Completed in {time.monotonic() - started:.1f}s', flush=True)
        result = read_result(root, spec['frames'], spec.get('parity', True))
        snapshots = result.pop('snapshots')
        result['name'] = run['name']
        with engine.open('rb') as binary:
            result['engine_sha256'] = hashlib.file_digest(binary, 'sha256').hexdigest()
        policy = root / 'games/attack-movement.sdd/luarules/gadgets/unit_attack_movement.lua'
        result['policy_sha256'] = hashlib.sha256(policy.read_bytes()).hexdigest()
        if spec.get('parity', True):
            if baseline is None:
                baseline = snapshots
            differences = [f for (f, *state), (_, *ref) in zip(snapshots, baseline) if state != ref]
            result['first_difference'] = differences[0] if differences else None
            result['differing_frames'] = len(differences)
        records.append(result)
        a.manifest.with_suffix('.results.json').write_text(json.dumps(records, indent=2) + '\n')
        print(json.dumps(result), flush=True)
    if any(r.get('first_difference') is not None for r in records):
        raise SystemExit('FAIL: per-frame state differs')


if __name__ == '__main__':
    main()
