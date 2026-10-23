/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>
#include <deque>
#include <limits>
#include <random>
#include <sstream>
#include "System/creg/Serializer.h"
#include "Sim/Units/CommandAI/CommandQueue.h"

// The queue is constructed by its owning command AI in the engine.
class CCommandAI {
public:
	CR_DECLARE_STRUCT(CCommandAI)
	CCommandQueue queue;
	void SetTagCounter(int value) { queue.tagCounter = value; }
	size_t Segments() const { return queue.storage.SegmentCount(); }
	creg::Class* QueueClass() const { return queue.GetClass(); }
	size_t Owned() const { return queue.storage.OwnedCommandCount(); }
	const SharedAttackList* List(size_t index = 0) const { return queue.storage.SharedList(index); }
};

CR_BIND(CCommandAI, )
CR_REG_METADATA(CCommandAI, (CR_MEMBER(queue)))

struct QueueSaveRoot {
	CR_DECLARE_STRUCT(QueueSaveRoot)
	~QueueSaveRoot() { for (auto* owner: owners) delete owner; }
	std::vector<CCommandAI*> owners;
};
CR_BIND(QueueSaveRoot, )
CR_REG_METADATA(QueueSaveRoot, (CR_MEMBER(owners)))

static void CheckCommand(const Command& actual, const Command& expected)
{
	CHECK(actual.GetID() == expected.GetID());
	CHECK(actual.GetID(true) == expected.GetID(true));
	CHECK(actual.GetTag() == expected.GetTag());
	CHECK(actual.GetOpts() == expected.GetOpts());
	CHECK(actual.GetTimeOut() == expected.GetTimeOut());
	REQUIRE(actual.GetNumParams() == expected.GetNumParams());
	for (unsigned i = 0; i < actual.GetNumParams(); ++i)
		CHECK(actual.GetParam(i) == expected.GetParam(i));
}

TEST_CASE("Command queue matches deque operations and preserves tags")
{
	CCommandAI owner;
	auto& queue = owner.queue;
	std::deque<Command> model;
	std::mt19937 random(8714);
	int tag = CCommandQueue::maxTagValue - 5;
	owner.SetTagCounter(tag);
	for (int step = 0; step < 2000; ++step) {
		const auto operation = random() % 7;
		if (operation < 3 || model.empty()) {
			Command command(step % 2 ? CMD_ATTACK : CMD_MOVE, SHIFT_KEY, float(step));
			command.SetAICmdID(step + 7);
			command.SetTimeOut(step + 99);
			Command expected = command;
			if (++tag >= CCommandQueue::maxTagValue) tag = 1;
			expected.SetTag(tag);
			if (operation == 0) {
				queue.push_front(command);
				model.push_front(expected);
			} else if (operation == 1) {
				const auto index = random() % (model.size() + 1);
				queue.insert(queue.begin() + index, command);
				model.insert(model.begin() + index, expected);
			} else {
				queue.push_back(command);
				model.push_back(expected);
			}
		} else if (operation == 3) {
			queue.pop_front(); model.pop_front();
		} else if (operation == 4) {
			queue.pop_back(); model.pop_back();
		} else if (operation == 5) {
			const auto first = random() % model.size();
			const auto last = first + random() % (model.size() - first + 1);
			queue.erase(queue.begin() + first, queue.begin() + last);
			model.erase(model.begin() + first, model.begin() + last);
		} else {
			queue.clear(); model.clear();
		}
		REQUIRE(queue.size() == model.size());
		const auto& readQueue = queue;
		for (size_t i = 0; i < model.size(); ++i)
			CheckCommand(readQueue[i], model[i]);
	}
}

TEST_CASE("Command queue front reference survives append and prepend")
{
	CCommandAI owner;
	auto& queue = owner.queue;
	queue.push_back(Command(CMD_ATTACK, 0, 123.0f));
	Command& active = queue.front();
	for (int i = 0; i < 200; ++i)
		queue.push_back(Command(CMD_ATTACK, SHIFT_KEY, float(i)));
	CHECK(&active == &queue.front());
	queue.push_front(Command(CMD_WAIT));
	active.SetTimeOut(42);
	CHECK(queue[1].GetTimeOut() == 42);
}

TEST_CASE("Attack batches share storage and retain independent progress and tags")
{
	std::vector<Command> commands;
	for (int i = 0; i < 200; ++i) {
		commands.emplace_back(CMD_ATTACK, SHIFT_KEY, float(i));
		commands.back().SetAICmdID(i + 100);
		commands.back().SetTimeOut(i + 1000);
	}
	std::vector<std::unique_ptr<CCommandAI>> owners;
	for (int i = 0; i < 1000; ++i) {
		owners.push_back(std::make_unique<CCommandAI>());
		owners.back()->SetTagCounter(CCommandQueue::maxTagValue - 3);
	}
	{
		SharedAttackBatch batch(commands);
		for (const Command& c: commands)
			for (auto& owner: owners)
				owner->queue.push_back(c);
	}
	for (const auto& owner: owners) {
		CHECK(owner->Segments() == 1);
		CHECK(owner->Owned() == 0);
		CHECK(owner->queue.size() == 200);
		for (size_t i = 0; i < commands.size(); ++i) {
			Command expected = commands[i];
			expected.SetTag(CommandQueueStorage::AdvanceTag(CCommandQueue::maxTagValue - 2, i));
			CheckCommand(owner->queue[i], expected);
		}
		CHECK(owner->Owned() == 0); // getters must not expand storage
	}
	auto& first = owners[0]->queue;
	Command& active = first.front();
	CHECK(owners[0]->Owned() == 1);
	first.push_front(Command(CMD_WAIT));
	active.SetTimeOut(17);
	CHECK(first[1].GetTimeOut() == 17);
	CHECK(owners[1]->queue[0].GetTimeOut() == 1000);
	first.pop_front();
	first.pop_front();
	CHECK(first.front().GetParam(0) == 1);
	CHECK(owners[1]->queue[0].GetParam(0) == 0);
	const unsigned tag = first[80].GetTag();
	first.erase(first.begin() + 4, first.begin() + 80);
	CHECK(first[4].GetTag() == tag);
	first.insert(first.begin() + 4, Command(CMD_MOVE, SHIFT_KEY, float3(10, 20, 30)));
	CHECK(first[5].GetTag() == tag);
	first.Edit(5).SetParam(0, 999);
	CHECK(owners[1]->queue[81].GetParam(0) == 81);
	first.clear();
	CHECK(owners[1]->queue.size() == 200);
}

TEST_CASE("Nested attack batches and unrelated callback commands do not alias")
{
	CCommandAI owner;
	std::vector<Command> outer{Command(CMD_ATTACK, SHIFT_KEY, 1.0f), Command(CMD_ATTACK, SHIFT_KEY, 2.0f)};
	std::vector<Command> inner{Command(CMD_ATTACK, SHIFT_KEY, 3.0f)};
	{
		SharedAttackBatch batch(outer);
		owner.queue.push_back(outer[0]);
		{
			SharedAttackBatch nested(inner);
			owner.queue.push_back(inner[0]);
			owner.queue.push_back(outer[1]); // different admission context
		}
		owner.queue.push_back(outer[1]);
		owner.queue.push_back(Command(outer[1])); // equal value, independent command
	}
	CHECK(owner.Owned() == 2);
	CHECK(owner.Segments() == 5);
	for (size_t i = 0; i < owner.queue.size(); ++i)
		CHECK(owner.queue[i].GetTag() == i + 1);
	CHECK(owner.queue[0].GetParam(0) == 1);
	CHECK(owner.queue[1].GetParam(0) == 3);
	CHECK(owner.queue[3].GetParam(0) == 2);
}

TEST_CASE("Shared queue mixed edits match independent logical commands")
{
	CCommandAI owner;
	std::deque<Command> model;
	std::vector<Command> commands;
	for (int i = 0; i < 200; ++i)
		commands.emplace_back(CMD_ATTACK, SHIFT_KEY, float(i));
	{
		SharedAttackBatch batch(commands);
		for (const auto& c: commands) {
			owner.queue.push_back(c);
			model.push_back(c);
			model.back().SetTag(model.size());
		}
	}
	std::mt19937 random(8935);
	unsigned tag = 200;
	for (int step = 0; step < 500; ++step) {
		auto& queue = owner.queue;
		if (model.empty() || random() % 3 == 0) {
			Command command(CMD_MOVE, SHIFT_KEY, float(step));
			for (int p = 0; p < 12; ++p) command.PushParam(float(p));
			const size_t at = random() % (model.size() + 1);
			queue.insert(queue.begin() + at, command);
			command.SetTag(++tag);
			model.insert(model.begin() + at, command);
		} else {
			const size_t at = random() % model.size();
			if (random() % 2) {
				queue.erase(queue.begin() + at);
				model.erase(model.begin() + at);
			} else {
				queue.Edit(at).SetTimeOut(step);
				model[at].SetTimeOut(step);
			}
		}
		REQUIRE(queue.size() == model.size());
		for (size_t i = 0; i < model.size(); ++i)
			CheckCommand(queue[i], model[i]);
	}
}

TEST_CASE("Shared queue save/load retains logical commands and next tag")
{
	CCommandAI owner;
	owner.SetTagCounter(CCommandQueue::maxTagValue - 2);
	std::vector<Command> commands{Command(CMD_ATTACK, SHIFT_KEY, 17.0f), Command(CMD_ATTACK, SHIFT_KEY, 23.0f)};
	{
		SharedAttackBatch batch(commands);
		for (const auto& command: commands) owner.queue.push_back(command);
	}
	Command custom(12345, ALT_KEY);
	for (int i = 0; i < 20; ++i) custom.PushParam(float(i));
	custom.SetAICmdID(456);
	custom.SetTimeOut(789);
	owner.queue.push_back(custom);
	std::stringstream stream;
	creg::COutputStreamSerializer output;
	output.SavePackage(&stream, &owner.queue, owner.QueueClass());
	CHECK(owner.Owned() == 1); // saving must not expand the live queue
	void* raw;
	creg::Class* type;
	creg::CInputStreamSerializer input;
	input.LoadPackage(&stream, raw, type);
	REQUIRE(type == owner.QueueClass());
	std::unique_ptr<CCommandQueue> loaded(static_cast<CCommandQueue*>(raw));
	REQUIRE(loaded->size() == owner.queue.size());
	for (size_t i = 0; i < loaded->size(); ++i)
		CheckCommand((*loaded)[i], owner.queue[i]);
	loaded->push_back(Command(CMD_WAIT));
	owner.queue.push_back(Command(CMD_WAIT));
	CheckCommand(loaded->back(), owner.queue.back());
}

TEST_CASE("Save/load preserves shared identity across queues and separate packages")
{
	std::vector<Command> commands;
	for (int i = 0; i < 200; ++i)
		commands.emplace_back(CMD_ATTACK, SHIFT_KEY, float(i));
	QueueSaveRoot root;
	for (int i = 0; i < 1000; ++i)
		root.owners.push_back(new CCommandAI);
	{
		SharedAttackBatch batch(commands);
		for (const Command& command: commands)
			for (CCommandAI* owner: root.owners)
				owner->queue.push_back(command);
	}
	root.owners[0]->queue.front(); // materialize; its provenance must survive loading
	root.owners[1]->queue.erase(root.owners[1]->queue.begin() + 5);
	root.owners[1]->queue.Edit(7).SetTimeOut(15);
	root.owners.push_back(new CCommandAI);
	{
		SharedAttackBatch another(commands); // equal contents are a distinct list
		for (const Command& command: commands)
			root.owners.back()->queue.push_back(command);
	}

	for (int iteration = 0; iteration < 2; ++iteration) {
		std::stringstream stream;
		creg::COutputStreamSerializer output;
		output.SavePackage(&stream, &root, root.GetClass());
		// An expanded 200,000-command save would take many megabytes.
		CHECK(stream.str().size() < 200000);
		void* raw;
		creg::Class* type;
		creg::CInputStreamSerializer input;
		input.LoadPackage(&stream, raw, type);
		REQUIRE(type == root.GetClass());
		std::unique_ptr<QueueSaveRoot> loaded(static_cast<QueueSaveRoot*>(raw));
		REQUIRE(loaded->owners.size() == root.owners.size());
		const auto* common = loaded->owners[0]->List();
		REQUIRE(common != nullptr);
		CHECK(common != root.owners[0]->List());
		for (size_t i = 0; i < root.owners.size(); ++i) {
			const auto* actual = loaded->owners[i];
			const auto* expected = root.owners[i];
			REQUIRE(actual->queue.size() == expected->queue.size());
			CHECK(actual->Owned() == expected->Owned());
			CHECK(actual->Segments() == expected->Segments());
			CHECK((actual->List() == common) == (i < 1000));
			CHECK(actual->queue.FindLastAttackTarget(199) == actual->queue.end() - 1);
			for (size_t j = 0; j < actual->queue.size(); ++j)
				CheckCommand(actual->queue[j], expected->queue[j]);
		}
		loaded->owners[0]->queue.RepeatFront();
		loaded->owners[0]->queue.pop_front();
		CHECK(loaded->owners[0]->Owned() == 0);
		CHECK(loaded->owners[0]->List(199) == common);
		loaded->owners[0]->queue.clear();
		CHECK(loaded->owners[2]->queue[0].GetParam(0) == 0);
	}
}

TEST_CASE("Repeat retains sharing, wraps tags, and respects private modifications")
{
	CCommandAI owner;
	std::vector<Command> commands;
	for (int i = 0; i < 4; ++i)
		commands.emplace_back(CMD_ATTACK, SHIFT_KEY, float(i));
	owner.SetTagCounter(CCommandQueue::maxTagValue - 3);
	{
		SharedAttackBatch batch(commands);
		for (const auto& command: commands) owner.queue.push_back(command);
	}
	const auto* identity = owner.List();
	unsigned nextTag = 2;
	for (int i = 0; i < 2000; ++i) {
		Command& active = owner.queue.front();
		const Command before = active;
		owner.queue.RepeatFront();
		CHECK(&active == &owner.queue.front());
		owner.queue.pop_front();
		Command expected = before;
		expected.SetTag(++nextTag);
		CheckCommand(owner.queue[3], expected);
		CHECK(owner.Owned() == 0);
		CHECK(owner.Segments() <= 2);
		CHECK(owner.List(3) == identity);
	}
	owner.queue.front().SetTimeOut(42);
	owner.queue.RepeatFront();
	owner.queue.pop_front();
	CHECK(owner.queue[3].GetTimeOut() == 42);
	CHECK(owner.List(3) == nullptr);
	CHECK(owner.Owned() == 1);
}

TEST_CASE("Indexed attack cancellation matches reverse scan after mutations")
{
	CCommandAI owner;
	auto& queue = owner.queue;
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float inf = std::numeric_limits<float>::infinity();
	std::vector<Command> commands;
	for (int i = 0; i < 100; ++i)
		commands.emplace_back(CMD_ATTACK, SHIFT_KEY, float(i % 9));
	commands.emplace_back(CMD_ATTACK, 0, -0.0f);
	commands.emplace_back(CMD_ATTACK, 0, nan);
	commands.emplace_back(CMD_ATTACK, 0, inf);
	{
		SharedAttackBatch batch(commands);
		for (const auto& c: commands) queue.push_back(c);
	}
	std::mt19937 random(8935);
	for (int step = 0; step < 500; ++step) {
		for (float target: {0.0f, 1.0f, 5.0f, 8.0f, 9.0f, -1.0f, nan, inf}) {
			auto expected = queue.end();
			for (auto it = queue.end(); it != queue.begin();) {
				const Command c = *--it;
				if ((c.GetID() == CMD_ATTACK || c.GetID() == CMD_FIGHT) && c.GetNumParams() == 1 && c.GetParam(0) == target) {
					expected = it;
					break;
				}
			}
			CHECK(queue.FindLastAttackTarget(target) == expected);
		}
		switch (random() % 5) {
			case 0: queue.Edit(random() % queue.size()).SetParam(0, float(random() % 9)); break;
			case 1: queue.RepeatFront(); queue.pop_front(); break;
			case 2: if (queue.size() > 1) queue.erase(queue.begin() + random() % queue.size()); break;
			case 3: queue.insert(queue.begin() + random() % queue.size(), Command(CMD_FIGHT, 0, float(random() % 9))); break;
			case 4: queue.insert(queue.begin(), Command(CMD_MOVE, 0, float(random() % 9))); break;
		}
	}
}
