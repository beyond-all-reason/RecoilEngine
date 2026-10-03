/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _COMMAND_QUEUE_H
#define _COMMAND_QUEUE_H

#include <deque>
#include "Command.h"

/// A wrapper class for std::deque<Command> to keep track of commands
class CCommandQueue {

	friend class CCommandAI;
	friend class CFactoryCAI;

	// see CommandAI.cpp for further creg stuff for this class
	CR_DECLARE_STRUCT(CCommandQueue)

	public:
		enum QueueType {
			CommandQueueType,
			NewUnitQueueType,
			BuildQueueType
		};

		inline QueueType GetType() const { return queueType; }

		inline bool GetRepeat() const { return repeat; }
		inline void SetRepeat(bool b) { repeat = b; }

		// state handed to the units this queue's commands are given to, -1 inherits the owner's
		inline int GetFireState() const { return fireState; }
		inline void SetFireState(int s) { fireState = s; }

	public:
		/// limit to a float's integer range
		static const int maxTagValue = (1 << 24); // 16777216

		typedef std::deque<Command> basis;

		typedef basis::size_type              size_type;
		typedef basis::iterator               iterator;
		typedef basis::const_iterator         const_iterator;
		typedef basis::reverse_iterator       reverse_iterator;
		typedef basis::const_reverse_iterator const_reverse_iterator;

		inline bool empty() const { return queue.empty(); }

		inline size_type size() const { return queue.size(); }

		inline void push_back(const Command& cmd);
		inline void push_front(const Command& cmd);

		void emplace_back(Command&& cmd) {
			queue.emplace_back(cmd);
			queue.back().SetTag(GetNextTag());
			queue.back().SetQueue(0);
		}
		void emplace_front(Command&& cmd) {
			queue.emplace_front(cmd);
			queue.front().SetTag(GetNextTag());
			queue.front().SetQueue(0);
		}

		inline iterator insert(iterator pos, const Command& cmd);

		inline void pop_back()
		{
			queue.pop_back();
		}
		inline void pop_front()
		{
			queue.pop_front();
		}

		inline iterator erase(iterator pos)
		{
			return queue.erase(pos);
		}
		inline iterator erase(iterator first, iterator last)
		{
			return queue.erase(first, last);
		}
		inline void clear()
		{
			queue.clear();
		}

		inline iterator       end()         { return queue.end(); }
		inline const_iterator end()   const { return queue.end(); }
		inline iterator       begin()       { return queue.begin(); }
		inline const_iterator begin() const { return queue.begin(); }

		inline reverse_iterator       rend()         { return queue.rend(); }
		inline const_reverse_iterator rend()   const { return queue.rend(); }
		inline reverse_iterator       rbegin()       { return queue.rbegin(); }
		inline const_reverse_iterator rbegin() const { return queue.rbegin(); }

		inline       Command& back()        { return queue.back(); }
		inline const Command& back()  const { return queue.back(); }
		inline       Command& front()       { return queue.front(); }
		inline const Command& front() const { return queue.front(); }

		inline       Command& at(size_type i)       { return queue.at(i); }
		inline const Command& at(size_type i) const { return queue.at(i); }

		inline       Command& operator[](size_type i)       { return queue[i]; }
		inline const Command& operator[](size_type i) const { return queue[i]; }

	public:
		explicit CCommandQueue(QueueType type = CommandQueueType) : queueType(type), tagCounter(0) {};
		CCommandQueue(const CCommandQueue&) = delete;
		CCommandQueue(CCommandQueue&&) = default;
		CCommandQueue& operator=(const CCommandQueue&) = delete;
		CCommandQueue& operator=(CCommandQueue&&) = default;

	private:
		inline int GetNextTag();
		inline void SetQueueType(QueueType type) { queueType = type; }

	private:
		std::deque<Command> queue;
		QueueType queueType;
		int tagCounter;

		bool repeat = false;
		int fireState = -1;
};


inline int CCommandQueue::GetNextTag()
{
	tagCounter++;
	if (tagCounter >= maxTagValue)
		tagCounter = 1;

	return tagCounter;
}


inline void CCommandQueue::push_back(const Command& cmd)
{
	queue.push_back(cmd);
	queue.back().SetTag(GetNextTag());
	queue.back().SetQueue(0);
}


inline void CCommandQueue::push_front(const Command& cmd)
{
	queue.push_front(cmd);
	queue.front().SetTag(GetNextTag());
	queue.front().SetQueue(0);
}


inline CCommandQueue::iterator CCommandQueue::insert(iterator pos, const Command& cmd)
{
	Command tmpCmd = cmd;
	tmpCmd.SetTag(GetNextTag());
	// the target only means something to the owner that routed it here
	tmpCmd.SetQueue(0);
	return queue.insert(pos, tmpCmd);
}


#endif // _COMMAND_QUEUE_H
