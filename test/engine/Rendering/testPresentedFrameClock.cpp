/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/PresentedFrameClock.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

#include <catch_amalgamated.hpp>

namespace {
	// feeds frames to a clock and keeps the wall time they add up to
	struct ClockRun {
		CPresentedFrameClock clock;
		double wall = 0.0;
		int fastSyncChanges = 0;

		float Step(float frameMs, float periodMs, bool vsync = true) {
			fastSyncChanges += clock.Update(frameMs, periodMs, vsync);
			wall += frameMs;
			return clock.GetFrameTime();
		}

		// the clock trails wall time by exactly its lag
		void CheckNoDrift() const {
			CHECK(wall - clock.GetTime() == Catch::Approx(clock.GetLag()).margin(0.01));
		}
	};

	// vsync'd frames: presents land on whole periods, draw starts jitter around them
	struct VsyncFrames {
		std::mt19937 rng{1234};
		std::uniform_real_distribution<float> noise;
		float lastNoise = 0.0f;

		explicit VsyncFrames(float jitterMs): noise(-jitterMs, jitterMs) {}

		float Next(float periods, float periodMs) {
			const float n = noise(rng);
			const float ms = periods * periodMs + n - lastNoise;
			lastNoise = n;
			return ms;
		}
	};

	float Spread(const std::vector<float>& v) {
		double sum = 0.0;
		double sq = 0.0;

		for (const float x: v) {
			sum += x;
			sq += x * x;
		}

		const double mean = sum / v.size();
		return float(std::sqrt(std::max<double>(0.0, sq / v.size() - mean * mean)) / mean);
	}
}


TEST_CASE("PresentedFrameClock")
{
	constexpr float PERIOD = 10.0f;
	constexpr float PAYBACK_MS = CPresentedFrameClock::LAG_PAYBACK_SECONDS * 1000.0f;

	ClockRun run;
	VsyncFrames frames(1.5f);

	SECTION("vsync off or an unknown refresh rate: the raw frame time") {
		for (int i = 0; i < 200; i++) {
			const float ms = frames.Next(1.0f, PERIOD) * 0.4f;

			CHECK(run.Step(ms, PERIOD, false) == ms);
			CHECK(run.Step(ms, 0.0f, true) == ms);
			CHECK(run.clock.GetLag() == 0.0f);
		}

		CHECK(run.fastSyncChanges == 0);
		run.CheckNoDrift();
	}

	SECTION("steady frames with uneven starts step by whole periods") {
		std::vector<float> raw;
		std::vector<float> presented;

		for (int i = 0; i < 1000; i++) {
			raw.push_back(frames.Next(1.0f, PERIOD));
			presented.push_back(run.Step(raw.back(), PERIOD));

			CHECK(std::abs(run.clock.GetLag()) < PERIOD);
		}

		CHECK(Spread(raw) > 0.1f);
		CHECK(Spread(presented) < 0.005f);
		CHECK(run.fastSyncChanges == 0);
		run.CheckNoDrift();
	}

	SECTION("a lag is paid back over about half a second") {
		constexpr float LAG = 3.0f;
		constexpr int FRAMES_PER_SECOND = int(1000.0f / PERIOD);

		run.Step(PERIOD + LAG, PERIOD);
		REQUIRE(run.clock.GetLag() == Catch::Approx(LAG * (1.0f - PERIOD / PAYBACK_MS)));

		for (int i = 0; i < FRAMES_PER_SECOND / 2; i++)
			run.Step(PERIOD, PERIOD);

		// exponential with a 0.5 s time constant: about 1 / e left after 0.5 s
		CHECK(run.clock.GetLag() < LAG * 0.4f);
		CHECK(run.clock.GetLag() > LAG * 0.3f);

		for (int i = 0; i < FRAMES_PER_SECOND * 3 / 2; i++)
			run.Step(PERIOD, PERIOD);

		CHECK(run.clock.GetLag() < LAG * 0.02f);
		run.CheckNoDrift();
	}

	SECTION("lag spike: frames of 4 periods or more pass raw, the clock settles the next frame") {
		run.Step(PERIOD + 3.0f, PERIOD);

		for (const float spikeMs: {300.0f, 41.0f, 1500.0f}) {
			const float lagBefore = run.clock.GetLag();
			const float presented = run.Step(spikeMs, PERIOD);

			// raw, plus part of the lag paid back
			CHECK(std::abs(presented - spikeMs) <= std::abs(lagBefore) + 0.001f);
			CHECK((presented - spikeMs) * lagBefore >= 0.0f);

			for (int i = 0; i < 5; i++) {
				const float next = run.Step(frames.Next(1.0f, PERIOD), PERIOD);

				CHECK(next == Catch::Approx(PERIOD).margin(0.1f));
			}
		}

		// under 4 periods still snaps
		const float lagBefore = run.clock.GetLag();
		CHECK(std::abs(run.Step(39.0f, PERIOD) - 40.0f) <= (std::abs(lagBefore) + 1.0f) * 40.0f / PAYBACK_MS + 0.001f);
		run.CheckNoDrift();
	}

	SECTION("catch-up: long irregular frames neither trail nor run ahead of wall time") {
		std::mt19937 rng(99);
		std::uniform_real_distribution<float> catchUpMs(100.0f, 600.0f);

		for (int i = 0; i < 200; i++)
			run.Step(frames.Next(1.0f, PERIOD), PERIOD);

		for (int i = 0; i < 40; i++) {
			const float ms = catchUpMs(rng);
			const float lagBefore = run.clock.GetLag();
			const float presented = run.Step(ms, PERIOD);

			CHECK(std::abs(presented - ms) <= std::abs(lagBefore) + 0.001f);
			CHECK(std::abs(run.clock.GetLag()) <= std::abs(lagBefore) + 0.001f);
		}

		for (int i = 0; i < 300; i++) {
			CHECK(run.Step(frames.Next(1.0f, PERIOD), PERIOD) == Catch::Approx(PERIOD).margin(0.1f));
			CHECK(std::abs(run.clock.GetLag()) < PERIOD);
		}

		CHECK(run.fastSyncChanges == 0);
		run.CheckNoDrift();
	}

	SECTION("frame rate lowered: whole periods while under 4, raw from there") {
		for (const float periods: {2.0f, 3.0f}) {
			int rawFrames = 0;

			// right after the step the window can read as unlocked for a while: raw then, no lag carried
			for (int i = 0; i < 100; i++) {
				const float ms = frames.Next(periods, PERIOD);
				const float pres = run.Step(ms, PERIOD);

				if (pres == ms && run.clock.GetLag() == 0.0f && i < CPresentedFrameClock::FRAME_WINDOW) {
					rawFrames++;
					continue;
				}

				CHECK(pres == Catch::Approx(periods * PERIOD).margin(0.25f));
			}

			CHECK(rawFrames < CPresentedFrameClock::FRAME_WINDOW);
		}

		for (int i = 0; i < 100; i++) {
			const float ms = frames.Next(5.0f, PERIOD);
			const float lagBefore = run.clock.GetLag();

			CHECK(std::abs(run.Step(ms, PERIOD) - ms) <= std::abs(lagBefore) + 0.001f);
		}

		CHECK(std::abs(run.clock.GetLag()) < PERIOD);
	}

	SECTION("unlocked frame rates (busy draws): presented follows the mean, the lag stays bounded") {
		// mean frame times at 100 Hz off whole periods, so the display mixes
		// 20 / 30, 30 / 40 and 50 / 60 ms presents
		std::mt19937 rng(7);
		std::uniform_real_distribution<float> noise(-1.0f, 1.0f);

		for (const float meanMs: {20.3f, 31.5f, 53.5f, 31.5f}) {
			const float targetMs = (meanMs < CPresentedFrameClock::MAX_SNAP_PERIODS * PERIOD) ? std::round(meanMs / PERIOD) * PERIOD : meanMs;
			const float biasMs = meanMs - targetMs;
			// the lag at which payback equals the bias each frame, plus room for the noise
			const float lagBound = std::abs(biasMs) * PAYBACK_MS / targetMs + 2.0f * PERIOD;

			std::vector<float> raw;
			std::vector<float> presented;

			for (int i = 0; i < 300; i++) {
				const float ms = meanMs + noise(rng);
				const float lagBefore = run.clock.GetLag();
				const float pres = run.Step(ms, PERIOD);

				if (ms >= CPresentedFrameClock::MAX_SNAP_PERIODS * PERIOD)
					CHECK(std::abs(pres - ms) <= std::abs(lagBefore) * ms / PAYBACK_MS + 0.001f);

				CHECK(std::abs(run.clock.GetLag()) < lagBound + 40.0f);

				if (i >= 200) {
					raw.push_back(ms);
					presented.push_back(pres);
					CHECK(std::abs(run.clock.GetLag()) < lagBound);
				}
			}

			const double rawSum = std::accumulate(raw.begin(), raw.end(), 0.0);
			const double presSum = std::accumulate(presented.begin(), presented.end(), 0.0);

			CHECK(presSum == Catch::Approx(rawSum).epsilon(0.01));
			CHECK(Spread(presented) <= Spread(raw) + 0.001f);
		}

		// back at the refresh rate: raw until the window is on the grid again, then any lag left is paid back
		const float lagBefore = std::abs(run.clock.GetLag());

		for (int i = 0; i < 50; i++) {
			const float ms = frames.Next(1.0f, PERIOD);
			const float pres = run.Step(ms, PERIOD);

			if (run.clock.IsUnlocked() && i < CPresentedFrameClock::FRAME_WINDOW) {
				CHECK(pres == ms);
				continue;
			}

			CHECK(pres == Catch::Approx(PERIOD).margin(lagBefore * PERIOD / PAYBACK_MS + 0.1f));
		}

		CHECK(!run.clock.IsUnlocked());

		CHECK(std::abs(run.clock.GetLag()) < lagBefore * 0.4f + 3.0f);

		for (int i = 0; i < 200; i++)
			run.Step(frames.Next(1.0f, PERIOD), PERIOD);

		CHECK(std::abs(run.clock.GetLag()) < 4.0f);
		CHECK(run.fastSyncChanges == 0);
	}

	SECTION("frames steadily between whole periods: raw once the window sees it, whole periods again after") {
		// 61.7, 36.1 and 40 fps at 100 Hz under draw load
		std::mt19937 rng(11);
		std::uniform_real_distribution<float> noise(-0.5f, 0.5f);

		for (const float meanMs: {16.2f, 27.7f, 25.0f}) {
			for (int i = 0; i < CPresentedFrameClock::FRAME_WINDOW; i++)
				run.Step(meanMs + noise(rng), PERIOD);

			CHECK(run.clock.IsUnlocked());
			CHECK(run.clock.GetLag() == 0.0f);

			for (int i = 0; i < 100; i++) {
				const float ms = meanMs + noise(rng);
				CHECK(run.Step(ms, PERIOD) == ms);
			}

			// back at the refresh rate: whole periods once the median is on the grid
			for (int i = 0; i < CPresentedFrameClock::FRAME_WINDOW; i++)
				run.Step(frames.Next(1.0f, PERIOD), PERIOD);

			CHECK(!run.clock.IsUnlocked());

			for (int i = 0; i < 50; i++)
				CHECK(run.Step(frames.Next(1.0f, PERIOD), PERIOD) == Catch::Approx(PERIOD).margin(0.1f));
		}

		CHECK(run.fastSyncChanges == 0);
	}

	SECTION("frames bunched after a sim frame still snap") {
		for (int i = 0; i < 300; i++) {
			for (const float ms: {3.0f, 17.0f, 10.0f})
				run.Step(ms, PERIOD);

			CHECK(!run.clock.IsUnlocked());
			CHECK(!run.clock.IsFastSync());
		}

		run.CheckNoDrift();
	}

	SECTION("refresh rate change: the new period from the next frame") {
		for (const float period: {PERIOD, 1000.0f / 60.0f, 1000.0f / 144.0f, PERIOD}) {
			run.Step(frames.Next(1.0f, period), period);

			for (int i = 0; i < 200; i++) {
				CHECK(run.Step(frames.Next(1.0f, period), period) == Catch::Approx(period).margin(period * 0.02f));
				CHECK(std::abs(run.clock.GetLag()) < period);
			}
		}

		run.CheckNoDrift();
	}

	SECTION("vsync toggled: raw while off, whole periods from the first frame back on") {
		for (int cycle = 0; cycle < 3; cycle++) {
			for (int i = 0; i < 100; i++) {
				const float ms = frames.Next(0.4f, PERIOD);
				CHECK(run.Step(ms, PERIOD, false) == ms);
			}

			for (int i = 0; i < 100; i++)
				CHECK(run.Step(frames.Next(1.0f, PERIOD), PERIOD) == Catch::Approx(PERIOD).margin(0.1f));
		}

		CHECK(run.fastSyncChanges == 0);
	}

	SECTION("fast sync: entered from the mean of 30 frames, left again, once each way") {
		VsyncFrames fast(0.5f);

		for (int i = 0; i < CPresentedFrameClock::FRAME_WINDOW - 1; i++)
			run.Step(fast.Next(0.4f, PERIOD), PERIOD);

		CHECK(!run.clock.IsFastSync());

		run.Step(fast.Next(0.4f, PERIOD), PERIOD);
		REQUIRE(run.clock.IsFastSync());
		CHECK(run.fastSyncChanges == 1);
		CHECK(run.clock.GetLag() == 0.0f);

		for (int i = 0; i < 100; i++) {
			const float ms = fast.Next(0.4f, PERIOD);
			CHECK(run.Step(ms, PERIOD) == ms);
		}

		int framesToLeave = 0;

		while (run.clock.IsFastSync() && framesToLeave < CPresentedFrameClock::FRAME_WINDOW * 2) {
			run.Step(frames.Next(1.0f, PERIOD), PERIOD);
			framesToLeave++;
		}

		CHECK(!run.clock.IsFastSync());
		CHECK(framesToLeave <= CPresentedFrameClock::FRAME_WINDOW);
		CHECK(run.fastSyncChanges == 2);

		for (int i = 0; i < 100; i++) {
			const float ms = frames.Next(1.0f, PERIOD);
			const float pres = run.Step(ms, PERIOD);

			// the unlocked window may need a few more frames than fast sync
			if (run.clock.IsUnlocked() && i < CPresentedFrameClock::FRAME_WINDOW) {
				CHECK(pres == ms);
				continue;
			}

			CHECK(pres == Catch::Approx(PERIOD).margin(0.15f));
		}
	}
}
