/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RAS_FILE_H
#define RAS_FILE_H

#include <array>
#include <vector>
#include <string>

#include "Lua/LuaHashString.h"
#include "CobScriptNames.h"
#include "RasOpCodes.h"
#include "System/UnorderedMap.hpp"

class CFileHandler;

/// Script format identifier
enum RasScriptFormat : int {
	RasFmtCOB  = 0,  /// Legacy COB bytecode (decoded at load)
	RasFmtRASC = 1,  /// Native RASC format (pre-decoded)
};

class CRasFile
{
public:
	CRasFile(CFileHandler& in, const std::string& scriptName);
	CRasFile(CRasFile&& f) { *this = std::move(f); }

	CRasFile& operator = (CRasFile&& f) {
		numStaticVars = f.numStaticVars;
		scriptFormat  = f.scriptFormat;

		code = std::move(f.code);
		decoded = std::move(f.decoded);
		decodedOffsets = std::move(f.decodedOffsets);
		decodedLengths = std::move(f.decodedLengths);
		maxStackDepth  = std::move(f.maxStackDepth);
		threadSafeFuncs = std::move(f.threadSafeFuncs);

		scriptNames = std::move(f.scriptNames);
		scriptOffsets = std::move(f.scriptOffsets);

		scriptLengths = std::move(f.scriptLengths);
		pieceNames = std::move(f.pieceNames);
		scriptIndex = std::move(f.scriptIndex);
		sounds = std::move(f.sounds);
		luaScripts = std::move(f.luaScripts);
		scriptMap = std::move(f.scriptMap);

		name = std::move(f.name);
		return *this;
	}

	int GetFunctionId(const std::string& name);

private:
	/// Part I.2+I.3: operand-aware decode walk + CALL/jump retargeting
	void decodeBytecode();

	/// Part III.3: Fast-path loader for pre-decoded RASC format
	bool loadRascFormat(const uint8_t* data, size_t dataSize);

	/// Part V: Determine which functions are thread-safe
	void analyzeThreadSafety();

public:
	int numStaticVars = 0;
	/// Script format: RasFmtCOB or RasFmtRASC
	int scriptFormat = RasFmtCOB;

	/// Raw 32-bit bytecode (decode input, kept for now)
	std::vector<int> code;
	/// Pre-decoded, read-only instruction stream (Part I)
	std::vector<RasInstr> decoded;
	/// Per-function entry point: script index -> first decoded instruction index
	std::vector<int> decodedOffsets;
	/// Per-function decoded instruction count (computed during decode)
	std::vector<int> decodedLengths;
	/// Per-function maximum stack depth (computed during decode)
	std::vector<int> maxStackDepth;
	/// Per-function thread-safety flag: 1 = safe for parallel execution
	std::vector<uint8_t> threadSafeFuncs;

	std::vector<std::string> scriptNames;
	std::vector<int> scriptOffsets;
	/// Assumes that the scripts are sorted by offset in the file
	std::vector<int> scriptLengths;
	std::vector<std::string> pieceNames;
	std::array<int, COBFN_NumUnitFuncs> scriptIndex;
	std::vector<int> sounds;
	std::vector<LuaHashString> luaScripts;
	spring::unordered_map<std::string, int> scriptMap;

	std::string name;
};

#endif // RAS_FILE_H

