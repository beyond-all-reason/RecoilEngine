/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdlib>
#include <deque>
#include <memory>
#include <vector>
#include "SharedAttackBatch.h"

// Ordinary entries own a stable Command. Shared entries describe a range of
// immutable attacks plus the first queue-local tag. Reading never materializes
// a range; mutable access materializes only the requested command.
class CommandQueueStorage {
public:
	static constexpr unsigned TAG_LIMIT = 1 << 24;
	static unsigned AdvanceTag(unsigned tag, size_t count) {
		return 1 + (tag - 1 + count) % (TAG_LIMIT - 1);
	}

	size_t size() const { return count; }
	bool empty() const { return count == 0; }
	// Match the last one-parameter ATTACK or FIGHT, including private edits.
	size_t FindLastAttackTarget(float target) const {
		size_t start = count;
		for (auto it = segments.rbegin(); it != segments.rend(); ++it) {
			const auto& segment = *it;
			start -= segment.length;
			if (segment.owned) {
				for (size_t i = segment.length; i != 0; --i) {
					const Command& c = (*segment.owned)[i - 1];
					if ((c.GetID() == CMD_ATTACK || c.GetID() == CMD_FIGHT) && c.GetNumParams() == 1 && c.GetParam(0) == target)
						return start + i - 1;
				}
			} else {
				const size_t index = segment.list->FindLastTarget(target, segment.first, segment.length);
				if (index != segment.list->size()) return start + index - segment.first;
			}
		}
		return count;
	}
	void Serialize(creg::ISerializer* serializer);
	size_t SegmentCount() const { return segments.size(); }
	const SharedAttackList* SharedList(size_t index) const {
		for (const auto& segment: segments) {
			if (index < segment.length) return segment.list.get();
			index -= segment.length;
		}
		return nullptr;
	}
	size_t OwnedCommandCount() const {
		size_t result = 0;
		for (const auto& segment: segments)
			if (segment.owned) result += segment.length;
		return result;
	}

	Command Read(size_t index) const {
		assert(index < count);
		for (const auto& segment: segments) {
			if (index >= segment.length) {
				index -= segment.length;
				continue;
			}
			if (segment.owned)
				return (*segment.owned)[index];
			Command result = (*segment.list)[segment.first + index];
			result.SetTag(AdvanceTag(segment.tag, index));
			return result;
		}
		std::abort();
	}

	Command& Edit(size_t index) {
		assert(index < count);
		size_t local = index;
		for (auto& segment: segments) {
			if (local >= segment.length) { local -= segment.length; continue; }
			if (segment.owned) return (*segment.owned)[local];
			break;
		}
		// Split may relocate descriptors, but never an owned command deque.
		const size_t at = Split(index);
		Split(index + 1);
		auto& segment = segments[at];
		if (!segment.owned) {
			segment.owned = std::make_unique<std::deque<Command>>();
			segment.owned->push_back((*segment.list)[segment.first]);
			segment.owned->back().SetTag(segment.tag);
			// Keep provenance for Repeat. Edit callers may change the private
			// command; AppendCopy checks it still matches before sharing again.
		}
		return segment.owned->front();
	}

	void AppendShared(SharedAttackBatch::List list, size_t index, unsigned tag) {
		assert(list && index < list->size());
		if (!segments.empty()) {
			auto& back = segments.back();
			if (!back.owned && back.list == list && back.first + back.length == index && AdvanceTag(back.tag, back.length) == tag) {
				++back.length;
				++count;
				return;
			}
		}
		segments.push_back({std::move(list), nullptr, index, 1, tag});
		++count;
	}

	void AppendCopy(size_t index, unsigned tag) {
		Command copy = Read(index);
		size_t local = index;
		for (const auto& segment: segments) {
			if (local >= segment.length) { local -= segment.length; continue; }
			if (segment.list) {
				const size_t sourceIndex = segment.first + local;
				const Command& original = (*segment.list)[sourceIndex];
				if (copy.GetID() == CMD_ATTACK && copy.GetNumParams() == 1 &&
					std::bit_cast<uint32_t>(copy.GetParam(0)) == std::bit_cast<uint32_t>(original.GetParam(0)) && copy.GetOpts() == original.GetOpts() &&
					copy.GetID(true) == original.GetID(true) && copy.GetTimeOut() == original.GetTimeOut()) {
					AppendShared(segment.list, sourceIndex, tag);
					return;
				}
			}
			break;
		}
		copy.SetTag(tag);
		Insert(size(), copy);
	}

	void Insert(size_t index, const Command& command) {
		assert(index <= count);
		Command copy = command; // the source may refer to this queue
		size_t local = index;
		for (auto& segment: segments) {
			if (local > segment.length) { local -= segment.length; continue; }
			if (segment.owned && !segment.list) {
				segment.owned->insert(segment.owned->begin() + local, copy);
				++segment.length;
				++count;
				return;
			}
			break;
		}
		auto owned = std::make_unique<std::deque<Command>>();
		owned->push_back(copy);
		const size_t at = Split(index);
		segments.insert(segments.begin() + at, Segment{{}, std::move(owned), 0, 1, 0});
		++count;
	}

	void Erase(size_t first, size_t last) {
		assert(first <= last && last <= count);
		size_t remaining = last - first;
		count -= remaining;
		for (size_t i = 0; remaining != 0;) {
			auto& segment = segments[i];
			if (first >= segment.length) { first -= segment.length; ++i; continue; }
			const size_t removed = std::min(remaining, segment.length - first);
			remaining -= removed;
			if (removed == segment.length) {
				segments.erase(segments.begin() + i);
			} else if (segment.owned) {
				segment.owned->erase(segment.owned->begin() + first, segment.owned->begin() + first + removed);
				segment.length -= removed;
				++i;
			} else if (first == 0) {
				segment.first += removed;
				segment.tag = AdvanceTag(segment.tag, removed);
				segment.length -= removed;
				++i;
			} else {
				const size_t end = first + removed;
				Segment tail{segment.list, nullptr, segment.first + end, segment.length - end, AdvanceTag(segment.tag, end)};
				segment.length = first;
				if (tail.length != 0)
					segments.insert(segments.begin() + i + 1, std::move(tail));
				++i;
			}
			first = 0;
		}
	}
	void clear() { segments.clear(); count = 0; }

private:
	struct Segment {
		SharedAttackBatch::List list;
		std::unique_ptr<std::deque<Command>> owned;
		size_t first;
		size_t length;
		unsigned tag;
	};

	// Establish a segment boundary before the logical index, preserving tags.
	size_t Split(size_t index) {
		assert(index <= count);
		for (size_t i = 0; i < segments.size(); ++i) {
			auto& segment = segments[i];
			if (index == 0)
				return i;
			if (index >= segment.length) {
				index -= segment.length;
				continue;
			}
			assert(segment.list);
			Segment tail{segment.list, nullptr, segment.first + index, segment.length - index, AdvanceTag(segment.tag, index)};
			segment.length = index;
			segments.insert(segments.begin() + i + 1, std::move(tail));
			return i + 1;
		}
		return segments.size();
	}

	std::vector<Segment> segments;
	size_t count = 0;
};
