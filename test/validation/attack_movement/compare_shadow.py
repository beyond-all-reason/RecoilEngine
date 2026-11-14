#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
"""Compare side-effect-free Lua plans to diagnostic native actions on the same state."""
import argparse, json, math, re, struct
from pathlib import Path

def compare(expected, actual, tolerance):
    if len(expected) != len(actual):
        return 'action_count', None
    largest = 0.0
    for left, right in zip(expected, actual):
        a, b = left.split(), right.split()
        if len(a) != len(b) or a[0] != b[0]:
            return 'action_kind', None
        for x, y in zip(a[1:], b[1:]):
            try:
                error = abs(struct.unpack('f', struct.pack('f', float(x)))[0] - struct.unpack('f', struct.pack('f', float(y)))[0])
            except ValueError:
                if x == y:
                    continue
                return 'action_argument', None
            except OverflowError:
                return 'nonfinite', None
            if not math.isfinite(error):
                return 'nonfinite', None
            if a[0].startswith('assign') and error != 0:
                return 'assignment_result', None
            largest = max(largest, error)
    return ('destination' if largest > tolerance else None), largest

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('log',type=Path)
    p.add_argument('--tolerance',type=float,default=0.01,help='Absolute world-unit tolerance; never applied to discrete actions')
    a=p.parse_args()
    stack=[]; records=[]
    log=a.log.read_text(errors='replace')
    if re.search(r'LUA_ERR|Error: \[Lua(?:Rules|Gaia)',log):
        raise SystemExit('Lua failure in shadow run')
    for line in log.splitlines():
        if '[NativeAttackBegin]' in line:
            stack.append(dict(key=line.split('[NativeAttackBegin] ')[1],native=[],shadows={}))
        elif '[AttackShadow' in line:
            m=re.search(r'\[AttackShadow([12])\] ?(.*)',line)
            stack[-1]['shadows'][m[1]]=m[2].split(' | ') if m[2] else []
        elif '[NativeAttackAction]' in line:
            stack[-1]['native'].append(line.split('[NativeAttackAction] ')[1])
        elif '[NativeAttackEnd]' in line:
            records.append(stack.pop())
    if stack or not records or '[AttackValidation] PASS replay' not in log:
        raise SystemExit('Incomplete shadow run')
    failures=[]; numerical=0; maximum=0; counts={}
    for row in records:
        if len(row['shadows']) != 2:
            failures.append(dict(key=row['key'],reason='missing_shadow'))
            continue
        comparisons=[(compare(actions,row['native'],a.tolerance),actions) for actions in row['shadows'].values()]
        passing=[entry for entry in comparisons if entry[0][0] is None]
        if not passing:
            failures.append(row)
            continue
        error=min(entry[0][1] for entry in passing)
        numerical += error > 0
        maximum=max(maximum,error)
        for action in row['native']:
            name=action.split()[0];counts[name]=counts.get(name,0)+1
    result=dict(updates=len(records),passing=len(records)-len(failures),failures=len(failures),
                numerically_different_updates=numerical,max_accepted_error=maximum,tolerance=a.tolerance,
                actions=counts,first_failures=failures[:20])
    output=a.log.with_name('shadow-results.json');output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
    if failures: raise SystemExit(1)
if __name__=='__main__': main()
