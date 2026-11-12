/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DISPLAY_TRANSFORMS_PBR_H
#define DISPLAY_TRANSFORMS_PBR_H

// CPU reference of the PBR display pass (planning decision D013: pure math in a
// header-only file without GL). The GLSL in shaders/GLSL/PBRDisplayFS.glsl
// implements the same functions; test/engine/Rendering/PBR/testPBRDisplayTransforms.cpp
// holds the golden values and Spring.TestDisplayTransforms() compares the GPU
// pass against ApplyDisplayPass() below. Change both files together.
//
// One display pass = decode the scene's storage transfer, multiply by the
// exposure, apply exactly one curve, encode once for an sRGB display.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace PBRDisplay {

struct RGB { float r, g, b; };

// The order is the shader's `displayTransform` uniform; append, never reorder.
enum class Transform : int32_t {
	KhronosNeutral = 0,
	AgX            = 1,
	ACES           = 2, // Narkowicz 2015 fit
	ACESHill       = 3, // Stephen Hill's fit of the RRT+ODT
	Hable          = 4,
	Uchimura       = 5,
	Reinhard       = 6,
	ReinhardJodie  = 7,
	FilmicHejl     = 8, // published form includes the display encode
	Unreal         = 9, // published form includes the display encode
	BarCustom      = 10,
	Legacy         = 11,
	LinearDebug    = 12,
	Count
};

// How the scene target stores colour (PBRSceneTransfer).
enum class SceneTransfer : int32_t {
	SRGBExtended = 0,
	Linear       = 1, // reserved: the legacy world does not write linear yet
};

static constexpr std::array<std::string_view, size_t(Transform::Count)> TRANSFORM_NAMES = {
	"khronos-neutral", "agx", "aces", "aces-hill", "hable", "uchimura", "reinhard",
	"reinhard-jodie", "filmic-hejl", "unreal", "bar-custom", "legacy", "linear-debug",
};

inline bool ParseTransform(std::string_view name, Transform& out) {
	for (size_t i = 0; i < TRANSFORM_NAMES.size(); ++i) {
		if (TRANSFORM_NAMES[i] != name)
			continue;
		out = Transform(i);
		return true;
	}
	return false;
}

inline std::string_view TransformName(Transform t) { return TRANSFORM_NAMES[size_t(t)]; }

// The two published curves whose output is already display-encoded.
inline bool CurveIncludesEncode(Transform t) { return t == Transform::FilmicHejl || t == Transform::Unreal; }

static constexpr float REC709_LUMA[3] = {0.2126f, 0.7152f, 0.0722f};

// IEC 61966-2-1 piecewise sRGB, extended above 1 by the same power segment so the
// srgb-extended storage transfer round-trips scene values above one.
inline float SRGBEncode(float x) {
	x = std::max(x, 0.0f);
	return (x <= 0.0031308f)? (x * 12.92f): (1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f);
}
inline float SRGBDecode(float x) {
	x = std::max(x, 0.0f); // the float scene can hold negatives an 8-bit window clamped
	return (x <= 0.04045f)? (x / 12.92f): std::pow((x + 0.055f) / 1.055f, 2.4f);
}
inline RGB SRGBEncode(RGB c) { return {SRGBEncode(c.r), SRGBEncode(c.g), SRGBEncode(c.b)}; }
inline RGB SRGBDecode(RGB c) { return {SRGBDecode(c.r), SRGBDecode(c.g), SRGBDecode(c.b)}; }

inline RGB Mul(const float m[3][3], RGB c) {
	return {
		m[0][0] * c.r + m[0][1] * c.g + m[0][2] * c.b,
		m[1][0] * c.r + m[1][1] * c.g + m[1][2] * c.b,
		m[2][0] * c.r + m[2][1] * c.g + m[2][2] * c.b,
	};
}
template<typename F> inline RGB PerChannel(RGB c, F f) { return {f(c.r), f(c.g), f(c.b)}; }
inline float Clamp01(float x) { return std::clamp(x, 0.0f, 1.0f); }

// Khronos PBR Neutral, reference implementation from the specification
// (github.com/KhronosGroup/ToneMapping, PBR_Neutral).
inline RGB KhronosNeutral(RGB c) {
	constexpr float START_COMPRESSION = 0.8f - 0.04f;
	constexpr float DESATURATION = 0.15f;

	const float x = std::min(c.r, std::min(c.g, c.b));
	const float offset = (x < 0.08f)? (x - 6.25f * x * x): 0.04f;
	c = {c.r - offset, c.g - offset, c.b - offset};

	const float peak = std::max(c.r, std::max(c.g, c.b));
	if (peak < START_COMPRESSION)
		return c;

	constexpr float d = 1.0f - START_COMPRESSION;
	const float newPeak = 1.0f - d * d / (peak + d - START_COMPRESSION);
	const float s = newPeak / peak;
	c = {c.r * s, c.g * s, c.b * s};

	const float g = 1.0f - 1.0f / (DESATURATION * (peak - newPeak) + 1.0f);
	return {c.r + (newPeak - c.r) * g, c.g + (newPeak - c.g) * g, c.b + (newPeak - c.b) * g};
}

// AgX base (Troy Sobotka), analytic form with the 6th-order fit of the default
// contrast curve (Benjamin Wrensch, 2023); returns linear light for the encode.
inline RGB AgX(RGB c) {
	static constexpr float INSET[3][3] = {
		{0.842479062253094f, 0.0784335999999992f, 0.0792237451477643f},
		{0.0423282422610123f, 0.878468636469772f, 0.0791661274605434f},
		{0.0423756549057051f, 0.0784336f, 0.879142973793104f},
	};
	static constexpr float OUTSET[3][3] = {
		{1.19687900512017f, -0.0980208811401368f, -0.0990297440797205f},
		{-0.0528968517574562f, 1.15190312990417f, -0.0989611768448433f},
		{-0.0529716355144438f, -0.0980434501171241f, 1.15107367264116f},
	};
	constexpr float MIN_EV = -12.47393f;
	constexpr float MAX_EV = 4.026069f;
	constexpr float MIN_POSITIVE = 1e-10f; // keeps log2 finite for black

	c = Mul(INSET, c);
	c = PerChannel(c, [&](float v) {
		v = std::clamp(std::log2(std::max(v, MIN_POSITIVE)), MIN_EV, MAX_EV);
		v = (v - MIN_EV) / (MAX_EV - MIN_EV);
		const float v2 = v * v;
		const float v4 = v2 * v2;
		return 15.5f * v4 * v2 - 40.14f * v4 * v + 31.96f * v4 - 6.868f * v2 * v + 0.4298f * v2 + 0.1191f * v - 0.00232f;
	});
	c = Mul(OUTSET, c);
	constexpr float AGX_DISPLAY_GAMMA = 2.2f; // the fit's own display transfer, undone here
	return PerChannel(c, [](float v) { return std::pow(std::max(v, 0.0f), AGX_DISPLAY_GAMMA); });
}

// Krzysztof Narkowicz, "ACES Filmic Tone Mapping Curve" (2015).
inline RGB ACESNarkowicz(RGB c) {
	return PerChannel(c, [](float x) {
		return Clamp01((x * (2.51f * x + 0.03f)) / (x * (2.43f * x + 0.59f) + 0.14f));
	});
}

// Stephen Hill's fit of the ACES RRT+ODT (BakingLab, ACES.hlsl).
inline RGB ACESHill(RGB c) {
	static constexpr float INPUT[3][3] = {
		{0.59719f, 0.35458f, 0.04823f},
		{0.07600f, 0.90834f, 0.01566f},
		{0.02840f, 0.13383f, 0.83777f},
	};
	static constexpr float OUTPUT[3][3] = {
		{ 1.60475f, -0.53108f, -0.07367f},
		{-0.10208f,  1.10813f, -0.00605f},
		{-0.00327f, -0.07276f,  1.07602f},
	};
	c = Mul(INPUT, c);
	c = PerChannel(c, [](float v) {
		return (v * (v + 0.0245786f) - 0.000090537f) / (v * (0.983729f * v + 0.4329510f) + 0.238081f);
	});
	return PerChannel(Mul(OUTPUT, c), Clamp01);
}

// John Hable's Uncharted 2 curve with its published exposure bias and white point.
inline float HablePartial(float x) {
	constexpr float A = 0.15f, B = 0.50f, C = 0.10f, D = 0.20f, E = 0.02f, F = 0.30f;
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}
inline RGB Hable(RGB c) {
	constexpr float EXPOSURE_BIAS = 2.0f;
	constexpr float WHITE_POINT = 11.2f;
	const float whiteScale = 1.0f / HablePartial(WHITE_POINT);
	return PerChannel(c, [whiteScale](float x) { return HablePartial(std::max(x, 0.0f) * EXPOSURE_BIAS) * whiteScale; });
}

// Hajime Uchimura, "HDR theory and practice" (CEDEC 2017), the GT curve with its
// published defaults.
inline RGB Uchimura(RGB c) {
	constexpr float P = 1.0f;  // max brightness
	constexpr float a = 1.0f;  // contrast
	constexpr float m = 0.22f; // linear section start
	constexpr float l = 0.4f;  // linear section length
	constexpr float cc = 1.33f; // black tightness
	constexpr float b = 0.0f;  // pedestal

	return PerChannel(c, [](float x) {
		x = std::max(x, 0.0f);
		const float l0 = ((P - m) * l) / a;
		const float S0 = m + l0;
		const float S1 = m + a * l0;
		const float C2 = (a * P) / (P - S1);
		const float CP = -C2 / P;

		const float t = std::clamp(x / m, 0.0f, 1.0f);
		const float w0 = 1.0f - t * t * (3.0f - 2.0f * t); // 1 - smoothstep(0, m, x)
		const float w2 = (x >= m + l0)? 1.0f: 0.0f;
		const float w1 = 1.0f - w0 - w2;

		const float T = m * std::pow(x / m, cc) + b;
		const float S = P - (P - S1) * std::exp(CP * (x - S0));
		const float L = m + a * (x - m);
		return T * w0 + L * w1 + S * w2;
	});
}

inline RGB Reinhard(RGB c) {
	return PerChannel(c, [](float x) { x = std::max(x, 0.0f); return x / (1.0f + x); });
}

// "Jodie" Reinhard (shadertoy 4dBcD1): luminance and per-channel Reinhard blended.
inline RGB ReinhardJodie(RGB c) {
	const float l = REC709_LUMA[0] * c.r + REC709_LUMA[1] * c.g + REC709_LUMA[2] * c.b;
	return PerChannel(c, [l](float x) {
		const float tc = x / (x + 1.0f);
		const float lc = x / (l + 1.0f);
		return lc + (tc - lc) * tc;
	});
}

// Jim Hejl and Richard Burgess-Dawson; the sRGB-like encode is part of the fit.
inline RGB FilmicHejl(RGB c) {
	return PerChannel(c, [](float x) {
		x = std::max(0.0f, x - 0.004f);
		return (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);
	});
}

// Unreal Engine 3 filmic approximation with gamma 2.2 baked in.
inline RGB Unreal(RGB c) {
	return PerChannel(c, [](float x) { x = std::max(x, 0.0f); return x / (x + 0.155f) * 1.019f; });
}

// BAR's CUS unit curve `CustomTM` (modelmaterials_gl4/templates/cus_gl4.frag.glsl)
// with the constants that shader actually uses: its `pbrParams` array is a
// GLSL constant (4.85, 0.85, 3.5, 0.85, 1.0), not the tonemapA-E config values
// (whose defaults are 4.75 and 0.75). CUS runs with modelGamma 1, so the curve
// works on display-encoded values; the pass re-encodes the decoded scene value
// first and applies no encode after it, which reproduces a CUS unit pixel.
static constexpr float BAR_CUSTOM_A = 4.85f;
static constexpr float BAR_CUSTOM_B = 0.85f;
static constexpr float BAR_CUSTOM_C = 3.5f;
static constexpr float BAR_CUSTOM_D = 0.85f;
static constexpr float BAR_CUSTOM_E = 1.0f;
inline RGB BarCustom(RGB encoded) {
	return PerChannel(encoded, [](float x) {
		return (x * (BAR_CUSTOM_A * x + BAR_CUSTOM_B)) / (x * (BAR_CUSTOM_C * x + BAR_CUSTOM_D) + BAR_CUSTOM_E);
	});
}

// The full pass for one stored scene value. `stored` is what the scene target
// holds; the result is what the pass writes into the display (before the
// window's 8-bit quantisation).
inline RGB ApplyDisplayPass(Transform t, SceneTransfer transfer, float exposureLinear, RGB stored) {
	if (t == Transform::Legacy)
		return stored; // today's image, unchanged (no decode, exposure or curve)

	RGB lin = (transfer == SceneTransfer::SRGBExtended)? SRGBDecode(stored): stored;
	lin = {lin.r * exposureLinear, lin.g * exposureLinear, lin.b * exposureLinear};

	switch (t) {
		case Transform::KhronosNeutral: return SRGBEncode(KhronosNeutral(lin));
		case Transform::AgX           : return SRGBEncode(AgX(lin));
		case Transform::ACES          : return SRGBEncode(ACESNarkowicz(lin));
		case Transform::ACESHill      : return SRGBEncode(ACESHill(lin));
		case Transform::Hable         : return SRGBEncode(Hable(lin));
		case Transform::Uchimura      : return SRGBEncode(Uchimura(lin));
		case Transform::Reinhard      : return SRGBEncode(Reinhard(lin));
		case Transform::ReinhardJodie : return SRGBEncode(ReinhardJodie(lin));
		case Transform::FilmicHejl    : return FilmicHejl(lin);
		case Transform::Unreal        : return Unreal(lin);
		case Transform::BarCustom     : return BarCustom(SRGBEncode(lin));
		case Transform::LinearDebug   : return SRGBEncode(PerChannel(lin, Clamp01));
		default                       : return stored;
	}
}

inline float ExposureLinear(float ev) { return std::exp2(ev); }

} // namespace PBRDisplay

#endif
