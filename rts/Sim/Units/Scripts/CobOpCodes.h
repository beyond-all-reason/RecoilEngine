#ifndef COB_OPCODES_H
#define COB_OPCODES_H

// Command documentation from http://visualta.tauniverse.com/Downloads/cob-commands.txt
// And some information from basm0.8 source (basm ops.txt)

// Model interaction
static constexpr int MOVE       = 0x10001000;
static constexpr int TURN       = 0x10002000;
static constexpr int SPIN       = 0x10003000;
static constexpr int STOP_SPIN  = 0x10004000;
static constexpr int SHOW       = 0x10005000;
static constexpr int HIDE       = 0x10006000;
static constexpr int CACHE      = 0x10007000;
static constexpr int DONT_CACHE = 0x10008000;
static constexpr int MOVE_NOW   = 0x1000B000;
static constexpr int TURN_NOW   = 0x1000C000;
static constexpr int SHADE      = 0x1000D000;
static constexpr int DONT_SHADE = 0x1000E000;
static constexpr int EMIT_SFX   = 0x1000F000;
static constexpr int SCALE     = 0x100A0000; // Added in Recoil
static constexpr int SCALE_NOW = 0x100A1000; // Added in Recoil

// Blocking operations
static constexpr int WAIT_TURN  = 0x10011000;
static constexpr int WAIT_MOVE  = 0x10012000;
static constexpr int WAIT_SCALE = 0x100A2000; // Added in Recoil
static constexpr int SLEEP      = 0x10013000;

// Stack manipulation
static constexpr int PUSH_CONSTANT    = 0x10021001;
static constexpr int PUSH_LOCAL_VAR   = 0x10021002;
static constexpr int PUSH_STATIC      = 0x10021004;
static constexpr int CREATE_LOCAL_VAR = 0x10022000;
static constexpr int POP_LOCAL_VAR    = 0x10023002;
static constexpr int POP_STATIC       = 0x10023004;
static constexpr int POP_STACK        = 0x10024000; ///< Not sure what this is supposed to do

// Arithmetic operations
static constexpr int ADD         = 0x10031000;
static constexpr int SUB         = 0x10032000;
static constexpr int MUL         = 0x10033000;
static constexpr int DIV         = 0x10034000;
static constexpr int MOD		  = 0x10034001; ///< spring specific
static constexpr int BITWISE_AND = 0x10035000;
static constexpr int BITWISE_OR  = 0x10036000;
static constexpr int BITWISE_XOR = 0x10037000;
static constexpr int BITWISE_NOT = 0x10038000;

// Native function calls
static constexpr int RAND           = 0x10041000;
static constexpr int GET_UNIT_VALUE = 0x10042000;
static constexpr int GET            = 0x10043000;

// Comparison
static constexpr int SET_LESS             = 0x10051000;
static constexpr int SET_LESS_OR_EQUAL    = 0x10052000;
static constexpr int SET_GREATER          = 0x10053000;
static constexpr int SET_GREATER_OR_EQUAL = 0x10054000;
static constexpr int SET_EQUAL            = 0x10055000;
static constexpr int SET_NOT_EQUAL        = 0x10056000;
static constexpr int LOGICAL_AND          = 0x10057000;
static constexpr int LOGICAL_OR           = 0x10058000;
static constexpr int LOGICAL_XOR          = 0x10059000;
static constexpr int LOGICAL_NOT          = 0x1005A000;

// Flow control
static constexpr int START           = 0x10061000;
static constexpr int CALL            = 0x10062000; ///< converted when executed
static constexpr int REAL_CALL       = 0x10062001; ///< spring custom
static constexpr int LUA_CALL        = 0x10062002; ///< spring custom
static constexpr int BATCH_LUA       = 0x10062004; ///< recoil custom
static constexpr int JUMP            = 0x10064000;
static constexpr int RETURN          = 0x10065000;
static constexpr int JUMP_NOT_EQUAL  = 0x10066000;
static constexpr int SIGNAL          = 0x10067000;
static constexpr int SET_SIGNAL_MASK = 0x10068000;

// Piece destruction
static constexpr int EXPLODE    = 0x10071000;
static constexpr int PLAY_SOUND = 0x10072000;

// Special functions
static constexpr int SET    = 0x10082000;
static constexpr int ATTACH = 0x10083000;
static constexpr int DROP   = 0x10084000;

// Recoil custom function signatures, should be the only opcode for the function
// and signals a reference, not an opcode to be actually executed.
static constexpr int SIGNATURE_LUA = 0x10090000;

// Indices for SET, GET, and GET_UNIT_VALUE for LUA return values
static constexpr int LUA0 = 110; // (LUA0 returns the lua call status, 0 or 1)
static constexpr int LUA1 = 111;
static constexpr int LUA2 = 112;
static constexpr int LUA3 = 113;
static constexpr int LUA4 = 114;
static constexpr int LUA5 = 115;
static constexpr int LUA6 = 116;
static constexpr int LUA7 = 117;
static constexpr int LUA8 = 118;
static constexpr int LUA9 = 119;

#if 0
static const char* GetOpcodeName(int opcode)
{
	switch (opcode) {
		case MOVE: return "move";
		case TURN: return "turn";
		case BATCH_LUA: return "batch-lua";
		case SPIN: return "spin";
		case STOP_SPIN: return "stop-spin";
		case SHOW: return "show";
		case HIDE: return "hide";
		case CACHE: return "cache";
		case DONT_CACHE: return "dont-cache";
		case TURN_NOW: return "turn-now";
		case MOVE_NOW: return "move-now";
		case SHADE: return "shade";
		case DONT_SHADE: return "dont-shade";
		case EMIT_SFX: return "sfx";

		case WAIT_TURN: return "wait-for-turn";
		case WAIT_MOVE: return "wait-for-move";
		case SLEEP: return "sleep";

		case PUSH_CONSTANT: return "pushc";
		case PUSH_LOCAL_VAR: return "pushl";
		case PUSH_STATIC: return "pushs";
		case CREATE_LOCAL_VAR: return "clv";
		case POP_LOCAL_VAR: return "popl";
		case POP_STATIC: return "pops";
		case POP_STACK: return "pop-stack";

		case ADD: return "add";
		case SUB: return "sub";
		case MUL: return "mul";
		case DIV: return "div";
		case MOD: return "mod";
		case BITWISE_AND: return "and";
		case BITWISE_OR: return "or";
		case BITWISE_XOR: return "xor";
		case BITWISE_NOT: return "not";

		case RAND: return "rand";
		case GET_UNIT_VALUE: return "getuv";
		case GET: return "get";

		case SET_LESS: return "setl";
		case SET_LESS_OR_EQUAL: return "setle";
		case SET_GREATER: return "setg";
		case SET_GREATER_OR_EQUAL: return "setge";
		case SET_EQUAL: return "sete";
		case SET_NOT_EQUAL: return "setne";
		case LOGICAL_AND: return "land";
		case LOGICAL_OR: return "lor";
		case LOGICAL_XOR: return "lxor";
		case LOGICAL_NOT: return "neg";

		case START: return "start";
		case CALL: return "call";
		case REAL_CALL: return "call";
		case LUA_CALL: return "lua_call";
		case JUMP: return "jmp";
		case RETURN: return "return";
		case JUMP_NOT_EQUAL: return "jne";
		case SIGNAL: return "signal";
		case SET_SIGNAL_MASK: return "mask";

		case EXPLODE: return "explode";
		case PLAY_SOUND: return "play-sound";

		case SET: return "set";
		case ATTACH: return "attach";
		case DROP: return "drop";

		case SIGNATURE_LUA: return "signature_lua";
	}

	return "unknown";
}
#endif


#endif // COB_OPCODES_H
