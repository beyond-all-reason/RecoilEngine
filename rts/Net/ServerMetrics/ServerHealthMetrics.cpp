/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "ServerHealthMetrics.h"

#include <algorithm>
#include <map>
#include <string>

#include <prometheus/gauge.h>
#include <prometheus/registry.h>

#include "Game/GameVersion.h"
#include "Net/GameParticipant.h"
#include "Net/GameServer.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Metrics/Helpers.h"
#include "System/Metrics/Metrics.h"

using metrics::msToSecs;


void ServerHealthMetrics::Init(prometheus::Registry& registry, const std::string& gameIDHex)
{
	const auto gaugeFamily = [&](const char* name, const char* help) {
		return &prometheus::BuildGauge().Name(name).Help(help).Register(registry);
	};
	const auto gauge = [&](const char* name, const char* help) { return &gaugeFamily(name, help)->Add({}); };

	metricServerFrame = gauge("recoil_server_frame",
		"Current server simulation frame");
	metricMaxLag = gauge("recoil_server_max_lag_seconds",
		"Worst player lag in seconds: how far behind the server the most-behind player's last acked sim frame is. Derived from the frame backlog priced at the current speed, so a speed change rescales it even when the connection is unchanged. recoil_server_max_lag_frames is the speed-invariant alternative");
	metricMaxLagFrames = gauge("recoil_server_max_lag_frames",
		"Worst player lag in sim frames");
	metricMaxCpu = gauge("recoil_server_max_cpu_usage",
		"Highest client-reported cpu usage in [0,1] across players");
	metricMedianLag = gauge("recoil_server_median_lag_seconds",
		"Median player lag in seconds. Only computed when SpeedControl=1. Note: a speed change rescales this metric even if the connection is unchanged");
	metricMedianCpu = gauge("recoil_server_median_cpu_usage",
		"Median client cpu usage used as input to lag protection. Only computed when SpeedControl=1");

	auto& participants = *gaugeFamily("recoil_server_participants",
		"Connected participants by type. Includes local loopback clients.");
	metricPlayers = &participants.Add({{"type", "player"}});
	metricSpectators = &participants.Add({{"type", "spectator"}});

	metricInternalSpeed = gauge("recoil_server_speed_factor",
		"Speed the simulation is actually running at (1.0 = normal), lowered by lag protection");
	metricWantedSpeed = gauge("recoil_server_wanted_speed_factor",
		"Speed requested by the users (1.0 = normal)");
	metricPaused = gauge("recoil_server_paused",
		"1 while the game is paused");

	metricGameStartTs = gauge("recoil_server_game_start_timestamp_seconds",
		"Unix time the game started");

	// The game id is a label here on an info metric rather than a constant label
	// on every family. This is a prometheus convention, because it isolates churn
	// to a single metric series.
	gaugeFamily("recoil_server_info", "Constant 1, labels carry engine build and game identity")
		->Add({{"engine_version", SpringVersion::GetFull()}, {"gameid", gameIDHex}}).Set(1);

	if (!metrics::PerPlayerEnabled())
		return;

	metricPlayerLag = gaugeFamily("recoil_server_per_player_lag_seconds",
		"How far behind the server the player's last acked sim frame is, in seconds. Derived from the frame backlog priced at the current speed, so a speed change rescales it even when the connection is unchanged. recoil_server_per_player_lag_frames is the speed-invariant alternative");
	metricPlayerLagFrames = gaugeFamily("recoil_server_per_player_lag_frames",
		"How far the player's last acked sim frame is behind the server, in sim frames");
	metricPlayerCpu = gaugeFamily("recoil_server_per_player_cpu_usage",
		"Client-reported cpu usage in [0,1]");
	// The name is here rather than on every per-player series for the same
	// reason the game id is: it keeps the series themselves stable/reduces
	// churn.
	metricPlayerInfo = gaugeFamily("recoil_server_per_player_info",
		"Constant 1, labels map a player slot to their name");
}


void ServerHealthMetrics::SetGameStartTime(double unixSecs)
{
	if (metricGameStartTs != nullptr)
		metricGameStartTs->Set(unixSecs);
}


ServerHealthMetrics::PlayerMetrics& ServerHealthMetrics::GetPlayerSlot(int playerId)
{
	if (playerMetrics.size() <= static_cast<size_t>(playerId))
		playerMetrics.resize(playerId + 1);

	return playerMetrics[playerId];
}


void ServerHealthMetrics::Update(const CGameServer& server)
{
	int numPlayers = 0;
	int numSpectators = 0;
	float maxLag = 0.0f;
	int maxLagFrames = 0;
	float maxCpu = 0.0f;

	for (const GameParticipant& p: server.players) {
		if (p.clientLink == nullptr)
			continue;

		if (p.spectator)
			numSpectators++;
		else
			numPlayers++;

		PlayerMetrics& pm = GetPlayerSlot(p.id);

		// player info metric is kept for the entire game once published, unlike
		// the other gauges which disappear on player disconnects.
		if (metrics::PerPlayerEnabled() && pm.info == nullptr) {
			pm.info = &metricPlayerInfo->Add({{"playerid", std::to_string(p.id)}, {"player", p.name}});
			pm.info->Set(1);
		}

		if (server.gameHasStarted && p.myState == GameParticipant::INGAME) {
			const int lagFrames = std::max(0, server.serverFrameNum - p.lastFrameResponse);

			// clamped to avoid a divide by zero
			const float simSpeed = std::max(0.1f, server.internalSpeed);
			const float lagMs = (lagFrames * 1000.0f) / (GAME_SPEED * simSpeed);
			const float cpu = std::clamp(p.cpuUsage, 0.0f, 1.0f);

			maxLag = std::max(maxLag, lagMs);
			maxLagFrames = std::max(maxLagFrames, lagFrames);
			maxCpu = std::max(maxCpu, cpu);

			if (metrics::PerPlayerEnabled()) {
				if (pm.lagSeconds == nullptr) {
					const std::map<std::string, std::string> labels = {{"playerid", std::to_string(p.id)}};

					pm.lagSeconds = &metricPlayerLag->Add(labels);
					pm.lagFrames = &metricPlayerLagFrames->Add(labels);
					pm.cpuUsage = &metricPlayerCpu->Add(labels);
				}

				pm.lagSeconds->Set(lagMs * msToSecs);
				pm.lagFrames->Set(lagFrames);
				pm.cpuUsage->Set(cpu);
			}
		} else if (pm.lagSeconds != nullptr) {
			// dropped so we don't have a frozen gauge scrapes like a live one
			metricPlayerLag->Remove(pm.lagSeconds);
			metricPlayerLagFrames->Remove(pm.lagFrames);
			metricPlayerCpu->Remove(pm.cpuUsage);

			pm.lagSeconds = nullptr;
			pm.lagFrames = nullptr;
			pm.cpuUsage = nullptr;
		}
	}

	metricMaxLag->Set(maxLag * msToSecs);
	metricMaxLagFrames->Set(maxLagFrames);
	metricMaxCpu->Set(maxCpu);
	metricServerFrame->Set(server.serverFrameNum);
	// medianPing is LagProtection's per-player lag, in milliseconds
	metricMedianLag->Set(server.medianPing * msToSecs);
	metricMedianCpu->Set(server.medianCpu);
	metricPlayers->Set(numPlayers);
	metricSpectators->Set(numSpectators);
	metricInternalSpeed->Set(server.internalSpeed);
	metricWantedSpeed->Set(server.userSpeedFactor);
	metricPaused->Set(server.isPaused ? 1 : 0);
}
