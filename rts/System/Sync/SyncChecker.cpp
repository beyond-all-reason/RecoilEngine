/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#ifdef SYNCCHECK

#include "SyncChecker.h"

// This cannot be included in the header file (SyncChecker.h) because include conflicts will occur.
#include "System/Threading/ThreadPool.h"
#include "System/HashSpec.h"

#include <cstring>
#include <vector>


unsigned CSyncChecker::g_checksum;
unsigned CSyncChecker::g_prevChecksum;
int CSyncChecker::inSyncedCode;

void CSyncChecker::NewFrame()
{
	g_checksum = 0xfade1eaf;
#ifdef SYNC_HISTORY
	LogHistory();
#endif // SYNC_HISTORY
}

void CSyncChecker::debugSyncCheckThreading()
{
	assert(ThreadPool::GetThreadNum() == 0);
}

struct alignas(64) ThreadChecksum { unsigned value; };
static std::array<ThreadChecksum, ThreadPool::MAX_THREADS> threadChecksums;

// master compat: per thread, every deferred write as u32 size (TAG_U32 for Sync(uint32_t)) + bytes,
// and the byte range of each work item
struct alignas(64) DeferredLog {
	std::vector<uint8_t> bytes;
	std::vector<std::pair<uint32_t, uint32_t>> items;
};
static std::array<DeferredLog, ThreadPool::MAX_THREADS> deferredLogs;
static constexpr uint32_t TAG_U32 = 0xFFFFFFFFu;

static void LogDeferred(const void* p, uint32_t tag, unsigned size)
{
	std::vector<uint8_t>& bytes = deferredLogs[ThreadPool::GetThreadNum()].bytes;
	const size_t n = bytes.size();

	bytes.resize(n + sizeof(tag) + size);
	std::memcpy(&bytes[n], &tag, sizeof(tag));
	std::memcpy(&bytes[n + sizeof(tag)], p, size);
}

void CSyncChecker::SetDeferred(bool b)
{
	deferred = b;

	if (!b || !masterCompat)
		return;

	for (DeferredLog& log: deferredLogs) {
		log.bytes.clear();
		log.items.clear();
	}
}

void CSyncChecker::ResetThreadChecksum()
{
	const int thread = ThreadPool::GetThreadNum();

	if (masterCompat) {
		DeferredLog& log = deferredLogs[thread];
		log.items.emplace_back(log.bytes.size(), log.bytes.size());
		return;
	}

	threadChecksums[thread].value = 0xfade1eaf;
}

unsigned CSyncChecker::GetThreadChecksum()
{
	const int thread = ThreadPool::GetThreadNum();

	if (masterCompat) {
		DeferredLog& log = deferredLogs[thread];
		log.items.back().second = log.bytes.size();
		return (unsigned(thread) << 24) | unsigned(log.items.size() - 1);
	}

	return threadChecksums[thread].value;
}

void CSyncChecker::ReplayDeferred(unsigned item)
{
	const DeferredLog& log = deferredLogs[item >> 24];
	const auto [beg, end] = log.items[item & 0xFFFFFF];

	for (uint32_t i = beg; i < end; ) {
		uint32_t tag;
		std::memcpy(&tag, &log.bytes[i], sizeof(tag));
		i += sizeof(tag);

		if (tag == TAG_U32) {
			uint32_t val;
			std::memcpy(&val, &log.bytes[i], sizeof(val));
			Sync(val);
			i += sizeof(val);
		} else {
			Sync(&log.bytes[i], tag);
			i += tag;
		}
	}
}

void CSyncChecker::Sync(uint32_t val)
{
	if (deferred) {
		if (masterCompat) {
			LogDeferred(&val, TAG_U32, sizeof(val));
			return;
		}
		unsigned& checksum = threadChecksums[ThreadPool::GetThreadNum()].value;
		checksum = spring::hash_combine(val, checksum);
		return;
	}
#ifdef DEBUG_SYNC_MT_CHECK
	// Sync calls should not be occurring in multi-threaded sections
	debugSyncCheckThreading();
#endif
	g_checksum = spring::hash_combine(val, g_checksum);
	//LOG("[Sync::Checker] chksum=%u\n", g_checksum);

#ifdef SYNC_HISTORY
	LogHistory();
#endif // SYNC_HISTORY
}

void CSyncChecker::Sync(const void* p, unsigned size)
{
	if (deferred) {
		if (masterCompat) {
			LogDeferred(p, size, size);
			return;
		}
		unsigned& checksum = threadChecksums[ThreadPool::GetThreadNum()].value;
		checksum = spring::LiteHash(p, size, checksum);
		return;
	}
#ifdef DEBUG_SYNC_MT_CHECK
	// Sync calls should not be occurring in multi-threaded sections
	debugSyncCheckThreading();
#endif
	// most common cases first, make it easy for compiler to optimize for it
	// simple xor is not enough to detect multiple zeroes, e.g.
	g_checksum = spring::LiteHash(p, size, g_checksum);
	//LOG("[Sync::Checker] chksum=%u\n", g_checksum);

#ifdef SYNC_HISTORY
	LogHistory();
#endif // SYNC_HISTORY
}

#ifdef SYNC_HISTORY

unsigned CSyncChecker::nextHistoryIndex = 0;
unsigned CSyncChecker::nextFrameIndex = 0;
std::array<unsigned, MAX_SYNC_HISTORY> CSyncChecker::logs;
std::array<unsigned, MAX_SYNC_HISTORY_FRAMES> CSyncChecker::logFrames;

void CSyncChecker::NewGameFrame()
{
	logFrames[nextFrameIndex++] = nextHistoryIndex;
	if (nextFrameIndex == MAX_SYNC_HISTORY_FRAMES)
		nextFrameIndex = 0;
}

void CSyncChecker::LogHistory()
{
	logs[nextHistoryIndex++] = g_checksum;
	if (nextHistoryIndex == MAX_SYNC_HISTORY)
		nextHistoryIndex = 0;
}

std::tuple<unsigned, unsigned, unsigned*> CSyncChecker::GetFrameHistory(unsigned rewindFrames)
{
	int endFrameIndex = nextFrameIndex - rewindFrames;
	int startFrameIndex = endFrameIndex - 1;

	if (endFrameIndex < 0)
		endFrameIndex = MAX_SYNC_HISTORY_FRAMES + endFrameIndex;
	if (startFrameIndex < 0)
		startFrameIndex = MAX_SYNC_HISTORY_FRAMES + startFrameIndex;

	return std::make_tuple(logFrames[startFrameIndex], logFrames[endFrameIndex], logs.data());
}

#endif // SYNC_HISTORY

#endif // SYNCCHECK
