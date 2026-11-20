#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
"""Reproduce #3330 validation in the local BAR development workspace."""
import argparse
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


def run(argv, **kwargs):
    print('+', ' '.join(map(str, argv)), flush=True)
    subprocess.run(list(map(str, argv)), check=True, **kwargs)


def digest(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def inventory(root):
    # Record bytes actually used, not merely the declared historical revision.
    return {str(p.relative_to(root)): digest(p) for p in sorted(root.rglob('*')) if p.is_file()}


def revision(repo):
    def git(*args):
        return subprocess.check_output(['git', '-C', str(repo), *args], text=True).strip()
    if git('status', '--porcelain'):
        raise RuntimeError(f'Source checkout must be clean: {repo}')
    return dict(commit=git('rev-parse', 'HEAD'), tree=git('rev-parse', 'HEAD^{tree}'),
                submodules=git('submodule', 'status', '--recursive'))


def detach(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.unlink(missing_ok=True)
    path.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workspace', type=Path, default=Path('/home/aron/Projects/bar'))
    parser.add_argument('--output', type=Path)
    parser.add_argument('--preflight', action='store_true')
    parser.add_argument('--only', choices=['all', 'filters'], default='all')
    args = parser.parse_args()
    work = args.workspace.resolve()
    repo = Path(__file__).resolve().parents[3]
    suite = Path(__file__).resolve().parent
    bar = work / 'BAR-pr9154-api'
    historical = work / 'pr3330-api-20261027/historical-game'
    parent = work / 'pr3330-master-20261027/parent-engine'
    expected_parent = 'aa692a3a03d404ceed4c1f727554dcfc62aa5acd97c3d7ad23193177a2aee335'
    sources = dict(engine=revision(repo), bar=revision(bar))
    if digest(parent / 'spring-headless') != expected_parent:
        raise RuntimeError('Frozen parent binary differs from the recorded 30a80f69e3 baseline')
    replays = []
    for prefix, frames in [('d948756', 36000), ('6571756', 18000), ('995c756', 18000)]:
        matches = sorted((work / 'demos/replays').glob(prefix + '*.sdfz'))
        if len(matches) != 1:
            raise RuntimeError(f'Expected exactly one replay for {prefix}: {matches}')
        replays.append((prefix, frames, matches[0]))
    for path in [historical / 'modinfo.lua', parent / 'base', work / 'bar-dev/data/maps', repo / 'build-amd64-linux/CMakeCache.txt']:
        if not path.exists():
            raise RuntimeError(f'Missing prerequisite: {path}')
    if args.preflight:
        print(json.dumps(sources, indent=2))
        print('Preflight passed; no builds or engine runs started.')
        return
    output = (args.output or work / ('pr3330-validation-' + datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S'))).resolve()
    if not output.is_relative_to(work):
        raise RuntimeError('Output must be a new directory inside the development workspace')
    output.mkdir(exist_ok=False)
    summary = dict(status='running', sources=sources, runs=[],
                   parent_commit='30a80f69e30fdb5415ccb91ed44b6e9b150c443a',
                   free_bytes_before=shutil.disk_usage(output).free)
    def save():
        (output / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    save()
    try:
        with (output / 'build.log').open('w') as log:
            run(['./docker-build-v2/build.sh', '--compile', '-j', '16', 'linux', '-t', 'engine-headless', 'test_TargetCheckResult'],
                cwd=repo, env={**os.environ, 'CONTAINER_RUNTIME': 'podman'}, stdout=log, stderr=subprocess.STDOUT)
        run([repo / 'build-amd64-linux/test/test_TargetCheckResult'])
        run([sys.executable, '-m', 'unittest', 'discover', '-s', suite, '-p', 'test_*.py'])
        if dict(engine=revision(repo), bar=revision(bar)) != sources:
            raise RuntimeError('Sources changed while building')
        candidate = output / 'candidate-engine'
        candidate.mkdir()
        for name in ['spring-headless', 'base']:
            run(['cp', '-a', '--reflink=always', repo / 'build-amd64-linux' / name, candidate / name])
        frozen_parent = output / 'parent-engine'
        frozen_parent.mkdir()
        for name in ['spring-headless', 'base']:
            run(['cp', '-a', '--reflink=always', parent / name, frozen_parent / name])
        run(['cp', '-a', '--reflink=always', suite, output / 'suite'])
        suite = output / 'suite'
        summary['binaries'] = dict(candidate=inventory(candidate), parent=inventory(frozen_parent))
        spec = importlib.util.spec_from_file_location('attack_compare', suite / 'compare.py')
        compare = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(compare)
        handler_pattern = r'function gadgetHandler:AttackCommandMovement\(.*?\nend'
        handler = re.search(handler_pattern, (bar / 'luarules/gadgets.lua').read_text(), re.S).group()
        baseline = {}
        def execute(group, mode, game_source, replay, frames, fixture=None):
            root = output / f'{group}-{mode}'
            binary = (frozen_parent if mode == 'parent' else candidate) / 'spring-headless'
            game = root / 'games/attack-movement.sdd'
            record = dict(group=group, mode=mode, frames=frames, source_game=str(game_source))
            summary['runs'].append(record)
            try:
                run([sys.executable, suite / 'prepare.py', replay, root, '--bar', game_source,
                     '--assets', work / 'bar-dev/data', '--engine', binary,
                     '--mode', 'native' if mode == 'parent' else mode, '--frames', frames])
                detach(game / 'luarules/gadgets/unit_attack_movement.lua', (bar / 'luarules/gadgets/unit_attack_movement.lua').read_bytes())
                handler_path = game / 'luarules/gadgets.lua'
                text, count = re.subn(handler_pattern, lambda _: handler, handler_path.read_text(), flags=re.S)
                if count != 1:
                    raise RuntimeError('Historical handler must contain exactly one movement callback')
                detach(handler_path, text.encode())
                # The historical game already carries older observer gadgets.
                # Remove them from this overlay before installing exactly one observer.
                for stale in ['test_attack_parity.lua', 'test_attack_branches.lua']:
                    (game / 'luarules/gadgets' / stale).unlink(missing_ok=True)
                names = ['parity.lua'] if fixture in (None, 'branches') else []
                if fixture == 'branches': names += ['branches.lua']
                if fixture == 'filters': names += ['filters.lua']
                if fixture == 'blockers': names += ['check.lua']
                if fixture == 'dispatch': names += ['dispatch_rules.lua', 'dispatch_later.lua']
                for name in names:
                    detach(game / 'luarules/gadgets' / ('test_' + name), (suite / name).read_bytes())
                if fixture == 'dispatch':
                    detach(game / 'LuaGaia/main.lua', (suite / 'dispatch_gaia.lua').read_bytes())
                input_path = root / 'input.sdfz'
                if fixture:
                    script = (suite / ('blockers.txt' if fixture == 'filters' else fixture + '.txt')).read_text()
                    script = script.replace('attackmovementmode=lua', 'attackmovementmode=' + ('native' if mode == 'parent' else mode))
                    if fixture == 'filters': script = script.replace('attackmovementvalidation=blockers', 'attackmovementvalidation=filters')
                    input_path = root / 'input.txt'
                    input_path.write_text(script)
                files = inventory(game)
                (root / 'game-sha256.json').write_text(json.dumps(files, sort_keys=True, indent=2))
                record.update(game_manifest_sha256=digest(root / 'game-sha256.json'),
                              input_sha256=digest(input_path), original_replay_sha256=digest(replay),
                              engine_sha256=digest(binary), policy_sha256=digest(game / 'luarules/gadgets/unit_attack_movement.lua'))
                save()
                with (root / 'console.log').open('w') as log:
                    run([binary, '--only-local', '--isolation', '--write-dir', root, input_path],
                        stdout=log, stderr=subprocess.STDOUT, timeout=3600)
                log = (root / 'infolog.txt').read_text(errors='replace')
                if '[AttackValidation] FAIL' in log or re.search(r'LUA_ERR|Fatal:|Error: \[Lua(?:Rules|Gaia)|callin=AttackCommandMovement trace=', log):
                    raise RuntimeError(f'Engine/Lua error in {root}')
                if fixture not in (None, 'branches'):
                    expected = {'filters': 2, 'blockers': 86, 'dispatch': 9}[fixture]
                    if f'[AttackValidation] PASS {fixture}={expected}' not in log:
                        raise RuntimeError(f'Missing completion marker in {root}')
                else:
                    data = compare.read_result(root, frames, True)
                    snapshots = data.pop('snapshots')
                    if mode == 'parent': baseline[group] = snapshots
                    differences = [a[0] for a, b in zip(snapshots, baseline[group]) if a != b]
                    record.update(differing_frames=len(differences), first_difference=next(iter(differences), None))
                    if differences: raise RuntimeError(f'Parity mismatch: {record}')
                record['status'] = 'passed'
            finally:
                if game.exists(): shutil.rmtree(game)
                save()
        execute('filters', 'native', bar, replays[0][2], 100, 'filters')
        if args.only == 'all':
            for fixture in ['blockers', 'dispatch']:
                execute(fixture, 'lua', bar, replays[0][2], 10000, fixture)
            for group, game_source in [('historical-scenarios', historical), ('current-scenarios', bar)]:
                for mode in ['parent', 'native', 'fallback', 'lua']:
                    execute(group, mode, game_source, replays[0][2], 5280, 'branches')
            for prefix, frames, replay in replays:
                for mode in ['parent', 'native', 'fallback', 'lua']:
                    execute(prefix, mode, historical, replay, frames)
        if dict(engine=revision(repo), bar=revision(bar)) != sources:
            raise RuntimeError('Sources changed during validation')
        summary['status'] = 'passed'
    except BaseException as exc:
        summary.update(status='failed', error=str(exc))
        raise
    finally:
        summary['free_bytes_after'] = shutil.disk_usage(output).free
        save()
        print('Results:', output / 'summary.json', flush=True)


if __name__ == '__main__':
    main()
