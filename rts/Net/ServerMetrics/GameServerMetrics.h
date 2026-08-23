/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "System/Misc/SpringTime.h"

class CGameServer;

/**
 * @brief every prometheus series the game server exports
 *
 * This is a facade over groups of metrics. It owns the metric endpoint's
 * lifetime and the publish interval, and passes everything else to the metric
 * groups.
 *
 * Every method no-ops when metrics are disabled (the default) at runtime,
 * so callers don't have to test for it.
 *
 * Entry points are called from both the netcode and the game thread, but always
 * under CGameServer::gameServerMutex.
 */
class ServerMetrics
{
public:
	void Init();

	/**
	 * @brief stop the endpoint and drop the registry
	 *
	 * There is one registry per process, not per game, so a finished game's series
	 * will go on being served until this drops them. This method invalidates every metric
	 * pointer held here, so any callers must make sure they clear their refs.
	 */
	void Shutdown();

	/// re-publish every value. This is rate-limited internally, so it's cheap to
	/// call per loop
	void Update(const CGameServer& server);

private:
	/// while false (during startup or metrics disabled), Update() is a no-op
	bool registered = false;

	/// when Update() last republished. This must be the *server's update clock*
	/// (CGameServer::lastUpdate) rather than wall time
	spring_time lastPublishTime = spring_notime;
};
