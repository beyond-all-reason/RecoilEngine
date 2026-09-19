/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CommandQueue.h"

#include <algorithm>
#include <cassert>
#include <vector>

#include "Sim/Units/Unit.h"
#include "System/EventHandler.h"


bool CCommandQueue::WantsAddedEvents() const
{
	assert(owner != nullptr);
	return eventHandler.HasCommandAddedClients();
}

bool CCommandQueue::WantsRemovedEvents() const
{
	assert(owner != nullptr);
	return eventHandler.HasCommandRemovedClients();
}


void CCommandQueue::NotifyAdded(const Command& cmd) const
{
	if (!WantsAddedEvents())
		return;

	// prevent two call-ins interfering with receiving one another's commands
	const Command copy = cmd;

	eventHandler.UnitCommandAdded(owner, copy, queueType);
}

void CCommandQueue::NotifyRemoved(const Command& cmd) const
{
	if (!WantsRemovedEvents())
		return;

	// no copy, unlike NotifyAdded, so the cmd must be removed already from the queue

	eventHandler.UnitCommandRemoved(owner, cmd, queueType);
}


CCommandQueue::iterator CCommandQueue::ClampedIterator(size_type idx)
{
	return (queue.begin() + std::min(idx, queue.size()));
}


void CCommandQueue::push_back(const Command& cmd)
{
	queue.push_back(cmd);
	queue.back().SetTag(GetNextTag());

	NotifyAdded(queue.back());
}

void CCommandQueue::push_front(const Command& cmd)
{
	queue.push_front(cmd);
	queue.front().SetTag(GetNextTag());

	NotifyAdded(queue.front());
}


void CCommandQueue::emplace_back(Command&& cmd)
{
	queue.emplace_back(cmd);
	queue.back().SetTag(GetNextTag());

	NotifyAdded(queue.back());
}

void CCommandQueue::emplace_front(Command&& cmd)
{
	queue.emplace_front(cmd);
	queue.front().SetTag(GetNextTag());

	NotifyAdded(queue.front());
}


CCommandQueue::iterator CCommandQueue::insert(iterator pos, const Command& cmd)
{
	Command tmpCmd = cmd;
	tmpCmd.SetTag(GetNextTag());

	const size_type idx = static_cast<size_type>(pos - queue.begin());
	const iterator it = queue.insert(pos, tmpCmd);

	if (!WantsAddedEvents())
		return it;

	NotifyAdded(queue[idx]);
	return ClampedIterator(idx);
}


void CCommandQueue::pop_back()
{
	// added safety for multiple lua callins interfering destructively
	if (queue.empty())
		return;

	if (!WantsRemovedEvents()) {
		queue.pop_back();
		return;
	}

	const Command cmd = queue.back();

	queue.pop_back();
	NotifyRemoved(cmd);
}

void CCommandQueue::pop_front()
{
	if (queue.empty())
		return;

	if (!WantsRemovedEvents()) {
		queue.pop_front();
		return;
	}

	const Command cmd = queue.front();

	queue.pop_front();
	NotifyRemoved(cmd);
}


CCommandQueue::iterator CCommandQueue::erase(iterator pos)
{
	if (!WantsRemovedEvents())
		return queue.erase(pos);

	const size_type idx = static_cast<size_type>(pos - queue.begin());
	const Command cmd = *pos;

	queue.erase(pos);
	NotifyRemoved(cmd);

	return ClampedIterator(idx);
}

CCommandQueue::iterator CCommandQueue::erase(iterator first, iterator last)
{
	if (!WantsRemovedEvents())
		return queue.erase(first, last);

	const size_type idx = static_cast<size_type>(first - queue.begin());
	const std::vector<Command> removed(first, last);

	queue.erase(first, last);

	for (const Command& cmd: removed)
		NotifyRemoved(cmd);

	return ClampedIterator(idx);
}


void CCommandQueue::clear()
{
	if (!WantsRemovedEvents()) {
		queue.clear();
		return;
	}

	const std::vector<Command> removed(queue.begin(), queue.end());

	queue.clear();

	for (const Command& cmd: removed)
		NotifyRemoved(cmd);
}
