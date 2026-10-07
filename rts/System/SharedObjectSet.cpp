/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SharedObjectSet.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include "Object.h"

class SharedObjectSetNode {
	CR_DECLARE_STRUCT(SharedObjectSetNode)
public:
	using Ptr = std::shared_ptr<const SharedObjectSetNode>;
	SharedObjectSetNode() = default;
	SharedObjectSetNode(CObject* value, int64_t key, Ptr left, Ptr right):
		value(value), key(key), left(std::move(left)), right(std::move(right)),
		height(1 + std::max(Height(this->left), Height(this->right))),
		size(1 + Size(this->left) + Size(this->right)) {}

	static unsigned Height(const Ptr& node) { return node ? node->height : 0; }
	static size_t Size(const Ptr& node) { return node ? node->size : 0; }
	static Ptr Make(CObject* value, int64_t key, Ptr left, Ptr right) {
		auto result = Ptr(new SharedObjectSetNode(value, key, std::move(left), std::move(right)));
		result->owner = result;
		return result;
	}
	static Ptr Balance(CObject* value, int64_t key, Ptr left, Ptr right) {
		if (Height(left) > Height(right) + 1) {
			if (Height(left->left) >= Height(left->right))
				return Make(left->value, left->key, left->left, Make(value, key, left->right, right));
			const auto& mid = left->right;
			return Make(mid->value, mid->key, Make(left->value, left->key, left->left, mid->left), Make(value, key, mid->right, right));
		}
		if (Height(right) > Height(left) + 1) {
			if (Height(right->right) >= Height(right->left))
				return Make(right->value, right->key, Make(value, key, left, right->left), right->right);
			const auto& mid = right->left;
			return Make(mid->value, mid->key, Make(value, key, left, mid->left), Make(right->value, right->key, mid->right, right->right));
		}
		return Make(value, key, std::move(left), std::move(right));
	}
	static Ptr Change(const Ptr& node, CObject* object, int64_t key, bool erase) {
		if (!node) return erase ? nullptr : Make(object, key, {}, {});
		if (key < node->key) {
			auto left = Change(node->left, object, key, erase);
			if (left == node->left) return node;
			return Balance(node->value, node->key, std::move(left), node->right);
		}
		if (key > node->key) {
			auto right = Change(node->right, object, key, erase);
			if (right == node->right) return node;
			return Balance(node->value, node->key, node->left, std::move(right));
		}
		if (!erase) return node;
		if (!node->left) return node->right;
		if (!node->right) return node->left;
		auto next = node->right;
		while (next->left) next = next->left;
		return Balance(next->value, next->key, node->left, Change(node->right, next->value, next->key, true));
	}
	static void Visit(const Ptr& node, const std::function<void(CObject*)>& visitor) {
		if (!node) return;
		Visit(node->left, visitor);
		visitor(node->value);
		Visit(node->right, visitor);
	}
	static void SerializePtr(creg::ISerializer* s, Ptr& pointer) {
#ifdef USING_CREG
		void* raw = const_cast<SharedObjectSetNode*>(pointer.get());
		s->SerializeObjectPtr(&raw, StaticClass());
		if (s->IsWriting()) return;
		if (!raw) { pointer.reset(); return; }
		// Nodes are non-embedded objects: creg allocates them before resolving
		// these references. Fields can be filled later, without tree operations.
		auto* node = static_cast<SharedObjectSetNode*>(raw);
		pointer = node->owner.lock();
		if (!pointer) {
			pointer = Ptr(node);
			node->owner = pointer;
		}
#endif
	}
	void Serialize(creg::ISerializer* s) {
		SerializePtr(s, left);
		SerializePtr(s, right);
	}
	CObject* value = nullptr;
	int64_t key = 0;
	Ptr left, right;
	unsigned height = 1;
	size_t size = 1;
	mutable std::weak_ptr<const SharedObjectSetNode> owner;
};

CR_BIND(SharedObjectSetNode, )
CR_REG_METADATA(SharedObjectSetNode, (
	CR_MEMBER(value), CR_MEMBER(key), CR_MEMBER(height), CR_MEMBER(size),
	CR_IGNORED(left), CR_IGNORED(right), CR_IGNORED(owner), CR_SERIALIZER(Serialize)
))
CR_BIND(SharedObjectSet, )
CR_REG_METADATA(SharedObjectSet, (CR_IGNORED(root), CR_SERIALIZER(Serialize)))

struct SharedObjectSet::Batch::Cache {
	struct Key {
		const SharedObjectSetNode* source;
		int64_t objectID; // never reuse a cached operation for a recycled address
		bool erase;
		bool operator==(const Key&) const = default;
	};
	struct Hash {
		size_t operator()(const Key& key) const {
			return std::hash<const void*>{}(key.source) ^ (std::hash<int64_t>{}(key.objectID) << 1) ^ size_t(key.erase);
		}
	};
	struct Entry { SharedObjectSetNode::Ptr source, result; };
	std::unordered_map<Key, Entry, Hash> transitions;
};

thread_local SharedObjectSet::Batch* SharedObjectSet::Batch::current = nullptr;
SharedObjectSet::Batch::Batch(): cache(std::make_unique<Cache>()), previous(current) { current = this; }
SharedObjectSet::Batch::~Batch() { current = previous; }

bool SharedObjectSet::Mutate(CObject* object, bool erase)
{
	const auto key = object->GetSyncID();
	if (Batch::current == nullptr) {
		auto next = SharedObjectSetNode::Change(root, object, key, erase);
		const bool changed = next != root;
		root = std::move(next);
		return changed;
	}
	auto& cache = Batch::current->cache->transitions;
	const Batch::Cache::Key operation{root.get(), key, erase};
	if (const auto found = cache.find(operation); found != cache.end()) {
		const bool changed = found->second.result != root;
		root = found->second.result;
		return changed;
	}
	auto next = SharedObjectSetNode::Change(root, object, key, erase);
	// Bound retained historical roots for highly divergent or enormous batches.
	// Eviction affects sharing only, never logical membership or iteration.
	if (cache.size() >= 32768) cache.clear();
	cache.emplace(operation, Batch::Cache::Entry{root, next});
	const bool changed = next != root;
	root = std::move(next);
	return changed;
}

bool SharedObjectSet::Insert(CObject* object) { return Mutate(object, false); }
bool SharedObjectSet::Erase(CObject* object) { return Mutate(object, true); }
bool SharedObjectSet::Contains(const CObject* object) const
{
	const auto key = object->GetSyncID();
	for (auto node = root; node;) {
		if (node->key == key) return true;
		node = (key < node->key) ? node->left : node->right;
	}
	return false;
}
CObject* SharedObjectSet::First() const
{
	assert(root);
	auto node = root;
	while (node->left) node = node->left;
	return node->value;
}
size_t SharedObjectSet::Size() const { return SharedObjectSetNode::Size(root); }
void SharedObjectSet::Visit(const std::function<void(CObject*)>& visitor) const
{
	// Keep the traversed version alive across reentrant callbacks.
	const auto snapshot = root;
	SharedObjectSetNode::Visit(snapshot, visitor);
}
void SharedObjectSet::Serialize(creg::ISerializer* s) { SharedObjectSetNode::SerializePtr(s, root); }
