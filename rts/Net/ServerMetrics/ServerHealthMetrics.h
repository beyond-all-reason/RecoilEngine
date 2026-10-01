/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <string>
#include <vector>

namespace prometheus
{
	template<typename T> class Family;
	class Gauge;
	class Registry;
}

class CGameServer;

/**
 * @brief the recoil_server_ series: game info, frame progress, speed, participants
 *
 * Owned by ServerMetrics. Participant-level rather than conn-level,
 * because a local host lags like any other player even though the
 * loopback connection has no network to describe.
 */
class ServerHealthMetrics
{
public:
	void Init(prometheus::Registry& registry, const std::string& gameIDHex);
	void Update(const CGameServer& server);

	void SetGameStartTime(double unixSecs);

private:
	struct PlayerMetrics {
		prometheus::Gauge* lagSeconds = nullptr;
		prometheus::Gauge* lagFrames = nullptr;
		prometheus::Gauge* cpuUsage = nullptr;
	};

	/// get this player's metric slot, created on first use
	PlayerMetrics& GetPlayerSlot(int playerId);

	std::vector<PlayerMetrics> playerMetrics;

	prometheus::Gauge* metricServerFrame = nullptr;
	prometheus::Gauge* metricMaxLag = nullptr;
	prometheus::Gauge* metricMaxLagFrames = nullptr;
	prometheus::Gauge* metricMaxCpu = nullptr;
	prometheus::Gauge* metricMedianLag = nullptr;
	prometheus::Gauge* metricMedianCpu = nullptr;
	prometheus::Gauge* metricPlayers = nullptr;
	prometheus::Gauge* metricSpectators = nullptr;
	prometheus::Gauge* metricInternalSpeed = nullptr;
	prometheus::Gauge* metricWantedSpeed = nullptr;
	prometheus::Gauge* metricPaused = nullptr;
	prometheus::Gauge* metricGameStartTs = nullptr;

	// per-player families. always null unless MetricsPerPlayer is on
	prometheus::Family<prometheus::Gauge>* metricPlayerLag = nullptr;
	prometheus::Family<prometheus::Gauge>* metricPlayerLagFrames = nullptr;
	prometheus::Family<prometheus::Gauge>* metricPlayerCpu = nullptr;
};
