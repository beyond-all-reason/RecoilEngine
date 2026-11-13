// PBRDisplayCurves.glsl -- the display curves of the PBR HDR display pass, shared with BAR's PBR unit shader.
// The engine prepends this file to PBRDisplayFS.glsl (SceneTarget.cpp: the shader loader has no #include); BAR's
// cus_gl4.lua inserts it into cus_gl4_pbr.frag.glsl from the same VFS path, so the curve a unit shader applies while
// it owns the curve (PBRSceneTarget=direct) and the curve of the display pass can never drift apart.
// No version directive (the engine moves the first one it finds) and only these names at global scope: TM_*, REC709_LUMA, UE_*, SRGBEncode, SRGBDecode, the
// curve functions and PBRApplyDisplayTransform. The CPU reference is rts/Rendering/GL/DisplayTransforms_pbr.h;
// change both together (test_PBRDisplayTransforms, Spring.TestDisplayTransforms).
// The TM_ values are the C++ enum PBRDisplay::Transform.

const int TM_KHRONOS_NEUTRAL = 0;
const int TM_AGX             = 1;
const int TM_ACES            = 2;
const int TM_ACES_HILL       = 3;
const int TM_HABLE           = 4;
const int TM_UCHIMURA        = 5;
const int TM_REINHARD        = 6;
const int TM_REINHARD_JODIE  = 7;
const int TM_FILMIC_HEJL     = 8;
const int TM_UNREAL          = 9;
const int TM_BAR_CUSTOM      = 10;
const int TM_LEGACY          = 11;
const int TM_LINEAR_DEBUG    = 12;


const vec3 REC709_LUMA = vec3(0.2126, 0.7152, 0.0722);

vec3 SRGBEncode(vec3 x) {
	x = max(x, vec3(0.0));
	return mix(1.055 * pow(x, vec3(1.0 / 2.4)) - 0.055, x * 12.92, lessThanEqual(x, vec3(0.0031308)));
}

vec3 SRGBDecode(vec3 x) {
	x = max(x, vec3(0.0)); // the float scene can hold negatives an 8-bit window clamped
	return mix(pow((x + 0.055) / 1.055, vec3(2.4)), x / 12.92, lessThanEqual(x, vec3(0.04045)));
}

vec3 KhronosNeutral(vec3 c) {
	const float START_COMPRESSION = 0.8 - 0.04;
	const float DESATURATION = 0.15;

	float x = min(c.r, min(c.g, c.b));
	float offset = (x < 0.08) ? (x - 6.25 * x * x) : 0.04;
	c -= offset;

	float peak = max(c.r, max(c.g, c.b));
	if (peak < START_COMPRESSION)
		return c;

	const float d = 1.0 - START_COMPRESSION;
	float newPeak = 1.0 - d * d / (peak + d - START_COMPRESSION);
	c *= newPeak / peak;

	float g = 1.0 - 1.0 / (DESATURATION * (peak - newPeak) + 1.0);
	return mix(c, vec3(newPeak), g);
}

vec3 AgX(vec3 c) {
	// column-major: each vec3 is one column of the C++ row-major matrix
	const mat3 INSET = mat3(
		0.842479062253094, 0.0423282422610123, 0.0423756549057051,
		0.0784335999999992, 0.878468636469772, 0.0784336,
		0.0792237451477643, 0.0791661274605434, 0.879142973793104);
	const mat3 OUTSET = mat3(
		1.19687900512017, -0.0528968517574562, -0.0529716355144438,
		-0.0980208811401368, 1.15190312990417, -0.0980434501171241,
		-0.0990297440797205, -0.0989611768448433, 1.15107367264116);
	const float MIN_EV = -12.47393;
	const float MAX_EV = 4.026069;

	c = INSET * c;
	c = clamp(log2(max(c, vec3(1e-10))), MIN_EV, MAX_EV);
	c = (c - MIN_EV) / (MAX_EV - MIN_EV);
	vec3 c2 = c * c;
	vec3 c4 = c2 * c2;
	c = 15.5 * c4 * c2 - 40.14 * c4 * c + 31.96 * c4 - 6.868 * c2 * c + 0.4298 * c2 + 0.1191 * c - 0.00232;
	c = OUTSET * c;
	return pow(max(c, vec3(0.0)), vec3(2.2));
}

vec3 ACESNarkowicz(vec3 x) {
	return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

vec3 ACESHill(vec3 c) {
	const mat3 INPUT = mat3(
		0.59719, 0.07600, 0.02840,
		0.35458, 0.90834, 0.13383,
		0.04823, 0.01566, 0.83777);
	const mat3 OUTPUT = mat3(
		1.60475, -0.10208, -0.00327,
		-0.53108, 1.10813, -0.07276,
		-0.07367, -0.00605, 1.07602);
	c = INPUT * c;
	c = (c * (c + 0.0245786) - 0.000090537) / (c * (0.983729 * c + 0.4329510) + 0.238081);
	return clamp(OUTPUT * c, 0.0, 1.0);
}

vec3 HablePartial(vec3 x) {
	const float A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30;
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 Hable(vec3 c) {
	const float EXPOSURE_BIAS = 2.0;
	const float WHITE_POINT = 11.2;
	return HablePartial(max(c, vec3(0.0)) * EXPOSURE_BIAS) / HablePartial(vec3(WHITE_POINT));
}

vec3 Uchimura(vec3 x) {
	const float P = 1.0, a = 1.0, m = 0.22, l = 0.4, cc = 1.33, b = 0.0;

	x = max(x, vec3(0.0));
	float l0 = ((P - m) * l) / a;
	float S0 = m + l0;
	float S1 = m + a * l0;
	float C2 = (a * P) / (P - S1);
	float CP = -C2 / P;

	vec3 w0 = 1.0 - smoothstep(0.0, m, x);
	vec3 w2 = step(m + l0, x);
	vec3 w1 = 1.0 - w0 - w2;

	vec3 T = m * pow(x / m, vec3(cc)) + b;
	vec3 S = P - (P - S1) * exp(CP * (x - S0));
	vec3 L = m + a * (x - m);
	return T * w0 + L * w1 + S * w2;
}

vec3 Reinhard(vec3 x) {
	x = max(x, vec3(0.0));
	return x / (1.0 + x);
}

vec3 ReinhardJodie(vec3 c) {
	float l = dot(c, REC709_LUMA);
	vec3 tc = c / (c + 1.0);
	return mix(c / (l + 1.0), tc, tc);
}

vec3 FilmicHejl(vec3 x) {
	x = max(vec3(0.0), x - 0.004);
	return (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
}

// Unreal Engine 5.5 Filmic display transform with the default post-process settings (PostProcessTonemap.usf,
// PostProcessCombineLUTs.usf, TonemapCommon.ush FilmToneMap, ACES.ush): sRGB in, AP1 working space, gamut
// expansion 1.0, blue correction 0.6, film slope 0.88, toe 0.55, shoulder 0.26, black clip 0, white clip 0.04,
// back to sRGB primaries and the exact sRGB encode. The analytic chain, not the 32^3 LUT (they agree to 1/255 on
// the reference table); white balance at 6500 K is the identity within 0.1 % and is left out.
// The combined matrices are products of the ACES.ush matrices (planning look-gap ue55_display_transform.md).
const mat3 UE_SRGB_2_AP1 = mat3(0.6130974024, 0.07019372251, 0.02061559286, 0.3395231461, 0.9163538791, 0.1095697729, 0.04737945145, 0.01345239859, 0.8698146341);
const mat3 UE_AP1_2_SRGB = mat3(1.705050993, -0.1302564175, -0.02400335679, -0.6217921205, 1.140804736, -0.1289689761, -0.08325887216, -0.01054831904, 1.152972333);
const mat3 UE_AP1_2_AP0 = mat3(0.6954522413, 0.04479456338, -0.005525882568, 0.1406786965, 0.8596711184, 0.004025210277, 0.1638690622, 0.0955343182, 1.001500672);
const mat3 UE_AP0_2_AP1_FILM = mat3(1.451439316, -0.0765537734, 0.0083161484, -0.2365107469, 1.1762297, -0.0060324498, -0.2149285693, -0.0996759264, 0.9977163014);
const mat3 UE_EXPAND = mat3(1.370412372, -0.08343349166, -0.02579332091, -0.3292921877, 1.097092748, -0.09862579884, -0.0636831194, -0.01086137953, 1.203694953);
const mat3 UE_BLUE = mat3(0.9386393778, 8.360085378e-11, 2.131872912e-12, 1.023595929e-10, 0.8307941329, -5.633071266e-12, 0.06136062213, 0.1692058671, 1);
const mat3 UE_BLUE_INV = mat3(1.065374875, -3.455255821e-07, 1.98654478e-08, 1.446785097e-06, 1.203663524, 2.120795747e-08, -0.06537100528, -0.2036677199, 0.9999996001);
const vec3 UE_AP1_RGB2Y = vec3(0.2722287168, 0.6740817658, 0.0536895174);
const float UE_FILM_SLOPE = 0.88;
const float UE_FILM_TOE = 0.55;
const float UE_FILM_SHOULDER = 0.26;
const float UE_FILM_BLACK_CLIP = 0.0;
const float UE_FILM_WHITE_CLIP = 0.04;
const float UE_BLUE_CORRECTION = 0.6;
const float UE_EXPAND_GAMUT = 1.0;
const float UE_LUT_INPUT_MAX = 43.7052; // the LUT's log-encoded input range saturates here

float UeSaturation(vec3 c) {
	float mn = min(min(c.r, c.g), c.b);
	float mx = max(max(c.r, c.g), c.b);
	return (max(mx, 1e-10) - max(mn, 1e-10)) / max(mx, 1e-2);
}
float UeYC(vec3 c) {
	float chroma = sqrt(max(c.b * (c.b - c.g) + c.g * (c.g - c.r) + c.r * (c.r - c.b), 0.0));
	return (c.r + c.g + c.b + 1.75 * chroma) / 3.0;
}
float UeSigmoidShaper(float x) {
	float t = max(1.0 - abs(0.5 * x), 0.0);
	return 0.5 * (1.0 + sign(x) * (1.0 - t * t));
}
float UeGlowFwd(float yc, float gain, float mid) {
	return (yc <= 2.0 / 3.0 * mid) ? gain : ((yc >= 2.0 * mid) ? 0.0 : gain * (mid / yc - 0.5));
}
float UeHue(vec3 c) {
	float h = (c.r == c.g && c.g == c.b) ? 0.0 : degrees(atan(sqrt(3.0) * (c.g - c.b), 2.0 * c.r - c.g - c.b));
	if (h < 0.0)
		h += 360.0;
	return clamp(h, 0.0, 360.0);
}

vec3 UeFilmToneMap(vec3 ap1) {
	vec3 ap0 = UE_AP1_2_AP0 * ap1;
	float sat = UeSaturation(ap0);
	float s = UeSigmoidShaper((sat - 0.4) / 0.2);
	ap0 *= 1.0 + UeGlowFwd(UeYC(ap0), 0.05 * s, 0.08);           // ACES glow module
	float hue = UeHue(ap0);
	if (hue > 180.0)
		hue -= 360.0;
	float hueWeight = smoothstep(0.0, 1.0, 1.0 - abs(2.0 * hue / 135.0));
	hueWeight *= hueWeight;
	ap0.r += hueWeight * sat * (0.03 - ap0.r) * (1.0 - 0.82);       // ACES red modifier
	vec3 w = max(vec3(0.0), UE_AP0_2_AP1_FILM * ap0);
	w = mix(vec3(dot(w, UE_AP1_RGB2Y)), w, 0.96);
	float toeScale = 1.0 + UE_FILM_BLACK_CLIP - UE_FILM_TOE;
	float shoulderScale = 1.0 + UE_FILM_WHITE_CLIP - UE_FILM_SHOULDER;
	float bt = (0.18 + UE_FILM_BLACK_CLIP) / toeScale - 1.0;
	float toeMatch = log(0.18) / log(10.0) - 0.5 * log((1.0 + bt) / (1.0 - bt)) * (toeScale / UE_FILM_SLOPE);
	float straightMatch = (1.0 - UE_FILM_TOE) / UE_FILM_SLOPE - toeMatch;
	float shoulderMatch = UE_FILM_SHOULDER / UE_FILM_SLOPE - straightMatch;
	vec3 logc = log(max(w, vec3(1e-30))) / log(10.0);
	vec3 straightC = UE_FILM_SLOPE * (logc + straightMatch);
	vec3 toeC = -UE_FILM_BLACK_CLIP + (2.0 * toeScale) / (1.0 + exp((-2.0 * UE_FILM_SLOPE / toeScale) * (logc - toeMatch)));
	vec3 shoulderC = (1.0 + UE_FILM_WHITE_CLIP) - (2.0 * shoulderScale) / (1.0 + exp((2.0 * UE_FILM_SLOPE / shoulderScale) * (logc - shoulderMatch)));
	toeC = mix(straightC, toeC, lessThan(logc, vec3(toeMatch)));
	shoulderC = mix(straightC, shoulderC, greaterThan(logc, vec3(shoulderMatch)));
	vec3 t = clamp((logc - toeMatch) / (shoulderMatch - toeMatch), 0.0, 1.0);
	if (shoulderMatch < toeMatch)
		t = 1.0 - t; // true for the default settings (toe 0.55, shoulder 0.26)
	t = (3.0 - 2.0 * t) * t * t;
	vec3 tone = mix(toeC, shoulderC, t);
	tone = mix(vec3(dot(tone, UE_AP1_RGB2Y)), tone, 0.93);
	return max(vec3(0.0), tone);
}

// exposed scene-linear sRGB -> encoded display value (the encode is part of the chain)
vec3 UnrealFilmic(vec3 lin) {
	lin = clamp(lin, vec3(0.0), vec3(UE_LUT_INPUT_MAX));
	vec3 c = UE_SRGB_2_AP1 * lin;
	float luma = dot(c, UE_AP1_RGB2Y);
	if (luma > 0.0) {
		vec3 d = c / luma - 1.0;
		float expand = (1.0 - exp2(-4.0 * dot(d, d))) * (1.0 - exp2(-4.0 * UE_EXPAND_GAMUT * luma * luma));
		c = mix(c, UE_EXPAND * c, expand);
	}
	c = mix(c, UE_BLUE * c, UE_BLUE_CORRECTION);
	c = UeFilmToneMap(c);
	c = mix(c, UE_BLUE_INV * c, UE_BLUE_CORRECTION);
	return clamp(SRGBEncode(max(vec3(0.0), UE_AP1_2_SRGB * c)), 0.0, 1.0);
}

// BAR's CUS CustomTM with BAR's default tonemapA-E; it works on display-encoded
// values (CUS runs with modelGamma 1), so no encode follows it.
vec3 BarCustom(vec3 encoded) {
	const float A = 4.85, B = 0.85, C = 3.5, D = 0.85, E = 1.0; // CUS's own constants (DisplayTransforms_pbr.h)
	return (encoded * (A * encoded + B)) / (encoded * (C * encoded + D) + E);
}

// Unreal's backbuffer dither (UE 5.5 PostProcessTonemap.usf: GrainFromUV, triangular grain, OutColor += grain *
// BackbufferQuantizationDithering, 1/255 for an 8-bit target): hides banding in dark gradients. `seed` moves the grain
// every frame; amount 0 switches it off.
vec3 PBRDitherDisplay(vec3 encoded, vec2 pixel, float seed, float amount) {
	float uniformGrain = fract(sin(pixel.x + seed + (pixel.y + seed) * 543.31) * 493013.0);
	float n = uniformGrain * 2.0 - 1.0;
	float triangular = sign(n) - sign(n) * sqrt(clamp(1.0 - abs(n), 0.0, 1.0));
	return encoded + triangular * amount;
}

// One curve for an exposed scene-linear value; returns the encoded display value. legacy has no curve of its own:
// the display pass passes the stored value through before calling this, and a material shader that owns the curve
// treats it as khronos-neutral (today's look of the PBR units).
vec3 PBRApplyDisplayTransform(vec3 lin, int transform) {
	switch (transform) {
		case TM_AGX            : return SRGBEncode(AgX(lin));
		case TM_ACES           : return SRGBEncode(ACESNarkowicz(lin));
		case TM_ACES_HILL      : return SRGBEncode(ACESHill(lin));
		case TM_HABLE          : return SRGBEncode(Hable(lin));
		case TM_UCHIMURA       : return SRGBEncode(Uchimura(lin));
		case TM_REINHARD       : return SRGBEncode(Reinhard(lin));
		case TM_REINHARD_JODIE : return SRGBEncode(ReinhardJodie(lin));
		case TM_FILMIC_HEJL    : return FilmicHejl(lin);
		case TM_UNREAL         : return UnrealFilmic(lin);
		case TM_BAR_CUSTOM     : return BarCustom(SRGBEncode(lin));
		case TM_LINEAR_DEBUG   : return SRGBEncode(clamp(lin, 0.0, 1.0));
		default                : return SRGBEncode(KhronosNeutral(lin));
	}
}
