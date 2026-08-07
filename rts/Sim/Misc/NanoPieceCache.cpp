/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "NanoPieceCache.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/Scripts/UnitScript.h"

#include "System/Misc/TracyDefs.h"

CR_BIND(NanoPieceCache, )

CR_REG_METADATA(NanoPieceCache, (
	CR_MEMBER(nanoPieces),
	CR_MEMBER(lastNanoPieceCnt),
	CR_MEMBER(curBuildPowerMask),
	CR_IGNORED(nanoPieceCursor)
))

void NanoPieceCache::UpdateNanoPieces(CUnitScript* ownerScript) {
	RECOIL_DETAILED_TRACY_ZONE;
	curBuildPowerMask |= (1 << (MASK_BITS - 1));

	if (!nanoPieces.empty()) {
		const unsigned cnt = nanoPieces.size();
		// Keep the existing synced RNG advance: nano-piece selection used to
		// consume one value per successful work tick before the rate gate.
		static_cast<void>(gsRNG.NextInt(cnt));
	}

	if (lastNanoPieceCnt <= MAX_QUERYNANOPIECE_CALLS) {
		// only do so 30 times and then use the cache
		const int scriptPiece = ownerScript->QueryNanoPiece();
		const int modelPiece  = ownerScript->ScriptToModel(scriptPiece);

		if (auto* p = ownerScript->SafeGetPiece(scriptPiece); p) {
			if (std::find(nanoPieces.begin(), nanoPieces.end(), modelPiece) != nanoPieces.end()) {
				// already in cache
				lastNanoPieceCnt++;
			} else {
				nanoPieces.push_back(modelPiece);
				lastNanoPieceCnt = 0;
			}
		} else {
			lastNanoPieceCnt++;
		}
	}
}

int NanoPieceCache::GetNextNanoPiece() {
	if (nanoPieces.empty())
		return -1;

	const unsigned cnt = nanoPieces.size();
	const int nanoPiece = nanoPieces[nanoPieceCursor % cnt];
	nanoPieceCursor = (nanoPieceCursor + 1) % cnt;
	return nanoPiece;
}

int NanoPieceCache::GetNanoPiece(CUnitScript* ownerScript) {
	UpdateNanoPieces(ownerScript);
	return GetNextNanoPiece();
}

