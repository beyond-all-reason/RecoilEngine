/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>
#include <array>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include "System/Object.h"
#include "System/creg/Serializer.h"

namespace {
struct Observer: CObject {
	int id = 0;
	std::vector<int>* events = nullptr;
	std::function<void()> action;
	const void* ListenersIdentity() const {
		const auto it = listenersDepTbl.find(DEPENDENCE_COMMANDQUE);
		return it == listenersDepTbl.end() ? nullptr : listeners[it->second].shared.RootIdentity();
	}
	const void* ListeningIdentity() const {
		const auto it = listeningDepTbl.find(DEPENDENCE_COMMANDQUE);
		return it == listeningDepTbl.end() ? nullptr : listening[it->second].shared.RootIdentity();
	}
	void DependentDied(CObject*) override {
		events->push_back(id);
		if (action) action();
	}
	std::vector<int> Filtered() const {
		std::array<int, 32> ids{};
		FilterListeners([](const CObject* obj, int* id) {
			*id = static_cast<const Observer*>(obj)->id;
			return true;
		}, ids);
		return {ids.begin() + 1, ids.begin() + 1 + ids[0]};
	}
};
}

TEST_CASE("Death dependencies retain type and creation order, deduplication and filtering")
{
	std::vector<int> events;
	std::array<Observer, 6> listeners;
	for (int i = 0; i < 6; ++i) { listeners[i].id = i; listeners[i].events = &events; }
	auto target = std::make_unique<Observer>();
	for (int i: {5, 1, 4, 0, 3, 2})
		listeners[i].AddDeathDependence(target.get(), DEPENDENCE_COMMANDQUE);
	listeners[2].AddDeathDependence(target.get(), DEPENDENCE_COMMANDQUE);
	listeners[4].AddDeathDependence(target.get(), DEPENDENCE_ATTACKER);
	listeners[0].AddDeathDependence(target.get(), DEPENDENCE_ORDERTARGET);
	listeners[3].DeleteDeathDependence(target.get(), DEPENDENCE_COMMANDQUE);
	CHECK(target->Filtered() == std::vector<int>{0, 1, 2, 4, 5, 4, 0});
	target.reset();
	CHECK(events == std::vector<int>{4, 0, 1, 2, 4, 5, 0});
}

TEST_CASE("Nested target death returns to the original listener sequence")
{
	std::vector<int> events;
	std::array<Observer, 4> listeners;
	for (int i = 0; i < 4; ++i) { listeners[i].id = i; listeners[i].events = &events; }
	auto first = std::make_unique<Observer>();
	auto second = std::make_unique<Observer>();
	for (auto& listener: listeners) listener.AddDeathDependence(first.get(), DEPENDENCE_COMMANDQUE);
	listeners[2].AddDeathDependence(second.get(), DEPENDENCE_COMMANDQUE);
	listeners[3].AddDeathDependence(second.get(), DEPENDENCE_COMMANDQUE);
	listeners[0].action = [&] { second.reset(); };
	first.reset();
	CHECK(events == std::vector<int>{0, 2, 3, 1, 2, 3});
}

TEST_CASE("Removed listeners and destroyed subscribers do not receive death notifications")
{
	std::vector<int> events;
	auto target = std::make_unique<Observer>();
	auto listener = std::make_unique<Observer>();
	listener->events = &events;
	listener->AddDeathDependence(target.get(), DEPENDENCE_COMMANDQUE);
	listener.reset();
	CHECK(target->Filtered().empty());
	target.reset();
	CHECK(events.empty());
}

TEST_CASE("Persistent dependency sets match ordered membership through divergent mutations")
{
	std::array<CObject, 128> objects;
	std::array<SharedObjectSet, 16> sets;
	std::array<std::set<int>, 16> models;
	std::mt19937 random(8935);
	for (int step = 0; step < 5000; ++step) {
		SharedObjectSet::Batch batch;
		const int which = random() % sets.size();
		const int value = random() % objects.size();
		if (random() % 2) CHECK(sets[which].Insert(&objects[value]) == models[which].insert(value).second);
		else CHECK(sets[which].Erase(&objects[value]) == bool(models[which].erase(value)));
		if (step % 7 == 0) {
			const int other = (which + 1) % sets.size();
			sets[other] = sets[which];
			models[other] = models[which];
		}
		std::vector<int> values;
		sets[which].Visit([&](CObject* object) { values.push_back(object - objects.data()); });
		CHECK(values == std::vector<int>(models[which].begin(), models[which].end()));
		CHECK(sets[which].Size() == models[which].size());
		CHECK(sets[which].Contains(&objects[value]) == models[which].contains(value));
	}
}

TEST_CASE("Shared dependency graph keeps one target set and one listener set for a full batch")
{
	std::vector<int> events;
	std::array<Observer, 1000> listeners;
	std::vector<std::unique_ptr<Observer>> targets;
	for (int i = 0; i < 200; ++i) targets.push_back(std::make_unique<Observer>());
	for (int i = 0; i < 1000; ++i) { listeners[i].id = i; listeners[i].events = &events; }
	{
		SharedObjectSet::Batch batch;
		for (auto& target: targets)
			for (auto& listener: listeners)
				listener.AddDeathDependence(target.get(), DEPENDENCE_COMMANDQUE);
	}
	for (const auto& target: targets)
		CHECK(target->ListenersIdentity() == targets[0]->ListenersIdentity());
	for (const auto& listener: listeners)
		CHECK(listener.ListeningIdentity() == listeners[0].ListeningIdentity());
	// A death updates all identical target sets to another common root.
	targets[57].reset();
	REQUIRE(events.size() == listeners.size());
	for (int i = 0; i < 1000; ++i) {
		CHECK(events[i] == i);
		CHECK(listeners[i].ListeningIdentity() == listeners[0].ListeningIdentity());
	}
	{
		SharedObjectSet::Batch batch;
		for (auto& target: targets) if (target)
			listeners[25].DeleteDeathDependence(target.get(), DEPENDENCE_COMMANDQUE);
	}
	CHECK(listeners[25].ListeningIdentity() == nullptr);
	for (const auto& target: targets) if (target)
		CHECK(target->ListenersIdentity() == targets[0]->ListenersIdentity());
	// Individual removals must not remove another unit's edge.
	listeners[40].DeleteDeathDependence(targets[0].get(), DEPENDENCE_COMMANDQUE);
	events.clear();
	targets[0].reset();
	CHECK(events.size() == 998);
	CHECK(std::find(events.begin(), events.end(), 25) == events.end());
	CHECK(std::find(events.begin(), events.end(), 40) == events.end());
}

struct DependencySaveRoot {
	CR_DECLARE_STRUCT(DependencySaveRoot)
	std::vector<CObject*> objects;
	std::vector<SharedObjectSet> sets;
	~DependencySaveRoot() { for (auto* object: objects) delete object; }
};
CR_BIND(DependencySaveRoot, )
CR_REG_METADATA(DependencySaveRoot, (CR_MEMBER(objects), CR_MEMBER(sets)))

TEST_CASE("Dependency save packages preserve shared graph identity and independent mutations")
{
	DependencySaveRoot source;
	for (int i = 0; i < 200; ++i) source.objects.push_back(new CObject);
	source.sets.resize(1000);
	{
		SharedObjectSet::Batch batch;
		for (auto* object: source.objects)
			for (auto& set: source.sets) set.Insert(object);
	}
	for (int iteration = 0; iteration < 2; ++iteration) {
		std::stringstream stream;
		creg::COutputStreamSerializer output;
		output.SavePackage(&stream, &source, source.GetClass());
		CHECK(stream.str().size() < 100000);
		void* raw = nullptr;
		creg::Class* type = nullptr;
		creg::CInputStreamSerializer input;
		input.LoadPackage(&stream, raw, type);
		REQUIRE(type == source.GetClass());
		std::unique_ptr<DependencySaveRoot> loaded(static_cast<DependencySaveRoot*>(raw));
		REQUIRE(loaded->sets.size() == 1000);
		for (const auto& set: loaded->sets) {
			CHECK(set.RootIdentity() == loaded->sets[0].RootIdentity());
			CHECK(set.Size() == 200);
			for (auto* object: loaded->objects) CHECK(set.Contains(object));
		}
		loaded->sets[0].Erase(loaded->objects[37]);
		CHECK_FALSE(loaded->sets[0].Contains(loaded->objects[37]));
		CHECK(loaded->sets[1].Contains(loaded->objects[37]));
	}
}

TEST_CASE("Nested dependency batches restore sharing and do not alias recycled objects")
{
	SharedObjectSet a, b, c;
	std::optional<CObject> object;
	object.emplace();
	SharedObjectSet::Batch outer;
	a.Insert(&*object);
	{
		SharedObjectSet::Batch inner;
		b.Insert(&*object);
		CHECK(b.RootIdentity() != a.RootIdentity());
	}
	c.Insert(&*object);
	CHECK(c.RootIdentity() == a.RootIdentity());
	a.Erase(&*object);
	b.Erase(&*object);
	c.Erase(&*object);
	object.reset();
	object.emplace(); // same address, a different CObject identity
	a.Insert(&*object);
	CHECK(a.Contains(&*object));
	CHECK(a.Erase(&*object));
	CHECK(a.Empty());
}
