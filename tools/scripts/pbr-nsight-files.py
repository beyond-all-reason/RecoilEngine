#!/usr/bin/env python3
"""Writes and cleans the NVIDIA Nsight Graphics files of a PBR test run.

WHY
  run-pbr-nsight.sh drives Nsight Graphics (ngfx.exe) without anyone at the
  screen. Two file jobs around that launch are data processing, which the
  project keeps in Python rather than bash:
    * Recoil uses OpenGL calls Nsight's frame debugger does not support
      (display lists, glFog*, glTexEnvi, ...). An unattended capture then stops
      to ask whether to continue, which the command line cannot answer, and
      fails with "Preparing to enter frame debugger mode... Failed". A project
      file with Ignore Incompatibilities turned on avoids the question. The
      same project turns off hardware performance metrics, which need GPU
      performance counters (administrator-only on the development machine).
    * A C++ capture's metadata.json stores the complete environment of the
      launching shell (process_environment), which can include session tokens.
      It must be removed before a capture is copied, shared or recorded.

USAGES (every way this script is called)
  write-project OUTPUT [--default PATH]
      Copies Nsight's default project (default: %LOCALAPPDATA%/NVIDIA
      Corporation/NVIDIA Nsight Graphics/.default.ngfx-proj, which exists after
      ngfx or ngfx-ui has run once), gives it a new uuid, marks it as a user
      project, and sets for the OpenGL Frame Debugger ("fd") and Generate C++
      Capture ("cpp") activities:
        activity/<a>/troubleshooting/ignoreincompatibilities = true
        activity/<a>/troubleshooting/hardwarepmmetrics       = false
      These are the keys the GUI writes for Troubleshooting > Ignore
      Incompatibilities and Collect Hardware Performance Metrics.
  strip-environment CAPTURE_DIRECTORY...
      Replaces process_environment in each CAPTURE_DIRECTORY/metadata.json with
      a one-line note, keeping every other field. A directory without
      metadata.json, or one already stripped, is reported and left alone.
  Callers: tools/scripts/run-pbr-nsight.sh (both subcommands, every run);
  by hand for a capture made from the GUI.
  Exit status: 0 on success; 1 with a message on stderr when the default
  project is missing or a metadata.json cannot be read.

HOW TO USE (expected output)
  From the Recoil worktree in Git Bash (python -I ignores user site packages
  and the current directory; only the standard library is used):
    python -I tools/scripts/pbr-nsight-files.py write-project test-results/pbr-nsight/recoil-nocounters.ngfx-proj
  prints
    Project: test-results/pbr-nsight/recoil-nocounters.ngfx-proj (Ignore Incompatibilities on, hardware metrics off: fd, cpp)
  and
    python -I tools/scripts/pbr-nsight-files.py strip-environment test-results/pbr-nsight/nsight/CppCaptures/spring__2026_10_20__02_42_37
  prints
    Stripped process_environment (87 entries): .../spring__2026_10_20__02_42_37/metadata.json

HOW TO EXTEND (worked example: also turn off driver instrumentation)
  The Frame Debugger documentation suggests turning off driver
  instrumentation first when a capture crashes. To make that a project
  option:
    1. add the key to PROJECT_SETTINGS below:
         "troubleshooting/driverinstrumentation": False,
    2. it is then written for every activity in PROJECT_ACTIVITIES; mention it
       in the write-project line of USAGES above.
  To find a key's name, change the option in ngfx-ui, save the project and
  compare the saved .ngfx-proj with the default one.
"""

import argparse
import json
import os
import sys
import uuid

# Activities the project settings apply to: "fd" is the OpenGL Frame Debugger,
# "cpp" Generate C++ Capture. GPU Trace runs without a project.
PROJECT_ACTIVITIES = ("fd", "cpp")
# Per-activity settings, as "activity/<a>/<key>" in the project's launcher table.
PROJECT_SETTINGS = {
    "troubleshooting/ignoreincompatibilities": True,
    "troubleshooting/hardwarepmmetrics": False,
}
# What process_environment becomes: a list, as Nsight wrote it, so a tool
# reading the file still finds the expected type.
STRIPPED_ENVIRONMENT = ["<removed by pbr-nsight-files.py: the original list held the launching shell environment>"]


def default_project_path():
    """Nsight's per-user default project, written by its first start."""
    base = os.environ.get("LOCALAPPDATA", "")
    return os.path.join(base, "NVIDIA Corporation", "NVIDIA Nsight Graphics", ".default.ngfx-proj")


def write_project(output, default):
    """Writes the Recoil project to `output`; see USAGES. Returns the exit status."""
    source = default or default_project_path()
    if not os.path.isfile(source):
        print(f"Nsight default project not found: {source}. Start ngfx-ui.exe or ngfx.exe once, "
              "or pass --default.", file=sys.stderr)
        return 1
    with open(source, encoding="utf-8") as stream:
        project = json.load(stream)
    # the default project is marked internal and has a fixed uuid; a copy must
    # not look like it to the GUI if someone opens both
    project["is_internal"] = False
    project["uuid"] = "{%s}" % uuid.uuid4()
    launcher = project.setdefault("launcher", {})
    for activity in PROJECT_ACTIVITIES:
        for key, value in PROJECT_SETTINGS.items():
            launcher[f"activity/{activity}/{key}"] = value
    directory = os.path.dirname(os.path.abspath(output))
    os.makedirs(directory, exist_ok=True)
    with open(output, "w", encoding="utf-8") as stream:
        json.dump(project, stream, indent=4)
    print(f"Project: {output} (Ignore Incompatibilities on, hardware metrics off: {', '.join(PROJECT_ACTIVITIES)})")
    return 0


def strip_environment(directories):
    """Removes process_environment from each capture's metadata.json; see USAGES."""
    status = 0
    for directory in directories:
        path = os.path.join(directory, "metadata.json")
        if not os.path.isfile(path):
            print(f"No metadata.json (left alone): {directory}")
            continue
        try:
            with open(path, encoding="utf-8") as stream:
                metadata = json.load(stream)
        except (OSError, ValueError) as error:
            print(f"Cannot read {path}: {error}", file=sys.stderr)
            status = 1
            continue
        environment = metadata.get("process_environment")
        if environment is None or environment == STRIPPED_ENVIRONMENT or len(environment) <= 1:
            print(f"Already without environment: {path}")
            continue
        metadata["process_environment"] = STRIPPED_ENVIRONMENT
        with open(path, "w", encoding="utf-8") as stream:
            json.dump(metadata, stream, indent=4)
        print(f"Stripped process_environment ({len(environment)} entries): {path}")
    return status


def main():
    parser = argparse.ArgumentParser(description="Nsight Graphics files of a PBR test run (see the file header).")
    commands = parser.add_subparsers(dest="command", required=True)
    project = commands.add_parser("write-project", help="write the Recoil .ngfx-proj")
    project.add_argument("output")
    project.add_argument("--default", help="default project to copy (default: Nsight's per-user one)")
    strip = commands.add_parser("strip-environment", help="remove process_environment from C++ captures")
    strip.add_argument("directories", nargs="+")
    arguments = parser.parse_args()
    if arguments.command == "write-project":
        return write_project(arguments.output, arguments.default)
    return strip_environment(arguments.directories)


if __name__ == "__main__":
    sys.exit(main())
