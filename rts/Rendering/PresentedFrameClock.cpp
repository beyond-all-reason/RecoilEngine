/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PresentedFrameClock.h"

#include <algorithm>
#include <cmath>
#include <numeric>

// Raw with vsync off, an unknown refresh rate, fast sync, unlocked frames, and for frames of
// MAX_SNAP_PERIODS or more (those still pay back the lag).
bool CPresentedFrameClock::Update(float frameTimeMs, float displayPeriodMs, bool vsyncEnabled)
{
	const bool vsyncPeriodKnown = (vsyncEnabled && displayPeriodMs > 0.0f);
	bool fastSyncChanged = false;

	// frame times taken at another refresh rate say nothing about this one
	if (!vsyncPeriodKnown || displayPeriodMs != windowPeriod) {
		allFrames.Reset();
		snapFrames.Reset();
		fastSync = false;
		unlocked = false;
	}

	if (vsyncPeriodKnown) {
		windowPeriod = displayPeriodMs;
		allFrames.Add(frameTimeMs);
		fastSyncChanged = UpdateFastSync(displayPeriodMs);
		UpdateUnlocked(frameTimeMs, displayPeriodMs);
	}

	float presentedMs = frameTimeMs;

	if (vsyncPeriodKnown && !fastSync && !unlocked && frameTimeMs > 0.0f) {
		const bool snap = (frameTimeMs < MAX_SNAP_PERIODS * displayPeriodMs);
		const float targetMs = snap ? std::max(1.0f, std::round(frameTimeMs / displayPeriodMs)) * displayPeriodMs : frameTimeMs;
		const float newLag = lag + frameTimeMs - targetMs;
		const float payback = newLag * std::min(1.0f, targetMs / (LAG_PAYBACK_SECONDS * 1000.0f));

		lag = newLag - payback;
		presentedMs = targetMs + payback;
	} else {
		lag = 0.0f;
	}

	frameTime = presentedMs;
	time += presentedMs;

	return fastSyncChanged;
}

void CPresentedFrameClock::Override(float frameTimeMs)
{
	time += (frameTimeMs - frameTime);
	frameTime = frameTimeMs;
}

void CPresentedFrameClock::FrameWindow::Add(float frameTimeMs)
{
	sum += (frameTimeMs - times[index]);
	times[index] = frameTimeMs;
	index = (index + 1) % FRAME_WINDOW;
	count = std::min(count + 1, FRAME_WINDOW);

	// resum once per window so float rounding cannot accumulate
	if (index == 0)
		sum = std::accumulate(times.begin(), times.end(), 0.0f);
}

float CPresentedFrameClock::FrameWindow::Median() const
{
	std::array<float, FRAME_WINDOW> sorted = times;
	const auto mid = sorted.begin() + count / 2;

	std::nth_element(sorted.begin(), mid, sorted.begin() + count);
	return *mid;
}

// vsync on but the mean frame time well under a period: frames outpace the refresh (fast sync).
// The mean, not the median: frames queued after a sim frame bunch up but still drain at the refresh rate.
bool CPresentedFrameClock::UpdateFastSync(float displayPeriodMs)
{
	// a vsync'd frame lasts a period or more: 0.75 leaves room for timer noise, the gap to 0.9 stops flapping
	constexpr float ENTER_BELOW_PERIODS = 0.75f;
	constexpr float LEAVE_ABOVE_PERIODS = 0.9f;

	if (!allFrames.Full())
		return false;

	const float meanPeriods = allFrames.Mean() / displayPeriodMs;
	const bool detected = fastSync ? (meanPeriods < LEAVE_ABOVE_PERIODS) : (meanPeriods < ENTER_BELOW_PERIODS);

	if (detected == fastSync)
		return false;

	fastSync = detected;
	lag = 0.0f;
	return true;
}

// A busy draw taking a steady time between whole periods: rounding every frame the same way builds a
// large lag, and the raw frame time is the smoother clock. Both mean and median must be off the grid:
// the median holds through a frame rate step, the mean through frames bunched after a sim frame.
void CPresentedFrameClock::UpdateUnlocked(float frameTimeMs, float displayPeriodMs)
{
	// 0.12 periods is clear of timer noise, the gap to 0.06 stops flapping
	constexpr float ENTER_OFF_PERIODS = 0.12f;
	constexpr float LEAVE_OFF_PERIODS = 0.06f;

	// a lag spike says nothing about the frame rate
	if (frameTimeMs >= MAX_SNAP_PERIODS * displayPeriodMs)
		return;

	snapFrames.Add(frameTimeMs);

	if (!snapFrames.Full())
		return;

	const auto OffGrid = [displayPeriodMs](float ms) {
		const float periods = ms / displayPeriodMs;
		return std::abs(periods - std::round(periods));
	};

	const float off = std::min(OffGrid(snapFrames.Mean()), OffGrid(snapFrames.Median()));
	const bool detected = (off > (unlocked ? LEAVE_OFF_PERIODS : ENTER_OFF_PERIODS));

	if (detected == unlocked)
		return;

	unlocked = detected;
	lag = 0.0f;
}
