#include <catch_amalgamated.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

#include "System/Object.h"

namespace {
class FinalizationObserver: public CObject
{
public:
	void DependentDied(CObject* object) override
	{
		notifications.push_back(object);
		if (onNotification)
			onNotification(object);
	}

	bool Watches(CObject* object, DependenceType type)
	{
		const auto& dependencies = GetListening(type);
		return std::find(dependencies.begin(), dependencies.end(), object) != dependencies.end();
	}

	std::vector<CObject*> notifications;
	std::function<void(CObject*)> onNotification;
};
}

TEST_CASE("Object finalization notifies once before destruction")
{
	FinalizationObserver listener;
	auto target = std::make_unique<CObject>();
	listener.AddDeathDependence(target.get(), DEPENDENCE_COMMANDQUE);
	listener.AddDeathDependence(target.get(), DEPENDENCE_TARGET);
	target->NotifyDeathDependences();
	CHECK(target->DeathDependencesNotified());
	CHECK(listener.notifications.size() == 2);
	CHECK_FALSE(listener.Watches(target.get(), DEPENDENCE_COMMANDQUE));
	CHECK_FALSE(listener.Watches(target.get(), DEPENDENCE_TARGET));
	target->NotifyDeathDependences();
	target.reset();
	CHECK(listener.notifications.size() == 2);
}

TEST_CASE("Unfinalized objects still notify from their destructor")
{
	FinalizationObserver listener;
	auto target = std::make_unique<CObject>();
	listener.AddDeathDependence(target.get(), DEPENDENCE_COMMANDQUE);
	target.reset();
	CHECK(listener.notifications.size() == 1);
}

TEST_CASE("Object finalization rejects new dependencies and recursive notification")
{
	FinalizationObserver listener;
	FinalizationObserver lateListener;
	CObject target;
	listener.AddDeathDependence(&target, DEPENDENCE_TARGET);
	listener.onNotification = [&](CObject* object) {
		object->NotifyDeathDependences();
		lateListener.AddDeathDependence(object, DEPENDENCE_COMMANDQUE);
	};
	target.NotifyDeathDependences();
	CHECK(listener.notifications.size() == 1);
	CHECK_FALSE(lateListener.Watches(&target, DEPENDENCE_COMMANDQUE));
	CHECK(lateListener.notifications.empty());
}

TEST_CASE("Finalized objects still release their outgoing dependencies")
{
	CObject target;
	auto listener = std::make_unique<FinalizationObserver>();
	listener->AddDeathDependence(&target, DEPENDENCE_TARGET);
	listener->NotifyDeathDependences();
	CHECK(listener->Watches(&target, DEPENDENCE_TARGET));
	listener->DeleteDeathDependence(&target, DEPENDENCE_TARGET);
	CHECK_FALSE(listener->Watches(&target, DEPENDENCE_TARGET));
	listener->AddDeathDependence(&target, DEPENDENCE_TARGET);
	CHECK_FALSE(listener->Watches(&target, DEPENDENCE_TARGET));
	listener.reset();
	target.NotifyDeathDependences();
}
