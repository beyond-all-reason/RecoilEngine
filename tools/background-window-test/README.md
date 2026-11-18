# Background window test

Measures what `--background-window` promises: the engine window never takes the keyboard focus, never minimizes, stays behind the focused window and, on Windows, is never replaced by a "Not Responding" ghost window.
The switch is described in the guide `doc/site/content/docs/guides/getting-started/headless-and-dedicated.md`.

`background_window_test.py` starts the engine once per case and samples its window state: on Windows through `user32` (ctypes), on Linux (X11) through `xprop` and `xdotool`.
It only reads window state; it never clicks, types or moves a window, since real input would move the focus and spoil the measurement.

## Run it

```bash
# Windows
python tools\background-window-test\background_window_test.py --engine C:\path\to\install\spring.exe
# Linux; needs xdotool and xprop (Debian/Ubuntu packages xdotool and x11-utils)
python3 tools/background-window-test/background_window_test.py --engine /path/to/install/spring
```

Use an installed build (`base/` next to the binary).
No game or map is needed: without a start script the engine opens its built-in menu and the script stops it after each case.
Keep a terminal or an editor focused and do not click the engine windows; it takes about 1.5 minutes with the default 25 s per case.
`--cases`, `--seconds` and `--out` choose the cases, the sampling time and the output folder; the exit code is 0 when every case with the switch passes, 1 when one fails, 2 on a usage error.

## Cases

| Case | Engine started with | Expected |
|---|---|---|
| `off` | nothing new | the "before" result, not judged; on Windows it takes the focus only when the script was started from the focused window, because Windows lets a process take the foreground only when its launcher had it |
| `switch` | `--background-window` | `PASS` |
| `fullscreen` | `--background-window`, `Fullscreen = 1` in `springsettings.cfg` | `PASS`, still a plain window, and the file does not get `Fullscreen = 0` (the engine drops `Fullscreen = 1` itself when it saves, because 1 is the default) |

A case passes when the probe reports 0 focus, 0 minimized, 0 above the focused window (and 0 ghost on Windows) and the infolog has `background window: shown without activation, lowered, never minimized`.
Each case leaves its write directory, infolog and per-sample CSV in the output folder the script prints.

## Example output (Windows 11, `--cases switch,fullscreen`)

```text
== switch : spring --background-window
   samples=125 window_seen=120 engine_foreground=0 ghost_foreground=0 hung=0 minimized=0 above_foreground=0 shell_foreground=0 no_foreground=0
   VERDICT background: PASS
== fullscreen : spring --background-window
   samples=125 window_seen=120 engine_foreground=0 ghost_foreground=0 hung=0 minimized=0 above_foreground=0 shell_foreground=0 no_foreground=0
   VERDICT background: PASS

Case        Probe    InfologLine  Windowed  Result
switch      PASS     True         -         PASS
fullscreen  PASS     True         True      PASS
```

## Limits

- The "above the focused window" count only means something while an ordinary window has the focus. On Windows, samples where the shell has the focus (taskbar, Start, notifications, system dialogs) are counted as `shell_foreground` and not judged.
- On Linux the window manager must publish `_NET_ACTIVE_WINDOW` and `_NET_CLIENT_LIST_STACKING`; without the stacking list the verdict is `PARTIAL` ("above" not measured).
- macOS is not covered.
