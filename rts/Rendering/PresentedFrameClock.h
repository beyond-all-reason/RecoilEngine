/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _PRESENTED_FRAME_CLOCK_H
#define _PRESENTED_FRAME_CLOCK_H

#include <array>

// How long a draw frame stays on screen (MILLIseconds): with vsync on, whole display periods, the
// remainder paid back over LAG_PAYBACK_SECONDS. Otherwise (see Update) the raw frame time.
class CPresentedFrameClock {
public:
	/// at this many periods or more, rounding a frame to whole periods adds more unevenness than it removes
	static constexpr float MAX_SNAP_PERIODS = 4.0f;

	/// spreads a dropped frame's extra period over many frames, follows a frame rate change within a second
	static constexpr float LAG_PAYBACK_SECONDS = 0.5f;

	/// frames in each detection window, half a second at 60 fps
	static constexpr int FRAME_WINDOW = 30;

	/// returns true when fast sync was entered or left this frame
	bool Update(float frameTimeMs, float displayPeriodMs, bool vsyncEnabled);

	/// replaces this frame's value (the UpdateTimeOffset call-in)
	void Override(float frameTimeMs);

	float GetFrameTime() const { return frameTime; }
	double GetTime() const { return time; }
	float GetLag() const { return lag; }
	bool IsFastSync() const { return fastSync; }
	bool IsUnlocked() const { return unlocked; }
	float GetMeanFrameTime() const { return allFrames.Mean(); }

private:
	// the last FRAME_WINDOW raw frame times
	struct FrameWindow {
		void Add(float frameTimeMs);
		void Reset() { *this = {}; }
		bool Full() const { return count == FRAME_WINDOW; }
		float Mean() const { return (count > 0) ? (sum / count) : 0.0f; }
		float Median() const;

		std::array<float, FRAME_WINDOW> times = {};
		float sum = 0.0f;
		int index = 0;
		int count = 0;
	};

	bool UpdateFastSync(float displayPeriodMs);
	void UpdateUnlocked(float frameTimeMs, float displayPeriodMs);

private:
	/// how long this draw frame advances by on screen
	float frameTime = 0.0f;

	/// the sum of frameTime
	double time = 0.0;

	/// wall time not yet paid into time
	float lag = 0.0f;

	FrameWindow allFrames;
	FrameWindow snapFrames; ///< only frames under MAX_SNAP_PERIODS
	float windowPeriod = 0.0f; ///< the display period both were taken at

	/// vsync on but frames outpace the refresh
	bool fastSync = false;

	/// vsync on but frames take a steady time between whole periods
	bool unlocked = false;
};

#endif // _PRESENTED_FRAME_CLOCK_H
