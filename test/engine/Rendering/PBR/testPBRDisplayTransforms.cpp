/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// CPU test of the PBR display pass curves (planning M1, increment I6; decision
// D013: header-only math with a CPU test). It checks
//   1. every PBRDisplayTransform curve in rts/Rendering/GL/DisplayTransforms_pbr.h
//      against float64 golden values from an independent implementation
//      (generate_display_transform_golden.py next to this file);
//   2. every curve is finite and monotonic on a gray ramp from 0 to 64;
//   3. the 18% gray card at PBRExposureEV=0 and the exposure mapping 2^EV;
//   4. the srgb-extended storage transfer round-trips values above one;
//   5. legacy passes the stored value through unchanged, and every name parses.
// The GPU side (the same curves in PBRDisplayFS.glsl) is checked in game by
// Spring.TestDisplayTransforms() against the same header.
//
// Run: build the test_PBRDisplayTransforms target (part of `tests`), then run it
// or `ctest -R PBRDisplayTransforms`. Expected: all assertions pass.

#include <cmath>
#include <string_view>

#include "Rendering/GL/DisplayTransforms_pbr.h"

#include <catch_amalgamated.hpp>

namespace {
	using namespace PBRDisplay;

	struct GoldenRow {
		const char* transform;
		float in[3];
		float out[3];
	};

	// float evaluation against float64 goldens; the curves divide and take
	// pow/exp/log2, which keep about 6 significant digits in float
	constexpr float GOLDEN_TOLERANCE = 2e-5f;
	constexpr float ROUND_TRIP_RELATIVE_TOLERANCE = 1e-5f;
	constexpr float GRAY_CARD = 0.18f;
	// sRGB encode of Khronos PBR Neutral at the gray card: the curve only
	// subtracts its 0.04 toe offset below the compression start (0.18 -> 0.14)
	constexpr float GRAY_CARD_KHRONOS_NEUTRAL = 0.410020902f;
	constexpr float RAMP_MAX = 64.0f;
	constexpr int RAMP_STEPS = 4096;
	// a later sample may dip below an earlier one by float rounding only
	constexpr float MONOTONIC_SLACK = 1e-6f;

	// GOLDEN BEGIN (generate_display_transform_golden.py)
	static const GoldenRow GOLDEN[] = {
		{"khronos-neutral", {0, 0, 0}, {0, 0, 0}},
		{"khronos-neutral", {0.02, 0.02, 0.02}, {0.0323, 0.0323, 0.0323}},
		{"khronos-neutral", {0.18, 0.18, 0.18}, {0.410020902, 0.410020902, 0.410020902}},
		{"khronos-neutral", {1, 1, 1}, {0.940091376, 0.940091376, 0.940091376}},
		{"khronos-neutral", {4, 4, 4}, {0.992603267, 0.992603267, 0.992603267}},
		{"khronos-neutral", {64, 64, 64}, {0.999600777, 0.999600777, 0.999600777}},
		{"khronos-neutral", {3, 0.75, 0.1}, {0.989550598, 0.668471941, 0.526254365}},
		{"agx", {0, 0, 0}, {0, 0, 0}},
		{"agx", {0.02, 0.02, 0.02}, {0.143874623, 0.143887484, 0.143888284}},
		{"agx", {0.18, 0.18, 0.18}, {0.500460297, 0.500530692, 0.500535072}},
		{"agx", {1, 1, 1}, {0.791773135, 0.79191077, 0.791919334}},
		{"agx", {4, 4, 4}, {0.936039715, 0.936222288, 0.936233649}},
		{"agx", {64, 64, 64}, {0.998460904, 0.99867022, 0.998683245}},
		{"agx", {3, 0.75, 0.1}, {0.947999699, 0.76800698, 0.542053001}},
		{"aces", {0, 0, 0}, {0, 0, 0}},
		{"aces", {0.02, 0.02, 0.02}, {0.103028727, 0.103028727, 0.103028727}},
		{"aces", {0.18, 0.18, 0.18}, {0.553457572, 0.553457572, 0.553457572}},
		{"aces", {1, 1, 1}, {0.908230496, 0.908230496, 0.908230496}},
		{"aces", {4, 4, 4}, {0.98822271, 0.98822271, 0.98822271}},
		{"aces", {64, 64, 64}, {1, 1, 1}},
		{"aces", {3, 0.75, 0.1}, {0.979384791, 0.873407929, 0.389811962}},
		{"aces-hill", {0, 0, 0}, {0, 0, 0}},
		{"aces-hill", {0.02, 0.02, 0.02}, {0.0418394687, 0.0418394687, 0.0418390652}},
		{"aces-hill", {0.18, 0.18, 0.18}, {0.358457391, 0.358457391, 0.358455668}},
		{"aces-hill", {1, 1, 1}, {0.808954034, 0.808954034, 0.808950435}},
		{"aces-hill", {4, 4, 4}, {0.958888394, 0.958888394, 0.95888417}},
		{"aces-hill", {64, 64, 64}, {1, 1, 1}},
		{"aces-hill", {3, 0.75, 0.1}, {0.988233002, 0.778437105, 0.429217493}},
		{"hable", {0, 0, 0}, {0, 0, 0}},
		{"hable", {0.02, 0.02, 0.02}, {0.129911725, 0.129911725, 0.129911725}},
		{"hable", {0.18, 0.18, 0.18}, {0.393471092, 0.393471092, 0.393471092}},
		{"hable", {1, 1, 1}, {0.730673507, 0.730673507, 0.730673507}},
		{"hable", {4, 4, 4}, {0.96306656, 0.96306656, 0.96306656}},
		{"hable", {64, 64, 64}, {1.10495148, 1.10495148, 1.10495148}},
		{"hable", {3, 0.75, 0.1}, {0.925038152, 0.671605354, 0.301963057}},
		{"uchimura", {0, 0, 0}, {0, 0, 0}},
		{"uchimura", {0.02, 0.02, 0.02}, {0.0953732753, 0.0953732753, 0.0953732753}},
		{"uchimura", {0.18, 0.18, 0.18}, {0.46015269, 0.46015269, 0.46015269}},
		{"uchimura", {1, 1, 1}, {0.920128402, 0.920128402, 0.920128402}},
		{"uchimura", {4, 4, 4}, {0.999875523, 0.999875523, 0.999875523}},
		{"uchimura", {64, 64, 64}, {1, 1, 1}},
		{"uchimura", {3, 0.75, 0.1}, {0.998944794, 0.857691402, 0.326381387}},
		{"reinhard", {0, 0, 0}, {0, 0, 0}},
		{"reinhard", {0.02, 0.02, 0.02}, {0.150005204, 0.150005204, 0.150005204}},
		{"reinhard", {0.18, 0.18, 0.18}, {0.426946133, 0.426946133, 0.426946133}},
		{"reinhard", {1, 1, 1}, {0.735356983, 0.735356983, 0.735356983}},
		{"reinhard", {4, 4, 4}, {0.906331753, 0.906331753, 0.906331753}},
		{"reinhard", {64, 64, 64}, {0.993206585, 0.993206585, 0.993206585}},
		{"reinhard", {3, 0.75, 0.1}, {0.880825021, 0.686188716, 0.333453323}},
		{"reinhard-jodie", {0, 0, 0}, {0, 0, 0}},
		{"reinhard-jodie", {0.02, 0.02, 0.02}, {0.150005204, 0.150005204, 0.150005204}},
		{"reinhard-jodie", {0.18, 0.18, 0.18}, {0.426946133, 0.426946133, 0.426946133}},
		{"reinhard-jodie", {1, 1, 1}, {0.735356983, 0.735356983, 0.735356983}},
		{"reinhard-jodie", {4, 4, 4}, {0.906331753, 0.906331753, 0.906331753}},
		{"reinhard-jodie", {64, 64, 64}, {0.993206585, 0.993206585, 0.993206585}},
		{"reinhard-jodie", {3, 0.75, 0.1}, {0.957632036, 0.650063034, 0.247645881}},
		{"filmic-hejl", {0, 0, 0}, {0, 0, 0}},
		{"filmic-hejl", {0.02, 0.02, 0.02}, {0.107979529, 0.107979529, 0.107979529}},
		{"filmic-hejl", {0.18, 0.18, 0.18}, {0.508028282, 0.508028282, 0.508028282}},
		{"filmic-hejl", {1, 1, 1}, {0.841188288, 0.841188288, 0.841188288}},
		{"filmic-hejl", {4, 4, 4}, {0.954133441, 0.954133441, 0.954133441}},
		{"filmic-hejl", {64, 64, 64}, {0.996986174, 0.996986174, 0.996986174}},
		{"filmic-hejl", {3, 0.75, 0.1}, {0.939886027, 0.800108785, 0.375042805}},
		{"unreal", {0, 0, 0}, {0, 0, 0}},
		{"unreal", {0.02, 0.02, 0.02}, {0.116457143, 0.116457143, 0.116457143}},
		{"unreal", {0.18, 0.18, 0.18}, {0.547522388, 0.547522388, 0.547522388}},
		{"unreal", {1, 1, 1}, {0.882251082, 0.882251082, 0.882251082}},
		{"unreal", {4, 4, 4}, {0.980986763, 0.980986763, 0.980986763}},
		{"unreal", {64, 64, 64}, {1.01653807, 1.01653807, 1.01653807}},
		{"unreal", {3, 0.75, 0.1}, {0.968938193, 0.844475138, 0.399607843}},
		{"bar-custom", {0, 0, 0}, {0, 0, 0}},
		{"bar-custom", {0.02, 0.02, 0.02}, {0.198897646, 0.198897646, 0.198897646}},
		{"bar-custom", {0.18, 0.18, 0.18}, {0.666536628, 0.666536628, 0.666536628}},
		{"bar-custom", {1, 1, 1}, {1.06542056, 1.06542056, 1.06542056}},
		{"bar-custom", {4, 4, 4}, {1.2460526, 1.2460526, 1.2460526}},
		{"bar-custom", {64, 64, 64}, {1.35982343, 1.35982343, 1.35982343}},
		{"bar-custom", {3, 0.75, 0.1}, {1.21882046, 1.0106181, 0.515317325}},
		{"linear-debug", {0, 0, 0}, {0, 0, 0}},
		{"linear-debug", {0.02, 0.02, 0.02}, {0.151703719, 0.151703719, 0.151703719}},
		{"linear-debug", {0.18, 0.18, 0.18}, {0.46135613, 0.46135613, 0.46135613}},
		{"linear-debug", {1, 1, 1}, {1, 1, 1}},
		{"linear-debug", {4, 4, 4}, {1, 1, 1}},
		{"linear-debug", {64, 64, 64}, {1, 1, 1}},
		{"linear-debug", {3, 0.75, 0.1}, {1, 0.880825021, 0.349190213}},
	};
	// GOLDEN END
}

TEST_CASE("PBRDisplayTransforms")
{
	SECTION("names") {
		for (int t = 0; t < int(Transform::Count); ++t) {
			Transform parsed;
			REQUIRE(ParseTransform(TransformName(Transform(t)), parsed));
			CHECK(int(parsed) == t);
		}
		Transform unused;
		CHECK_FALSE(ParseTransform("tony-mcmapface", unused)); // reserved, needs a LUT file
	}

	SECTION("golden values") {
		for (const GoldenRow& row: GOLDEN) {
			Transform t;
			REQUIRE(ParseTransform(row.transform, t));
			// the golden inputs are scene-linear: use the linear transfer
			const RGB out = ApplyDisplayPass(t, SceneTransfer::Linear, 1.0f, {row.in[0], row.in[1], row.in[2]});
			const float got[3] = {out.r, out.g, out.b};
			for (int c = 0; c < 3; ++c) {
				INFO(row.transform << " in " << row.in[0] << "," << row.in[1] << "," << row.in[2] << " channel " << c);
				CHECK(std::fabs(got[c] - row.out[c]) <= GOLDEN_TOLERANCE * std::max(1.0f, std::fabs(row.out[c])));
			}
		}
	}

	SECTION("finite and monotonic on a gray ramp") {
		for (int t = 0; t < int(Transform::Count); ++t) {
			float prev = -1.0f;
			for (int i = 0; i <= RAMP_STEPS; ++i) {
				const float x = RAMP_MAX * float(i) / float(RAMP_STEPS);
				const RGB out = ApplyDisplayPass(Transform(t), SceneTransfer::Linear, 1.0f, {x, x, x});
				INFO(TransformName(Transform(t)) << " at " << x);
				REQUIRE(std::isfinite(out.r));
				REQUIRE(std::isfinite(out.g));
				REQUIRE(std::isfinite(out.b));
				CHECK(out.r + MONOTONIC_SLACK >= prev);
				prev = out.r;
			}
		}
	}

	SECTION("gray card and exposure") {
		CHECK(ExposureLinear(0.0f) == 1.0f);
		CHECK(ExposureLinear(1.0f) == 2.0f);
		CHECK(ExposureLinear(-1.0f) == 0.5f);

		// the stored value of a legacy gray card is its sRGB encoding
		const float stored = SRGBEncode(GRAY_CARD);
		const RGB out = ApplyDisplayPass(Transform::KhronosNeutral, SceneTransfer::SRGBExtended, ExposureLinear(0.0f), {stored, stored, stored});
		CHECK(std::fabs(out.r - GRAY_CARD_KHRONOS_NEUTRAL) <= GOLDEN_TOLERANCE);

		// one stop more exposure equals doubling the scene light
		const RGB brighter = ApplyDisplayPass(Transform::KhronosNeutral, SceneTransfer::SRGBExtended, ExposureLinear(1.0f), {stored, stored, stored});
		const RGB doubled = ApplyDisplayPass(Transform::KhronosNeutral, SceneTransfer::Linear, 1.0f, {2.0f * GRAY_CARD, 2.0f * GRAY_CARD, 2.0f * GRAY_CARD});
		CHECK(std::fabs(brighter.r - doubled.r) <= GOLDEN_TOLERANCE);
	}

	SECTION("srgb-extended round trip above one") {
		for (const float x: {0.0f, 0.001f, 0.18f, 0.5f, 1.0f, 4.0f, 64.0f}) {
			INFO("value " << x);
			CHECK(std::fabs(SRGBDecode(SRGBEncode(x)) - x) <= ROUND_TRIP_RELATIVE_TOLERANCE * std::max(1.0f, x));
		}
		CHECK(SRGBEncode(4.0f) > 1.0f); // kept above one, not clamped
		CHECK(SRGBDecode(-0.5f) == 0.0f); // negatives an 8-bit window would have clamped
	}

	SECTION("legacy is today's image") {
		const RGB stored = {0.25f, 1.5f, 0.0f};
		const RGB out = ApplyDisplayPass(Transform::Legacy, SceneTransfer::SRGBExtended, ExposureLinear(3.0f), stored);
		CHECK(out.r == stored.r);
		CHECK(out.g == stored.g);
		CHECK(out.b == stored.b);
	}

	SECTION("one encode only") {
		for (int t = 0; t < int(Transform::Count); ++t) {
			const bool baked = CurveIncludesEncode(Transform(t));
			const std::string_view name = TransformName(Transform(t));
			CHECK(baked == (name == "filmic-hejl" || name == "unreal"));
		}
	}
}
