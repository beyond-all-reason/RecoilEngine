/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// Recoil's check of the shared PBR BRDF golden table (planning decision D011).
//
// BAR and Recoil cannot share code, so the BRDF convention is shared as data:
// BAR generates modelmaterials_gl4/validation/pbr_brdf_golden.lua (float64,
// generate_pbr_brdf_golden.py) and Recoil keeps a verbatim copy next to this
// file, with the SHA-512 of its text in pbr_brdf_golden.lua.sha512. This test
//   1. proves the copy is unchanged: the SHA-512 of its LF-normalised text
//      equals the recorded one (BAR stores .lua files with CRLF, a copy may be
//      LF; the generator and BAR's self-test hash the same LF text);
//   2. evaluates the convention's functions on the CPU in float, the precision
//      engine C++ uses, and compares every tuple's alpha, D, V, F, specular,
//      diffuse and BRDF;
//   3. integrates the split-sum DFG at the table's DFG sample points in double,
//      with the same Hammersley/GGX importance sampling as the engine's
//      engine-generated source (CubeMapHandler::GeneratePBRDFG), and compares.
// Convention BAR_PBR_V1_GLTF_GGX_SMITHCORR_SCHLICK_LAMBERT_FMIX_DFG_SCALE_BIAS:
// GGX D, height-correlated Smith V (with the 1 / (4 NoL NoV) folded in),
// Schlick F with F90 = 1, glTF fresnel_mix diffuse (1 - F) c (1 - m) / pi,
// perceptual roughness clamped to minRoughness, alpha = r^2,
// F0 = mix(dielectricF0, c, m).
//
// When the table changes in BAR: copy the new file here unchanged and put the
// digest that generate_pbr_brdf_golden.py --check prints into the .sha512 file.
//
// Run: build the test_PBRBrdfGolden target (it is part of `tests`), then run
// it or `ctest -R PBRBrdfGolden`. Expected: all assertions pass.

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "System/Sync/SHA512.hpp"

#include <catch_amalgamated.hpp>

#ifndef PBR_GOLDEN_DIR
#error "PBR_GOLDEN_DIR must name the folder of pbr_brdf_golden.lua (set in test/CMakeLists.txt)"
#endif

namespace {
	constexpr const char* CONVENTION = "BAR_PBR_V1_GLTF_GGX_SMITHCORR_SCHLICK_LAMBERT_FMIX_DFG_SCALE_BIAS";
	// relative tolerance of the float evaluation against the float64 golden
	// values (BAR's GPU check uses the same 1e-4; float keeps about 6e-8, the
	// division-heavy D and V near rMin lose a few digits more)
	constexpr double BRDF_RELATIVE_TOLERANCE = 1e-4;
	// absolute tolerance of the double DFG integration against the table's 9
	// printed decimals (the arithmetic is the same, so only rounding differs)
	constexpr double DFG_ABSOLUTE_TOLERANCE = 1e-6;
	// Hammersley samples of the DFG integral; the table's samples are taken
	// with the same count (pbr_dfg.lua, CubeMapHandler::PBR_DFG_SAMPLES)
	constexpr unsigned int DFG_SAMPLES = 1024;
	constexpr double PI_D = 3.14159265358979323846;
	constexpr float PI_F = 3.14159265358979323846f;

	std::string ReadFile(const std::string& path)
	{
		std::ifstream file(path, std::ios::binary);
		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	}

	std::string WithoutCarriageReturns(const std::string& text)
	{
		std::string result;
		result.reserve(text.size());
		for (const char c: text) {
			if (c != '\r')
				result.push_back(c);
		}
		return result;
	}

	// value after "<key> = " in a line, or NaN when the key is absent; keys are
	// matched with a leading space or tab so "D" does not match inside "NoH"
	double Field(const std::string& line, const std::string& key)
	{
		for (const char* prefix: {" ", "\t"}) {
			const std::string token = prefix + key + " = ";
			const size_t at = line.find(token);
			if (at != std::string::npos)
				return std::strtod(line.c_str() + at + token.size(), nullptr);
		}
		return std::nan("");
	}

	// the three numbers of "<key> = { a, b, c }"
	std::array<double, 3> Vector(const std::string& line, const std::string& key)
	{
		std::array<double, 3> v = {std::nan(""), std::nan(""), std::nan("")};
		const std::string token = " " + key + " = {";
		const size_t at = line.find(token);
		if (at == std::string::npos)
			return v;
		const char* cursor = line.c_str() + at + token.size();
		for (double& x: v) {
			char* end = nullptr;
			x = std::strtod(cursor, &end);
			cursor = end;
			while (*cursor == ',' || *cursor == ' ')
				++cursor;
		}
		return v;
	}

	struct Tuple {
		std::string name;
		double NoL, NoV, NoH, VoH, roughness, metallic;
		std::array<double, 3> base;
		double alpha, D, V;
		std::array<double, 3> F, specular, diffuse, brdf;
	};

	struct Table {
		std::string convention;
		double version = 0, dielectricF0 = 0, minRoughness = 0;
		std::vector<Tuple> tuples;
		std::vector<std::array<double, 4>> dfg; // NoV, roughness, A, B
	};

	// Reads the generated layout (one tuple = two lines: inputs, expected values;
	// DFG samples one per line inside "dfg = {"). The file is generated, so a
	// line-oriented reader is enough.
	Table Parse(const std::string& text)
	{
		Table table;
		std::istringstream lines(text);
		std::string line;
		bool inDFG = false;
		while (std::getline(lines, line)) {
			if (line.find("convention = \"") != std::string::npos) {
				const size_t first = line.find('"') + 1;
				table.convention = line.substr(first, line.rfind('"') - first);
			} else if (line.find("\tversion = ") == 0) {
				table.version = Field(line, "version");
			} else if (line.find("\tdielectricF0 = ") == 0) {
				table.dielectricF0 = Field(line, "dielectricF0");
			} else if (line.find("\tminRoughness = ") == 0) {
				table.minRoughness = Field(line, "minRoughness");
			} else if (line.find("dfg = {") != std::string::npos) {
				inDFG = true;
			} else if (line.find("name = \"") != std::string::npos) {
				Tuple t;
				const size_t first = line.find("name = \"") + 8;
				t.name = line.substr(first, line.find('"', first) - first);
				t.NoL = Field(line, "NoL"); t.NoV = Field(line, "NoV"); t.NoH = Field(line, "NoH"); t.VoH = Field(line, "VoH");
				t.roughness = Field(line, "roughness"); t.metallic = Field(line, "metallic"); t.base = Vector(line, "base");
				std::string expected;
				std::getline(lines, expected);
				t.alpha = Field(expected, "alpha"); t.D = Field(expected, "D"); t.V = Field(expected, "V");
				t.F = Vector(expected, "F"); t.specular = Vector(expected, "specular");
				t.diffuse = Vector(expected, "diffuse"); t.brdf = Vector(expected, "brdf");
				table.tuples.push_back(t);
			} else if (inDFG && line.find('{') != std::string::npos) {
				std::array<double, 4> sample = {};
				const char* cursor = line.c_str() + line.find('{') + 1;
				for (double& x: sample) {
					char* end = nullptr;
					x = std::strtod(cursor, &end);
					cursor = end + 1;
				}
				table.dfg.push_back(sample);
			}
		}
		return table;
	}

	// The convention in float, as engine C++ evaluates it. D uses the
	// algebraically identical form (alpha / ((1 - NoH^2) + (NoH alpha)^2))^2 / pi:
	// the textbook alpha^2 / (pi (NoH^2 (alpha^2 - 1) + 1)^2) cancels in float at
	// the rMin clamp (BAR's GPU check measured 0.6 %).
	float D_GGX(float nDotH, float alpha)
	{
		const float a = nDotH * alpha;
		const float k = alpha / ((1.0f - nDotH * nDotH) + a * a);
		return k * k / PI_F;
	}

	float V_SmithGGXCorrelated(float nDotV, float nDotL, float alpha)
	{
		const float a2 = alpha * alpha;
		const float ggxV = nDotL * std::sqrt(nDotV * nDotV * (1.0f - a2) + a2);
		const float ggxL = nDotV * std::sqrt(nDotL * nDotL * (1.0f - a2) + a2);
		return 0.5f / (ggxV + ggxL);
	}

	float F_Schlick(float vDotH, float f0, float f90)
	{
		return f0 + (f90 - f0) * std::pow(1.0f - vDotH, 5.0f);
	}

	double Relative(double measured, double expected)
	{
		return std::abs(measured - expected) / std::max(std::abs(expected), 1e-6);
	}

	// Van der Corput radical inverse of a 32-bit index (bit reversal), as in
	// CubeMapHandler::GeneratePBRDFG and generate_pbr_dfg.py
	double RadicalInverse(uint32_t bits)
	{
		bits = (bits << 16u) | (bits >> 16u);
		bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
		bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
		bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
		bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		return static_cast<double>(bits) * 2.3283064365386963e-10; // / 2^32
	}

	// split-sum (A, B) for one NdotV and perceptual roughness: GGX importance
	// sampling of h, l = reflect(-v, h), weight V 4 (v.h) (n.l) / (n.h)
	std::array<double, 2> DFG(double nDotV, double roughness)
	{
		const double alpha = roughness * roughness;
		const double a2 = alpha * alpha;
		const double v[3] = {std::sqrt(1.0 - nDotV * nDotV), 0.0, nDotV};
		double a = 0.0, b = 0.0;
		for (unsigned int i = 0; i < DFG_SAMPLES; ++i) {
			const double phi = 2.0 * PI_D * (static_cast<double>(i) / DFG_SAMPLES);
			const double xiY = RadicalInverse(i);
			const double cosH = std::sqrt((1.0 - xiY) / (1.0 + (a2 - 1.0) * xiY));
			const double sinH = std::sqrt(1.0 - cosH * cosH);
			const double h[3] = {sinH * std::cos(phi), sinH * std::sin(phi), cosH};
			const double vDotH = v[0] * h[0] + v[1] * h[1] + v[2] * h[2];
			const double nDotL = 2.0 * vDotH * h[2] - v[2];
			if (nDotL <= 0.0)
				continue;
			const double vh = std::min(std::max(vDotH, 0.0), 1.0);
			const double ggxV = nDotL * std::sqrt(nDotV * nDotV * (1.0 - a2) + a2);
			const double ggxL = nDotV * std::sqrt(nDotL * nDotL * (1.0 - a2) + a2);
			const double gv = (0.5 / (ggxV + ggxL)) * 4.0 * vh * nDotL / h[2];
			const double fc = std::pow(1.0 - vh, 5.0);
			a += (1.0 - fc) * gv;
			b += fc * gv;
		}
		return {a / DFG_SAMPLES, b / DFG_SAMPLES};
	}
}

TEST_CASE("PBRBrdfGolden")
{
	const std::string dir = PBR_GOLDEN_DIR;
	const std::string text = ReadFile(dir + "/pbr_brdf_golden.lua");
	REQUIRE(!text.empty());

	SECTION("the copy is BAR's table unchanged (SHA-512 of the LF text)") {
		const std::string lf = WithoutCarriageReturns(text);
		sha512::raw_digest digest;
		sha512::calc_digest(std::vector<uint8_t>(lf.begin(), lf.end()), digest);
		std::string recorded = ReadFile(dir + "/pbr_brdf_golden.lua.sha512");
		recorded = recorded.substr(0, recorded.find_first_of("\r\n"));
		CHECK(sha512::dump_digest(digest) == recorded);
	}

	const Table table = Parse(text);

	SECTION("the table declares the convention this test implements") {
		CHECK(table.convention == CONVENTION);
		CHECK(table.version == 2.0);
		CHECK(table.tuples.size() == 8);
		CHECK(table.dfg.size() == 7);
	}

	SECTION("float D, V, F, specular, diffuse and BRDF reproduce every tuple") {
		for (const Tuple& t: table.tuples) {
			INFO(t.name);
			const float r = std::min(std::max(static_cast<float>(t.roughness), static_cast<float>(table.minRoughness)), 1.0f);
			const float alpha = r * r;
			const float d = D_GGX(static_cast<float>(t.NoH), alpha);
			const float vis = V_SmithGGXCorrelated(static_cast<float>(t.NoV), static_cast<float>(t.NoL), alpha);
			CHECK(Relative(alpha, t.alpha) <= BRDF_RELATIVE_TOLERANCE);
			CHECK(Relative(d, t.D) <= BRDF_RELATIVE_TOLERANCE);
			CHECK(Relative(vis, t.V) <= BRDF_RELATIVE_TOLERANCE);
			for (int c = 0; c < 3; ++c) {
				const float base = static_cast<float>(t.base[c]);
				const float metallic = static_cast<float>(t.metallic);
				const float f0 = static_cast<float>(table.dielectricF0) + (base - static_cast<float>(table.dielectricF0)) * metallic;
				const float f = F_Schlick(static_cast<float>(t.VoH), f0, 1.0f);
				const float specular = d * vis * f;
				const float diffuse = (1.0f - f) * base * (1.0f - metallic) / PI_F;
				CHECK(Relative(f, t.F[c]) <= BRDF_RELATIVE_TOLERANCE);
				CHECK(Relative(specular, t.specular[c]) <= BRDF_RELATIVE_TOLERANCE);
				CHECK(Relative(diffuse, t.diffuse[c]) <= BRDF_RELATIVE_TOLERANCE);
				CHECK(Relative(diffuse + specular, t.brdf[c]) <= BRDF_RELATIVE_TOLERANCE);
			}
		}
	}

	SECTION("the double DFG integral reproduces every DFG sample") {
		for (const auto& sample: table.dfg) {
			const std::array<double, 2> ab = DFG(sample[0], sample[1]);
			INFO("NoV " << sample[0] << " roughness " << sample[1]);
			CHECK(std::abs(ab[0] - sample[2]) <= DFG_ABSOLUTE_TOLERANCE);
			CHECK(std::abs(ab[1] - sample[3]) <= DFG_ABSOLUTE_TOLERANCE);
		}
	}
}
