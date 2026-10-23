/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <functional>
#include <memory>
#include "System/creg/creg_cond.h"

class CObject;
class SharedObjectSetNode;

// Persistent ordered sets share identical mutation histories during a batch.
// Iteration remains by CObject creation order; there is no notification proxy.
class SharedObjectSet {
	CR_DECLARE_STRUCT(SharedObjectSet)
public:
	class Batch {
	public:
		Batch();
		~Batch();
		Batch(const Batch&) = delete;
		Batch& operator=(const Batch&) = delete;
	private:
		friend class SharedObjectSet;
		struct Cache;
		std::unique_ptr<Cache> cache;
		Batch* previous;
		static thread_local Batch* current;
	};

	bool Insert(CObject* object);
	bool Erase(CObject* object);
	bool Contains(const CObject* object) const;
	bool Empty() const { return root == nullptr; }
	CObject* First() const;
	size_t Size() const;
	const void* RootIdentity() const { return root.get(); }
	void Visit(const std::function<void(CObject*)>& visitor) const;
	void Serialize(creg::ISerializer* serializer);

private:
	bool Mutate(CObject* object, bool erase);
	std::shared_ptr<const SharedObjectSetNode> root;
};
