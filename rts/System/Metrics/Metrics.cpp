/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "Metrics.h"

#include <cassert>
#include <memory>
#include <string>

#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

CONFIG(int, MetricsPort)
	.defaultValue(0)
	.minimumValue(0)
	.maximumValue(65535)
	.description("Port for the Prometheus /metrics HTTP endpoint. 0, the default, disables it. One port per process, so hosts running several instances must assign a distinct one to each.");

CONFIG(std::string, MetricsBindAddress)
	.defaultValue("127.0.0.1")
	.description("Address the Prometheus /metrics HTTP endpoint binds to. The endpoint has no authentication and no TLS, so bind it to a routable address only behind something that does access control.");

namespace {
	std::shared_ptr<prometheus::Registry> registry;
	std::unique_ptr<prometheus::Exposer> exposer;

	bool initialized = false;
	bool enabled = false;
}

void metrics::Init()
{
	if (initialized)
		return;

	initialized = true;

	const int port = configHandler->GetInt("MetricsPort");

	if (port == 0)
		return;

	const std::string endpoint = configHandler->GetString("MetricsBindAddress") + ":" + std::to_string(port);

	try {
		registry = std::make_shared<prometheus::Registry>();
		exposer = std::make_unique<prometheus::Exposer>(endpoint);
		exposer->RegisterCollectable(registry);
		enabled = true;
		LOG("[Metrics] serving metrics on http://%s/metrics", endpoint.c_str());
	} catch (const std::exception& ex) {
		LOG_L(L_ERROR, "[Metrics] failed to start /metrics endpoint on %s: %s", endpoint.c_str(), ex.what());
		exposer.reset();
		registry.reset();
	}
}

void metrics::Shutdown()
{
	enabled = false;

	exposer.reset();
	registry.reset();

	initialized = false;
}

bool metrics::Enabled()
{
	return enabled;
}

prometheus::Registry& metrics::GetRegistry()
{
	assert(registry != nullptr);
	return *registry;
}
