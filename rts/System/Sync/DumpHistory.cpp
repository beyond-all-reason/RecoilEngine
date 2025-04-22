/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <fstream>

#include "DumpHistory.h"
#include "SyncChecker.h"

#include "Game/Game.h"
#include "Game/GlobalUnsynced.h"
#include "Net/GameServer.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"


void DumpHistory(std::fstream& file, int frameNum, bool serverRequest)
{
#ifdef SYNC_HISTORY
	if (!gs->cheatEnabled && !serverRequest)
		return;

	if (frameNum < gs->frameNum - MAX_SYNC_HISTORY_FRAMES) {
		LOG("[%s] request for history beyond history limit (%d)", __func__, frameNum);
		return;
	}
	const auto [startIndex, endIndex, data] = CSyncChecker::GetFrameHistory(gs->frameNum - frameNum);

	unsigned firstRangeSize;
	unsigned secondRangeSize = 0;
	if (endIndex > startIndex) {
		firstRangeSize = endIndex - startIndex;
	} else {
		firstRangeSize = MAX_SYNC_HISTORY - startIndex;
		secondRangeSize = endIndex + 1;
	}

	LOG("[%s] dump history at %d (%d records)", __func__, frameNum, firstRangeSize + secondRangeSize);

	file << std::format("internal frame checksums ({}):\n", frameNum);

	for (int i = startIndex; i < startIndex + firstRangeSize; ++i) {
		file << std::format("\t{:08x}\n", data[i]);
	}
	for (int i = 0; i < secondRangeSize; ++i) {
		file << std::format("\t{:08x}\n", data[i]);
	}
#endif // SYNC_HISTORY
}

void DumpHistoryBinary(int dumpId, int frameNum, bool serverRequest)
{
	
#ifdef SYNC_HISTORY
	if (!gs->cheatEnabled && !serverRequest)
		return;

	if (frameNum < gs->frameNum - MAX_SYNC_HISTORY_FRAMES) {
		LOG("[%s] request for history beyond history limit (%d)", __func__, gs->frameNum);
		return;
	}
	const auto [startIndex, endIndex, data] = CSyncChecker::GetFrameHistory(gs->frameNum - frameNum);

	unsigned firstRangeSize;
	unsigned secondRangeSize = 0;
	if (endIndex > startIndex) {
		firstRangeSize = endIndex - startIndex;
	} else {
		firstRangeSize = MAX_SYNC_HISTORY - startIndex;
		secondRangeSize = endIndex + 1;
	}


	std::fstream file;

	LOG("[%s] dumping history (for %d)", __func__, gs->frameNum);

	std::string name = (gameServer != nullptr)? "Server": "Client";
	name += "GameHistory-";
	name += IntToString(dumpId);
	name += "-[";
	name += IntToString(gs->frameNum);
	name += "].bin";

	file.open(name.c_str(), std::ios::out|std::ios::binary);

	if (file.is_open()) {
		unsigned version = 0;
		LOG("[%s] starting dump-file \"%s\"", __func__, name.c_str());

		file.write((char *)&version, sizeof(unsigned));
		file.write((char *)game->gameID, sizeof(unsigned char)*16);
		// gu->myPlayerNum

		file.write((char *)&data[startIndex], sizeof(unsigned)*firstRangeSize);
		if (secondRangeSize > 0)
			file.write((char *)data, sizeof(unsigned)*secondRangeSize);

		LOG("[%s] finished dump-file \"%s\"", __func__, name.c_str());
	}

	if (file.bad() || !file.is_open())
		return;

	file.close();
#endif // SYNC_HISTORY
}

