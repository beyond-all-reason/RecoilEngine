/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRINGTIME_H
#define SPRINGTIME_H

#include "System/creg/creg_cond.h"

#include <cstdint>
#include <cassert>

#undef gt
#include <chrono>
namespace chrono { using namespace std::chrono; }

namespace spring_clock {
	// NOTE:
	//   1e-x are double-precision literals but T can be float
	//   floats only provide ~6 decimal digits of precision so
	//   ToSecs is inaccurate in that case
	//   these cannot be written as integer divisions or tests
	//   will fail because of intermediate conversions to FP32
	template<typename T> static T ToSecs     (int64_t ns) { return static_cast<T>(ns * 1e-9); }
	template<typename T> static T ToMilliSecs(int64_t ns) { return static_cast<T>(ns * 1e-6); }
	template<typename T> static T ToMicroSecs(int64_t ns) { return static_cast<T>(ns * 1e-3); }
	template<typename T> static T ToNanoSecs (int64_t ns) { return static_cast<T>(ns       ); }

	// specializations
	template<> int64_t ToSecs     <int64_t>(int64_t ns) { return (ns / int64_t(1e9)); }
	template<> int64_t ToMilliSecs<int64_t>(int64_t ns) { return (ns / int64_t(1e6)); }
	template<> int64_t ToMicroSecs<int64_t>(int64_t ns) { return (ns / int64_t(1e3)); }

	template<typename T> static int64_t FromSecs     (const T  s) { return ( s * int64_t(1e9)); }
	template<typename T> static int64_t FromMilliSecs(const T ms) { return (ms * int64_t(1e6)); }
	template<typename T> static int64_t FromMicroSecs(const T us) { return (us * int64_t(1e3)); }
	template<typename T> static int64_t FromNanoSecs (const T ns) { return static_cast<int64_t>(ns); }

	void PushTickRate(bool hres = false);
	void PopTickRate();

	// number of ticks since clock epoch
	int64_t GetTicks();
	const char* GetName();
}

// class Timer
struct spring_time {
private:
	CR_DECLARE_STRUCT(spring_time)
public:
	spring_time()
		: x(0)
	{}
	template<typename T> explicit spring_time(const T millis): x(spring_clock::FromMilliSecs(millis)) {}

	spring_time& operator+=(const spring_time st)       { x += st.x; return *this; }
	spring_time& operator-=(const spring_time st)       { x -= st.x; return *this; }
	spring_time& operator%=(const spring_time mt)       { x %= mt.x; return *this; }
	spring_time& operator*=(int n)                      { x *= n; return *this; }
	spring_time& operator*=(float n)                    { x *= n; return *this; }
	spring_time   operator-(const spring_time st) const { return spring_time_native(x - st.x); }
	spring_time   operator+(const spring_time st) const { return spring_time_native(x + st.x); }
	spring_time   operator%(const spring_time mt) const { return spring_time_native(x % mt.x); }
	bool          operator<(const spring_time st) const { return (x <  st.x); }
	bool          operator>(const spring_time st) const { return (x >  st.x); }
	bool         operator<=(const spring_time st) const { return (x <= st.x); }
	bool         operator>=(const spring_time st) const { return (x >= st.x); }

	spring_time   operator*(const int n)   const { return spring_time_native(x * n); }
	spring_time   operator*(const float n) const { return spring_time_native(x * n); }

	// short-hands
	int64_t toSecsi()        const { return (toSecs     <int64_t>()); }
	int64_t toMilliSecsi()   const { return (toMilliSecs<int64_t>()); }
	int64_t toMicroSecsi()   const { return (toMicroSecs<int64_t>()); }
	int64_t toNanoSecsi()    const { return (toNanoSecs <int64_t>()); }

	float toSecsf()      const { return (toSecs     <float>()); }
	float toMilliSecsf() const { return (toMilliSecs<float>()); }
	float toMicroSecsf() const { return (toMicroSecs<float>()); }
	float toNanoSecsf()  const { return (toNanoSecs <float>()); }

	// wrappers
	template<typename T> T toSecs()      const { return spring_clock::ToSecs     <T>(x); }
	template<typename T> T toMilliSecs() const { return spring_clock::ToMilliSecs<T>(x); }
	template<typename T> T toMicroSecs() const { return spring_clock::ToMicroSecs<T>(x); }
	template<typename T> T toNanoSecs()  const { return spring_clock::ToNanoSecs <T>(x); }


	bool isDuration() const { return (x != 0); }
	bool isTime() const { return (x > 0); }

	void sleep(bool forceThreadSleep = false);
	void sleep_until();


	static spring_time gettime(bool init = false) { assert(xs != 0 || init); return spring_time_native(spring_clock::GetTicks()); }
	static spring_time getstarttime() { assert(xs != 0); return spring_time_native(xs); }
	static spring_time getelapsedtime() { return (gettime() - getstarttime()); }

	static void setstarttime(const spring_time t) { assert(xs == 0); xs = t.x; assert(xs != 0); }

	static spring_time fromNanoSecs (int64_t ns) { return spring_time_native(spring_clock::FromNanoSecs( ns)); }
	static spring_time fromMicroSecs(int64_t us) { return spring_time_native(spring_clock::FromMicroSecs(us)); }
	static spring_time fromMilliSecs(int64_t ms) { return spring_time_native(spring_clock::FromMilliSecs(ms)); }
	static spring_time fromSecs     (int64_t  s) { return spring_time_native(spring_clock::FromSecs     ( s)); }

private:
	// convert integer to spring_time (n is interpreted as number of nanoseconds)
	static spring_time spring_time_native(const int64_t n) { spring_time s; s.x = n; return s; }
	void Serialize(creg::ISerializer* s);

private:
	int64_t x;
	// initial time (the "Spring epoch", program start)
	// all other time-points *must* be larger than this
	// if the clock is monotonically increasing
	static int64_t xs;
};



static const spring_time spring_notime(0);
static const spring_time spring_nulltime(0);

//#define spring_gettime()      spring_time::gettime()
#define spring_gettime()      spring_time::getelapsedtime()
#define spring_getstarttime() spring_time::getstarttime()
#define spring_now()          spring_time::getelapsedtime()

#define spring_tomsecs(t) ((t).toMilliSecsi())
#define spring_istime(t) ((t).isTime())
#define spring_sleep(t) ((t).sleep())

#define spring_msecs(msecs) spring_time(msecs)
#define spring_secs(secs) spring_time((secs) * 1000)

#define spring_difftime(now, before)   (now - before)
#define spring_diffsecs(now, before)  ((now - before).toSecsi())
#define spring_diffmsecs(now, before) ((now - before).toMilliSecsi())

#ifdef UNIT_TEST
struct InitSpringTime{
	InitSpringTime()
	{
		spring_clock::PushTickRate(true);
		spring_time::setstarttime(spring_time::gettime(true));
	}
};
#endif

#endif // SPRINGTIME_H
