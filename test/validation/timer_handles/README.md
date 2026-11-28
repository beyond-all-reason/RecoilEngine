# Timer handle validation

```sh
python3 test/validation/timer_handles/run.py build-amd64-linux/spring \
  --bar /path/to/Beyond-All-Reason --assets /path/to/bar-dev/data \
  --results /tmp/timer-fixed-results
```

Use `--baseline` with an unmodified engine. In LuaUI the baseline demonstrates
that a foreign `gl.CreateQuery` lightuserdata is accepted as a microsecond timer.
GC is stopped while that query exists and the query is explicitly deleted before
GC resumes. No freed query memory is dereferenced and no address is printed.

Both runs validate millisecond/microsecond differences, seconds/milliseconds
output, legacy resolution-flag interpretation, both frame-timer variants,
ordinary invalid arguments and GC behavior
in LuaUI and unsynced LuaRules/LuaGaia. The fixed run additionally checks reversed
timers, retained handles through allocation churn and
foreign userdata. If unsafe debug metatable functions remain available, a Font
forged as a timer is also rejected and its original metatable restored.

## API compatibility

`GetTimer`, `GetTimerMicros` and `GetFrameTimer` return opaque full userdata. Pass
these directly to `DiffTimers`; they are no longer integer-like lightuserdata.
Matching millisecond timers and microsecond timers with `fromMicroSecs=true`
retain their units. The caller's resolution flag still determines interpretation,
even when it does not match the timer source: BAR has existing microsecond timer
calls that intentionally omit the flag. Reversed timer order now produces a Lua
error instead of unsigned underflow or a native assertion. Differences use
Lua-number precision rather than an
intermediate float. Equal timestamps need not have equal handle identities.

Timestamp bytes are stored in a per-state weak-key registry
table, never read from caller-provided userdata memory. Even retagging unrelated
native userdata cannot disclose its payload through `DiffTimers`. A script that
already has unsafe registry access may tamper with timer data, but only strings
of the expected length are decoded; this is not a complete debug sandbox fix.

The runner uses development assets and temporary isolated overlays, preserves
logs and removes each overlay. It requires graphical occlusion-query support.
LuaIntro/LuaMenu, 32-bit targets and sanitizer builds are not covered.
