/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "ConnectionStats.h"

#include "System/SafeUtil.h"
#include "System/SpringFormat.h"

namespace netcode
{

std::string FormatConnectionStats(const BasicStats& s)
{
	return spring::format("\t%u bytes sent\n\t%u bytes recv'd\n", s.sentBytes, s.receivedBytes);
}

std::string FormatConnectionStats(const UdpStats& s)
{
	std::string msg = spring::format("\t%u bytes sent\n\t%u bytes recv'd\n", s.sentBytes, s.receivedBytes);
	msg += spring::format("\t%u packets sent   (%.3f bytes/packet)\n",
		s.sentPackets, spring::SafeDivide(s.sentBytes * 1.0f, s.sentPackets * 1.0f));
	msg += spring::format("\t%u packets recv'd (%.3f bytes/packet)\n",
		s.receivedPackets, spring::SafeDivide(s.receivedBytes * 1.0f, s.receivedPackets * 1.0f));
	msg += spring::format("\t{%.3fx, %.3fx} relative protocol overhead {up, down}\n",
		spring::SafeDivide(s.sentOverheadBytes * 1.0f, s.sentBytes * 1.0f),
		spring::SafeDivide(s.receivedOverheadBytes * 1.0f, s.receivedBytes * 1.0f));
	msg += spring::format("\t%u incoming chunks dropped, %u outgoing chunks resent\n",
		s.duplicateIncomingChunks, s.resentOutgoingChunks);
	msg += spring::format("\t%u incoming chunks processed\n", s.processedIncomingChunks);
	return msg;
}

}
