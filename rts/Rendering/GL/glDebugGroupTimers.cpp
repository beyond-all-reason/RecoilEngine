/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "glDebugGroupTimers.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "myGL.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

CONFIG(int, GLDebugGroupTimers)
	.defaultValue(0)
	.minimumValue(0)
	.description("GPU-time every GL debug group (render pass) and log a table every N frames, 0 = off");

namespace {
	constexpr int NUM_FRAMES = 4; // query sets in flight; a set is read NUM_FRAMES - 1 frames after it was written
	constexpr size_t MAX_RECORDS = 512; // per frame

	struct Record {
		const char* name;
		GLuint queries[2];
	};
	struct FrameSet {
		std::vector<Record> records;
	};
	struct Stat {
		double ms = 0.0;
		uint32_t calls = 0;
	};

	int logInterval = -1; // -1 = config not read yet
	bool initialized = false;
	uint32_t frameNum = 0;
	uint32_t framesAccumulated = 0;

	FrameSet frameSets[NUM_FRAMES];
	std::vector<size_t> openStack; // indices into the current frame set's records (the vector may reallocate)
	std::vector<GLuint> queryPool;
	std::unordered_map<std::string, Stat> stats;

	GLuint PopQuery() {
		if (queryPool.empty()) {
			GLuint ids[32];
			glGenQueries(32, ids);
			queryPool.insert(queryPool.end(), ids, ids + 32);
		}
		const GLuint q = queryPool.back();
		queryPool.pop_back();
		return q;
	}

	void Collect(FrameSet& set) {
		for (const Record& rec : set.records) {
			GLuint64 t0 = 0;
			GLuint64 t1 = 0;
			// the set is NUM_FRAMES - 1 frames old, the results are in by now; if not, this waits
			glGetQueryObjectui64v(rec.queries[0], GL_QUERY_RESULT, &t0);
			glGetQueryObjectui64v(rec.queries[1], GL_QUERY_RESULT, &t1);
			Stat& stat = stats[rec.name];
			stat.ms += (t1 >= t0) ? double(t1 - t0) * 1e-6 : 0.0;
			stat.calls += 1;
			queryPool.push_back(rec.queries[0]);
			queryPool.push_back(rec.queries[1]);
		}
		set.records.clear();
	}

	void Report() {
		if (framesAccumulated == 0)
			return;

		std::vector<std::pair<std::string, Stat>> rows(stats.begin(), stats.end());
		std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.second.ms > b.second.ms; });

		const double inv = 1.0 / double(framesAccumulated);
		LOG("[GLDebugGroupTimers] GPU ms per frame over %u frames (nested groups include their children)", framesAccumulated);
		for (const auto& [name, stat] : rows) {
			LOG("[GLDebugGroupTimers] %8.3f ms  %6.2f calls  %s", stat.ms * inv, double(stat.calls) * inv, name.c_str());
		}
		stats.clear();
		framesAccumulated = 0;
	}
}

bool GL::DebugGroupTimers::Enabled()
{
	if (logInterval < 0) {
		logInterval = configHandler->GetInt("GLDebugGroupTimers");
		if (logInterval > 0 && !GLAD_GL_ARB_timer_query) {
			LOG_L(L_WARNING, "[GLDebugGroupTimers] GL_ARB_timer_query is not supported, timers disabled");
			logInterval = 0;
		}
	}
	return (logInterval > 0);
}

void GL::DebugGroupTimers::Begin(const char* name)
{
	FrameSet& set = frameSets[frameNum % NUM_FRAMES];
	if (set.records.size() >= MAX_RECORDS)
		return;

	initialized = true;
	set.records.push_back({ name, { PopQuery(), PopQuery() } });
	openStack.push_back(set.records.size() - 1);
	glQueryCounter(set.records.back().queries[0], GL_TIMESTAMP);
}

void GL::DebugGroupTimers::End()
{
	if (openStack.empty())
		return;

	FrameSet& set = frameSets[frameNum % NUM_FRAMES];
	const Record& rec = set.records[openStack.back()];
	openStack.pop_back();
	glQueryCounter(rec.queries[1], GL_TIMESTAMP);
}

void GL::DebugGroupTimers::EndFrame()
{
	if (!initialized)
		return;

	openStack.clear();
	frameNum += 1;
	framesAccumulated += 1;

	// read the oldest set, which this frame's Begin() calls will overwrite next
	Collect(frameSets[frameNum % NUM_FRAMES]);

	if (framesAccumulated >= uint32_t(logInterval))
		Report();
}

void GL::DebugGroupTimers::Kill()
{
	if (!initialized)
		return;

	for (FrameSet& set : frameSets) {
		for (const Record& rec : set.records)
			glDeleteQueries(2, rec.queries);
		set.records.clear();
	}
	if (!queryPool.empty())
		glDeleteQueries(queryPool.size(), queryPool.data());
	queryPool.clear();
	openStack.clear();
	stats.clear();
	initialized = false;
}
