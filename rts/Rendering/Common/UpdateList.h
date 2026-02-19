#pragma once

#include <cstdint>
#include <tuple>
#include <optional>
#include <vector>
#include <algorithm>

#include "System/SafeUtil.h"
#include "System/creg/creg_cond.h"
#include "System/Misc/TracyDefs.h"

class UpdateList {
	CR_DECLARE_STRUCT(UpdateList)
public:
	using ValueT = uint8_t;
	using ConstIterator = typename std::vector<ValueT>::const_iterator;
	using ConstIteratorPair = std::pair<ConstIterator, ConstIterator>;

	using Iterator = typename std::vector<ValueT>::iterator;
	using IteratorPair = std::pair<Iterator, Iterator>;
public:
	UpdateList()
		: updateList()
		, changed(false)
	{
		SetTrueValue();
	}
	UpdateList(size_t initialSize)
		: updateList(initialSize)
		, changed(initialSize > 0)
	{
		SetTrueValue();
	}

	void SetTrueValue(ValueT val = ValueT{ 1 }) { TypedTrue = val; }

	size_t Size() const { return updateList.size(); }
	size_t Capacity() const { return updateList.capacity(); }
	bool Empty() const { return updateList.empty(); }

	void Trim(size_t newLessThanOrEqualSize);
	void Resize(size_t newSize) { updateList.resize(newSize); SetNeedUpdateAll(); }
	void Reserve(size_t reservedSize) { updateList.reserve(reservedSize); }
	void Clear() { *this = std::move(UpdateList()); }

	void SetUpdate(size_t first, size_t count);
	void SetUpdate(const IteratorPair& it);
	void SetUpdate(size_t offset);

	void DecrementUpdate(size_t first, size_t count);
	void DecrementUpdate(IteratorPair it);
	void DecrementUpdate(size_t offset);

	void SetNeedUpdateAll();
	void ResetNeedUpdateAll();
	void CalcNeedUpdateAll();

	void EmplaceBackUpdate();
	void PopBack();
	void PopBack(size_t N);

	bool NeedUpdate() const { return changed; }

	std::optional<ConstIteratorPair> GetNext(const std::optional<ConstIteratorPair>& prev = std::nullopt) const;
	std::optional<IteratorPair> GetNext(const std::optional<IteratorPair>& prev = std::nullopt);
	std::pair<size_t, size_t> GetOffsetAndSize(const ConstIteratorPair& it) const;
	std::pair<size_t, size_t> GetOffsetAndSize(const IteratorPair& it);
private:
	ValueT TypedTrue;
	static constexpr ValueT TypedFalse = ValueT{ 0 };
	std::vector<ValueT> updateList;
	bool changed;
};
