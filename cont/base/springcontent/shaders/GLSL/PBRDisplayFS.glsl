#version 330

// The one PBR display pass (planning M1, increment I6): decode the scene
// target's storage transfer, apply the manual exposure, run exactly one curve
// and encode once for an sRGB display. The CPU reference with the same
// functions is rts/Rendering/GL/DisplayTransforms_pbr.h; change both together.
// Uniform values match the C++ enums PBRDisplay::Transform and SceneTransfer.

uniform sampler2D sceneColor;
uniform int displayTransform;
uniform int sceneTransfer;
uniform float exposureLinear;

out vec4 fragColor;

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

const int TRANSFER_SRGB_EXTENDED = 0;

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

vec3 Unreal(vec3 x) {
	x = max(x, vec3(0.0));
	return x / (x + 0.155) * 1.019;
}

// BAR's CUS CustomTM with BAR's default tonemapA-E; it works on display-encoded
// values (CUS runs with modelGamma 1), so no encode follows it.
vec3 BarCustom(vec3 encoded) {
	const float A = 4.75, B = 0.75, C = 3.5, D = 0.85, E = 1.0;
	return (encoded * (A * encoded + B)) / (encoded * (C * encoded + D) + E);
}

void main() {
	vec4 stored = texelFetch(sceneColor, ivec2(gl_FragCoord.xy), 0);

	if (displayTransform == TM_LEGACY) {
		fragColor = stored;
		return;
	}

	vec3 lin = (sceneTransfer == TRANSFER_SRGB_EXTENDED) ? SRGBDecode(stored.rgb) : stored.rgb;
	lin *= exposureLinear;

	vec3 outColor;
	switch (displayTransform) {
		case TM_KHRONOS_NEUTRAL: outColor = SRGBEncode(KhronosNeutral(lin)); break;
		case TM_AGX            : outColor = SRGBEncode(AgX(lin)); break;
		case TM_ACES           : outColor = SRGBEncode(ACESNarkowicz(lin)); break;
		case TM_ACES_HILL      : outColor = SRGBEncode(ACESHill(lin)); break;
		case TM_HABLE          : outColor = SRGBEncode(Hable(lin)); break;
		case TM_UCHIMURA       : outColor = SRGBEncode(Uchimura(lin)); break;
		case TM_REINHARD       : outColor = SRGBEncode(Reinhard(lin)); break;
		case TM_REINHARD_JODIE : outColor = SRGBEncode(ReinhardJodie(lin)); break;
		case TM_FILMIC_HEJL    : outColor = FilmicHejl(lin); break;
		case TM_UNREAL         : outColor = Unreal(lin); break;
		case TM_BAR_CUSTOM     : outColor = BarCustom(SRGBEncode(lin)); break;
		case TM_LINEAR_DEBUG   : outColor = SRGBEncode(clamp(lin, 0.0, 1.0)); break;
		default                : outColor = stored.rgb; break;
	}

	// alpha passes through so window captures keep today's alpha
	fragColor = vec4(outColor, stored.a);
}
