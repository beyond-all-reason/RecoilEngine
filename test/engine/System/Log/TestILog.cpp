
#include "System/Log/ILog.h"
#include "System/Log/FileSink.h"
#include "System/Log/StreamSink.h"
#include "System/Log/LogUtil.h"
#include "System/Log/DefaultFilter.h"

#include <catch_amalgamated.hpp>

#include <cstdarg>
#include <sstream>
#include <atomic>
#include <thread>
#include <vector>



#define LOG_SECTION_EMPTY      ""
#define LOG_SECTION_DEFINED    "defined-section"
#define LOG_SECTION_ONE_TIME_0 "one-time-section"
#define LOG_SECTION_ONE_TIME_1 "other-one-time-section"

LOG_REGISTER_SECTION(LOG_SECTION_EMPTY)
LOG_REGISTER_SECTION(LOG_SECTION_DEFINED)
LOG_REGISTER_SECTION(LOG_SECTION_ONE_TIME_0)
// test what happens if not registered
//LOG_REGISTER_SECTION(LOG_SECTION_ONE_TIME_1)

namespace {
	struct LogStream {
		LogStream() {
			logFile = GetTempLogFile();
			printf("\tNOTE: logging to temporary log file: %s\n", logFile.c_str());
			log_file_addLogFile(logFile.c_str());
			log_sink_stream_setLogStream(&logStream);
		}
		~LogStream() {
			log_sink_stream_setLogStream(NULL);
			log_file_removeLogFile(logFile.c_str());
			remove(logFile.c_str());
		}

		std::string GetTempLogFile() {

			char* tmpName = tmpnam(NULL);
			assert(tmpName != NULL);
			return tmpName;
		}

		std::stringstream logStream;
		std::string logFile;
	};
}

static inline void test_log_sl(int line, std::stringstream& logStream,
		bool enabled, const char* section, int level, const char* fmt, ...)
{
	if (enabled) {
		char logMessage[64 + 1024];
		va_list arguments;
		va_start(arguments, fmt);
		vsnprintf(logMessage, sizeof(logMessage), fmt, arguments);
		va_end(arguments);
		std::string expected = std::string()
				+ (!LOG_SECTION_IS_DEFAULT(section) ? (std::string("[") + std::string(section) + "] ") : "")
				+ ((level != LOG_LEVEL_INFO) ? (std::string(log_util_levelToString(level)) + ": ") : "")
				+ logMessage
				+ "\n";

		INFO("line(" << line << ") Expected: \"" << expected.c_str() << "\"");
		CHECK(logStream.str() == expected);
		logStream.str(std::string());
	} else {
		INFO("line(" << line << ") Expected: \"\"");
		CHECK(logStream.gcount() == 0);
	}
}

#define TLOG_SL(section, level, fmt, ...) \
	test_log_sl(__LINE__, ls.logStream, LOG_IS_ENABLED_S(section, level), section, LOG_LEVE##level, fmt, ##__VA_ARGS__)

#define TLOG_S(section, fmt, ...) \
	TLOG_SL(section, L_INFO, fmt, ##__VA_ARGS__)

#define TLOG_L(level, fmt, ...) \
	TLOG_SL(LOG_SECTION_DEFAULT, level, fmt, ##__VA_ARGS__)

#define TLOG(fmt, ...) \
	TLOG_SL(LOG_SECTION_DEFAULT, L_INFO, fmt, ##__VA_ARGS__)

namespace {
	class PrePostMainLogTest {
	public:
		PrePostMainLogTest() {

			//std::stringstream logStream;
			//log_sink_stream_setLogStream(&logStream);
			LOG("static ctor log test");
			// We can not use Catch here, cause it is not yet initialized,
			// so we depend on asserts spread throughout the logging system.
			//TLOG("static ctor log test");
			//log_sink_stream_setLogStream(NULL);
		}

		~PrePostMainLogTest() {

			//std::stringstream logStream;
			//log_sink_stream_setLogStream(&logStream);
			LOG("static dtor log test");
			// We can not use Catch here, cause it is already
			// uninitialized, so we depend on asserts spread throughout the
			// logging system.
			//TLOG("static dtor log test");
			//log_sink_stream_setLogStream(NULL);
		}
	};
}

// test if logging works very early & very late in program live-time
static PrePostMainLogTest prePostMainLogTest;

static LogStream ls;

TEST_CASE("Default")
{
	LOG( "Testing default logging level (INFO)");
	TLOG("Testing default logging level (INFO)");
}


TEST_CASE("Levels")
{
	LOG_L( L_ERROR, "Static min log level is: %i", _LOG_LEVEL_MIN);
	TLOG_L(L_ERROR, "Static min log level is: %i", _LOG_LEVEL_MIN);

#define TEST_LOG_LEVEL(level) \
	LOG_L( level, "Testing logging level: (%i) %s", LOG_LEVE##level, #level); \
	TLOG_L(level, "Testing logging level: (%i) %s", LOG_LEVE##level, #level)

	TEST_LOG_LEVEL(L_DEBUG);
	TEST_LOG_LEVEL(L_INFO);
	TEST_LOG_LEVEL(L_WARNING);
	TEST_LOG_LEVEL(L_ERROR);
	TEST_LOG_LEVEL(L_FATAL);

#undef TEST_LOG_LEVEL
}


TEST_CASE("IsSingleInstruction")
{
	// do NOT add braces here, as that would invalidate the test!
	int x = 0;
	if      (x == 0) {
		LOG("(IsSingleInstruction) Test");
	} else if (x == 1) {
		LOG("(IsSingleInstruction) if");
	} else {
		LOG("(IsSingleInstruction) LOG() is a single instruction.");
	}

	TLOG("(IsSingleInstruction) Test");
}


static bool TestDefaultSection1()
{
	return (LOG_SECTION_EQUAL("", LOG_SECTION_DEFAULT));
}

static bool TestDefaultSection2()
{
	const std::string foo = "";
	return (LOG_SECTION_EQUAL(foo.c_str(), LOG_SECTION_DEFAULT));
}


TEST_CASE("Sections")
{
	LOG( "Testing logging section: <default> (level: default)");
	TLOG("Testing logging section: <default> (level: default)");

	CHECK(TestDefaultSection1());
	CHECK(TestDefaultSection2());

	LOG_L( L_INFO, "Testing logging section: <default> (level: info)");
	TLOG_L(L_INFO, "Testing logging section: <default> (level: info)");

	#undef  LOG_SECTION_CURRENT
	#define LOG_SECTION_CURRENT LOG_SECTION_EMPTY
	LOG_L(                     L_DEBUG, "Testing logging section: \"" LOG_SECTION_EMPTY "\"");
	TLOG_SL(LOG_SECTION_EMPTY, L_DEBUG, "Testing logging section: \"" LOG_SECTION_EMPTY "\"");

	#undef  LOG_SECTION_CURRENT
	#define LOG_SECTION_CURRENT LOG_SECTION_DEFINED
	LOG_L(                       L_DEBUG, "Testing logging section: \"" LOG_SECTION_DEFINED "\"");
	TLOG_SL(LOG_SECTION_DEFINED, L_DEBUG, "Testing logging section: \"" LOG_SECTION_DEFINED "\"");

	LOG_S( LOG_SECTION_ONE_TIME_0, "Testing logging section: <temporary-section> (level: default)");
	TLOG_S(LOG_SECTION_ONE_TIME_0, "Testing logging section: <temporary-section> (level: default)");

	LOG_SL( LOG_SECTION_ONE_TIME_1, L_INFO, "Testing logging section: <temporary-section> (level: INFO)");
	TLOG_SL(LOG_SECTION_ONE_TIME_1, L_INFO, "Testing logging section: <temporary-section> (level: INFO)");

	#undef  LOG_SECTION_CURRENT
	#define LOG_SECTION_CURRENT LOG_SECTION_DEFAULT
}


TEST_CASE("IsEnabled")
{
	if (LOG_IS_ENABLED_STATIC(L_DEBUG)) {
		// *do heavy, log-output related processing here*
		LOG_L( L_DEBUG, "Testing LOG_IS_ENABLED_STATIC");
		TLOG_L(L_DEBUG, "Testing LOG_IS_ENABLED_STATIC");
	}

	if (LOG_IS_ENABLED(L_DEBUG)) {
		// *do heavy, log-output related processing here*
		LOG_L(L_DEBUG, "Testing LOG_IS_ENABLED");
	}
	TLOG_L(   L_DEBUG, "Testing LOG_IS_ENABLED");


	if (LOG_IS_ENABLED_STATIC_S("one-time-section", L_DEBUG)) {
		// *do heavy, log-output related processing here*
		LOG_SL( "one-time-section", L_DEBUG, "Testing LOG_IS_ENABLED_STATIC_S");
		TLOG_SL("one-time-section", L_DEBUG, "Testing LOG_IS_ENABLED_STATIC_S");
	}

	if (LOG_IS_ENABLED_S("other-one-time-section", L_DEBUG)) {
		// *do heavy, log-output related processing here*
		LOG_SL("other-one-time-section", L_DEBUG, "Testing LOG_IS_ENABLED_S");
	}
	TLOG_SL(   "other-one-time-section", L_DEBUG, "Testing LOG_IS_ENABLED_S");
}


////////////////////////////////////////////////////////////////////////////////
// Multithreaded logging tests.
//
// These prove the logging machinery is race-free under ThreadSanitizer. On
// Linux test_ILog is built with -fsanitize=thread by default.
// They are tagged hidden ([.multithreaded]) so the normal `make check` run
// skips them; run them explicitly by naming the tag:
//     ./test/test_ILog "[multithreaded]"
// Each case asserts only after join (Catch2 macros are not thread-safe) and
// restores any global logging state it mutates so later cases are unaffected.

namespace {
	// barrier so all worker threads start hammering at once
	void waitForStart(const std::atomic<bool>& startFlag) {
		while (!startFlag.load())
			std::this_thread::yield();
	}
}

// N threads emit a flood of records at mixed levels/sections.
TEST_CASE("ConcurrentRecordDispatch", "[.multithreaded]")
{
	constexpr int NUM_THREADS     = 8;
	constexpr int MSGS_PER_THREAD = 2000;

	std::atomic<bool> startFlag{false};
	std::atomic<int>  completed{0};

	auto worker = [&](int tid) {
		waitForStart(startFlag);
		for (int i = 0; i < MSGS_PER_THREAD; ++i) {
			switch ((tid + i) % 5) {
				case 0: LOG_L(L_DEBUG,   "[T%d] msg %d", tid, i); break;
				case 1: LOG_L(L_INFO,    "[T%d] msg %d", tid, i); break;
				case 2: LOG_L(L_WARNING, "[T%d] msg %d", tid, i); break;
				case 3: LOG_L(L_ERROR,   "[T%d] msg %d", tid, i); break;
				case 4: LOG_SL(LOG_SECTION_DEFINED, L_INFO, "[T%d] sec msg %d", tid, i); break;
			}
		}
		completed.fetch_add(1);
	};

	std::vector<std::thread> threads;
	for (int t = 0; t < NUM_THREADS; ++t)
		threads.emplace_back(worker, t);

	startFlag.store(true);
	for (auto& th : threads)
		th.join();

	CHECK(completed.load() == NUM_THREADS);

	ls.logStream.str(std::string()); // don't leak content into later exact-match cases
}

// Writer threads log continuously while a mutator thread churns the shared
// logFiles container (add/remove of a separate file) that the record path
// iterates.
TEST_CASE("ConcurrentSinkRegistryChurn", "[.multithreaded]")
{
	constexpr int NUM_WRITERS = 6;
	constexpr int WRITE_ITERS = 4000;
	constexpr int CHURN_ITERS = 400;

	char* tmp = tmpnam(nullptr);
	REQUIRE(tmp != nullptr);
	const std::string churnFile = tmp;

	std::atomic<bool> startFlag{false};
	std::atomic<int>  completed{0};

	auto writer = [&](int tid) {
		waitForStart(startFlag);
		for (int i = 0; i < WRITE_ITERS; ++i)
			LOG_L(L_INFO, "[W%d] churn-write %d", tid, i);
		completed.fetch_add(1);
	};

	auto mutator = [&]() {
		waitForStart(startFlag);
		for (int i = 0; i < CHURN_ITERS; ++i) {
			log_file_addLogFile(churnFile.c_str());
			log_file_removeLogFile(churnFile.c_str());
		}
	};

	std::vector<std::thread> threads;
	for (int t = 0; t < NUM_WRITERS; ++t)
		threads.emplace_back(writer, t);
	threads.emplace_back(mutator);

	startFlag.store(true);
	for (auto& th : threads)
		th.join();

	CHECK(completed.load() == NUM_WRITERS);

	// cleanup: deregister and delete the churn file
	log_file_removeLogFile(churnFile.c_str());
	remove(churnFile.c_str());
	ls.logStream.str(std::string());
}

// Reader threads read a section's min level while one thread flips that level. 
TEST_CASE("ConcurrentFilterChanges", "[.multithreaded]")
{
	constexpr int NUM_READERS = 6;
	constexpr int READ_ITERS  = 200000;
	constexpr int FLIP_ITERS  = 200000;

	// the section's default level (it is not overridden at this point), used as
	// the "erase" value below
	const int savedLevel  = log_filter_section_getMinLevel(LOG_SECTION_DEFINED);
	const int insertLevel = (savedLevel != LOG_LEVEL_WARNING) ? LOG_LEVEL_WARNING : LOG_LEVEL_ERROR;

	std::atomic<bool> startFlag{false};
	std::atomic<int>  completed{0};

	auto reader = [&](int /*tid*/) {
		waitForStart(startFlag);
		int sink = 0;
		for (int i = 0; i < READ_ITERS; ++i)
			sink += log_filter_section_getMinLevel(LOG_SECTION_DEFINED);
		completed.fetch_add(1);
		(void) sink;
	};

	auto flipper = [&]() {
		waitForStart(startFlag);
		// Alternate a non-default level (inserts into sectionMinLevels) with the
		// section default (erases it) so the shared array is mutated continuously.
		// TODO: simplify to just any two log levels after #3052 merges
		for (int i = 0; i < FLIP_ITERS; ++i)
			log_filter_section_setMinLevel((i & 1) ? insertLevel : savedLevel, LOG_SECTION_DEFINED);
	};

	std::vector<std::thread> threads;
	for (int t = 0; t < NUM_READERS; ++t)
		threads.emplace_back(reader, t);
	threads.emplace_back(flipper);

	startFlag.store(true);
	for (auto& th : threads)
		th.join();

	CHECK(completed.load() == NUM_READERS);

	// restore the section level we churned so later cases see the original state
	log_filter_section_setMinLevel(savedLevel, LOG_SECTION_DEFINED);
	ls.logStream.str(std::string());
}

// Writer threads log while one thread flips the global min level and repeat
// limit. Those are atomics specifically so the log_frontend_isEnabled() fast
// path can read them without taking the log mutex; this exercises that
// write-while-read (a non-atomic version would race here under TSan).
TEST_CASE("ConcurrentGlobalFilterChanges", "[.multithreaded]")
{
	constexpr int NUM_WRITERS = 6;
	constexpr int WRITE_ITERS = 4000;
	constexpr int FLIP_ITERS  = 2000;

	const int savedLevel  = log_filter_global_getMinLevel();
	const int savedRepeat = log_filter_getRepeatLimit();

	std::atomic<bool> startFlag{false};
	std::atomic<int>  completed{0};

	auto writer = [&](int tid) {
		waitForStart(startFlag);
		// every LOG_* reads the atomic global min level on the fast path; as the
		// flipper toggles it, some records are rejected there and some pass on
		for (int i = 0; i < WRITE_ITERS; ++i)
			LOG_L(L_INFO, "[G%d] global %d", tid, i);
		completed.fetch_add(1);
	};

	auto flipper = [&]() {
		waitForStart(startFlag);
		for (int i = 0; i < FLIP_ITERS; ++i) {
			log_filter_global_setMinLevel((i & 1) ? LOG_LEVEL_DEBUG : LOG_LEVEL_WARNING);
			log_filter_setRepeatLimit((i & 1) ? 1 : 5);
		}
	};

	std::vector<std::thread> threads;
	for (int t = 0; t < NUM_WRITERS; ++t)
		threads.emplace_back(writer, t);
	threads.emplace_back(flipper);

	startFlag.store(true);
	for (auto& th : threads)
		th.join();

	CHECK(completed.load() == NUM_WRITERS);

	// restore the globals we churned so later cases see the original state
	log_filter_global_setMinLevel(savedLevel);
	log_filter_setRepeatLimit(savedRepeat);
	ls.logStream.str(std::string());
}

