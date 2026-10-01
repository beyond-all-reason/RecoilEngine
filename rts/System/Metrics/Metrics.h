/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

namespace prometheus {
	class Registry;
}

/// the process-wide Prometheus registry and its /metrics endpoint
namespace metrics {
	/**
	 * @brief start the embedded Prometheus /metrics endpoint
	 *
	 * Reads MetricsPort / MetricsBindAddress. An unset port keeps the endpoint
	 * off. Idempotent until the next Shutdown().
	 */
	void Init();

	/// stop serving and drop the registry, invalidating every metric pointer in it
	void Shutdown();

	/// true iff the /metrics endpoint is being served
	bool Enabled();

	/// true iff per-player metrics should be exported alongside the
	/// server-wide aggregates. These multiply the series count by the
	/// number of participants
	bool PerPlayerEnabled();

	/// registry to Register() metric families in, only valid when Enabled()
	prometheus::Registry& GetRegistry();
}
