/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <memory>
#include <algorithm>
#include <map>
#include <vector>
#include "Command.h"

// A separately registered creg object preserves list identity across queues.
// Commands stay immutable; progress and tags belong to each queue range.
class SharedAttackList {
	CR_DECLARE_STRUCT(SharedAttackList)
public:
	using Ptr = std::shared_ptr<const SharedAttackList>;
	static Ptr Create(std::vector<Command> commands) {
		auto result = Ptr(new SharedAttackList(std::move(commands)));
		result->owner = result;
		return result;
	}

	const Command& operator[](size_t index) const { return commands[index]; }
	size_t size() const { return commands.size(); }

	// Index only immutable inputs. Build lazily so loaded lists have their
	// commands populated before indexing; no queue-local state is cached.
	size_t FindLastTarget(float target, size_t first, size_t length) const {
		if (target != target) return commands.size(); // NaN never compares equal
		if (!indexed) {
			for (size_t i = 0; i < commands.size(); ++i) {
				const float value = commands[i].GetParam(0);
				if (value == value) targetPositions[value].push_back(i);
			}
			indexed = true;
		}
		const auto found = targetPositions.find(target);
		if (found == targetPositions.end()) return commands.size();
		const auto& positions = found->second;
		auto end = std::lower_bound(positions.begin(), positions.end(), first + length);
		if (end == positions.begin() || *--end < first) return commands.size();
		return *end;
	}

private:
	friend class CommandQueueStorage;
	SharedAttackList() = default;
	explicit SharedAttackList(std::vector<Command> commands): commands(std::move(commands)) {}
	SharedAttackList(const SharedAttackList&) = delete;
	SharedAttackList& operator=(const SharedAttackList&) = delete;

	// creg constructs a non-embedded list before deserializing queues. Adopt it
	// once, then reuse that control block for all references in the package.
	Ptr RetainLoaded() const {
		if (auto result = owner.lock())
			return result;
		auto result = Ptr(this);
		owner = result;
		return result;
	}

	std::vector<Command> commands;
	mutable std::weak_ptr<const SharedAttackList> owner;
	mutable std::map<float, std::vector<size_t>> targetPositions;
	mutable bool indexed = false;
};
