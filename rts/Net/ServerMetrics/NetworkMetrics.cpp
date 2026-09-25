/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "NetworkMetrics.h"

#include <map>
#include <string>
#include <variant>

#include <prometheus/counter.h>
#include <prometheus/registry.h>

#include "Net/GameParticipant.h"
#include "Net/GameServer.h"
#include "System/Metrics/Helpers.h"
#include "System/Metrics/Metrics.h"
#include "System/Net/ConnectionStats.h"

using metrics::DeltaSince;


void NetworkMetrics::Init(prometheus::Registry& registry)
{
	const auto counterFamily = [&](const char* name, const char* help) {
		return &prometheus::BuildCounter().Name(name).Help(help).Register(registry);
	};

	// direction is a label rather than part of the name, to make querying
	// more intuitive
	auto& bytes = *counterFamily("recoil_network_bytes_total",
		"Bytes over all client connections, by direction");
	metricTotalSentBytes = &bytes.Add({{"direction", "outgoing"}});
	metricTotalRecvBytes = &bytes.Add({{"direction", "incoming"}});

	auto& packets = *counterFamily("recoil_network_packets_total",
		"UDP packets over all client connections, by direction");
	metricTotalSentPackets = &packets.Add({{"direction", "outgoing"}});
	metricTotalRecvPackets = &packets.Add({{"direction", "incoming"}});

	perPlayerEnabled = metrics::PerPlayerEnabled();
	if (!perPlayerEnabled)
		return;

	metricBytes = counterFamily("recoil_network_per_connection_bytes_total",
		"Bytes over this client connection, by direction. No series exists for local loopback connections");
	metricPackets = counterFamily("recoil_network_per_connection_packets_total",
		"UDP packets over this client connection, by direction");
}


NetworkMetrics::ConnectionMetrics& NetworkMetrics::ConnectionSlot(int playerId)
{
	if (connectionMetrics.size() <= static_cast<size_t>(playerId))
		connectionMetrics.resize(playerId + 1);

	return connectionMetrics[playerId];
}


void NetworkMetrics::ResetConnectionDeltas(int playerId)
{
	ConnectionSlot(playerId) = ConnectionMetrics{};
}


void NetworkMetrics::Update(const CGameServer& server)
{
	const auto publishDelta = [this](
		prometheus::Counter* total, ConnectionMetrics::DeltaCounter& dc, double cur, double scale = 1.0
	) {
		const double delta = DeltaSince(cur, dc.last) * scale;

		total->Increment(delta);

		if (perPlayerEnabled)
			dc.player->Increment(delta);
	};

	for (const GameParticipant& p: server.players) {
		ConnectionMetrics& cm = ConnectionSlot(p.id);

		if (p.clientLink == nullptr) {
			cm = ConnectionMetrics{};
			continue;
		}

		const netcode::ConnectionStats linkStats = p.clientLink->GetStats();
		const auto* udp = std::get_if<netcode::UdpStats>(&linkStats);

		// skip for loopback connections
		if (udp == nullptr)
			continue;

		const netcode::UdpStats& stats = *udp;

		// Label by player slot id. See recoil_server_per_player_info to resolve to names.
		if (perPlayerEnabled && cm.sentBytes.player == nullptr) {
			const std::string playerId = std::to_string(p.id);
			cm.sentBytes.player   = &metricBytes->Add({{"playerid", playerId}, {"direction", "outgoing"}});
			cm.recvBytes.player   = &metricBytes->Add({{"playerid", playerId}, {"direction", "incoming"}});
			cm.sentPackets.player = &metricPackets->Add({{"playerid", playerId}, {"direction", "outgoing"}});
			cm.recvPackets.player = &metricPackets->Add({{"playerid", playerId}, {"direction", "incoming"}});
		}

		publishDelta(metricTotalSentBytes, cm.sentBytes, stats.sentBytes);
		publishDelta(metricTotalRecvBytes, cm.recvBytes, stats.receivedBytes);
		publishDelta(metricTotalSentPackets, cm.sentPackets, stats.accumulated.sentPackets);
		publishDelta(metricTotalRecvPackets, cm.recvPackets, stats.accumulated.receivedPackets);
	}
}
