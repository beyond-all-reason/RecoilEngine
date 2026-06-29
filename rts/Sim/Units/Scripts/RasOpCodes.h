/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RAS_OPCODES_H
#define RAS_OPCODES_H

#include <cstdint>

// ============================================================================
// Dense byte opcode enum (Part I.1)
// Values match the RASC --shortopcodes byte values from bos2cob_py3.py.
// Generated from a single X-macro so enum, switch cases, label tables, and
// name strings all stay in sync.
// ============================================================================

#define RAS_OPCODE_LIST(X) \
	X(Move,          0x01) \
	X(Turn,          0x02) \
	X(Spin,          0x03) \
	X(StopSpin,      0x04) \
	X(Show,          0x05) \
	X(Hide,          0x06) \
	X(Cache,         0x28) \
	X(DontCache,     0x29) \
	X(MoveNow,       0x0B) \
	X(TurnNow,       0x0C) \
	X(Shade,         0x0D) \
	X(DontShade,     0x0E) \
	X(EmitSfx,       0x0F) \
	X(Scale,         0x0A) \
	X(ScaleNow,      0x10) \
	X(WaitTurn,      0x11) \
	X(WaitMove,      0x12) \
	X(Sleep,         0x13) \
	X(WaitScale,     0x14) \
	X(PushConstant,  0x21) \
	X(PushLocalVar,  0x22) \
	X(PushStatic,    0x23) \
	X(CreateLocalVar,0x24) \
	X(PopLocalVar,   0x25) \
	X(PopStatic,     0x26) \
	X(PopStack,      0x27) \
	X(PushStaticIdx, 0x2A) \
	X(PopStaticIdx,  0x2B) \
	X(Add,           0x31) \
	X(Sub,           0x32) \
	X(Mul,           0x33) \
	X(Div,           0x34) \
	X(Mod,           0x30) \
	X(BitwiseAnd,    0x35) \
	X(BitwiseOr,     0x36) \
	X(BitwiseXor,    0x37) \
	X(BitwiseNot,    0x38) \
	X(Absolute,      0x39) \
	X(Minimum,       0x3A) \
	X(Maximum,       0x3B) \
	X(Sign,          0x3C) \
	X(Clamp,         0x3D) \
	X(AddImm,        0x4B) \
	X(MulImm,        0x4C) \
	X(TurnRel,       0x4D) \
	X(MoveRel,       0x4E) \
	X(ExplodeRel,    0x4F) \
	X(ScaleRel,      0x50) \
	X(DeltaHeading,  0x3E) \
	X(Msine,         0x3F) \
	X(Mcosine,       0x40) \
	X(Rand,          0x41) \
	X(GetUnitValue,  0x42) \
	X(Get,           0x43) \
	X(SetLess,       0x51) \
	X(SetLessOrEqual,0x52) \
	X(SetGreater,    0x53) \
	X(SetGreaterOrEq,0x54) \
	X(SetEqual,      0x55) \
	X(SetNotEqual,   0x56) \
	X(LogicalAnd,    0x57) \
	X(LogicalOr,     0x58) \
	X(LogicalXor,    0x59) \
	X(LogicalNot,    0x5A) \
	X(Start,         0x61) \
	X(Call,          0x62) \
	X(Jump,          0x64) \
	X(Return,        0x65) \
	X(JumpNotEqual,  0x66) \
	X(Signal,        0x67) \
	X(SetSignalMask, 0x68) \
	X(LuaCall,       0x69) \
	X(BatchLua,      0x6A) \
	X(LuaUnsynced,   0x6B) \
	X(Explode,       0x71) \
	X(PlaySound,     0x72) \
	X(Set,           0x82) \
	X(Attach,        0x83) \
	X(Drop,          0x84) \
	X(SignatureLua,  0x90) \
	X(BadOpcode,     0xFF)

enum class RasOp : uint8_t {
	#define X(name, val) name = val,
	RAS_OPCODE_LIST(X)
	#undef X
};

// Human-readable name for each opcode.
inline const char* RasOpName(RasOp op)
{
	switch (op) {
		#define X(name, val) case RasOp::name: return #name;
		RAS_OPCODE_LIST(X)
		#undef X
	}
	return "Unknown";
}

// Number of inline operand words (32-bit) that follow the opcode byte.
// 0 = no operands, 1 = one inline word, 2 = two inline words.
inline constexpr int RasOpOperandWords(RasOp op)
{
	switch (op) {
		// 2-operand ops
		case RasOp::Move:
		case RasOp::MoveNow:
		case RasOp::Turn:
		case RasOp::TurnNow:
		case RasOp::Spin:
		case RasOp::StopSpin:
		case RasOp::Start:
		case RasOp::Call:
		case RasOp::LuaCall:
		case RasOp::BatchLua:
		case RasOp::LuaUnsynced:
			return 2;
		// 1-operand ops
		case RasOp::PushConstant:
		case RasOp::PushLocalVar:
		case RasOp::PushStatic:
		case RasOp::PopLocalVar:
		case RasOp::PopStatic:
		case RasOp::PushStaticIdx:
		case RasOp::PopStaticIdx:
		case RasOp::Jump:
		case RasOp::JumpNotEqual:
		case RasOp::Explode:
		case RasOp::PlaySound:
		case RasOp::EmitSfx:
		case RasOp::Scale:
		case RasOp::ScaleNow:
		case RasOp::WaitTurn:
		case RasOp::WaitMove:
		case RasOp::WaitScale:
		case RasOp::Shade:
		case RasOp::DontShade:
		case RasOp::Cache:
		case RasOp::DontCache:
		case RasOp::Hide:
		case RasOp::Show:
		case RasOp::SetSignalMask:
		case RasOp::AddImm:
		case RasOp::MulImm:
			return 1;
		// 0-operand ops (default)
		default:
			return 0;
	}
}

// Total number of entries in the jump table (full byte space).
static constexpr int RAS_OP_TABLE_SIZE = 256;

/// Returns true if the opcode does not touch global/shared state.
/// Thread-safe ops can execute on the MT path without the global lock.
inline bool RasOpIsThreadSafe(RasOp op)
{
	switch (op) {
		// Explicitly UNSAFE - touches global Lua state
		case RasOp::LuaCall:
		case RasOp::SignatureLua:
			return false;
		// Everything else is thread-safe
		default:
			return true;
	}
}

// ============================================================================
// Legacy 32-bit opcode constants (backward compatibility)
// These match the original COB bytecode format. New code should prefer
// the RasOp enum; the RawToRasOp() function maps 32-bit -> byte during load.
// ============================================================================

// We generate the 32-bit constants from the same X-macro by appending the
// original 32-bit value.  To avoid duplicating the mapping, we define a
// companion macro that carries the 32-bit value alongside the byte value.

#define RAS_OPCODE_WITH_RAW(X) \
	X(Move,          0x01, 0x10001000) \
	X(Turn,          0x02, 0x10002000) \
	X(Spin,          0x03, 0x10003000) \
	X(StopSpin,      0x04, 0x10004000) \
	X(Show,          0x05, 0x10005000) \
	X(Hide,          0x06, 0x10006000) \
	X(Cache,         0x28, 0x10007000) \
	X(DontCache,     0x29, 0x10008000) \
	X(MoveNow,       0x0B, 0x1000B000) \
	X(TurnNow,       0x0C, 0x1000C000) \
	X(Shade,         0x0D, 0x1000D000) \
	X(DontShade,     0x0E, 0x1000E000) \
	X(EmitSfx,       0x0F, 0x1000F000) \
	X(Scale,         0x0A, 0x100A0000) \
	X(ScaleNow,      0x10, 0x100A1000) \
	X(WaitTurn,      0x11, 0x10011000) \
	X(WaitMove,      0x12, 0x10012000) \
	X(Sleep,         0x13, 0x10013000) \
	X(WaitScale,     0x14, 0x100A2000) \
	X(PushConstant,  0x21, 0x10021001) \
	X(PushLocalVar,  0x22, 0x10021002) \
	X(PushStatic,    0x23, 0x10021004) \
	X(CreateLocalVar,0x24, 0x10022000) \
	X(PopLocalVar,   0x25, 0x10023002) \
	X(PopStatic,     0x26, 0x10023004) \
	X(PopStack,      0x27, 0x10024000) \
	X(PushStaticIdx, 0x2A, 0x00000000) \
	X(PopStaticIdx,  0x2B, 0x00000000) \
	X(Add,           0x31, 0x10031000) \
	X(Sub,           0x32, 0x10032000) \
	X(Mul,           0x33, 0x10033000) \
	X(Div,           0x34, 0x10034000) \
	X(Mod,           0x30, 0x10034001) \
	X(BitwiseAnd,    0x35, 0x10035000) \
	X(BitwiseOr,     0x36, 0x10036000) \
	X(BitwiseXor,    0x37, 0x10037000) \
	X(BitwiseNot,    0x38, 0x10038000) \
	X(Absolute,      0x39, 0x00000000) \
	X(Minimum,       0x3A, 0x00000000) \
	X(Maximum,       0x3B, 0x00000000) \
	X(Sign,          0x3C, 0x00000000) \
	X(Clamp,         0x3D, 0x00000000) \
	X(AddImm,        0x4B, 0x00000000) \
	X(MulImm,        0x4C, 0x00000000) \
	X(TurnRel,       0x4D, 0x00000000) \
	X(MoveRel,       0x4E, 0x00000000) \
	X(ExplodeRel,    0x4F, 0x00000000) \
	X(ScaleRel,      0x50, 0x00000000) \
	X(DeltaHeading,  0x3E, 0x00000000) \
	X(Msine,         0x3F, 0x00000000) \
	X(Mcosine,       0x40, 0x00000000) \
	X(Rand,          0x41, 0x10041000) \
	X(GetUnitValue,  0x42, 0x10042000) \
	X(Get,           0x43, 0x10043000) \
	X(SetLess,       0x51, 0x10051000) \
	X(SetLessOrEqual,0x52, 0x10052000) \
	X(SetGreater,    0x53, 0x10053000) \
	X(SetGreaterOrEq,0x54, 0x10054000) \
	X(SetEqual,      0x55, 0x10055000) \
	X(SetNotEqual,   0x56, 0x10056000) \
	X(LogicalAnd,    0x57, 0x10057000) \
	X(LogicalOr,     0x58, 0x10058000) \
	X(LogicalXor,    0x59, 0x10059000) \
	X(LogicalNot,    0x5A, 0x1005A000) \
	X(Start,         0x61, 0x10061000) \
	X(Call,          0x62, 0x10062000) \
	X(Jump,          0x64, 0x10064000) \
	X(Return,        0x65, 0x10065000) \
	X(JumpNotEqual,  0x66, 0x10066000) \
	X(Signal,        0x67, 0x10067000) \
	X(SetSignalMask, 0x68, 0x10068000) \
	X(LuaCall,       0x69, 0x10062002) \
	X(BatchLua,      0x6A, 0x10062004) \
	X(LuaUnsynced,   0x6B, 0x00000000) \
	X(Explode,       0x71, 0x10071000) \
	X(PlaySound,     0x72, 0x10072000) \
	X(Set,           0x82, 0x10082000) \
	X(Attach,        0x83, 0x10083000) \
	X(Drop,          0x84, 0x10084000) \
	X(SignatureLua,  0x90, 0x10090000) \
	X(BadOpcode,     0xFF, 0x00000000)

// Compile-time guard: the standalone byte list and the byte+raw list must
// assign identical byte values, or RawToRasOp/name tables silently drift.
#define X(name, val, raw) static_assert((int)RasOp::name == val, "opcode byte drift: " #name);
RAS_OPCODE_WITH_RAW(X)
#undef X

// Legacy 32-bit constants (RAW_ prefix to avoid collision with enum).
// Used by file loader and existing Tick() switch.  New code: use RasOp enum.
static constexpr int RAW_Move          = 0x10001000;
static constexpr int RAW_Turn          = 0x10002000;
static constexpr int RAW_Spin          = 0x10003000;
static constexpr int RAW_StopSpin      = 0x10004000;
static constexpr int RAW_Show          = 0x10005000;
static constexpr int RAW_Hide          = 0x10006000;
static constexpr int RAW_Cache         = 0x10007000;
static constexpr int RAW_DontCache     = 0x10008000;
static constexpr int RAW_MoveNow       = 0x1000B000;
static constexpr int RAW_TurnNow       = 0x1000C000;
static constexpr int RAW_Shade         = 0x1000D000;
static constexpr int RAW_DontShade     = 0x1000E000;
static constexpr int RAW_EmitSfx       = 0x1000F000;
static constexpr int RAW_Scale         = 0x100A0000;
static constexpr int RAW_ScaleNow      = 0x100A1000;
static constexpr int RAW_WaitTurn      = 0x10011000;
static constexpr int RAW_WaitMove      = 0x10012000;
static constexpr int RAW_Sleep         = 0x10013000;
static constexpr int RAW_WaitScale     = 0x100A2000;
static constexpr int RAW_PushConstant  = 0x10021001;
static constexpr int RAW_PushLocalVar  = 0x10021002;
static constexpr int RAW_PushStatic    = 0x10021004;
static constexpr int RAW_CreateLocalVar= 0x10022000;
static constexpr int RAW_PopLocalVar   = 0x10023002;
static constexpr int RAW_PopStatic     = 0x10023004;
static constexpr int RAW_PopStack      = 0x10024000;
static constexpr int RAW_PushStaticIdx = 0x00000000;
static constexpr int RAW_PopStaticIdx  = 0x00000000;
static constexpr int RAW_Add           = 0x10031000;
static constexpr int RAW_Sub           = 0x10032000;
static constexpr int RAW_Mul           = 0x10033000;
static constexpr int RAW_Div           = 0x10034000;
static constexpr int RAW_Mod           = 0x10034001;
static constexpr int RAW_BitwiseAnd    = 0x10035000;
static constexpr int RAW_BitwiseOr     = 0x10036000;
static constexpr int RAW_BitwiseXor    = 0x10037000;
static constexpr int RAW_BitwiseNot    = 0x10038000;
static constexpr int RAW_Rand          = 0x10041000;
static constexpr int RAW_GetUnitValue  = 0x10042000;
static constexpr int RAW_Get           = 0x10043000;
static constexpr int RAW_SetLess       = 0x10051000;
static constexpr int RAW_SetLessOrEqual= 0x10052000;
static constexpr int RAW_SetGreater    = 0x10053000;
static constexpr int RAW_SetGreaterOrEq= 0x10054000;
static constexpr int RAW_SetEqual      = 0x10055000;
static constexpr int RAW_SetNotEqual   = 0x10056000;
static constexpr int RAW_LogicalAnd    = 0x10057000;
static constexpr int RAW_LogicalOr     = 0x10058000;
static constexpr int RAW_LogicalXor    = 0x10059000;
static constexpr int RAW_LogicalNot    = 0x1005A000;
static constexpr int RAW_Start         = 0x10061000;
static constexpr int RAW_Call          = 0x10062000;
static constexpr int RAW_LuaCall       = 0x10062002;
static constexpr int RAW_BatchLua      = 0x10062004;
static constexpr int RAW_LuaUnsynced   = 0x00000000;
static constexpr int RAW_Jump          = 0x10064000;
static constexpr int RAW_Return        = 0x10065000;
static constexpr int RAW_JumpNotEqual  = 0x10066000;
static constexpr int RAW_Signal        = 0x10067000;
static constexpr int RAW_SetSignalMask = 0x10068000;
static constexpr int RAW_Explode       = 0x10071000;
static constexpr int RAW_PlaySound     = 0x10072000;
static constexpr int RAW_Set           = 0x10082000;
static constexpr int RAW_Attach        = 0x10083000;
static constexpr int RAW_Drop          = 0x10084000;
static constexpr int RAW_SignatureLua  = 0x10090000;
static constexpr int RAW_TurnRel       = 0x00000000;
static constexpr int RAW_MoveRel       = 0x00000000;
static constexpr int RAW_ExplodeRel    = 0x00000000;
static constexpr int RAW_ScaleRel      = 0x00000000;

// Backward-compatible aliases (unprefixed, for existing code).
// TODO: migrate callers to RasOp enum, then remove.
static constexpr int MOVE        = RAW_Move;
static constexpr int TURN        = RAW_Turn;
static constexpr int SPIN        = RAW_Spin;
static constexpr int STOP_SPIN   = RAW_StopSpin;
static constexpr int SHOW        = RAW_Show;
static constexpr int HIDE        = RAW_Hide;
static constexpr int CACHE       = RAW_Cache;
static constexpr int DONT_CACHE  = RAW_DontCache;
static constexpr int MOVE_NOW    = RAW_MoveNow;
static constexpr int TURN_NOW    = RAW_TurnNow;
static constexpr int SHADE       = RAW_Shade;
static constexpr int DONT_SHADE  = RAW_DontShade;
static constexpr int EMIT_SFX    = RAW_EmitSfx;
static constexpr int SCALE       = RAW_Scale;
static constexpr int SCALE_NOW   = RAW_ScaleNow;
static constexpr int WAIT_TURN   = RAW_WaitTurn;
static constexpr int WAIT_MOVE   = RAW_WaitMove;
static constexpr int SLEEP       = RAW_Sleep;
static constexpr int WAIT_SCALE  = RAW_WaitScale;
static constexpr int PUSH_CONSTANT    = RAW_PushConstant;
static constexpr int PUSH_LOCAL_VAR   = RAW_PushLocalVar;
static constexpr int PUSH_STATIC      = RAW_PushStatic;
static constexpr int CREATE_LOCAL_VAR = RAW_CreateLocalVar;
static constexpr int POP_LOCAL_VAR    = RAW_PopLocalVar;
static constexpr int POP_STATIC       = RAW_PopStatic;
static constexpr int POP_STACK        = RAW_PopStack;
static constexpr int ADD         = RAW_Add;
static constexpr int SUB         = RAW_Sub;
static constexpr int MUL         = RAW_Mul;
static constexpr int DIV         = RAW_Div;
static constexpr int MOD         = RAW_Mod;
static constexpr int BITWISE_AND = RAW_BitwiseAnd;
static constexpr int BITWISE_OR  = RAW_BitwiseOr;
static constexpr int BITWISE_XOR = RAW_BitwiseXor;
static constexpr int BITWISE_NOT = RAW_BitwiseNot;
static constexpr int RAND            = RAW_Rand;
static constexpr int GET_UNIT_VALUE  = RAW_GetUnitValue;
static constexpr int GET             = RAW_Get;
static constexpr int SET_LESS             = RAW_SetLess;
static constexpr int SET_LESS_OR_EQUAL    = RAW_SetLessOrEqual;
static constexpr int SET_GREATER          = RAW_SetGreater;
static constexpr int SET_GREATER_OR_EQUAL = RAW_SetGreaterOrEq;
static constexpr int SET_EQUAL            = RAW_SetEqual;
static constexpr int SET_NOT_EQUAL        = RAW_SetNotEqual;
static constexpr int LOGICAL_AND          = RAW_LogicalAnd;
static constexpr int LOGICAL_OR           = RAW_LogicalOr;
static constexpr int LOGICAL_XOR          = RAW_LogicalXor;
static constexpr int LOGICAL_NOT          = RAW_LogicalNot;
static constexpr int START            = RAW_Start;
static constexpr int CALL             = RAW_Call;
static constexpr int LUA_CALL         = RAW_LuaCall;
static constexpr int BATCH_LUA        = RAW_BatchLua;
static constexpr int LUA_UNSYNCED     = RAW_LuaUnsynced;
static constexpr int JUMP             = RAW_Jump;
static constexpr int RETURN           = RAW_Return;
static constexpr int JUMP_NOT_EQUAL   = RAW_JumpNotEqual;
static constexpr int SIGNAL           = RAW_Signal;
static constexpr int SET_SIGNAL_MASK  = RAW_SetSignalMask;
static constexpr int EXPLODE          = RAW_Explode;
static constexpr int PLAY_SOUND       = RAW_PlaySound;
static constexpr int SET              = RAW_Set;
static constexpr int ATTACH           = RAW_Attach;
static constexpr int DROP             = RAW_Drop;
static constexpr int SIGNATURE_LUA    = RAW_SignatureLua;

// Lua return-value indices (unchanged)
static constexpr int LUA0 = 110;
static constexpr int LUA1 = 111;
static constexpr int LUA2 = 112;
static constexpr int LUA3 = 113;
static constexpr int LUA4 = 114;
static constexpr int LUA5 = 115;
static constexpr int LUA6 = 116;
static constexpr int LUA7 = 117;
static constexpr int LUA8 = 118;
static constexpr int LUA9 = 119;

// Legacy-only opcodes (have a non-zero 32-bit COB raw value).
// RASC-only opcodes (Absolute, Minimum, Maximum, Sign, Clamp, AddImm,
// MulImm, TurnRel, MoveRel, ExplodeRel, ScaleRel, DeltaHeading, Msine,
// Mcosine, PushStaticIdx, PopStaticIdx, LuaUnsynced) are excluded — they
// never appear in raw COB bytecode and have no RAW_ mapping.
#define RAS_LEGACY_OPCODE_LIST(X) \
	X(Move,          0x01) \
	X(Turn,          0x02) \
	X(Spin,          0x03) \
	X(StopSpin,      0x04) \
	X(Show,          0x05) \
	X(Hide,          0x06) \
	X(Cache,         0x28) \
	X(DontCache,     0x29) \
	X(MoveNow,       0x0B) \
	X(TurnNow,       0x0C) \
	X(Shade,         0x0D) \
	X(DontShade,     0x0E) \
	X(EmitSfx,       0x0F) \
	X(Scale,         0x0A) \
	X(ScaleNow,      0x10) \
	X(WaitTurn,      0x11) \
	X(WaitMove,      0x12) \
	X(Sleep,         0x13) \
	X(WaitScale,     0x14) \
	X(PushConstant,  0x21) \
	X(PushLocalVar,  0x22) \
	X(PushStatic,    0x23) \
	X(CreateLocalVar,0x24) \
	X(PopLocalVar,   0x25) \
	X(PopStatic,     0x26) \
	X(PopStack,      0x27) \
	X(Add,           0x31) \
	X(Sub,           0x32) \
	X(Mul,           0x33) \
	X(Div,           0x34) \
	X(Mod,           0x30) \
	X(BitwiseAnd,    0x35) \
	X(BitwiseOr,     0x36) \
	X(BitwiseXor,    0x37) \
	X(BitwiseNot,    0x38) \
	X(Rand,          0x41) \
	X(GetUnitValue,  0x42) \
	X(Get,           0x43) \
	X(SetLess,       0x51) \
	X(SetLessOrEqual,0x52) \
	X(SetGreater,    0x53) \
	X(SetGreaterOrEq,0x54) \
	X(SetEqual,      0x55) \
	X(SetNotEqual,   0x56) \
	X(LogicalAnd,    0x57) \
	X(LogicalOr,     0x58) \
	X(LogicalXor,    0x59) \
	X(LogicalNot,    0x5A) \
	X(Start,         0x61) \
	X(Call,          0x62) \
	X(Jump,          0x64) \
	X(Return,        0x65) \
	X(JumpNotEqual,  0x66) \
	X(Signal,        0x67) \
	X(SetSignalMask, 0x68) \
	X(LuaCall,       0x69) \
	X(BatchLua,      0x6A) \
	X(Explode,       0x71) \
	X(PlaySound,     0x72) \
	X(Set,           0x82) \
	X(Attach,        0x83) \
	X(Drop,          0x84) \
	X(SignatureLua,  0x90)

// Map a raw 32-bit COB opcode to the dense byte enum.
// Returns RasOp::BadOpcode for unrecognized values.
inline RasOp RawToRasOp(int raw32)
{
	switch (raw32) {
		#define X(name, val) case RAW_##name: return RasOp::name;
		RAS_LEGACY_OPCODE_LIST(X)
		#undef X
		default: return RasOp::BadOpcode;
	}
}

// Inverse map: dense byte enum -> original 32-bit COB opcode value.
// Returns 0 for RASC-only opcodes that have no legacy COB equivalent.
inline constexpr int RasOpToRaw(RasOp op)
{
	switch (op) {
		#define X(name, val) case RasOp::name: return RAW_##name;
		RAS_LEGACY_OPCODE_LIST(X)
		#undef X
		default: return 0;
	}
}

// RasInstr flags (bitmask in flags byte, reserved for future use)

// Decoded instruction structure (Part I internal representation).
// In-memory: natural alignment, 12 bytes (op/flags + 2 pad + a + b), fast
// random access for the hot dispatch loop. The on-disk RASC layout is packed
// to 10 bytes (no padding after op/flags) and is parsed field-by-field on
// load, so the runtime struct must NOT be packed. Keep these two separate.
struct RasInstr {
	uint8_t op;       // RasOp byte value
	uint8_t flags;    // RAS_INSTR_* bitmask
	int32_t a;        // first inline operand / jump target / funcId
	int32_t b;        // second inline operand / argCount / immediate
};
static_assert(sizeof(RasInstr) == 12, "RasInstr must be 12 bytes (in memory)");

// On-disk per-instruction stride in the RASC binary: op(1)+flags(1)+a(4)+b(4).
static constexpr size_t RASC_DISK_INSTR_SIZE = 10;

#endif // RAS_OPCODES_H
