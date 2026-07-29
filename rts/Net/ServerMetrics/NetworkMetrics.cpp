/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "NetworkMetrics.h"

#include <algorithm>
#include <map>
#include <string>
#include <variant>

#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/registry.h>

#include "Net/GameParticipant.h"
#include "Net/GameServer.h"
#include "System/GlobalConfig.h"
#include "System/Metrics/Helpers.h"
#include "System/Metrics/Metrics.h"
#include "System/Net/ConnectionStats.h"

using metrics::DeltaSince;


void NetworkMetrics::Init(prometheus::Registry& registry)
{
	const auto counterFamily = [&](const char* name, const char* help) {
		return &prometheus::BuildCounter().Name(name).Help(help).Register(registry);
	};
	const auto gaugeFamily = [&](const char* name, const char* help) {
		return &prometheus::BuildGauge().Name(name).Help(help).Register(registry);
	};
	const auto gauge = [&](const char* name, const char* help) { return &gaugeFamily(name, help)->Add({}); };

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

	metricTotalOutgoingBw = gauge("recoil_network_outgoing_bandwidth_bytes_per_second",
		"Rolling average of the send rate over all client connections");
	metricTotalUnackedOutgoingChunks = gauge("recoil_network_unacked_outgoing_chunks",
		"Chunks sent to clients and not yet acked, summed over all connections");
	metricTotalOutgoingResendQueueDepth = gauge("recoil_network_outgoing_resend_queue_depth",
		"Chunks queued for retransmission, summed over all connections. A sustained non-zero depth means chunks are being flagged for resend faster than they clear, e.g. under packet loss or a stalled link");
	metricTotalIncomingReorderQueueDepth = gauge("recoil_network_incoming_reorder_queue_depth",
		"Chunks from clients held in the reorder buffer behind a missing chunk, summed over all connections. Fills on benign reordering as well as on real loss");
	metricTotalOutgoingQueueBytes = gauge("recoil_network_outgoing_queue_bytes",
		"Application bytes queued for clients and not yet transmitted, summed over all connections");

	gauge("recoil_network_outgoing_bandwidth_limit_bytes_per_second",
		"Configured per-connection outgoing bandwidth cap in bytes/sec (0 = unlimited). A static config value (LinkOutgoingBandwidth), not a live measurement")->Set(globalConfig.linkOutgoingBandwidth);

	perPlayerEnabled = metrics::PerPlayerEnabled();
	if (!perPlayerEnabled)
		return;

	metricBytes = counterFamily("recoil_network_per_connection_bytes_total",
		"Bytes over this client connection, by direction. No series exists for local loopback connections");
	metricPackets = counterFamily("recoil_network_per_connection_packets_total",
		"UDP packets over this client connection, by direction");
	metricOutgoingBw = gaugeFamily("recoil_network_per_connection_outgoing_bandwidth_bytes_per_second",
		"Rolling average of the send rate to this client, as used by outgoing bandwidth limiting");
	metricUnackedOutgoingChunks = gaugeFamily("recoil_network_per_connection_unacked_outgoing_chunks",
		"Chunks sent to this client and not yet acked");
	metricOutgoingResendQueueDepth = gaugeFamily("recoil_network_per_connection_outgoing_resend_queue_depth",
		"Chunks queued for retransmission to this client");
	metricIncomingReorderQueueDepth = gaugeFamily("recoil_network_per_connection_incoming_reorder_queue_depth",
		"Chunks from this client held in the reorder buffer behind a missing chunk");
	metricOutgoingQueueBytes = gaugeFamily("recoil_network_per_connection_outgoing_queue_bytes",
		"Application bytes queued for this client and not yet transmitted");
}


void NetworkMetrics::ReleaseConnectionGauges(ConnectionMetrics& cm)
{
	const auto release = [](prometheus::Family<prometheus::Gauge>* family, prometheus::Gauge*& child) {
		if (child == nullptr)
			return;

		family->Remove(child);
		child = nullptr;
	};

	release(metricOutgoingBw, cm.outgoingBw);
	release(metricUnackedOutgoingChunks, cm.unackedOutgoingChunks);
	release(metricOutgoingResendQueueDepth, cm.outgoingResendQueueDepth);
	release(metricIncomingReorderQueueDepth, cm.incomingReorderQueueDepth);
	release(metricOutgoingQueueBytes, cm.outgoingQueueBytes);
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
	float totalOutgoingBw = 0.0f;
	unsigned int totalUnackedOutgoingChunks = 0;
	unsigned int totalOutgoingResendQueueDepth = 0;
	unsigned int totalIncomingReorderQueueDepth = 0;
	unsigned int totalOutgoingQueueBytes = 0;

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
			// don't let the metrics for a disconnected player get scraped
			ReleaseConnectionGauges(cm);
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
			const std::map<std::string, std::string> labels = {{"playerid", playerId}};
			cm.sentBytes.player   = &metricBytes->Add({{"playerid", playerId}, {"direction", "outgoing"}});
			cm.recvBytes.player   = &metricBytes->Add({{"playerid", playerId}, {"direction", "incoming"}});
			cm.sentPackets.player = &metricPackets->Add({{"playerid", playerId}, {"direction", "outgoing"}});
			cm.recvPackets.player = &metricPackets->Add({{"playerid", playerId}, {"direction", "incoming"}});
			cm.outgoingBw         = &metricOutgoingBw->Add(labels);
			cm.unackedOutgoingChunks      = &metricUnackedOutgoingChunks->Add(labels);
			cm.outgoingResendQueueDepth   = &metricOutgoingResendQueueDepth->Add(labels);
			cm.incomingReorderQueueDepth  = &metricIncomingReorderQueueDepth->Add(labels);
			cm.outgoingQueueBytes     = &metricOutgoingQueueBytes->Add(labels);
		}

		publishDelta(metricTotalSentBytes, cm.sentBytes, stats.sentBytes);
		publishDelta(metricTotalRecvBytes, cm.recvBytes, stats.receivedBytes);
		publishDelta(metricTotalSentPackets, cm.sentPackets, stats.accumulated.sentPackets);
		publishDelta(metricTotalRecvPackets, cm.recvPackets, stats.accumulated.receivedPackets);

		totalOutgoingBw += stats.live.outgoingBandwidthBytesPerSec;
		totalUnackedOutgoingChunks += stats.live.unackedOutgoingChunks;
		totalOutgoingResendQueueDepth += stats.live.outgoingResendQueueDepth;
		totalIncomingReorderQueueDepth += stats.live.incomingReorderQueueDepth;
		totalOutgoingQueueBytes += stats.live.outgoingQueueBytes;

		if (perPlayerEnabled) {
			cm.outgoingBw->Set(stats.live.outgoingBandwidthBytesPerSec);
			cm.unackedOutgoingChunks->Set(stats.live.unackedOutgoingChunks);
			cm.outgoingResendQueueDepth->Set(stats.live.outgoingResendQueueDepth);
			cm.incomingReorderQueueDepth->Set(stats.live.incomingReorderQueueDepth);
			cm.outgoingQueueBytes->Set(stats.live.outgoingQueueBytes);
		}
	}

	metricTotalOutgoingBw->Set(totalOutgoingBw);
	metricTotalUnackedOutgoingChunks->Set(totalUnackedOutgoingChunks);
	metricTotalOutgoingResendQueueDepth->Set(totalOutgoingResendQueueDepth);
	metricTotalIncomingReorderQueueDepth->Set(totalIncomingReorderQueueDepth);
	metricTotalOutgoingQueueBytes->Set(totalOutgoingQueueBytes);
}
