"""From Adam Burgess's scripts/visreg/compare.py in qant-au/tellurion.

Compare visreg pictures with their baselines (visreg.sh).

Needs the Pillow package: python3 -m pip install -r visreg/requirements.txt

Usage: compare.py <baseline-dir> <run-dir> [--max-mean M] [--max-bad-pct P]
                  [--max-tile-pct T]
A pixel is bad when any one channel differs by more than --bad-level. A
picture fails when its mean per-channel difference exceeds M, when more than
P percent of its pixels are bad, or when more than T percent of the pixels in
any 64 px window are bad. The whole-picture limits average a small wrong
region away; the window limit is what catches one.
An optional limits.txt in the baseline dir overrides the limits per picture,
one line each: <name.png> <max-mean> <max-bad-pct> [<max-tile-pct>]
(# starts a comment).
Writes <name>-diff.png beside each failing picture and prints VISREG_OK or
VISREG_FAIL with one line per picture.
"""
import argparse
import sys
from pathlib import Path

try:
    from PIL import Image, ImageChops
except ImportError:
    sys.exit("VISREG_FAIL compare.py needs the Pillow package: "
             "python3 -m pip install -r visreg/requirements.txt")

# Window side and step in pixels. Windows overlap so that a wrong region
# counts wherever it falls: one of 64 x 64 px fills at least 87 percent of
# some window, and one of 71 px or more fills a window completely.
TILE = 64
STEP = 8


def worst_window(mask):
    """Largest share of bad pixels in any TILE px window of a 0/1 L image.

    Returns (percent, x, y) with the window's top left corner.
    """
    w, h = mask.size
    data = mask.tobytes()
    xs = range(0, w, STEP)
    # bad pixels per STEP px cell, then a summed-area table over the cells
    sat = [[0] * (len(xs) + 1)]
    for cy in range(0, h, STEP):
        cells = [0] * len(xs)
        for y in range(cy, min(cy + STEP, h)):
            o = y * w
            for i, x in enumerate(xs):
                cells[i] += data.count(1, o + x, o + min(x + STEP, w))
        row, acc = [0], 0
        for i, c in enumerate(cells):
            acc += c
            row.append(sat[-1][i + 1] + acc)
        sat.append(row)
    nx, ny, k = len(xs), len(sat) - 1, TILE // STEP
    best = (0.0, 0, 0)
    for j in range(max(1, ny - k + 1)):
        j2 = min(j + k, ny)
        for i in range(max(1, nx - k + 1)):
            i2 = min(i + k, nx)
            n = sat[j2][i2] - sat[j][i2] - sat[j2][i] + sat[j][i]
            # a window on the right or bottom edge may be cut short
            area = (min(i2 * STEP, w) - i * STEP) * (min(j2 * STEP, h) - j * STEP)
            if 100.0 * n / area > best[0]:
                best = (100.0 * n / area, i * STEP, j * STEP)
    return best


ap = argparse.ArgumentParser()
ap.add_argument("base")
ap.add_argument("run")
ap.add_argument("--max-mean", type=float, default=3.0)
ap.add_argument("--max-bad-pct", type=float, default=2.0)
ap.add_argument("--max-tile-pct", type=float, default=60.0)
ap.add_argument("--bad-level", type=int, default=40)
a = ap.parse_args()

base, run = Path(a.base), Path(a.run)
fails = 0
limits = {}
lf = base / "limits.txt"
if lf.exists():
    for line in lf.read_text().splitlines():
        f = line.split("#", 1)[0].split()
        if len(f) in (3, 4):
            limits[f[0]] = [float(v) for v in f[1:]]
names = sorted(p.name for p in base.glob("*.png") if not p.name.endswith("-diff.png"))
if not names:
    print(f"VISREG_FAIL no baselines in {base}")
    sys.exit(1)
for name in names:
    r = run / name
    if not r.exists():
        print(f"  {name}: MISSING from run")
        fails += 1
        continue
    b_img = Image.open(base / name).convert("RGB")
    r_img = Image.open(r).convert("RGB")
    if b_img.size != r_img.size:
        print(f"  {name}: SIZE {r_img.size} vs baseline {b_img.size}")
        fails += 1
        continue
    diff = ImageChops.difference(b_img, r_img)
    hist = diff.histogram()  # 3 x 256
    px = b_img.size[0] * b_img.size[1]
    mean = sum(i * c for ch in range(3) for i, c in enumerate(hist[ch * 256:(ch + 1) * 256])) / (px * 3)
    # The strongest channel decides: a luma mix would hide a change in blue alone.
    red, green, blue = diff.split()
    peak = ImageChops.lighter(ImageChops.lighter(red, green), blue)
    mask = peak.point(lambda v: 1 if v > a.bad_level else 0)
    bad_pct = 100.0 * mask.histogram()[1] / px
    tile_pct, tx, ty = worst_window(mask)
    lim = limits.get(name, [])
    max_mean, max_bad = lim[:2] if lim else (a.max_mean, a.max_bad_pct)
    max_tile = lim[2] if len(lim) > 2 else a.max_tile_pct
    ok = mean <= max_mean and bad_pct <= max_bad and tile_pct <= max_tile
    print(f"  {name}: mean {mean:.2f}  bad {bad_pct:.2f}%  tile {tile_pct:.1f}% at {tx},{ty}  {'ok' if ok else 'FAIL'}")
    if not ok:
        fails += 1
        diff.point(lambda v: min(255, v * 4)).save(run / name.replace(".png", "-diff.png"))
print("VISREG_OK" if fails == 0 else f"VISREG_FAIL {fails} of {len(names)} pictures - see {run}")
sys.exit(0 if fails == 0 else 1)
