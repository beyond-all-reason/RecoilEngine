/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

#include <vector>

#include "Lua/LuaImmediateBatch.h"

namespace {
	constexpr uint32_t QUADS = 7; // the value does not matter to the accumulator
	constexpr uint32_t LINE_LOOP = 2;

	// stands in for glGetFloatv and counts how often it is asked
	struct FakeStart {
		int calls = 0;
		float color[4] = {0.1f, 0.2f, 0.3f, 0.4f};
		float texCoord[4] = {0.5f, 0.6f, 0.0f, 1.0f};

		BatchStartValueFn Fn()
		{
			return [this](BatchAttrib attrib, unsigned, float* out) {
				calls++;
				if (attrib == BatchAttrib::COLOR) {
					std::copy(color, color + 4, out);
				} else if (attrib == BatchAttrib::TEXCOORD) {
					std::copy(texCoord, texCoord + 4, out);
				} else {
					out[0] = 0.0f;
				}
			};
		}
	};

	std::vector<float> ToVec(const float* p, size_t n)
	{
		return std::vector<float>(p, p + n);
	}
}


TEST_CASE("LuaImmediateBatch position only")
{
	LuaImmediateBatch b;
	b.Begin(QUADS);
	b.Vertex(1, 2, 3, 1);
	b.Vertex(4, 5, 6, 1);
	const BatchLayout l = b.End();

	CHECK(l.draw);
	CHECK(l.mode == QUADS);
	CHECK(l.vertexCount == 2);
	CHECK(ToVec(l.position, 8) == std::vector<float>{1, 2, 3, 1, 4, 5, 6, 1});

	SECTION("untouched attributes are not activated")
	{
		CHECK(l.color == nullptr);
		CHECK(l.normal == nullptr);
		CHECK(l.secondaryColor == nullptr);
		CHECK(l.fogCoord == nullptr);
		CHECK(l.edgeFlag == nullptr);
		for (const float* t: l.texCoord)
			CHECK(t == nullptr);
		CHECK_FALSE(l.setColor);
		CHECK(l.setTexCoordMask == 0);
	}

	SECTION("pointer sizes are pinned")
	{
		CHECK(BatchLayout::POSITION_SIZE == 4);
		CHECK(BatchLayout::TEXCOORD_SIZE == 4);
	}
}


TEST_CASE("LuaImmediateBatch activates only touched attributes")
{
	LuaImmediateBatch b;
	b.Begin(QUADS);
	b.TexCoord(2, 0.25f, 0.5f, 0.0f, 1.0f);
	b.Vertex(1, 2, 0, 1);
	const BatchLayout l = b.End();

	CHECK(l.color == nullptr);
	CHECK(l.texCoord[0] == nullptr);
	CHECK(l.texCoord[1] == nullptr);
	REQUIRE(l.texCoord[2] != nullptr);
	CHECK(ToVec(l.texCoord[2], 4) == std::vector<float>{0.25f, 0.5f, 0.0f, 1.0f});
}


TEST_CASE("LuaImmediateBatch back-fills a late activation")
{
	FakeStart start;
	LuaImmediateBatch b(start.Fn());

	SECTION("colour set after two vertices")
	{
		const float red[4] = {1, 0, 0, 1};
		b.Begin(LINE_LOOP);
		b.Vertex(0, 0, 0, 1);
		b.Vertex(1, 0, 0, 1);
		CHECK(start.calls == 0);
		b.Color(red);
		b.Vertex(1, 1, 0, 1);
		const BatchLayout l = b.End();

		CHECK(start.calls == 1);
		REQUIRE(l.color != nullptr);
		CHECK(ToVec(l.color, 12) == std::vector<float>{
			0.1f, 0.2f, 0.3f, 0.4f,
			0.1f, 0.2f, 0.3f, 0.4f,
			1, 0, 0, 1});
	}

	SECTION("an attribute touched before the first vertex reads nothing")
	{
		const float red[4] = {1, 0, 0, 1};
		b.Begin(LINE_LOOP);
		b.Color(red);
		b.Vertex(0, 0, 0, 1);
		b.Vertex(1, 0, 0, 1);
		const BatchLayout l = b.End();

		CHECK(start.calls == 0);
		REQUIRE(l.color != nullptr);
		CHECK(ToVec(l.color, 8) == std::vector<float>{1, 0, 0, 1, 1, 0, 0, 1});
	}

	SECTION("an attribute keeps its value for later vertices")
	{
		const float red[4] = {1, 0, 0, 1};
		b.Begin(LINE_LOOP);
		b.Color(red);
		b.Vertex(0, 0, 0, 1);
		b.Vertex(1, 0, 0, 1);
		b.Vertex(1, 1, 0, 1);
		const BatchLayout l = b.End();

		REQUIRE(l.color != nullptr);
		CHECK(ToVec(l.color, 12) == std::vector<float>{1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1});
	}

	SECTION("texture coordinates back-fill all four components")
	{
		b.Begin(QUADS);
		b.Vertex(0, 0, 0, 1);
		b.TexCoord(0, 1, 1, 0, 1);
		b.Vertex(1, 0, 0, 1);
		const BatchLayout l = b.End();

		REQUIRE(l.texCoord[0] != nullptr);
		CHECK(ToVec(l.texCoord[0], 8) == std::vector<float>{0.5f, 0.6f, 0.0f, 1.0f, 1, 1, 0, 1});
	}

	SECTION("a late touch that no vertex follows is not drawn but still applies")
	{
		const float red[4] = {1, 0, 0, 1};
		b.Begin(LINE_LOOP);
		b.Vertex(0, 0, 0, 1);
		b.Color(red);
		const BatchLayout l = b.End();

		CHECK(l.color == nullptr);
		CHECK(l.setColor);
		CHECK(l.finalColor == std::array<float, 4>{1, 0, 0, 1});
	}
}


TEST_CASE("LuaImmediateBatch reapplies the final value after End")
{
	LuaImmediateBatch b;
	const float red[4] = {1, 0, 0, 1};
	const float blue[4] = {0, 0, 1, 0.5f};

	b.Begin(QUADS);
	b.Color(red);
	b.Vertex(0, 0, 0, 1);
	b.Color(blue);
	b.Vertex(1, 0, 0, 1);
	b.Normal(0, 1, 0);
	b.TexCoord(1, 0.5f, 0.5f, 0, 1);
	const BatchLayout l = b.End();

	CHECK(l.setColor);
	CHECK(l.finalColor == std::array<float, 4>{0, 0, 1, 0.5f});
	CHECK(l.setNormal);
	CHECK(l.finalNormal[1] == 1.0f);
	CHECK(l.setTexCoordMask == (1u << 1));
	CHECK(l.finalTexCoord[1] == std::array<float, 4>{0.5f, 0.5f, 0, 1});
	CHECK_FALSE(l.setFogCoord);
}


TEST_CASE("LuaImmediateBatch edge flags")
{
	LuaImmediateBatch b;
	b.Begin(QUADS);
	b.EdgeFlag(false);
	b.Vertex(0, 0, 0, 1);
	b.EdgeFlag(true);
	b.Vertex(1, 0, 0, 1);
	const BatchLayout l = b.End();

	REQUIRE(l.edgeFlag != nullptr);
	CHECK(l.edgeFlag[0] == 0);
	CHECK(l.edgeFlag[1] == 1);
	CHECK(l.finalEdgeFlag);
}


TEST_CASE("LuaImmediateBatch counts nesting depth")
{
	LuaImmediateBatch b;
	CHECK_FALSE(b.Active());

	b.Begin(QUADS);
	CHECK(b.Active());
	b.Vertex(0, 0, 0, 1);

	b.Begin(LINE_LOOP); // joins the outer block, keeps its mode
	CHECK(b.Active());
	b.Vertex(1, 0, 0, 1);

	const BatchLayout inner = b.End();
	CHECK_FALSE(inner.draw);
	CHECK(b.Active());

	b.Vertex(2, 0, 0, 1);
	const BatchLayout outer = b.End();
	CHECK_FALSE(b.Active());
	CHECK(outer.draw);
	CHECK(outer.mode == QUADS);
	CHECK(outer.vertexCount == 3);
}


TEST_CASE("LuaImmediateBatch ignores calls outside a block")
{
	LuaImmediateBatch b;
	b.Vertex(1, 2, 3, 1);
	CHECK_FALSE(b.End().draw); // unbalanced End

	b.Begin(QUADS);
	const BatchLayout l = b.End();
	CHECK_FALSE(l.draw); // empty block
	CHECK(l.vertexCount == 0);
}


TEST_CASE("LuaImmediateBatch starts each block clean")
{
	FakeStart start;
	LuaImmediateBatch b(start.Fn());
	const float red[4] = {1, 0, 0, 1};

	b.Begin(QUADS);
	b.Color(red);
	b.Vertex(0, 0, 0, 1);
	b.End();

	b.Begin(LINE_LOOP);
	b.Vertex(5, 5, 5, 1);
	const BatchLayout l = b.End();

	CHECK(l.vertexCount == 1);
	CHECK(l.color == nullptr);
	CHECK_FALSE(l.setColor);
}
