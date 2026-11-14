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


# Unreal Engine 5.5 Filmic display transform (default post-process settings), float64, written from the UE shader
# sources (PostProcessCombineLUTs.usf CombineLUTsCommon, TonemapCommon.ush FilmToneMap, ACES.ush). The combined
# matrices are the products of the ACES.ush matrices that the header and the GLSL use too.
UE_SRGB_2_AP1 = [[0.6130974024, 0.3395231461, 0.04737945145], [0.07019372251, 0.9163538791, 0.01345239859], [0.02061559286, 0.1095697729, 0.8698146341]]
UE_AP1_2_SRGB = [[1.705050993, -0.6217921205, -0.08325887216], [-0.1302564175, 1.140804736, -0.01054831904], [-0.02400335679, -0.1289689761, 1.152972333]]
UE_AP1_2_AP0 = [[0.6954522413, 0.1406786965, 0.1638690622], [0.04479456338, 0.8596711184, 0.0955343182], [-0.005525882568, 0.004025210277, 1.001500672]]
UE_AP0_2_AP1_FILM = [[1.451439316, -0.2365107469, -0.2149285693], [-0.0765537734, 1.1762297, -0.0996759264], [0.0083161484, -0.0060324498, 0.9977163014]]
UE_EXPAND = [[1.370412372, -0.3292921877, -0.0636831194], [-0.08343349166, 1.097092748, -0.01086137953], [-0.02579332091, -0.09862579884, 1.203694953]]
UE_BLUE = [[0.9386393778, 1.023595929e-10, 0.06136062213], [8.360085378e-11, 0.8307941329, 0.1692058671], [2.131872912e-12, -5.633071266e-12, 1.0]]
UE_BLUE_INV = [[1.065374875, 1.446785097e-06, -0.06537100528], [-3.455255821e-07, 1.203663524, -0.2036677199], [1.98654478e-08, 2.120795747e-08, 0.9999996001]]
UE_AP1_RGB2Y = (0.2722287168, 0.6740817658, 0.0536895174)


def _lerp(a, b, t):
    return tuple(x + (y - x) * t for x, y in zip(a, b))


def _dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def _ue_film_tone_map(ap1):
    ap0 = mat(UE_AP1_2_AP0, ap1)
    mn, mx = min(ap0), max(ap0)
    sat = (max(mx, 1e-10) - max(mn, 1e-10)) / max(mx, 1e-2)
    sx = (sat - 0.4) / 0.2
    st = max(1 - abs(0.5 * sx), 0.0)
    shaper = 0.5 * (1 + math.copysign(1, sx) * (1 - st * st)) if sx != 0 else 0.5
    chroma = math.sqrt(max(ap0[2] * (ap0[2] - ap0[1]) + ap0[1] * (ap0[1] - ap0[0]) + ap0[0] * (ap0[0] - ap0[2]), 0.0))
    yc = (sum(ap0) + 1.75 * chroma) / 3
    gain, mid = 0.05 * shaper, 0.08
    glow = gain if yc <= 2 / 3 * mid else (0.0 if yc >= 2 * mid else gain * (mid / yc - 0.5))
    ap0 = tuple(x * (1 + glow) for x in ap0)
    if ap0[0] == ap0[1] == ap0[2]:
        hue = 0.0
    else:
        hue = math.degrees(math.atan2(math.sqrt(3) * (ap0[1] - ap0[2]), 2 * ap0[0] - ap0[1] - ap0[2]))
    if hue < 0:
        hue += 360
    hue = min(max(hue, 0.0), 360.0)
    if hue > 180:
        hue -= 360
    hx = min(max(1 - abs(2 * hue / 135), 0.0), 1.0)
    weight = (hx * hx * (3 - 2 * hx)) ** 2
    ap0 = (ap0[0] + weight * sat * (0.03 - ap0[0]) * (1 - 0.82), ap0[1], ap0[2])
    w = tuple(max(0.0, x) for x in mat(UE_AP0_2_AP1_FILM, ap0))
    w = _lerp((_dot(w, UE_AP1_RGB2Y),) * 3, w, 0.96)
    slope, toe, shoulder, black, white = 0.88, 0.55, 0.26, 0.0, 0.04
    toe_scale, shoulder_scale = 1 + black - toe, 1 + white - shoulder
    bt = (0.18 + black) / toe_scale - 1
    toe_match = math.log10(0.18) - 0.5 * math.log((1 + bt) / (1 - bt)) * (toe_scale / slope)
    straight_match = (1 - toe) / slope - toe_match
    shoulder_match = shoulder / slope - straight_match
    tone = []
    for x in w:
        log_c = math.log10(max(x, 1e-30))
        straight = slope * (log_c + straight_match)
        toe_c = -black + (2 * toe_scale) / (1 + math.exp(min((-2 * slope / toe_scale) * (log_c - toe_match), 700)))
        sh_c = (1 + white) - (2 * shoulder_scale) / (1 + math.exp(min((2 * slope / shoulder_scale) * (log_c - shoulder_match), 700)))
        toe_c = toe_c if log_c < toe_match else straight
        sh_c = sh_c if log_c > shoulder_match else straight
        t = min(max((log_c - toe_match) / (shoulder_match - toe_match), 0.0), 1.0)
        if shoulder_match < toe_match:
            t = 1 - t
        t = (3 - 2 * t) * t * t
        tone.append(toe_c + (sh_c - toe_c) * t)
    tone = _lerp((_dot(tone, UE_AP1_RGB2Y),) * 3, tuple(tone), 0.93)
    return tuple(max(0.0, x) for x in tone)


def unreal(c):
    lin = tuple(min(max(x, 0.0), 43.7052) for x in c)
    a = mat(UE_SRGB_2_AP1, lin)
    luma = _dot(a, UE_AP1_RGB2Y)
    if luma > 0:
        d = tuple(x / luma - 1 for x in a)
        expand = (1 - 2 ** (-4 * _dot(d, d))) * (1 - 2 ** (-4 * 1.0 * luma * luma))
        a = _lerp(a, mat(UE_EXPAND, a), expand)
    a = _lerp(a, mat(UE_BLUE, a), 0.6)
    a = _ue_film_tone_map(a)
    a = _lerp(a, mat(UE_BLUE_INV, a), 0.6)
    film = tuple(max(0.0, x) for x in mat(UE_AP1_2_SRGB, a))
    return tuple(min(max(srgb_encode(x), 0.0), 1.0) for x in film)


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
