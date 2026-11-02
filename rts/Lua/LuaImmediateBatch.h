/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

/**
 * Which per-vertex attribute a back-fill value is wanted for.
 */
enum class BatchAttrib {
	COLOR,
	NORMAL,
	SECONDARY_COLOR,
	FOG_COORD,
	EDGE_FLAG,
	TEXCOORD
};

/**
 * Supplies the value an attribute had when the block started.
 * Writes up to four floats to out. For EDGE_FLAG, out[0] is 0 or 1.
 * The unit argument is only meaningful for TEXCOORD.
 */
using BatchStartValueFn = std::function<void(BatchAttrib attrib, unsigned unit, float* out)>;

/**
 * Everything LuaImmediateBatch::Issue needs to draw one block, as plain data.
 * Array pointers refer to storage owned by the batch and stay valid until the
 * next outermost Begin().
 */
struct BatchLayout {
	static constexpr int MAX_TEXCOORD_UNITS = 32;

	// the vertex pointer sizes never vary, see coding-agents/LUA_IMMEDIATE_BATCHING.md
	static constexpr int POSITION_SIZE = 4;
	static constexpr int COLOR_SIZE = 4;
	static constexpr int NORMAL_SIZE = 3;
	static constexpr int SECONDARY_COLOR_SIZE = 3;
	static constexpr int FOG_COORD_SIZE = 1;
	static constexpr int TEXCOORD_SIZE = 4;

	bool draw = false; // false for nested and unbalanced End() calls, and for empty blocks
	uint32_t mode = 0;
	int32_t vertexCount = 0;

	// null when the block never touched the attribute, so the current value applies
	const float* position = nullptr;
	const float* color = nullptr;
	const float* normal = nullptr;
	const float* secondaryColor = nullptr;
	const float* fogCoord = nullptr;
	const uint8_t* edgeFlag = nullptr;
	std::array<const float*, MAX_TEXCOORD_UNITS> texCoord = {};

	// attributes the block touched keep their last value afterwards, as in immediate mode
	bool setColor = false;
	bool setNormal = false;
	bool setSecondaryColor = false;
	bool setFogCoord = false;
	bool setEdgeFlag = false;
	uint32_t setTexCoordMask = 0;

	std::array<float, 4> finalColor = {};
	std::array<float, 4> finalNormal = {};
	std::array<float, 4> finalSecondaryColor = {};
	float finalFogCoord = 0.0f;
	bool finalEdgeFlag = true;
	std::array<std::array<float, 4>, MAX_TEXCOORD_UNITS> finalTexCoord = {};
};

/**
 * Collects the vertices of one gl.BeginEnd block in memory so it can be drawn
 * with a single glDrawArrays instead of one glVertex call per vertex.
 *
 * End() is pure and produces a BatchLayout. Issue() makes the GL calls and is
 * defined in LuaImmediateBatchGL.cpp so the rest can be tested without GL.
 */
class LuaImmediateBatch {
public:
	/**
	 * @param startValue supplies attribute values at block start for back-fill.
	 * With none, the GL initial values are used.
	 */
	explicit LuaImmediateBatch(BatchStartValueFn startValue = nullptr);

	bool Active() const { return (depth > 0); }

	void Begin(uint32_t mode);
	void Vertex(float x, float y, float z, float w);
	void Color(const float* rgba);
	void Normal(float x, float y, float z);
	void TexCoord(unsigned unit, float s, float t, float r, float q);
	void SecondaryColor(float r, float g, float b);
	void FogCoord(float f);
	void EdgeFlag(bool e);
	BatchLayout End();

	/// Draws the layout and restores the GL state it changed.
	static void Issue(const BatchLayout& layout);

	/// Reads the current GL value of an attribute with glGetFloatv.
	static void ReadGLValue(BatchAttrib attrib, unsigned unit, float* out);

private:
	struct Attribute {
		std::vector<float> data;
		std::array<float, 4> current = {};
		std::array<float, 4> start = {};
		int size = 0;
		bool touched = false;
		bool active = false;
	};

	void Touch(Attribute& a, BatchAttrib kind, unsigned unit);
	void Reset();

	BatchStartValueFn startValue;

	int depth = 0;
	uint32_t mode = 0;
	int32_t vertexCount = 0;

	std::vector<float> position;

	Attribute color;
	Attribute normal;
	Attribute secondaryColor;
	Attribute fogCoord;
	Attribute edgeFlag;
	std::array<Attribute, BatchLayout::MAX_TEXCOORD_UNITS> texCoord;

	std::vector<uint8_t> edgeFlagBytes;
	std::vector<Attribute*> touchedAttributes;
};
