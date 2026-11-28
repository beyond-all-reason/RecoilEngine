#!/usr/bin/env python3
"""Measures what --background-window promises: the engine window never takes the keyboard focus, never
minimizes, stays behind the focused window and, on Windows, is never replaced by a "Not Responding" ghost.

Starts the engine once per case without a game or map (it opens its built-in menu) and samples its window
state without sending any input: real input would move the focus and spoil the measurement.
Windows uses user32 through ctypes; Linux (X11) needs xdotool and xprop, and a window manager that publishes
_NET_ACTIVE_WINDOW and _NET_CLIENT_LIST_STACKING. See README.md for the cases and verdicts.

    python3 background_window_test.py --engine /path/to/spring [--seconds 25] [--cases off,switch,fullscreen] [--out DIR]

Exit code 0 when every case with the switch passes, 1 when one fails, 2 on a usage error.
"""
import argparse
import csv
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile
import time

LOG_LINE = "background window: shown without activation, lowered, never minimized"
BASE_CONFIG = [
    "XResolutionWindowed = 1280",
    "YResolutionWindowed = 720",
    "WindowPosX = 40",
    "WindowPosY = 40",
    "WindowBorderless = 0",
    "LogFlush = 1",
]
CASES = {
    "off": (["Fullscreen = 0"], []),
    "switch": (["Fullscreen = 0"], ["--background-window"]),
    "fullscreen": (["Fullscreen = 1"], ["--background-window"]),
}
COUNTERS = ["samples", "window_seen", "engine_foreground", "ghost_foreground", "hung", "minimized",
    "above_foreground", "shell_foreground", "no_foreground"]


class WindowsProbe:
    # owners of the taskbar, Start, search and notification surfaces: "above" is not judged while they have the focus
    SHELL_OWNERS = {"explorer.exe", "startmenuexperiencehost.exe", "shellexperiencehost.exe", "searchhost.exe",
        "searchapp.exe", "shellhost.exe"}
    GW_HWNDNEXT = 2
    PROCESS_QUERY_LIMITED_INFORMATION = 0x1000

    def __init__(self):
        import ctypes
        import ctypes.wintypes as wt
        self.ct, self.wt = ctypes, wt
        self.user32 = ctypes.WinDLL("user32")
        self.kernel32 = ctypes.WinDLL("kernel32")
        for name in ("GetForegroundWindow", "GetTopWindow", "GetWindow"):
            getattr(self.user32, name).restype = wt.HWND
        self.user32.GetTopWindow.argtypes = [wt.HWND]
        self.user32.GetWindow.argtypes = [wt.HWND, wt.UINT]
        self.enum_proc = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)

    def pid_of(self, hwnd):
        pid = self.wt.DWORD()
        self.user32.GetWindowThreadProcessId(hwnd, self.ct.byref(pid))
        return pid.value

    def exe_of(self, pid):
        h = self.kernel32.OpenProcess(self.PROCESS_QUERY_LIMITED_INFORMATION, False, pid)
        if not h:
            return ""
        try:
            buf = self.ct.create_unicode_buffer(1024)
            size = self.wt.DWORD(len(buf))
            ok = self.kernel32.QueryFullProcessImageNameW(h, 0, buf, self.ct.byref(size))
            return buf.value.rsplit("\\", 1)[-1].lower() if ok else ""
        finally:
            self.kernel32.CloseHandle(h)

    def class_of(self, hwnd):
        buf = self.ct.create_unicode_buffer(256)
        self.user32.GetClassNameW(hwnd, buf, len(buf))
        return buf.value

    def engine_window(self, pid):
        found = []

        def cb(hwnd, _):
            if self.pid_of(hwnd) == pid and self.user32.IsWindowVisible(hwnd) and not self.user32.GetWindow(hwnd, 4):
                found.append(hwnd)  # 4 = GW_OWNER: an unowned visible top-level window, as Process.MainWindowHandle
            return True

        self.user32.EnumWindows(self.enum_proc(cb), 0)
        return found[0] if found else None

    def is_above(self, a, b):
        h = self.user32.GetTopWindow(None)
        while h:
            if h == a:
                return True
            if h == b:
                return False
            h = self.user32.GetWindow(h, self.GW_HWNDNEXT)
        return False

    def sample(self, pid):
        fg = self.user32.GetForegroundWindow()
        shell = bool(fg) and self.exe_of(self.pid_of(fg)) in self.SHELL_OWNERS
        row = {"no_foreground": int(not fg), "shell_foreground": int(shell),
            "ghost_foreground": int(bool(fg) and self.class_of(fg) == "Ghost")}
        win = self.engine_window(pid)
        row["window_seen"] = int(win is not None)
        row["engine_foreground"] = int(win is not None and fg == win)
        row["hung"] = int(win is not None and bool(self.user32.IsHungAppWindow(win)))
        row["minimized"] = int(win is not None and bool(self.user32.IsIconic(win)))
        comparable = win is not None and fg and fg != win and not shell
        row["above_foreground"] = int(bool(comparable) and self.is_above(win, fg))
        return row

    def summary_ok(self, counts):
        return True


class X11Probe:
    def __init__(self):
        for tool in ("xdotool", "xprop"):
            if shutil.which(tool) is None:
                sys.exit(f"{tool} not found (Debian/Ubuntu packages xdotool and x11-utils)")
        self.stacking = True

    @staticmethod
    def run(*args):
        return subprocess.run(args, capture_output=True, text=True).stdout

    @staticmethod
    def ids(text):
        return [int(h, 16) for h in re.findall(r"0x[0-9a-fA-F]+", text)]

    def engine_window(self, pid):
        # a minimized window is unmapped, so no --onlyvisible; the top-level one has a title
        for w in self.run("xdotool", "search", "--pid", str(pid)).split():
            if '= "' in self.run("xprop", "-id", w, "WM_NAME"):
                return int(w)
        return None

    def sample(self, pid):
        active_ids = self.ids(self.run("xprop", "-root", "_NET_ACTIVE_WINDOW"))
        active = active_ids[0] if active_ids else 0
        win = self.engine_window(pid)
        row = {"no_foreground": int(not active), "shell_foreground": 0, "ghost_foreground": 0, "hung": 0,
            "window_seen": int(win is not None), "engine_foreground": int(win is not None and win == active),
            "minimized": 0, "above_foreground": 0}
        if win is None:
            return row
        row["minimized"] = int("_NET_WM_STATE_HIDDEN" in self.run("xprop", "-id", str(win), "_NET_WM_STATE"))
        stack = self.ids(self.run("xprop", "-root", "_NET_CLIENT_LIST_STACKING"))  # bottom to top
        if not stack:
            self.stacking = False
        elif not row["engine_foreground"] and active and win in stack and active in stack:
            row["above_foreground"] = int(stack.index(win) > stack.index(active))
        return row

    def summary_ok(self, counts):
        return self.stacking


def probe_case(probe, proc, seconds, interval, csv_path):
    counts = dict.fromkeys(COUNTERS, 0)
    rows = []
    end = time.monotonic() + seconds
    exited = False
    while time.monotonic() < end:
        if proc.poll() is not None:
            exited = True
            break
        row = probe.sample(proc.pid)
        counts["samples"] += 1
        for key, value in row.items():
            counts[key] += value
        rows.append({"t": round(time.monotonic(), 3), **row})
        time.sleep(interval)
    if rows:
        with open(csv_path, "w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=list(rows[0]))
            w.writeheader()
            w.writerows(rows)
    ok = (counts["window_seen"] > 0 and counts["engine_foreground"] == 0 and counts["ghost_foreground"] == 0
        and counts["minimized"] == 0 and counts["above_foreground"] == 0)
    if exited:
        verdict = "EXITED"
    elif not ok:
        verdict = "FAIL"
    else:
        verdict = "PASS" if probe.summary_ok(counts) else "PARTIAL"  # PARTIAL: no stacking list, "above" unmeasured
    return counts, verdict


def stayed_windowed(infolog, config):
    # only windowed display modes, and the switch must not have saved the user's Fullscreen = 1 as 0
    text = infolog.read_text(encoding="utf-8", errors="replace") if infolog.exists() else ""
    modes = re.findall(r"display-mode set to .*\((\w+)::", text)
    saved_off = re.search(r"^\s*Fullscreen\s*=\s*0", config.read_text(encoding="utf-8"), re.M)
    return bool(modes) and "fullscreen" not in modes and not saved_off


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--engine", required=True, help="spring executable of an installed build (base/ next to it)")
    ap.add_argument("--seconds", type=float, default=25, help="how long each case is sampled")
    ap.add_argument("--interval", type=float, default=0.2, help="seconds between samples")
    ap.add_argument("--cases", default="off,switch,fullscreen", help="comma-separated subset of: " + ", ".join(CASES))
    ap.add_argument("--out", default=os.path.join(tempfile.gettempdir(), "recoil-background-window-test"),
        help="one write directory per case is created below this folder")
    a = ap.parse_args()

    engine = pathlib.Path(a.engine).resolve()
    if not engine.is_file():
        print(f"engine not found: {engine}")
        return 2
    cases = [c for c in a.cases.split(",") if c]
    if any(c not in CASES for c in cases):
        print(f"unknown case in {a.cases}; known: {', '.join(CASES)}")
        return 2
    probe = WindowsProbe() if sys.platform == "win32" else X11Probe()

    table, failed = [], False
    for case in cases:
        config_lines, args = CASES[case]
        wd = pathlib.Path(a.out) / case
        shutil.rmtree(wd, ignore_errors=True)
        wd.mkdir(parents=True)
        config = wd / "springsettings.cfg"
        config.write_text("\n".join(BASE_CONFIG + config_lines) + "\n", encoding="ascii")
        print(f"== {case} : spring {' '.join(args)}", flush=True)
        with open(wd / "stdout.txt", "w", encoding="utf-8") as out:
            proc = subprocess.Popen([str(engine), "--isolation", "--isolation-dir", str(engine.parent),
                "--write-dir", str(wd), *args], stdout=out, stderr=subprocess.STDOUT)
            try:
                counts, verdict = probe_case(probe, proc, a.seconds, a.interval, wd / "probe.csv")
            finally:
                proc.terminate()
                try:
                    proc.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    proc.kill()
        print("   " + " ".join(f"{k}={v}" for k, v in counts.items()))
        print(f"   VERDICT background: {verdict}")
        has_line = LOG_LINE in ((wd / "infolog.txt").read_text(encoding="utf-8", errors="replace")
            if (wd / "infolog.txt").exists() else "")
        windowed = stayed_windowed(wd / "infolog.txt", config) if case == "fullscreen" else None
        if case == "off":
            result = "-"  # the control: what the probe sees without the switch, never judged
        elif verdict == "PASS" and has_line and windowed is not False:
            result = "PASS"
        else:
            result, failed = "FAIL", True
        table.append((case, verdict, has_line, "-" if windowed is None else windowed, result))

    print()
    print(f"{'Case':<11} {'Probe':<8} {'InfologLine':<12} {'Windowed':<9} Result")
    for row in table:
        print(f"{row[0]:<11} {row[1]:<8} {str(row[2]):<12} {str(row[3]):<9} {row[4]}")
    print(f"write directories, infologs and probe CSVs: {a.out}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
