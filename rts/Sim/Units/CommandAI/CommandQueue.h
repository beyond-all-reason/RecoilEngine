/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _COMMAND_QUEUE_H
#define _COMMAND_QUEUE_H

#include <compare>
#include <iterator>
#include <stdexcept>
#include "CommandQueueStorage.h"

/// Logical commands backed by private entries and shared attack ranges.
class CCommandQueue {

	friend class CCommandAI;
	friend class CFactoryCAI;

	// see CommandQueue.cpp for serialization
	CR_DECLARE_STRUCT(CCommandQueue)

	public:
		enum QueueType {
			CommandQueueType,
			NewUnitQueueType,
			BuildQueueType
		};

		inline QueueType GetType() const { return queueType; }

	public:
		/// limit to a float's integer range
		static const int maxTagValue = (1 << 24); // 16777216

		// Iterators read logical values. A returned Command owns its parameters;
		// no scratch reference can escape or be invalidated by another read.
		class const_iterator {
		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type = Command;
			using difference_type = std::ptrdiff_t;
			using reference = const Command;
			struct pointer {
				Command command;
				const Command* operator->() const { return &command; }
			};
			const_iterator() = default;
			const_iterator(const CCommandQueue* owner, difference_type position): owner(owner), position(position) {}
			const Command operator*() const { return owner->Read(position); }
			pointer operator->() const { return {operator*()}; }
			const Command operator[](difference_type offset) const { return *(*this + offset); }
			const_iterator& operator++() { ++position; return *this; }
			const_iterator& operator--() { --position; return *this; }
			const_iterator operator++(int) { auto old = *this; ++*this; return old; }
			const_iterator operator--(int) { auto old = *this; --*this; return old; }
			const_iterator& operator+=(difference_type offset) { position += offset; return *this; }
			const_iterator& operator-=(difference_type offset) { position -= offset; return *this; }
			friend const_iterator operator+(const_iterator it, difference_type offset) { return it += offset; }
			friend const_iterator operator+(difference_type offset, const_iterator it) { return it += offset; }
			friend const_iterator operator-(const_iterator it, difference_type offset) { return it -= offset; }
			friend difference_type operator-(const_iterator a, const_iterator b) { assert(a.owner == b.owner); return a.position - b.position; }
			bool operator==(const const_iterator&) const = default;
			auto operator<=>(const const_iterator& other) const { assert(owner == other.owner); return position <=> other.position; }
		private:
			const CCommandQueue* owner = nullptr;
			difference_type position = 0;
		};
		using size_type = size_t;
		using iterator = const_iterator;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
		using reverse_iterator = const_reverse_iterator;

		inline bool empty() const { return storage.empty(); }

		inline size_type size() const { return storage.size(); }

		inline void push_back(const Command& cmd);
		inline void push_front(const Command& cmd);

		void emplace_back(Command&& cmd) {
			push_back(cmd);
		}
		void emplace_front(Command&& cmd) {
			push_front(cmd);
		}

		inline iterator insert(iterator pos, const Command& cmd);

		inline void pop_back()
		{
			storage.Erase(size() - 1, size());
		}
		inline void pop_front()
		{
			storage.Erase(0, 1);
		}

		inline iterator erase(iterator pos)
		{
			return erase(pos, pos + 1);
		}
		inline iterator erase(iterator first, iterator last)
		{
			const auto index = first - begin();
			storage.Erase(index, last - begin());
			return begin() + index;
		}
		inline void clear()
		{
			storage.clear();
		}

		inline iterator       end()         { return {this, static_cast<std::ptrdiff_t>(size())}; }
		inline const_iterator end()   const { return {this, static_cast<std::ptrdiff_t>(size())}; }
		inline iterator       begin()       { return {this, 0}; }
		inline const_iterator begin() const { return {this, 0}; }

		inline reverse_iterator       rend()         { return reverse_iterator(begin()); }
		inline const_reverse_iterator rend()   const { return reverse_iterator(begin()); }
		inline reverse_iterator       rbegin()       { return reverse_iterator(end()); }
		inline const_reverse_iterator rbegin() const { return reverse_iterator(end()); }

		inline       Command& back()        { return Edit(size() - 1); }
		inline       Command back()   const { return Read(size() - 1); }
		inline       Command& front()       { return Edit(0); }
		inline       Command front()  const { return Read(0); }

		inline const Command at(size_type i) const {
			if (i >= size()) throw std::out_of_range("command queue");
			return Read(i);
		}
		inline const Command operator[](size_type i) const { return Read(i); }

		Command Read(size_type i) const { return storage.Read(i); }
		Command& Edit(size_type i) { return storage.Edit(i); }
		iterator FindLastAttackTarget(float target) const { return begin() + storage.FindLastAttackTarget(target); }
		void RepeatFront() { storage.AppendCopy(0, GetNextTag()); }
		template<typename Predicate> void RemoveIf(Predicate predicate) {
			for (size_type i = 0; i < size();) {
				if (predicate(Read(i))) storage.Erase(i, i + 1);
				else ++i;
			}
		}
		void Serialize(creg::ISerializer* serializer);

	private:
		CCommandQueue() : queueType(CommandQueueType), tagCounter(0) {};
		CCommandQueue(const CCommandQueue&);
		CCommandQueue& operator=(const CCommandQueue&);

	private:
		inline int GetNextTag();
		inline void SetQueueType(QueueType type) { queueType = type; }

	private:
		CommandQueueStorage storage;
		QueueType queueType;
		int tagCounter;
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
	const unsigned tag = GetNextTag();
	size_t index;
	if (auto list = SharedAttackBatch::Find(cmd, index); list && queueType == CommandQueueType) {
		storage.AppendShared(std::move(list), index, tag);
		return;
	}
	Command copy = cmd;
	copy.SetTag(tag);
	storage.Insert(size(), copy);
}


inline void CCommandQueue::push_front(const Command& cmd)
{
	insert(begin(), cmd);
}


inline CCommandQueue::iterator CCommandQueue::insert(iterator pos, const Command& cmd)
{
	Command tmpCmd = cmd;
	tmpCmd.SetTag(GetNextTag());
	const auto index = pos - begin();
	storage.Insert(index, tmpCmd);
	return begin() + index;
}


#endif // _COMMAND_QUEUE_H
