/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VSYNC_H
#define VSYNC_H

class CVerticalSync {
public:
	void WrapNotifyOnChange();
	void WrapRemoveObserver();

	void Toggle();
	void SetInterval();
	void SetInterval(int i);

	/// swap without waiting for vblank until Resume; interval changes meanwhile are only recorded
	void Suspend();
	/// apply the configured interval again, including changes made while suspended
	void Resume();
	void ConfigNotify(const std::string& key, const std::string& value);
	int  GetInterval() const { return interval; }

	static CVerticalSync* GetInstance();

private:
	/// apply the configured interval to the context current on this thread
	void ApplyInterval() const;

	// must start at a value that can not occur in config; see SetInterval
	int interval = -1000;
	bool suspended = false;
};

#define verticalSync (CVerticalSync::GetInstance())

#endif /* VSYNC_H */
