/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GameParticipant.h"

#include "Net/Protocol/BaseNetProtocol.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Log/ILog.h"
#include "System/Net/Connection.h"

GameParticipant::GameParticipant() { aiClientLinks[MAX_AIS] = ClientLinkData(false); }

GameParticipant::~GameParticipant()
{
	if (myState == DISCONNECTING) {
		CloseConnection(true);
		myState = DISCONNECTED;
	}
}

void GameParticipant::SendData(std::shared_ptr<const netcode::RawPacket> packet)
{
	if (clientLink != nullptr && myState != GameParticipant::State::DISCONNECTING)
		clientLink->SendData(packet);
}

void GameParticipant::Connected(std::shared_ptr<netcode::CConnection> _link, bool local)
{
	CloseConnection(false);

	clientLink = _link;
	aiClientLinks[MAX_AIS].link.reset(new netcode::CLoopbackConnection());

	isLocal = local;
	myState = CONNECTED;
	lastFrameResponse = 0;
}

void GameParticipant::Kill(const std::string& reason, const bool flush)
{
	bool disconnected = true;

	if (clientLink != nullptr) {
		if (myState != GameParticipant::State::DISCONNECTING) {
			clientLink->SendData(CBaseNetProtocol::Get().SendQuit(reason));

			if (flush) {
				/* delay to make sure the Flush() performed by Close()
				 * has an effect (forced flushes are undesirable) */
				disconnectDelay = spring_gettime() + spring_time(1000);
				disconnected = false;
				LOG("%s: client disconnecting...", __func__);
			}
			else
				CloseConnection(false);
		}
		else
			disconnected = false;
	}

	aiClientLinks[MAX_AIS].link.reset();
#ifdef SYNCCHECK
	syncResponse.clear();
#endif

	myState = (disconnected) ? DISCONNECTED : DISCONNECTING;
}

void GameParticipant::CheckForExpiredConnection()
{
	if (myState == DISCONNECTING) {
		if (spring_gettime() >= disconnectDelay) {
			CloseConnection(true);
			myState = DISCONNECTED;
			LOG("%s: client disconnected after delay", __func__);
		}
	}
}

void GameParticipant::CloseConnection(bool flush)
{
	if (clientLink != nullptr) {
		LOG("%s: client connection closed", __func__);
		clientLink->Close(flush);
		clientLink.reset();
	}
}
