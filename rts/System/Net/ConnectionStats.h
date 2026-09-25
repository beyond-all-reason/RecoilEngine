/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <atomic>
#include <string>
#include <variant>

namespace netcode
{

/**
 * @brief basic byte counters for a loopback connection
 *
 * These stats are all a loopback connection can report, since
 * they aren't a true network connection.
 */
struct BasicStats {
	unsigned int sentBytes = 0;
	unsigned int receivedBytes = 0;
};

/**
 * @brief a real UDP connection's traffic counters
 *
 * This acts as a single extension point for real UDP connection telemetry.
 * If you add a field here, fill it in UDPConnection::GetStats().
 */
struct UdpStats {
	/// byte totals every connection keeps, whatever its transport
	unsigned int sentBytes = 0;
	unsigned int receivedBytes = 0;

	/**
	 * @brief stats the connection accumulates inline as events happen
	 */
	struct Accumulated {
		unsigned int sentPackets = 0;
		unsigned int receivedPackets = 0;
		/// chunks put back on the wire after having been sent once
		unsigned int resentOutgoingChunks = 0;
		/// chunks discarded on arrival because the same chunk had already been received
		unsigned int duplicateIncomingChunks = 0;
		/// cumulative protocol header bytes
		unsigned int sentOverheadBytes = 0;
		unsigned int receivedOverheadBytes = 0;
	} accumulated;

	/**
	 * @brief stats read off *periodically* from live state
	 */
	struct Live {
		/// inbound chunks delivered in order so far
		unsigned int processedIncomingChunks = 0;
		/// received but undeliverable until an earlier chunk arrives
		unsigned int incomingReorderQueueDepth = 0;
		/// handed to the link and not yet on the wire
		unsigned int outgoingQueueBytes = 0;
		float outgoingBandwidthBytesPerSec = 0.0f;
		unsigned int unackedOutgoingChunks = 0;
		unsigned int outgoingResendQueueDepth = 0;
	} live;
};

using ConnectionStats = std::variant<BasicStats, UdpStats>;

/**
 * @brief whether links should collect the optional telemetry in ConnectionStats
 *
 * Byte and packet counters are always kept, the rest exists purely to be
 * exported, so the rest are off unless the stats need to be exported somewhere.
 *
 * Atomic because a host client's own CNetProtocol link is already alive
 * when the server it just started flips this.
 */
inline std::atomic<bool> statsSampling = false;

inline void SetStatsSampling(bool enable) { statsSampling.store(enable, std::memory_order_relaxed); }
inline bool StatsSampling() { return statsSampling.load(std::memory_order_relaxed); }

std::string FormatConnectionStats(const BasicStats& stats);
std::string FormatConnectionStats(const UdpStats& stats);

}
