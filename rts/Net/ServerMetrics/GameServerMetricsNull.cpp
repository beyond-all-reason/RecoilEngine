/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

/*
 * ServerMetrics for a build without the endpoint, i.e. ENABLE_METRICS=OFF
 *
 * NOTE: Make sure you define every method the real implementation does, as
 * a missing one breaks this build at link time.
 */

#include <string>

#include "GameServerMetrics.h"

void ServerMetrics::Init(const std::string&) {}

void ServerMetrics::Shutdown() {}

void ServerMetrics::Update(const CGameServer&) {}

void ServerMetrics::SetGameStartTime(double) {}
