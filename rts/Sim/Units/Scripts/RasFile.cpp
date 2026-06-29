/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Sim/Misc/GlobalConstants.h"
#include "RasFile.h"
#include "RasOpCodes.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Log/ILog.h"
#include "System/Sound/ISound.h"
#include "System/Platform/byteorder.h"
#include "System/StringUtil.h"

#include <algorithm>
#include <locale>
#include <cctype>
#include <cstring>
#include <utility>

#include "System/Misc/TracyDefs.h"


//The following structure is taken from http://visualta.tauniverse.com/Downloads/ta-cob-fmt.txt
//Information on missing fields from Format_Cob.pas
typedef struct tagCOBHeader
{
	int VersionSignature; // 4 for TA, 6 for TA:K
	int NumberOfScripts;
	int NumberOfPieces;
	int TotalScriptLen;
	int NumberOfStaticVars;
	int Unknown_2; /* Always seems to be 0 */
	int OffsetToScriptCodeIndexArray;
	int OffsetToScriptNameOffsetArray;
	int OffsetToPieceNameOffsetArray;
	int OffsetToScriptCode;
	int Unknown_3; /* Always seems to point to first script name */

	int OffsetToSoundNameArray;		// These two are only found in TA:K scripts
	int NumberOfSounds;
} COBHeader;


#define READ_COBHEADER(ch,src)						\
do {									\
	unsigned int __tmp;						\
	unsigned short __isize = sizeof(unsigned int);			\
	unsigned int __c = 0;						\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).VersionSignature = (int)swabDWord(__tmp);			\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).NumberOfScripts = (int)swabDWord(__tmp);			\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).NumberOfPieces = (int)swabDWord(__tmp);			\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).TotalScriptLen = (int)swabDWord(__tmp);			\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).NumberOfStaticVars = (int)swabDWord(__tmp);		\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).Unknown_2 = (int)swabDWord(__tmp);				\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).OffsetToScriptCodeIndexArray = (int)swabDWord(__tmp);	\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).OffsetToScriptNameOffsetArray = (int)swabDWord(__tmp);	\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).OffsetToPieceNameOffsetArray = (int)swabDWord(__tmp);	\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).OffsetToScriptCode = (int)swabDWord(__tmp);		\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).Unknown_3 = (int)swabDWord(__tmp);				\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).OffsetToSoundNameArray = (int)swabDWord(__tmp);		\
	__c+=__isize;							\
	memcpy(&__tmp,&((src)[__c]),__isize);				\
	(ch).NumberOfSounds = (int)swabDWord(__tmp);			\
} while (0)


static std::vector<uint8_t> rasFileData;

// Defined later in this file; used by decodeBytecode() for stack-depth tracking.
static int stackDelta(RasOp op);


CRasFile::CRasFile(CFileHandler& in, const std::string& scriptName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	name.assign(scriptName);
	scriptIndex.fill(-1);

	// handle errors (this is fairly fatal..)
	if (in.FileSize() < 0) {
		LOG_L(L_FATAL, "[%s] could not find script \"%s\"", __func__, name.c_str());
		return;
	}

	if (!in.IsBuffered()) {
		rasFileData.clear();
		rasFileData.resize(in.FileSize());
		// read the entire thing, we will need it
		in.Read(rasFileData.data(), rasFileData.size());
	} else {
		rasFileData = std::move(in.GetBuffer());
	}

	// Detect format by magic number and dispatch to appropriate loader
	if (rasFileData.size() >= 4) {
		uint32_t magic = 0;
		memcpy(&magic, rasFileData.data(), 4);
		if (magic == 0x43534152) {  // "RASC"
			if (loadRascFormat(rasFileData.data(), rasFileData.size()))
				return;
		}
	}

	// time to parse
	COBHeader ch;
	READ_COBHEADER(ch, rasFileData.data());

	if (ch.NumberOfScripts == 0) {
		LOG_L(L_WARNING, "[%s] script \"%s\" is empty", __func__, name.c_str());
		return;
	}

	// prepare
	luaScripts.reserve(ch.NumberOfScripts);
	scriptNames.reserve(ch.NumberOfScripts);
	scriptOffsets.reserve(ch.NumberOfScripts);
	scriptLengths.reserve(ch.NumberOfScripts);
	pieceNames.reserve(ch.NumberOfPieces);

	// code accessor so we don't need to process code area yet, while being able to access its contents
	int* preCode = reinterpret_cast<int*>(&rasFileData[ch.OffsetToScriptCode]);

	for (int i = 0; i < ch.NumberOfScripts; ++i) {
		int ofs = *(int *) &rasFileData[ch.OffsetToScriptNameOffsetArray + i * 4];
		swabDWordInPlace(ofs);
		scriptNames.emplace_back(reinterpret_cast<const char*>(&rasFileData[ofs]));

		ofs = *(int *) &rasFileData[ch.OffsetToScriptCodeIndexArray + i * 4];
		swabDWordInPlace(ofs);

		if (scriptNames.back().find("lua_synced_") == 0) {
			luaScripts.emplace_back(scriptNames.back().c_str() + sizeof("lua_synced_") - 1);
		} else if (scriptNames.back().find("lua_unsynced_") == 0) {
			luaScripts.emplace_back(scriptNames.back().c_str() + sizeof("lua_unsynced_") - 1);
		} else if (swabDWord(*(preCode + ofs)) == SIGNATURE_LUA) {
			luaScripts.emplace_back(scriptNames[scriptNames.size() - 1].c_str());
		} else {
			luaScripts.emplace_back("");
		}

		scriptOffsets.push_back(ofs);
	}

	// check for zero-length scripts
	for (int i = 0; i < ch.NumberOfScripts - 1; ++i) {
		scriptLengths.push_back(scriptOffsets[i + 1] - scriptOffsets[i]);
	}

	scriptLengths.push_back(ch.TotalScriptLen - scriptOffsets[ch.NumberOfScripts - 1]);

	for (int i = 0; i < ch.NumberOfPieces; ++i) {
		int ofs = *(int *) &rasFileData[ch.OffsetToPieceNameOffsetArray + i * 4];
		swabDWordInPlace(ofs);
		pieceNames.emplace_back(StringToLower(reinterpret_cast<const char*>(&rasFileData[ofs])));
	}

	const int codeBytes = int(rasFileData.size()) - ch.OffsetToScriptCode;
	const int codeWords = codeBytes / 4 + 4;
	code.resize(codeWords);
	memcpy(code.data(), &rasFileData[ch.OffsetToScriptCode], codeBytes);
	for (int i = 0; i < codeWords; i++) {
		swabDWordInPlace(code[i]);
	}

	numStaticVars = ch.NumberOfStaticVars;

	// ---- Part I.2 + I.3: Decode raw bytecode into dense instruction stream ----
	decodeBytecode();

	// ---- Part V: Analyze thread-safety of each function ----
	analyzeThreadSafety();

	// if this is a TA:K script, read the sound names
	if (ch.VersionSignature == 6) {
		sounds.reserve(ch.NumberOfSounds);

		for (int i = 0; i < ch.NumberOfSounds; ++i) {
			int ofs = *(int *) &rasFileData[ch.OffsetToSoundNameArray + i * 4];
			// FIXME: this probably isn't correct
			swabDWordInPlace(ofs);

			const std::string s = {reinterpret_cast<const char*>(&rasFileData[ofs])};

			if (sound->HasSoundItem(s)) {
				sounds.push_back(sound->GetSoundId(s));
			} else {
				// Load the wave file and store the ID for future use
				sounds.push_back(sound->GetSoundId("sounds/" + s + ".wav"));
			}
		}
	}


	// create a reverse mapping (name->int)
	for (size_t i = 0, n = scriptNames.size(); i < n; ++i) {
		scriptMap[scriptNames[i]] = i;
	}

	// map common function names to indices
	for (const auto& pair: CCobUnitScriptNames::GetScriptMap()) {
		const int fn = GetFunctionId(pair.first);

		if (fn < 0)
			continue;

		scriptIndex[pair.second] = fn;
	}

}


int CRasFile::GetFunctionId(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto i = scriptMap.find(name);

	if (i != scriptMap.end())
		return i->second;

	return -1;
}

// Part I.2: Operand-aware decode walk.
// Converts raw 32-bit bytecode into dense RasInstr stream.
// Part I.3: CALL pre-resolution + jump retargeting (no in-place mutation).
void CRasFile::decodeBytecode()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int numScripts = static_cast<int>(scriptNames.size());
	if (numScripts == 0)
		return;

	decodedOffsets.resize(numScripts, -1);
	decodedLengths.resize(numScripts, 0);
	maxStackDepth.resize(numScripts, 0);
	threadSafeFuncs.resize(numScripts, 0);  // default: unsafe (serial)

	// Map from raw word offset -> decoded instruction index.
	// Used for jump retargeting in the second pass.
	std::vector<int> wordToInstr(code.size(), -1);

	// ---- Pass 1: Decode each function ----
	for (int fi = 0; fi < numScripts; ++fi) {
		// Skip Lua signature functions (references, not executable code)
		if (luaScripts[fi].GetString()[0] != '\0') {
			continue;
		}

		const int startWord = scriptOffsets[fi];
		const int lenWords  = scriptLengths[fi];
		const int endWord   = startWord + lenWords;

		decodedOffsets[fi] = static_cast<int>(decoded.size());
		const size_t funcStart = decoded.size();

		int stackDepth = 0;
		int maxDepth = 0;

		for (int w = startWord; w < endWord; ) {
			const int rawOp = code[w];
			const RasOp op = RawToRasOp(rawOp);
			const int operands = RasOpOperandWords(op);

			RasInstr instr;
			instr.op = static_cast<uint8_t>(op);
			instr.flags = 0;
			instr.a = 0;
			instr.b = 0;

			// Read inline operands
			if (operands >= 1) {
				instr.a = code[w + 1];
			}
			if (operands >= 2) {
				instr.b = code[w + 2];
			}

			// Part I.3: CALL pre-resolution.
			// Raw CALL(0x10062000) carries funcId in operand a, argCount in b.
			// Resolve based on target function name prefix:
			//   lua_synced_*   -> LuaCall
			//   lua_unsynced_* -> LuaUnsynced
			//   default        -> Call (regular function call)
			if (op == RasOp::Call) {
				const int funcId = instr.a;
				if (funcId >= 0 && funcId < numScripts) {
					const std::string& fn = scriptNames[funcId];
					if (fn.find("lua_synced_") == 0) {
						instr.op = static_cast<uint8_t>(RasOp::LuaCall);
					} else if (fn.find("lua_unsynced_") == 0) {
						instr.op = static_cast<uint8_t>(RasOp::LuaUnsynced);
					}
					// else: stays as Call (regular function call)
				}
			}

			wordToInstr[w] = static_cast<int>(decoded.size());
			decoded.push_back(instr);

			// Track stack depth for maxStackDepth computation
			stackDepth += stackDelta(op);
			if (stackDepth < 0)
				stackDepth = 0;
			if (stackDepth > maxDepth)
				maxDepth = stackDepth;

			// Advance past opcode + operands
			w += 1 + operands;
		}

		maxStackDepth[fi] = maxDepth;
		decodedLengths[fi] = static_cast<int>(decoded.size() - funcStart);
	}

	// ---- Pass 2: Retarget jump targets + finalize RealCall targets ----
	for (int fi = 0; fi < numScripts; ++fi) {
		if (luaScripts[fi].GetString()[0] != '\0')
			continue;

		const int instrStart = decodedOffsets[fi];
		if (instrStart < 0)
			continue;

		const int startWord = scriptOffsets[fi];
		const int lenWords  = scriptLengths[fi];
		const int endWord   = startWord + lenWords;

		int instrIdx = instrStart;
		for (int w = startWord; w < endWord; ) {
			if (wordToInstr[w] < 0) {
				// This word is an operand, skip
				break;
			}

			RasInstr& instr = decoded[instrIdx];
			const RasOp op = static_cast<RasOp>(instr.op);

			// Retarget JUMP and JUMP_NOT_EQUAL: operand a is a word offset,
			// convert to decoded instruction index.
			if (op == RasOp::Jump || op == RasOp::JumpNotEqual) {
				const int targetWord = instr.a;
				if (targetWord >= 0 && targetWord < static_cast<int>(code.size())) {
					instr.a = wordToInstr[targetWord];
					// If target wasn't an instruction start, it's a jump into
					// the middle of an operand sequence � this shouldn't happen
					// in valid scripts, but clamp to the next instruction if so.
					if (instr.a < 0) {
						instr.a = instrIdx + 1;
					}
				}
			}

			// RealCall: I.a stays as funcId; Tick() does decodedOffsets[I.a].
			// This keeps funcId available for CallInfo.functionId.

			const int operands = RasOpOperandWords(op);
			w += 1 + operands;
			instrIdx++;
		}
	}
}

// ============================================================================
// Part III.3: RASC fast-load loader
// ============================================================================

/// RASC binary header (all fields little-endian int32).
struct RascHeader {
	uint32_t magic;            // 0x43534152 "RASC"
	uint32_t version;          // 1
	uint32_t numScripts;
	uint32_t numPieces;
	uint32_t numStaticVars;
	uint32_t numSounds;
	uint32_t numInstructions;  // total decoded instruction count
	uint32_t offsetScriptNames;
	uint32_t offsetPieceNames;
	uint32_t offsetScriptInfo;
	uint32_t offsetDecoded;
	uint32_t offsetSoundNames; // 0 if none
};
static_assert(sizeof(RascHeader) == 48, "RascHeader size mismatch");

/// Per-script metadata in the ScriptInfo block (16 bytes each).
struct __attribute__((packed)) RascScriptInfo {
	int32_t  decodedOffset;
	int32_t  maxStackDepth;
	uint8_t  threadSafe;
	uint8_t  isLuaScript;
	uint8_t  padding[6];
};
static_assert(sizeof(RascScriptInfo) == 16, "RascScriptInfo size mismatch");

/// Read a little-endian uint32 from an arbitrary offset.
inline uint32_t readLe32(const uint8_t* data, size_t off)
{
	return static_cast<uint32_t>(data[off])
	     | (static_cast<uint32_t>(data[off + 1]) << 8)
	     | (static_cast<uint32_t>(data[off + 2]) << 16)
	     | (static_cast<uint32_t>(data[off + 3]) << 24);
}

/// Read a little-endian int32 from an arbitrary offset.
inline int32_t readLe32s(const uint8_t* data, size_t off)
{
	return static_cast<int32_t>(readLe32(data, off));
}

bool CRasFile::loadRascFormat(const uint8_t* data, size_t dataSize)
{
	constexpr size_t HDR_SIZE = sizeof(RascHeader);
	if (dataSize < HDR_SIZE) {
		LOG_L(L_ERROR, "[%s] RASC file too small (%d bytes)", __func__, (int)dataSize);
		return false;
	}

	// Parse header by reading LE fields individually.
	const uint32_t magic       = readLe32(data,  0);
	const uint32_t version     = readLe32(data,  4);
	const uint32_t numScripts  = readLe32(data,  8);
	const uint32_t numPieces   = readLe32(data, 12);
	const uint32_t numStaticVars = readLe32(data, 16);
	const uint32_t numSounds   = readLe32(data, 20);
	const uint32_t numInstructions = readLe32(data, 24);
	const uint32_t offScriptNames  = readLe32(data, 28);
	const uint32_t offPieceNames   = readLe32(data, 32);
	const uint32_t offScriptInfo   = readLe32(data, 36);
	const uint32_t offDecoded      = readLe32(data, 40);
	const uint32_t offSoundNames   = readLe32(data, 44);

	if (magic != 0x43534152) {
		return false;
	}
	if (version != 1) {
		LOG_L(L_ERROR, "[%s] unsupported RASC version %u", __func__, version);
		return false;
	}
	if (numScripts == 0) {
		LOG_L(L_WARNING, "[%s] RASC script \"%s\" is empty", __func__, name.c_str());
		return false;
	}

	scriptFormat = RasFmtRASC;
	this->numStaticVars = static_cast<int>(numStaticVars);

	// Reserve vectors
	scriptNames.reserve(numScripts);
	luaScripts.reserve(numScripts);
	scriptOffsets.reserve(numScripts);
	scriptLengths.reserve(numScripts);
	pieceNames.reserve(numPieces);

	// ---- Load script names (null-terminated strings) ----
	{
		size_t pos = offScriptNames;
		for (uint32_t i = 0; i < numScripts; ++i) {
			if (pos >= dataSize) {
				LOG_L(L_ERROR, "[%s] script names overflow at index %u", __func__, i);
				return false;
			}
			const char* s = reinterpret_cast<const char*>(data + pos);
			scriptNames.emplace_back(s);
			pos += strlen(s) + 1;  // +1 for null terminator

			// Detect Lua scripts by name prefix
			if (scriptNames.back().find("lua_synced_") == 0) {
				luaScripts.emplace_back(scriptNames.back().c_str() + sizeof("lua_synced_") - 1);
			} else if (scriptNames.back().find("lua_unsynced_") == 0) {
				luaScripts.emplace_back(scriptNames.back().c_str() + sizeof("lua_unsynced_") - 1);
			} else {
				luaScripts.emplace_back("");
			}
		}
	}

	// ---- Load piece names (null-terminated strings) ----
	{
		size_t pos = offPieceNames;
		for (uint32_t i = 0; i < numPieces; ++i) {
			if (pos >= dataSize) {
				LOG_L(L_ERROR, "[%s] piece names overflow at index %u", __func__, i);
				return false;
			}
			const char* s = reinterpret_cast<const char*>(data + pos);
			pieceNames.emplace_back(StringToLower(s));
			pos += strlen(s) + 1;
		}
	}

	// ---- Load script info block ----
	decodedOffsets.resize(numScripts, -1);
	decodedLengths.resize(numScripts, 0);
	maxStackDepth.resize(numScripts, 0);
	threadSafeFuncs.resize(numScripts, 0);

	{
		const size_t infoSize = numScripts * sizeof(RascScriptInfo);
		if (offScriptInfo + infoSize > dataSize) {
			LOG_L(L_ERROR, "[%s] script info overflow", __func__);
			return false;
		}
		const uint8_t* infoPtr = data + offScriptInfo;
		for (uint32_t i = 0; i < numScripts; ++i) {
			const size_t off = i * sizeof(RascScriptInfo);
			decodedOffsets[i] = readLe32s(infoPtr, off + 0);
			maxStackDepth[i]  = readLe32s(infoPtr, off + 4);
			threadSafeFuncs[i] = infoPtr[off + 8];
			// isLuaScript at off+9 is redundant with luaScripts vector
		}
	}

	// ---- Compute scriptOffsets and scriptLengths from decoded stream ----
	// In RASC format, each script's instructions are contiguous in the decoded
	// stream starting at decodedOffsets[i].  We derive scriptOffsets as the
	// byte offset within the decoded section, and scriptLengths as the number
	// of instructions.
	{
		scriptOffsets.resize(numScripts, 0);
		scriptLengths.resize(numScripts, 0);

		// Compute lengths: for each non-Lua script, length = next script's offset - this offset.
		// Gather valid (decodedOffset, index) pairs and sort.
		std::vector<std::pair<int, uint32_t>> validScripts;
		validScripts.reserve(numScripts);
		for (uint32_t i = 0; i < numScripts; ++i) {
			if (decodedOffsets[i] >= 0)
				validScripts.emplace_back(decodedOffsets[i], i);
		}
		std::sort(validScripts.begin(), validScripts.end());

		for (size_t k = 0; k < validScripts.size(); ++k) {
			const uint32_t si = validScripts[k].second;
			scriptOffsets[si] = validScripts[k].first;
			if (k + 1 < validScripts.size()) {
				scriptLengths[si] = validScripts[k + 1].first - validScripts[k].first;
			} else {
				// Last script: length = total instructions - start offset
				scriptLengths[si] = static_cast<int>(numInstructions) - validScripts[k].first;
			}
		}

		// decodedLengths mirrors scriptLengths for RASC (both are instruction counts)
		for (uint32_t i = 0; i < numScripts; ++i) {
			decodedLengths[i] = scriptLengths[i];
		}
	}

	// ---- Load decoded instruction stream ----
	{
		const size_t numInstr = numInstructions;
		const size_t decSize = numInstr * RASC_DISK_INSTR_SIZE;
		if (offDecoded + decSize > dataSize) {
			LOG_L(L_ERROR, "[%s] decoded stream overflow", __func__);
			return false;
		}

		decoded.resize(numInstr);

		// On-disk RasInstr: op(1) + flags(1) + a(4 LE) + b(4 LE) = 10 bytes packed.
		// In-memory RasInstr is natural 8 bytes; parse the disk stream field-by-
		// field so the two layouts stay decoupled (endian-safe on all hosts).
		const uint8_t* src = data + offDecoded;
		for (size_t i = 0; i < numInstr; ++i) {
			const size_t off = i * RASC_DISK_INSTR_SIZE;
			decoded[i].op    = src[off + 0];
			decoded[i].flags = src[off + 1];
			decoded[i].a     = readLe32s(src, off + 2);
			decoded[i].b     = readLe32s(src, off + 6);
		}
	}

	// ---- Load sound names (TA:K only) ----
	if (offSoundNames != 0 && numSounds > 0) {
		sounds.reserve(numSounds);
		size_t pos = offSoundNames;
		for (uint32_t i = 0; i < numSounds; ++i) {
			if (pos >= dataSize) break;
			const std::string s = reinterpret_cast<const char*>(data + pos);
			pos += s.size() + 1;

			if (sound->HasSoundItem(s)) {
				sounds.push_back(sound->GetSoundId(s));
			} else {
				sounds.push_back(sound->GetSoundId("sounds/" + s + ".wav"));
			}
		}
	}

	// ---- Build scriptMap (name -> index) ----
	for (size_t i = 0, n = scriptNames.size(); i < n; ++i) {
		scriptMap[scriptNames[i]] = static_cast<int>(i);
	}

	// ---- Map common function names to indices ----
	scriptIndex.fill(-1);
	for (const auto& pair: CCobUnitScriptNames::GetScriptMap()) {
		const int fn = GetFunctionId(pair.first);
		if (fn < 0)
			continue;
		scriptIndex[pair.second] = fn;
	}

	// ---- Part V: Analyze thread-safety of each function ----
	analyzeThreadSafety();

	return true;
}


// ============================================================================
// Part V: Thread-safety analysis pass
// ============================================================================

/// Analyzes the decoded instruction stream to determine which functions
/// are thread-safe (can run on the MT path without the global lock).
/// A function is thread-safe if all its opcodes are thread-safe and all
/// RealCall targets are also thread-safe.  Uses iterative fixed-point.
void CRasFile::analyzeThreadSafety()
{
	const int numFuncs = static_cast<int>(threadSafeFuncs.size());
	if (numFuncs == 0)
		return;

	// Initialize: assume all functions are unsafe
	std::fill(threadSafeFuncs.begin(), threadSafeFuncs.end(), uint8_t(0));

	bool changed = true;
	while (changed) {
		changed = false;
		for (int fi = 0; fi < numFuncs; ++fi) {
			// Skip Lua scripts (decodedOffsets < 1)
			if (decodedOffsets[fi] < 0)
				continue;
			// Already marked safe
			if (threadSafeFuncs[fi])
				continue;

			const int instrStart = decodedOffsets[fi];
			const int instrLen   = decodedLengths[fi];
			const int instrEnd   = instrStart + instrLen;

			bool allSafe = true;
			for (int ii = instrStart; ii < instrEnd; ++ii) {
				const RasInstr& I = decoded[ii];
				const RasOp op = static_cast<RasOp>(I.op);

				// Check if the opcode itself is thread-safe
				if (!RasOpIsThreadSafe(op)) {
					allSafe = false;
					break;
				}

				// Check Call targets: the called function must also be safe
				if (op == RasOp::Call) {
					const int targetFunc = I.a;
					if (targetFunc >= 0 && targetFunc < numFuncs) {
						if (!threadSafeFuncs[targetFunc]) {
							allSafe = false;
							break;
						}
					} else {
						// Invalid call target - conservative: mark unsafe
						allSafe = false;
						break;
					}
				}
			}

			if (allSafe) {
				threadSafeFuncs[fi] = 1;
				changed = true;  // may unlock functions that call this one
			}
		}
	}
}


// Helper: compute net stack depth change for an opcode.
// Positive = net push, negative = net pop, zero = balanced.
// This models the data stack (not call stack).
static int stackDelta(RasOp op)
{
	switch (op) {
		// Push 1 onto data stack
		case RasOp::PushConstant:
		case RasOp::PushLocalVar:
		case RasOp::PushStatic:
			return +1;
		// PushStaticIdx: pops 1 (index), pushes 1 (value) -> delta 0
		case RasOp::PushStaticIdx:
			return 0;
		// PopStaticIdx: pops 2 (value + index), pushes nothing -> delta -2
		case RasOp::PopStaticIdx:
			return -2;
		// Pop 1 from data stack
		case RasOp::PopLocalVar:
		case RasOp::PopStatic:
		case RasOp::PopStack:
		case RasOp::Jump:
		case RasOp::JumpNotEqual:
		case RasOp::Hide:
		case RasOp::Shade:
		case RasOp::DontShade:
		case RasOp::Cache:
		case RasOp::DontCache:
		case RasOp::Explode:
		case RasOp::PlaySound:
		case RasOp::EmitSfx:
		case RasOp::BitwiseNot:
		case RasOp::LogicalNot:
		case RasOp::SetSignalMask:
		case RasOp::Signal:
		case RasOp::Drop:
			return -1;
		// Pop 2, push 1 (arithmetic, comparison, logic)
		case RasOp::Add:
		case RasOp::Sub:
		case RasOp::Mul:
		case RasOp::Div:
		case RasOp::Mod:
		case RasOp::BitwiseAnd:
		case RasOp::BitwiseOr:
		case RasOp::BitwiseXor:
		case RasOp::SetLess:
		case RasOp::SetLessOrEqual:
		case RasOp::SetGreater:
		case RasOp::SetGreaterOrEq:
		case RasOp::SetEqual:
		case RasOp::SetNotEqual:
		case RasOp::LogicalAnd:
		case RasOp::LogicalOr:
		case RasOp::LogicalXor:
		case RasOp::Minimum:
		case RasOp::Maximum:
			return -1;
		// Pop 2, push 1 (RAND: r1, r2 -> result)
		case RasOp::Rand:
			return -1;
		// Pop 3, push 1 (CLAMP: val, min, max -> result)
		case RasOp::Clamp:
			return -2;
		// Pop 5, push 1 (GET)
		case RasOp::Get:
			return -4;
		// Pop 1, push 1 (no net change)
		case RasOp::GetUnitValue:
		case RasOp::Absolute:
		case RasOp::Sign:
		case RasOp::AddImm:
		case RasOp::MulImm:
			return 0;
		// Pop 2 (START: funcId, argCount)
		case RasOp::Start:
			return -2;
		// Pop 3 (ATTACH: unitId, pieceId, ?)
		case RasOp::Attach:
			return -3;
		// SET pops value-id and value
		case RasOp::Set:
			return -2;
		// Movement ops: pop operands
		case RasOp::Move:
		case RasOp::Turn:
			return -2;
		case RasOp::MoveNow:
		case RasOp::TurnNow:
		case RasOp::Scale:
		case RasOp::ScaleNow:
			return -1;
		case RasOp::ExplodeRel:
			return -1;
		case RasOp::TurnRel:
		case RasOp::MoveRel:
		case RasOp::ScaleRel:
			return -2;
		case RasOp::Spin:
		case RasOp::StopSpin:
			return -2;
		case RasOp::Show:
			return -1;
		// No stack effect
		case RasOp::Sleep:
		case RasOp::WaitTurn:
		case RasOp::WaitMove:
		case RasOp::WaitScale:
		case RasOp::CreateLocalVar:
		case RasOp::Return:
		case RasOp::LuaCall:
		case RasOp::BatchLua:
		case RasOp::LuaUnsynced:
		case RasOp::Call:
		case RasOp::SignatureLua:
		case RasOp::BadOpcode:
		default:
			return 0;
	}
}
