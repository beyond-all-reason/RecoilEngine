/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

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

	std::vector<CObject*> notifications;
	std::function<void(CObject*)> onNotification;
};

class DestructionTarget: public CObject
{
public:
	explicit DestructionTarget(bool& finished): finished(finished) {}
	~DestructionTarget() override { finished = true; }

private:
	bool& finished;
};
}

TEST_CASE("Native dependency callbacks follow derived object destruction")
{
	bool destroyed = false;
	FinalizationObserver listener;
	auto target = std::make_unique<DestructionTarget>(destroyed);
	listener.AddDeathDependence(target.get(), DEPENDENCE_COMMANDQUE);
	listener.AddDeathDependence(target.get(), DEPENDENCE_TARGET);
	listener.onNotification = [&](CObject*) { CHECK(destroyed); };
	CHECK(listener.notifications.empty());
	target.reset();
	CHECK(listener.notifications.size() == 2);
}

TEST_CASE("Deleting a listener removes its outgoing dependencies")
{
	auto target = std::make_unique<CObject>();
	bool notified = false;
	{
		FinalizationObserver listener;
		listener.onNotification = [&](CObject*) { notified = true; };
		listener.AddDeathDependence(target.get(), DEPENDENCE_TARGET);
	}
	target.reset();
	CHECK_FALSE(notified);
}

TEST_CASE("Pre-delete guards do not notify or reorder native destruction")
{
	bool destroyed = false;
	FinalizationObserver listener;
	FinalizationObserver lateListener;
	auto target = std::make_unique<DestructionTarget>(destroyed);
	listener.AddDeathDependence(target.get(), DEPENDENCE_TARGET);
	target->SetPreDeleting(true);
	lateListener.AddDeathDependence(target.get(), DEPENDENCE_TARGET);
	CHECK(target->IsPreDeleting());
	CHECK_FALSE(destroyed);
	CHECK(listener.notifications.empty());
	listener.onNotification = [&](CObject*) { CHECK(destroyed); };
	target->SetPreDeleting(false);
	target.reset();
	CHECK(listener.notifications.size() == 1);
	CHECK(lateListener.notifications.empty());
}
