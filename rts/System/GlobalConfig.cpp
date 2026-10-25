/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/GlobalConfig.h"

#ifndef UNITSYNC
#include "System/Config/ConfigHandler.h"
#include "System/Net/UDPConnection.h"
#include "System/StringHash.h"
#include "Rendering/TeamHighlight.h"

CONFIG(int, NetworkLossFactor)
	.defaultValue(netcode::UDPConnection::MIN_LOSS_FACTOR)
	.minimumValue(netcode::UDPConnection::MIN_LOSS_FACTOR)
	.maximumValue(netcode::UDPConnection::MAX_LOSS_FACTOR);

CONFIG(int, InitialNetworkTimeout)
	.defaultValue(30)
	.minimumValue(10).description("Time to wait for the initial connection to the game server.");

CONFIG(int, NetworkTimeout)
	.defaultValue(120)
	.minimumValue(0).description("Number of seconds before connection to game server is considered lost.");

CONFIG(int, ReconnectTimeout)
	.defaultValue(15)
	.minimumValue(0)
	.description("On client defines time to wait before reconnection is initiated with the "
	             "server. On server defines time to wait before reconnection from unknown "
	             "address can be accepted. Reconnect is required only when client's address "
	             "changed during the game. Value needs to be smaller than the value of "
	             "'InitialNetworkTimeout' and 'NetworkTimeout' to trigger. Value '0' "
	             "disables the feature on server or client side.");

CONFIG(int, MaximumTransmissionUnit)
	.defaultValue(1400)
	.minimumValue(400);

CONFIG(int, LinkOutgoingBandwidth)
	.defaultValue(64 * 1024)
	.minimumValue(0);

CONFIG(int, LinkIncomingSustainedBandwidth)
	.defaultValue(2 * 1024)
	.minimumValue(0);

CONFIG(int, LinkIncomingPeakBandwidth)
	.defaultValue(32 * 1024)
	.minimumValue(0);

CONFIG(int, LinkIncomingMaxPacketRate)
	.defaultValue(64)
	.minimumValue(0);

// maximum lag induced by command spam is:
// LinkIncomingMaxWaitingPackets / LinkIncomingMaxPacketRate = 8 seconds
CONFIG(int, LinkIncomingMaxWaitingPackets)
	.defaultValue(512)
	.minimumValue(0);

CONFIG(int, TeamHighlight)
	.defaultValue(CTeamHighlight::HIGHLIGHT_PLAYERS)
	.minimumValue(CTeamHighlight::HIGHLIGHT_FIRST)
	.maximumValue(CTeamHighlight::HIGHLIGHT_LAST);

CONFIG(bool, UseNetMessageSmoothingBuffer).defaultValue(true).description("Buffer network packets for a few frames in an attempt to reduce lag from packet time variance. Introduces a fixed lag.");

CONFIG(bool, LuaWritableConfigFile).defaultValue(true);
CONFIG(bool, VFSCacheArchiveFiles).defaultValue(true);

CONFIG(bool, DumpGameStateOnDesync).defaultValue(true).description("Enable writing clientgamestate and servergamestate dumps when a desync is detected");

CONFIG(float, MinSimDrawBalance).defaultValue(0.15f).description("Percent of the time for simulation is minimum spend for drawing. E.g. if set to 0.15 then 15% of the total cpu time is exclusively reserved for drawing.");
CONFIG(int, MinDrawFPS).defaultValue(2).description("Defines how many frames per second should minimally be rendered. To reach this number we will delay simframes.");

CONFIG(float, SelectThroughGround).defaultValue(200.0f).minimumValue(0.0f).description("How far beyond the ground to allow selecting objects.");

void GlobalConfig::Init()
{
	// Recommended semantics for "expert" type config values:
	// <0 = disable (if applicable)
	networkLossFactor = configHandler->GetInt("NetworkLossFactor");
	initialNetworkTimeout = configHandler->GetInt("InitialNetworkTimeout");
	networkTimeout = configHandler->GetInt("NetworkTimeout");
	reconnectTimeout = configHandler->GetInt("ReconnectTimeout");
	mtu = configHandler->GetInt("MaximumTransmissionUnit");

	ReadLinkLimits();

	useNetMessageSmoothingBuffer = configHandler->GetBool("UseNetMessageSmoothingBuffer");
	luaWritableConfigFile = configHandler->GetBool("LuaWritableConfigFile");
	vfsCacheArchiveFiles = configHandler->GetBool("VFSCacheArchiveFiles");

	dumpGameStateOnDesync = configHandler->GetBool("DumpGameStateOnDesync");

	minSimDrawBalance = configHandler->GetFloat("MinSimDrawBalance");
	minDrawFPS = configHandler->GetInt("MinDrawFPS");

	teamHighlight = configHandler->GetInt("TeamHighlight");

	selectThroughGround = configHandler->GetFloat("SelectThroughGround");
}

void GlobalConfig::ReadLinkLimits()
{
	linkOutgoingBandwidth = configHandler->GetInt("LinkOutgoingBandwidth");
	linkIncomingSustainedBandwidth = configHandler->GetInt("LinkIncomingSustainedBandwidth");
	linkIncomingPeakBandwidth = configHandler->GetInt("LinkIncomingPeakBandwidth");
	linkIncomingMaxPacketRate = configHandler->GetInt("LinkIncomingMaxPacketRate");
	linkIncomingMaxWaitingPackets = configHandler->GetInt("LinkIncomingMaxWaitingPackets");

	if (linkIncomingSustainedBandwidth > 0 && linkIncomingPeakBandwidth < linkIncomingSustainedBandwidth)
		linkIncomingPeakBandwidth = linkIncomingSustainedBandwidth;
	if (linkIncomingPeakBandwidth > 0 && linkIncomingSustainedBandwidth <= 0)
		linkIncomingSustainedBandwidth = linkIncomingPeakBandwidth;
	if (linkIncomingMaxPacketRate > 0 && linkIncomingSustainedBandwidth <= 0)
		linkIncomingSustainedBandwidth = linkIncomingPeakBandwidth = 1024 * 1024;
}

void GlobalConfig::InitConfigNotify()
{
	// not observed: MTU and the archive/Lua-write flags only matter when what uses them is created
	configHandler->NotifyOnChange(this, {
		"NetworkLossFactor",
		"InitialNetworkTimeout",
		"NetworkTimeout",
		"ReconnectTimeout",
		"LinkOutgoingBandwidth",
		"LinkIncomingSustainedBandwidth",
		"LinkIncomingPeakBandwidth",
		"LinkIncomingMaxPacketRate",
		"LinkIncomingMaxWaitingPackets",
		"UseNetMessageSmoothingBuffer",
		"DumpGameStateOnDesync",
		"MinSimDrawBalance",
		"MinDrawFPS",
		"TeamHighlight",
		"SelectThroughGround",
	});
}

void GlobalConfig::RemoveConfigNotify()
{
	configHandler->RemoveObserver(this);
}

void GlobalConfig::ConfigNotify(const std::string& key, const std::string& value)
{
	// per key, so a change does not undo what a command set for another one (/NetMsgSmoothing, /TeamHighlight)
	switch (hashString(key.c_str())) {
		// connections read the loss factor when they are made, the server takes it from the connect message
		case hashString("NetworkLossFactor"             ): { networkLossFactor = configHandler->GetInt(key); } break;
		case hashString("InitialNetworkTimeout"         ): { initialNetworkTimeout = configHandler->GetInt(key); } break;
		case hashString("NetworkTimeout"                ): { networkTimeout = configHandler->GetInt(key); } break;
		case hashString("ReconnectTimeout"              ): { reconnectTimeout = configHandler->GetInt(key); } break;
		case hashString("LinkOutgoingBandwidth"         ):
		case hashString("LinkIncomingSustainedBandwidth"):
		case hashString("LinkIncomingPeakBandwidth"     ):
		case hashString("LinkIncomingMaxPacketRate"     ):
		case hashString("LinkIncomingMaxWaitingPackets" ): { ReadLinkLimits(); } break;
		case hashString("UseNetMessageSmoothingBuffer"  ): { useNetMessageSmoothingBuffer = configHandler->GetBool(key); } break;
		case hashString("DumpGameStateOnDesync"         ): { dumpGameStateOnDesync = configHandler->GetBool(key); } break;
		case hashString("MinSimDrawBalance"             ): { minSimDrawBalance = configHandler->GetFloat(key); } break;
		case hashString("MinDrawFPS"                    ): { minDrawFPS = configHandler->GetInt(key); } break;
		case hashString("TeamHighlight"                 ): { teamHighlight = configHandler->GetInt(key); } break;
		case hashString("SelectThroughGround"           ): { selectThroughGround = configHandler->GetFloat(key); } break;
		default: {} break;
	}
}
#endif

GlobalConfig globalConfig;

