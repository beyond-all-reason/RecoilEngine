/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "GameServerMetrics.h"

#include "Net/GameServer.h"
#include "System/Metrics/Metrics.h"
#include "System/Net/ConnectionStats.h"

// how often metric values are republished, well below any sane scrape interval
static const spring_time metricsPublishInterval = spring_secs(1);


void ServerMetrics::Init(const std::string& gameIDHex)
{
	metrics::Init();

	netcode::SetStatsSampling(metrics::Enabled());

	if (!metrics::Enabled())
		return;

	auto& registry = metrics::GetRegistry();

	healthMetrics.Init(registry, gameIDHex);
	registered = true;
}


void ServerMetrics::Shutdown()
{
	netcode::SetStatsSampling(false);

	metrics::Shutdown();

	// the registry is gone, so every metric pointer cached in this object now
	// dangles. Assigning a default-constructed instance clears all of them at
	// once.
	*this = ServerMetrics{};
}


void ServerMetrics::Update(const CGameServer& server)
{
	if (!registered)
		return;

	if (lastPublishTime > (server.lastUpdate - metricsPublishInterval))
		return;

	// code to publish metrics goes here
	lastPublishTime = server.lastUpdate;

	healthMetrics.Update(server);
}

void ServerMetrics::SetGameStartTime(double unixSecs) {
	healthMetrics.SetGameStartTime(unixSecs);
}
