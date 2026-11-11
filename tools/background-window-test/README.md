# Background window test

Measures what `BackgroundWindow` / `--background-window` promises: the engine window never takes the keyboard focus, never minimizes, stays behind the focused window and, on Windows, is never replaced by a "Not Responding" ghost window.
The feature itself is described in the guide `doc/site/content/docs/guides/getting-started/background-window.md`.

| File | What it does |
|---|---|
| `run-test.ps1` | Windows: runs the four cases below and prints a table; exit code 0 when every feature case passes |
| `run-test.sh` | Linux (X11): the same |
| `window-focus-probe.ps1` | Windows: samples one engine process's window state through `user32.dll` |
| `window-focus-probe.sh` | Linux: the same through `xprop` and `xdotool search` |

The probes only read window state; they never click, type or move a window, since real input would move the focus and spoil the measurement.

## Run it

```bash
# Windows, PowerShell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\background-window-test\run-test.ps1 -Engine C:\path\to\install\spring.exe
# Linux; needs xdotool and xprop (Debian/Ubuntu packages xdotool and x11-utils)
tools/background-window-test/run-test.sh /path/to/install/spring
```

Use an installed build (`base/` next to the binary).
No game or map is needed: without a start script the engine opens its built-in menu and the script stops it after each case.
Keep this terminal or an editor focused and do not click the engine windows; it takes about 2 minutes with the default 25 s per case.

## Cases

| Case | Engine started with | Expected |
|---|---|---|
| `off` | nothing new | the "before" result, not judged; on Windows it takes the focus only when the script was started from the focused window, because Windows lets a process take the foreground only when its launcher had it |
| `switch` | `--background-window` | `PASS` |
| `fullscreen` | `--background-window`, `Fullscreen = 1` in `springsettings.cfg` | `PASS`, still a plain window, and the file does not get `Fullscreen = 0` (the engine drops `Fullscreen = 1` itself when it saves, because 1 is the default) |
| `config` | `BackgroundWindow = 1` in `springsettings.cfg` | `PASS` |

A feature case passes when the probe reports 0 focus, 0 minimized, 0 above the focused window (and 0 ghost on Windows) and the infolog has `background window: shown without activation, lowered, never minimized`.
Each case leaves its write directory, infolog and per-sample CSV in the output folder the script prints.

## Example output (Windows 11)

```text
Case       Probe InfologLine FullscreenKept Result
----       ----- ----------- -------------- ------
off        FAIL        False -              -
switch     PASS         True -              PASS
fullscreen PASS         True True           PASS
config     PASS         True -              PASS
```

## Limits

- The "above the focused window" count only means something while an ordinary window has the focus. The Windows probe skips samples where the shell has the focus (taskbar, Start, notifications, system dialogs) and reports them as `shell_foreground`.
- On Linux the window manager must publish `_NET_ACTIVE_WINDOW` and `_NET_CLIENT_LIST_STACKING`; without the stacking list the probe prints `stacking=no` and does not measure "above".
- macOS is not covered.
