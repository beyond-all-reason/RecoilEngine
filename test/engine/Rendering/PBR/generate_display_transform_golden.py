#!/usr/bin/env python3
"""Golden values of the PBR display transforms (planning M1, increment I6).

An independent float64 implementation of the published curves, written from the
sources named in rts/Rendering/GL/DisplayTransforms_pbr.h, not from that header.
It prints the C++ table that testPBRDisplayTransforms.cpp embeds; the test then
checks the engine's float header against it.

Usage (from the Recoil checkout):
    python -I test/engine/Rendering/PBR/generate_display_transform_golden.py
Paste the printed block between the GOLDEN markers of testPBRDisplayTransforms.cpp.
"""

import math

LUMA = (0.2126, 0.7152, 0.0722)


def mat(m, c):
    return tuple(sum(m[r][k] * c[k] for k in range(3)) for r in range(3))


def srgb_encode(x):
    x = max(x, 0.0)
    return x * 12.92 if x <= 0.0031308 else 1.055 * x ** (1 / 2.4) - 0.055


def srgb_decode(x):
    x = max(x, 0.0)
    return x / 12.92 if x <= 0.04045 else ((x + 0.055) / 1.055) ** 2.4


def khronos_neutral(c):
    start = 0.8 - 0.04
    desat = 0.15
    x = min(c)
    off = x - 6.25 * x * x if x < 0.08 else 0.04
    c = [v - off for v in c]
    peak = max(c)
    if peak < start:
        return tuple(c)
    d = 1 - start
    new_peak = 1 - d * d / (peak + d - start)
    c = [v * new_peak / peak for v in c]
    g = 1 - 1 / (desat * (peak - new_peak) + 1)
    return tuple(v + (new_peak - v) * g for v in c)


def agx(c):
    inset = ((0.842479062253094, 0.0784335999999992, 0.0792237451477643),
             (0.0423282422610123, 0.878468636469772, 0.0791661274605434),
             (0.0423756549057051, 0.0784336, 0.879142973793104))
    outset = ((1.19687900512017, -0.0980208811401368, -0.0990297440797205),
              (-0.0528968517574562, 1.15190312990417, -0.0989611768448433),
              (-0.0529716355144438, -0.0980434501171241, 1.15107367264116))
    lo, hi = -12.47393, 4.026069
    out = []
    for v in mat(inset, c):
        v = min(max(math.log2(max(v, 1e-10)), lo), hi)
        v = (v - lo) / (hi - lo)
        out.append(15.5 * v**6 - 40.14 * v**5 + 31.96 * v**4 - 6.868 * v**3 + 0.4298 * v**2 + 0.1191 * v - 0.00232)
    return tuple(max(v, 0.0) ** 2.2 for v in mat(outset, out))


def aces(c):
    return tuple(min(max((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0), 1.0) for x in c)


def aces_hill(c):
    inp = ((0.59719, 0.35458, 0.04823), (0.07600, 0.90834, 0.01566), (0.02840, 0.13383, 0.83777))
    outp = ((1.60475, -0.53108, -0.07367), (-0.10208, 1.10813, -0.00605), (-0.00327, -0.07276, 1.07602))
    c = mat(inp, c)
    c = [(v * (v + 0.0245786) - 0.000090537) / (v * (0.983729 * v + 0.4329510) + 0.238081) for v in c]
    return tuple(min(max(v, 0.0), 1.0) for v in mat(outp, c))


def hable_partial(x):
    a, b, c, d, e, f = 0.15, 0.50, 0.10, 0.20, 0.02, 0.30
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f


def hable(c):
    return tuple(hable_partial(max(x, 0.0) * 2.0) / hable_partial(11.2) for x in c)


def uchimura(c):
    P, a, m, l, cc, b = 1.0, 1.0, 0.22, 0.4, 1.33, 0.0
    out = []
    for x in c:
        x = max(x, 0.0)
        l0 = ((P - m) * l) / a
        S0 = m + l0
        S1 = m + a * l0
        C2 = (a * P) / (P - S1)
        CP = -C2 / P
        t = min(max(x / m, 0.0), 1.0)
        w0 = 1 - t * t * (3 - 2 * t)
        w2 = 1.0 if x >= m + l0 else 0.0
        w1 = 1 - w0 - w2
        T = m * (x / m) ** cc + b
        S = P - (P - S1) * math.exp(CP * (x - S0))
        L = m + a * (x - m)
        out.append(T * w0 + L * w1 + S * w2)
    return tuple(out)


def reinhard(c):
    return tuple(max(x, 0.0) / (1 + max(x, 0.0)) for x in c)


def reinhard_jodie(c):
    l = sum(LUMA[i] * c[i] for i in range(3))
    out = []
    for x in c:
        tc = x / (x + 1)
        lc = x / (l + 1)
        out.append(lc + (tc - lc) * tc)
    return tuple(out)


def filmic_hejl(c):
    out = []
    for x in c:
        x = max(0.0, x - 0.004)
        out.append((x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06))
    return tuple(out)


def unreal(c):
    return tuple(max(x, 0.0) / (max(x, 0.0) + 0.155) * 1.019 for x in c)


def bar_custom(encoded):
    A, B, C, D, E = 4.85, 0.85, 3.5, 0.85, 1.0  # the constants cus_gl4.frag.glsl uses
    return tuple((x * (A * x + B)) / (x * (C * x + D) + E) for x in encoded)


def enc(c):
    return tuple(srgb_encode(v) for v in c)


# (name, function of exposed scene-linear light -> display value)
TRANSFORMS = [
    ("khronos-neutral", lambda c: enc(khronos_neutral(c))),
    ("agx", lambda c: enc(agx(c))),
    ("aces", lambda c: enc(aces(c))),
    ("aces-hill", lambda c: enc(aces_hill(c))),
    ("hable", lambda c: enc(hable(c))),
    ("uchimura", lambda c: enc(uchimura(c))),
    ("reinhard", lambda c: enc(reinhard(c))),
    ("reinhard-jodie", lambda c: enc(reinhard_jodie(c))),
    ("filmic-hejl", filmic_hejl),
    ("unreal", unreal),
    ("bar-custom", lambda c: bar_custom(enc(c))),
    ("linear-debug", lambda c: enc(tuple(min(max(v, 0.0), 1.0) for v in c))),
]

# scene-linear inputs: black, a dark value, the 18% gray card, white, HDR values, a coloured highlight
INPUTS = [
    (0.0, 0.0, 0.0),
    (0.02, 0.02, 0.02),
    (0.18, 0.18, 0.18),
    (1.0, 1.0, 1.0),
    (4.0, 4.0, 4.0),
    (64.0, 64.0, 64.0),
    (3.0, 0.75, 0.1),
]


def main():
    print("// GOLDEN BEGIN (generate_display_transform_golden.py)")
    print("static const GoldenRow GOLDEN[] = {")
    for name, f in TRANSFORMS:
        for c in INPUTS:
            r = f(c)
            print('\t{"%s", {%.9g, %.9g, %.9g}, {%.9g, %.9g, %.9g}},' % ((name,) + c + r))
    print("};")
    print("// GOLDEN END")


if __name__ == "__main__":
    main()
