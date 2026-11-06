#!/usr/bin/env python3
# Copies every non-system dylib the engine and its AI libraries depend on into
# <engine>/lib, rewrites all install names to @rpath and copies the license files
# of the source packages.
#
# Libraries that are dlopen()ed at runtime are passed explicitly:
#   <dir>=<source dylib>[:<bundled name>], <dir> being "lib" or "mesa"
#
# Usage: bundle-dylibs.py <engine dir> <license dir> [<dir>=<dylib>[:<name>] ...]

import os
import re
import shutil
import subprocess
import sys

engine = os.path.abspath(sys.argv[1])
license_dir = os.path.abspath(sys.argv[2])
lib_dir = os.path.join(engine, "lib")
mesa_dir = os.path.join(engine, "mesa", "lib")
SYSTEM = ("/usr/lib/", "/System/")
LICENSE_NAMES = ("LICENSE", "LICENSE.txt", "LICENSE.TXT", "LICENSE.md", "COPYING", "COPYING.txt", "COPYING.LIB", "license.rst")


def run(*args):
	return subprocess.run(args, check=True, capture_output=True, text=True).stdout


def install_name(path):
	# empty for executables and for plugins built as bundles, like the AI libraries
	lines = run("otool", "-D", path).splitlines()[1:]
	return lines[0].strip() if lines else None


def deps(path):
	lines = run("otool", "-L", path).splitlines()[1:]
	names = [l.strip().split(" (")[0] for l in lines]
	if install_name(path) is not None:
		# first entry of a dylib is its own install name
		names = names[1:]
	return [n for n in names if not n.startswith(SYSTEM)]


def rpaths(path):
	out = run("otool", "-l", path)
	return re.findall(r"cmd LC_RPATH\n\s+cmdsize \d+\n\s+path (.+?) \(offset", out)


# bundled copy -> file it was copied from, whose neighbours its @rpath and
# @loader_path references point at
origin = {}


def resolve(name, referrer):
	referrer = origin.get(referrer, referrer)
	if name.startswith("@rpath/"):
		base = name[len("@rpath/"):]
		for d in (os.path.dirname(referrer), lib_dir, mesa_dir, "/opt/homebrew/lib", "/usr/local/lib"):
			candidate = os.path.join(d, base)
			if os.path.exists(candidate):
				return candidate
		raise SystemExit(f"cannot resolve {name} of {referrer}")
	if name.startswith("@loader_path/"):
		return os.path.normpath(os.path.join(os.path.dirname(referrer), name[len("@loader_path/"):]))
	return name


licensed = set()


def copy_license(src):
	m = re.match(r"(/(?:opt/homebrew|usr/local)/(?:Cellar|opt)/([^/]+)(?:/[^/]+)?)/", os.path.realpath(src))
	if m is None or m.group(2) in licensed:
		return
	keg = m.group(1)
	if not os.path.isdir(os.path.join(keg, "lib")):
		keg = os.path.dirname(keg)
	licensed.add(m.group(2))
	for n in LICENSE_NAMES:
		if os.path.isfile(os.path.join(keg, n)):
			shutil.copy(os.path.join(keg, n), os.path.join(license_dir, f"{m.group(2)}-{n}"))


bundled = {}
for arg in sys.argv[3:]:
	where, src = arg.split("=", 1)
	src, _, name = src.partition(":")
	dst = os.path.join(mesa_dir if where == "mesa" else lib_dir, name or os.path.basename(src))
	shutil.copy(os.path.realpath(src), dst)
	copy_license(src)
	bundled[os.path.basename(dst)] = dst
	origin[dst] = os.path.realpath(src)

queue = [os.path.join(engine, f) for f in ("spring", "pr-downloader", "libunitsync.dylib")] + list(bundled.values())
for root, _, files in os.walk(os.path.join(engine, "AI")):
	queue += [os.path.join(root, f) for f in files if f.endswith(".dylib")]
done = set()

while queue:
	path = queue.pop()
	if path in done:
		continue
	done.add(path)
	os.chmod(path, 0o755)

	for name in deps(path):
		base = os.path.basename(name)
		if base not in bundled:
			src = resolve(name, path)
			dst = os.path.join(lib_dir, base)
			shutil.copy(os.path.realpath(src), dst)
			copy_license(src)
			bundled[base] = dst
			origin[dst] = os.path.realpath(src)
		queue.append(bundled[base])
		if name != f"@rpath/{base}":
			run("install_name_tool", "-change", name, f"@rpath/{base}", path)

	if install_name(path) is not None:
		run("install_name_tool", "-id", f"@rpath/{os.path.basename(path)}", path)

	for rp in rpaths(path):
		run("install_name_tool", "-delete_rpath", rp, path)
	here = os.path.dirname(path)
	prefix = "@executable_path" if not path.endswith(".dylib") else "@loader_path"
	for d in (lib_dir, mesa_dir):
		rel = os.path.relpath(d, here)
		run("install_name_tool", "-add_rpath", prefix if rel == "." else f"{prefix}/{rel}", path)

print(f"bundled {len(bundled)} libraries, licenses of: {', '.join(sorted(licensed))}")
