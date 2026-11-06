/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CommandQueue.h"
#include "System/Exceptions.h"

CR_BIND(SharedAttackList, )
CR_REG_METADATA(SharedAttackList, (
	CR_MEMBER(commands),
	CR_IGNORED(owner),
	CR_IGNORED(targetPositions),
	CR_IGNORED(indexed)
))

CR_BIND(CCommandQueue, )
CR_REG_METADATA(CCommandQueue, (
	CR_IGNORED(storage),
	CR_SERIALIZER(Serialize),
	CR_MEMBER(queueType),
	CR_MEMBER(tagCounter)
))

void CCommandQueue::Serialize(creg::ISerializer* s)
{
	storage.Serialize(s);
}

void CommandQueueStorage::Serialize(creg::ISerializer* s)
{
	// Custom serializer layouts are not covered by creg's member checksum.
	unsigned format = 0x53515132; // SQQ2
	s->SerializeInt(&format, sizeof(format));
	if (format != 0x53515132)
		throw content_error("Unsupported command queue save format");

	size_t numSegments = segments.size();
	s->SerializeInt(&numSegments, sizeof(numSegments));
	if (!s->IsWriting()) {
		clear();
		segments.resize(numSegments);
	}

	for (auto& segment: segments) {
		size_t first = segment.first;
		size_t length = segment.length;
		unsigned tag = segment.tag;
		unsigned char privateCommands = (segment.owned != nullptr);
		s->SerializeInt(&first, sizeof(first));
		s->SerializeInt(&length, sizeof(length));
		s->SerializeInt(&tag, sizeof(tag));
		s->SerializeInt(&privateCommands, sizeof(privateCommands));

		// Lists are always non-embedded creg objects, allocated before any
		// queue is loaded. Their data is filled later by creg; do not inspect
		// the commands here. The reference is already resolved at this point.
		void* rawList = const_cast<SharedAttackList*>(segment.list.get());
		s->SerializeObjectPtr(&rawList, SharedAttackList::StaticClass());
		if (!s->IsWriting()) {
			if (length == 0 || privateCommands > 1 || (!rawList && !privateCommands))
				throw content_error("Invalid command queue range in save");
			segment.first = first;
			segment.length = length;
			segment.tag = tag;
			if (rawList)
				segment.list = static_cast<SharedAttackList*>(rawList)->RetainLoaded();
			if (privateCommands)
				segment.owned = std::make_unique<std::deque<Command>>(length);
			count += length;
		}

		if (privateCommands) {
			for (Command& command: *segment.owned)
				s->SerializeObjectInstance(&command, Command::StaticClass());
		}
	}
}
