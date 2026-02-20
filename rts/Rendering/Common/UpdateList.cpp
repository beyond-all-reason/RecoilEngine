#include "UpdateList.h"

CR_BIND(UpdateList, )
CR_REG_METADATA(UpdateList, (
	CR_MEMBER(TypedTrue),
	CR_MEMBER(updateList),
	CR_MEMBER(changed)
))

void UpdateList::SetNeedUpdateAll()
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::fill(updateList.begin(), updateList.end(), TypedTrue);
	changed = true;
}

void UpdateList::ResetNeedUpdateAll()
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::fill(updateList.begin(), updateList.end(), TypedFalse);
	changed = false;
}

void UpdateList::CalcNeedUpdateAll()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!changed)
		return;

	changed = false;
	for (auto updateListItem : updateList) {
		changed |= (updateListItem != TypedFalse);
	}
}

void UpdateList::Trim(size_t newLessThanOrEqualSize)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(newLessThanOrEqualSize <= updateList.size());
	updateList.resize(newLessThanOrEqualSize);
	// no need to modify the update status
}

void UpdateList::SetUpdate(size_t first, size_t count)
{
	RECOIL_DETAILED_TRACY_ZONE;

	auto beg = updateList.begin() + first;
	auto end = beg + count;

	SetUpdate(IteratorPair(beg, end));
}

void UpdateList::SetUpdate(const IteratorPair& it)
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::fill(it.first, it.second, TypedTrue);
	changed = true;
}

void UpdateList::SetUpdate(size_t offset)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(offset < updateList.size());
	updateList[offset] = TypedTrue;
	changed = true;
}

void UpdateList::DecrementUpdate(size_t first, size_t count)
{
	RECOIL_DETAILED_TRACY_ZONE;

	auto beg = updateList.begin() + first;
	auto end = beg + count;

	DecrementUpdate(IteratorPair(beg, end));
}

void UpdateList::DecrementUpdate(IteratorPair itPair)
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (auto it = itPair.first; it != itPair.second; ++it) {
		*it = spring::SafeCast<ValueT>(*it - ValueT{ 1 });
		changed |= (*it != TypedFalse);
	}
}

void UpdateList::DecrementUpdate(size_t offset)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(offset < updateList.size());
	auto& val = updateList[offset];

	val = spring::SafeCast<ValueT>(val - ValueT{ 1 });
	changed |= (val != TypedFalse);
}

void UpdateList::EmplaceBackUpdate()
{
	RECOIL_DETAILED_TRACY_ZONE;
	updateList.emplace_back(TypedTrue);
	changed = true;
}

void UpdateList::PopBack()
{
	updateList.pop_back();
	changed = true;
}

void UpdateList::PopBack(size_t N)
{
	while (N-- >= 0)
		updateList.pop_back();

	changed = true;
}

std::optional<UpdateList::ConstIteratorPair> UpdateList::GetNext(const std::optional<ConstIteratorPair>& prev) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const static auto Pred = [](ValueT v) { return (v == TypedFalse); };

	auto beg = prev.has_value() ? prev.value().second : updateList.begin();
	beg = std::find_if_not(beg, updateList.end(), Pred);
	auto end = std::find_if(beg, updateList.end(), Pred);

	if (beg == end)
		return std::nullopt;

	return std::make_optional(std::make_pair(beg, end));
}

std::optional<UpdateList::IteratorPair> UpdateList::GetNext(const std::optional<IteratorPair>& prev)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const static auto Pred = [](ValueT v) { return (v == TypedFalse); };

	auto beg = prev.has_value() ? prev.value().second : updateList.begin();
	beg = std::find_if_not(beg, updateList.end(), Pred);
	auto end = std::find_if(beg, updateList.end(), Pred);

	if (beg == end)
		return std::nullopt;

	return std::make_optional(std::make_pair(beg, end));
}

std::pair<size_t, size_t> UpdateList::GetOffsetAndSize(const ConstIteratorPair& it) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return std::make_pair(
		std::distance(updateList.begin(), it.first),
		std::distance(it.first, it.second)
	);
}

std::pair<size_t, size_t> UpdateList::GetOffsetAndSize(const IteratorPair& it)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return std::make_pair(
		std::distance(updateList.begin(), it.first),
		std::distance(it.first, it.second)
	);
}