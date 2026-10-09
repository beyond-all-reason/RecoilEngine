#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html
"""Replay parity using the existing BAR parallel/resumable replay harness."""
import argparse
import concurrent.futures as cf
import fcntl
import hashlib
import importlib.util
import json
import re
import signal
import subprocess
import sys
import threading
import time
from pathlib import Path

WORKSPACE = Path(__file__).resolve().parents[4]
SEARCH = WORKSPACE / 'pr8935-performance-20261003/replay-search-20261004'
VARIANTS = {'stable': 'base', 'stable-repeat': 'base-repeat', 'candidate': 'pr-original'}


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def samples(directory, finish, read_run):
    try:
        info, rows, complete = read_run(directory)
        expected = set(range(0, finish + 1, 150)) | {finish}
        valid = complete and set(rows) == expected
        valid = valid and all(re.fullmatch(r'[0-9a-fA-F]+', h or '') for pair in rows.values() for h in pair)
        return bool(valid), rows
    except (OSError, ValueError, KeyError):
        return False, {}


def compare(a, b, finish, read_run):
    av, ar = samples(a, finish, read_run)
    bv, br = samples(b, finish, read_run)
    common = sorted(ar.keys() & br.keys())
    first = next((f for f in common if ar[f] != br[f]), None)
    return {'equal': av and bv and ar == br, 'baseline_complete': av,
            'candidate_complete': bv, 'first_difference_frame': first,
            'baseline_samples': len(ar), 'candidate_samples': len(br)}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--candidate', required=True, type=Path, help='headless binary built from stable + the patch')
    p.add_argument('--stable', type=Path, default=WORKSPACE / 'replay-parity/engines/recoil-2026.07.04/spring-headless')
    p.add_argument('--stable-tag', default='2026.07.04', help='pinned stable release; deliberately not updated on resume')
    p.add_argument('--output', required=True, type=Path)
    p.add_argument('--search-root', type=Path, default=SEARCH)
    p.add_argument('--game', type=Path, default=WORKSPACE / 'pr8935-performance-20261003/base-game')
    p.add_argument('--maps', type=Path, default=WORKSPACE / 'bar-dev/data/maps')
    p.add_argument('--workers', type=int, default=2)
    p.add_argument('--timeout', type=int, default=14400, help='seconds per engine/preparation job')
    p.add_argument('--replay-id', action='append', help='restrict selected.json; repeatable')
    p.add_argument('--finish', type=int, help='short smoke-test endpoint; default: complete replay minus tail')
    p.add_argument('--prepare-only', action='store_true')
    p.add_argument('--self-test', action='store_true', help='allow identical engines to test the harness only')
    args = p.parse_args()
    if args.workers < 1 or args.timeout < 1 or (args.finish is not None and args.finish < 1):
        p.error('workers, timeout and finish must be positive')
    args.output = args.output.resolve()
    production = Path.home() / '.local/state/Beyond All Reason'
    for path in (args.output, args.game.resolve(), args.maps.resolve()):
        if path == production or production in path.parents:
            p.error('production BAR installation must not be used')
    harness = args.search_root.resolve() / 'full-benchmark/harness'
    runner = load_module('shared_queue_workers', args.search_root / 'find-target-lists.py')
    reader = load_module('shared_queue_report', harness / 'report.py').read_run
    engines = {}
    for label, path in [('stable', args.stable), ('candidate', args.candidate)]:
        path = path.resolve(strict=True)
        version = subprocess.check_output([str(path), '--version'], text=True).strip()
        if not re.search(r'\b' + re.escape(args.stable_tag) + r'(?:\b|[-.])', version):
            p.error(f'{label} is not based on {args.stable_tag}: {version}')
        base = path.parent / 'base'
        if not base.is_dir():
            p.error(f'{label}: engine needs its matching base assets beside the binary: {base}')
        engines[label] = {'path': str(path), 'sha256': sha(path), 'version': version,
                          'base_assets': {str(f.relative_to(base)): sha(f) for f in sorted(base.rglob('*')) if f.is_file()}}

    if engines['stable']['sha256'] == engines['candidate']['sha256'] and not args.self_test:
        p.error('identical binaries: use --self-test only for a harness smoke test')
    game = args.game.resolve(strict=True)
    git = lambda *cmd: subprocess.check_output(['git', '-C', str(game), *cmd], text=True).strip()
    if git('status', '--porcelain'):
        p.error('game checkout must be clean and pinned for reproducible comparisons')
    selected = json.loads((harness.parent / 'selected.json').read_text())
    if args.replay_id:
        missing = set(args.replay_id) - {r['id'] for r in selected}
        if missing:
            p.error(f'unknown replay IDs: {sorted(missing)}')
        selected = [r for r in selected if r['id'] in args.replay_id]
    observer = (harness / 'observer.lua').read_text()
    anchor = "   row(out,'state',id,Spring.GetUnitStates(id))"
    if observer.count(anchor) != 1:
        raise RuntimeError('observer changed: queue instrumentation needs review')
    observer = observer.replace(anchor, anchor + "\n   row(out,'queue',id,Spring.GetUnitCommands(id,-1))\n   row(out,'queueCount',id,Spring.GetUnitCommandCount(id))")
    event_anchor = ' function gadget:UnitCommand(unitID,unitDefID,team,cmdID,params,opts,tag)'
    if observer.count(event_anchor) != 1:
        raise RuntimeError('observer command callback changed')
    observer = observer.replace(event_anchor, event_anchor + "\n  event('command',unitID,cmdID,table.concat(params,','),opts.coded or 0,tag)")
    observer = observer.replace('else\n local trace,commands,hashes,controllers,frametimes,rawEvents',
        " function gadget:UnitCmdDone(unitID,unitDefID,team,cmdID,params,opts,tag)\n"
        "  event('done',unitID,cmdID,table.concat(params,','),opts.coded or 0,tag)\n end\n"
        'else\n local trace,commands,hashes,controllers,frametimes,rawEvents')
    parser = load_module('shared_queue_demo', harness / 'replay.py')
    inputs = []
    for row in selected:
        if sha(row['path']) != row['sha256']:
            raise RuntimeError(f'replay checksum changed: {row["path"]}')
        end = ((row['last_frame'] - 150) // 150) * 150
        finish = min(args.finish, end) if args.finish else end
        coverage = {'multiunit_attack_batches': 0, 'max_units': 0, 'max_attacks': 0}
        for frame, _, packet in parser.Demo(row['path']).packets:
            if frame > finish or packet[0] != 15:
                continue
            commands, units = parser.commands(packet)
            attacks = sum(cid == 20 and len(params) == 1 for cid, _, params, _ in commands)
            if units and len(units) > 1 and attacks:
                coverage['multiunit_attack_batches'] += 1
                coverage['max_units'] = max(coverage['max_units'], len(units))
                coverage['max_attacks'] = max(coverage['max_attacks'], attacks)
        inputs.append({'native_packet_coverage': coverage, 'id': row['id'], 'path': row['path'], 'sha256': row['sha256'],
                       'finish': min(args.finish, end) if args.finish else end})
    # Freeze map bytes as well: replacing an archive must invalidate resume.
    print('Hashing development map archives for resume provenance …', flush=True)
    maps = {str(path.resolve()): sha(path) for path in sorted(args.maps.rglob('*'))
            if path.is_file() and path.suffix.lower() in ('.sd7', '.sdz', '.sdp')}
    if not maps:
        p.error('no map archives found')
    manifest = {'engines': engines, 'stable_tag': args.stable_tag, 'game': str(game),
                'game_commit': git('rev-parse', 'HEAD'), 'maps': maps, 'replays': inputs,
                'observer_sha256': hashlib.sha256(observer.encode()).hexdigest(),
                'self_test': args.self_test,
                'harness': {str(path): sha(path) for path in [Path(__file__), args.search_root / 'find-target-lists.py',
                            harness / 'prepare.py', harness / 'replay.py', harness / 'report.py']}}
    args.output.mkdir(parents=True, exist_ok=True)
    with (args.output / '.lock').open('w') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        # A killed supervisor can leave its engine processes behind.
        for cmdline in Path('/proc').glob('[0-9]*/cmdline'):
            try:
                argv = cmdline.read_bytes().split(b'\0')
            except OSError:
                continue
            if b'--write-dir' in argv:
                value = argv[argv.index(b'--write-dir') + 1].decode(errors='replace')
                if Path(value).is_relative_to(args.output):
                    raise RuntimeError(f'engine still running in output: PID {cmdline.parent.name}')
        provenance = args.output / 'provenance.json'
        if provenance.exists() and json.loads(provenance.read_text()) != manifest:
            raise RuntimeError('inputs changed; use a new output directory')
        runner.save(provenance, manifest)
        for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
            signal.signal(sig, lambda *_: runner.STOP.set())
        done = threading.Event()
        heartbeat = threading.Thread(target=runner.heartbeat, args=(done,), daemon=True)
        heartbeat.start()
        try:
            for index, row in enumerate(inputs):
                directory = args.output / row['id']
                if (directory / 'prepared.json').exists():
                    continue
                if directory.exists():
                    directory.rename(directory.with_name(directory.name + '.interrupted-' + str(time.time_ns())))
                with (args.output / (row['id'] + '.prepare.log')).open('w') as log:
                    code = runner.run([sys.executable, str(harness / 'prepare.py'), row['path'], '--root', str(directory),
                                       '--finish', str(row['finish']), '--base-game', str(game), '--pr-game', str(game),
                                       '--maps', str(args.maps.resolve()), '--port', str(44000 + index * 4), '--allow-empty'],
                                      'prepare-' + row['id'][:8], args.timeout, log=log)
                if code:
                    raise RuntimeError(f'preparation failed: {row["id"]}')
                hashes = set()
                for name in VARIANTS.values():
                    root = directory / name
                    (root / 'games/parity.sdd/luarules/gadgets/dbg_replay_parity.lua').write_text(observer)
                    hashes.add(sha(root / 'input.sdfz'))
                if len(hashes) != 1:
                    raise RuntimeError('gameplay input differs between variants')
                runner.save(directory / 'prepared.json', {'identical_replay_sha256': hashes.pop()})
            if args.prepare_only:
                return 0

            def job(row, label, folder):
                directory = args.output / row['id'] / folder
                engine = engines['candidate' if label == 'candidate' else 'stable']
                if runner.STOP.is_set():
                    return
                if samples(directory, row['finish'], reader)[0]:
                    print(f'RESUME {row["id"][:8]} {label}', flush=True)
                    return
                print(f'START {row["id"][:8]} {label} endpoint={row["finish"]}', flush=True)
                started = time.monotonic()
                result = {'complete': False, 'exit_code': None, 'engine_sha256': engine['sha256']}
                runner.save(directory / 'run.json', result)
                # Clear stale observations before retry, including startup failures.
                for name in ('hashes.tsv', 'states.bin', 'infolog.txt', 'console.log'):
                    (directory / name).unlink(missing_ok=True)
                try:
                    with (directory / 'console.log').open('w') as log:
                        result['exit_code'] = runner.run([engine['path'], '--isolation', '--write-dir', str(directory),
                                                        str(directory / 'input.sdfz')], row['id'][:8] + '-' + label,
                                                       args.timeout, cwd=directory, log=log)
                    console = (directory / 'console.log').read_text(errors='replace')
                    result['lua_errors'] = any(token in console for token in ('RunCallInTraceback', 'LUA_ERRRUN', 'LUA_ERRSYNTAX', 'stack traceback'))
                    result['complete'] = bool(re.search(r'\[ReplayParity\] COMPLETE ' + str(row['finish']) + r'\b', console))
                except Exception as error:
                    result['error'] = str(error)
                finally:
                    result['seconds'] = time.monotonic() - started
                    runner.save(directory / 'run.json', result)
                print(f'END {row["id"][:8]} {label} valid={samples(directory, row["finish"], reader)[0]}', flush=True)

            with cf.ThreadPoolExecutor(max_workers=args.workers) as pool:
                futures = [pool.submit(job, row, label, folder) for row in inputs for label, folder in VARIANTS.items()]
                for future in cf.as_completed(futures):
                    future.result()
            results = {}
            for row in inputs:
                root = args.output / row['id']
                results[row['id']] = {'stable_repeat': compare(root / 'base', root / 'base-repeat', row['finish'], reader),
                                      'candidate': compare(root / 'base', root / 'pr-original', row['finish'], reader)}
            passed = all(v['equal'] for row in results.values() for v in row.values())
            runner.save(args.output / 'report.json', {'passed': passed, 'self_test': args.self_test, 'replays': results})
            print(json.dumps({'passed': passed, 'self_test': args.self_test, 'report': str(args.output / 'report.json')}))
            return 0 if passed else 1
        finally:
            done.set()
            heartbeat.join()


if __name__ == '__main__':
    sys.exit(main())
