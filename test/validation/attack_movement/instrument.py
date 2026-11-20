#!/usr/bin/env python3
# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
from pathlib import Path
import argparse
parser = argparse.ArgumentParser(description='Apply or restore temporary native-action tracing for shadow.lua')
parser.add_argument('action', choices=['apply', 'restore'])
parser.add_argument('backup', type=Path, help='Path outside the repository for the clean MobileCAI.cpp')
args = parser.parse_args()
root=Path(__file__).resolve().parents[3]
p=root/'rts/Sim/Units/CommandAI/MobileCAI.cpp'
s=p.read_text()
backup=args.backup
if args.action == 'restore':
 p.write_text(backup.read_text())
 raise SystemExit(0)
if '[NativeAttackBegin]' in s or backup.exists():
 raise SystemExit('Refusing to overwrite existing tracing or backup')
backup.write_text(s)
helper='''
// Validation-only instrumentation; not part of the proposed API.
namespace {
struct AttackTrace {
	CMobileCAI* cai;
	CUnit* unit;
	AttackTrace(CMobileCAI* c, CUnit* u, const Command& command): cai(c), unit(u) {
		LOG("[NativeAttackBegin] %d %d %d", gs->frameNum, unit->id, command.GetTag());
	}
	~AttackTrace() { LOG("[NativeAttackEnd]"); }
	void Move(const float3& p, const float3& from, float radius = SQUARE_SIZE) {
		LOG("[NativeAttackAction] move %.9g %.9g %.9g %.9g default", p.x, p.y, p.z, radius);
		cai->SetGoal(p, from, radius);
	}
	void Move(const float3& p, const float3& from, float radius, float speed) {
		LOG("[NativeAttackAction] move %.9g %.9g %.9g %.9g %.9g", p.x, p.y, p.z, radius, speed);
		cai->SetGoal(p, from, radius, speed);
	}
	void Stop() { LOG("[NativeAttackAction] stop"); cai->StopMove(); }
	void Face(const float3& p, float radius, bool aggressive) {
		LOG("[NativeAttackAction] face %.9g %.9g %.9g %.9g", p.x, p.y, p.z, radius);
		unit->moveType->KeepPointingTo(p, radius, aggressive);
	}
	void StopFace(const float3& p, float radius, bool aggressive) { Stop(); Face(p, std::max(radius, 10.f), aggressive); }
	void Finish() { Stop(); LOG("[NativeAttackAction] finish"); cai->FinishCommand(); }
	bool AttackUnit(CUnit* target, bool user, bool manual) {
		const bool accepted = unit->AttackUnit(target, user, manual);
		LOG("[NativeAttackAction] assignUnit %d %d %d %d", target->id, int(user), int(manual), int(accepted)); return accepted;
	}
	bool AttackGround(const float3& target, bool user, bool manual) {
		const bool accepted = unit->AttackGround(target, user, manual);
		LOG("[NativeAttackAction] assignGround %.9g %.9g %.9g %d %d %d", target.x, target.y, target.z, int(user), int(manual), int(accepted)); return accepted;
	}
};
}

'''
pos=s.index('void CMobileCAI::ExecuteObjectAttack(')
s=s[:pos]+helper+s[pos:]
start=s.index('void CMobileCAI::ExecuteObjectAttack(');end=s.index('void CMobileCAI::ExecuteAttack(',start)
chunk=s[start:end]
# Do not trace the pre-dispatch paralyze completion (no movement callback).
cut=chunk.index('\tif (CallAttackMovement(c))')
prefix=chunk[:cut];body=chunk[cut:]
for old,new in [('StopMoveAndFinishCommand(', 'trace.Finish('),('StopMoveAndKeepPointing(', 'trace.StopFace('),('StopMove(', 'trace.Stop('),('SetGoal(', 'trace.Move('),('owner->moveType->KeepPointingTo(', 'trace.Face('),('owner->AttackUnit(', 'trace.AttackUnit('),('owner->AttackGround(', 'trace.AttackGround(')]:
 body=body.replace(old,new)
body=body.replace('\tif (CallAttackMovement(c))', '\tAttackTrace trace(this, owner, c);\n\tif (CallAttackMovement(c))')
s=s[:start]+prefix+body+s[end:]
p.write_text(s)
