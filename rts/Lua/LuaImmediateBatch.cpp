/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaImmediateBatch.h"

#include <algorithm>

namespace {
	// GL initial values, used when no start value source is given
	void DefaultStartValue(BatchAttrib attrib, unsigned, float* out)
	{
		switch (attrib) {
			case BatchAttrib::COLOR:
				std::fill(out, out + 4, 1.0f);
				break;
			case BatchAttrib::NORMAL:
				out[0] = 0.0f; out[1] = 0.0f; out[2] = 1.0f;
				break;
			case BatchAttrib::TEXCOORD:
				out[0] = 0.0f; out[1] = 0.0f; out[2] = 0.0f; out[3] = 1.0f;
				break;
			case BatchAttrib::EDGE_FLAG:
				out[0] = 1.0f;
				break;
			default:
				out[0] = 0.0f;
				break;
		}
	}
}


LuaImmediateBatch::LuaImmediateBatch(BatchStartValueFn fn)
	: startValue(fn ? std::move(fn) : BatchStartValueFn(DefaultStartValue))
{
	color.size = BatchLayout::COLOR_SIZE;
	normal.size = BatchLayout::NORMAL_SIZE;
	secondaryColor.size = BatchLayout::SECONDARY_COLOR_SIZE;
	fogCoord.size = BatchLayout::FOG_COORD_SIZE;
	edgeFlag.size = 1;
	for (Attribute& a: texCoord)
		a.size = BatchLayout::TEXCOORD_SIZE;

	touchedAttributes.reserve(BatchLayout::MAX_TEXCOORD_UNITS + 5);
}


void LuaImmediateBatch::Reset()
{
	vertexCount = 0;
	position.clear();
	edgeFlagBytes.clear();
	touchedAttributes.clear();

	const auto reset = [](Attribute& a) {
		a.data.clear();
		a.touched = false;
		a.active = false;
	};
	reset(color);
	reset(normal);
	reset(secondaryColor);
	reset(fogCoord);
	reset(edgeFlag);
	for (Attribute& a: texCoord)
		reset(a);
}


void LuaImmediateBatch::Begin(uint32_t newMode)
{
	// a nested Begin joins the outer block
	if (depth++ > 0)
		return;

	Reset();
	mode = newMode;
}


void LuaImmediateBatch::Touch(Attribute& a, BatchAttrib kind, unsigned unit)
{
	if (a.touched)
		return;

	a.touched = true;
	touchedAttributes.push_back(&a);

	// only vertices already emitted need the block-start value
	if (vertexCount > 0)
		startValue(kind, unit, a.start.data());
}


void LuaImmediateBatch::Vertex(float x, float y, float z, float w)
{
	if (depth == 0)
		return;

	position.insert(position.end(), {x, y, z, w});

	for (Attribute* a: touchedAttributes) {
		if (!a->active) {
			// late activation, earlier vertices all had the block-start value
			a->active = true;
			for (int32_t i = 0; i < vertexCount; i++)
				a->data.insert(a->data.end(), a->start.begin(), a->start.begin() + a->size);
		}
		a->data.insert(a->data.end(), a->current.begin(), a->current.begin() + a->size);
	}

	vertexCount++;
}


void LuaImmediateBatch::Color(const float* rgba)
{
	if (depth == 0)
		return;

	Touch(color, BatchAttrib::COLOR, 0);
	std::copy(rgba, rgba + 4, color.current.begin());
}


void LuaImmediateBatch::Normal(float x, float y, float z)
{
	if (depth == 0)
		return;

	Touch(normal, BatchAttrib::NORMAL, 0);
	normal.current = {x, y, z, 0.0f};
}


void LuaImmediateBatch::TexCoord(unsigned unit, float s, float t, float r, float q)
{
	if (depth == 0 || unit >= texCoord.size())
		return;

	Attribute& a = texCoord[unit];
	Touch(a, BatchAttrib::TEXCOORD, unit);
	a.current = {s, t, r, q};
}


void LuaImmediateBatch::SecondaryColor(float r, float g, float b)
{
	if (depth == 0)
		return;

	Touch(secondaryColor, BatchAttrib::SECONDARY_COLOR, 0);
	secondaryColor.current = {r, g, b, 0.0f};
}


void LuaImmediateBatch::FogCoord(float f)
{
	if (depth == 0)
		return;

	Touch(fogCoord, BatchAttrib::FOG_COORD, 0);
	fogCoord.current = {f, 0.0f, 0.0f, 0.0f};
}


void LuaImmediateBatch::EdgeFlag(bool e)
{
	if (depth == 0)
		return;

	Touch(edgeFlag, BatchAttrib::EDGE_FLAG, 0);
	edgeFlag.current = {e ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f};
}


BatchLayout LuaImmediateBatch::End()
{
	BatchLayout layout;

	// unbalanced, or an inner End that leaves the outer block open
	if (depth == 0 || --depth > 0)
		return layout;

	layout.mode = mode;
	layout.vertexCount = vertexCount;
	layout.draw = (vertexCount > 0);

	if (layout.draw)
		layout.position = position.data();

	// an attribute that was touched but never reached a vertex is not drawn,
	// it still sets the current value like immediate mode would
	const auto arrayOf = [](const Attribute& a) { return a.active ? a.data.data() : nullptr; };

	layout.color = arrayOf(color);
	layout.normal = arrayOf(normal);
	layout.secondaryColor = arrayOf(secondaryColor);
	layout.fogCoord = arrayOf(fogCoord);

	if (edgeFlag.active) {
		edgeFlagBytes.assign(edgeFlag.data.size(), 0);
		for (size_t i = 0; i < edgeFlag.data.size(); i++)
			edgeFlagBytes[i] = (edgeFlag.data[i] != 0.0f) ? 1 : 0;
		layout.edgeFlag = edgeFlagBytes.data();
	}

	for (size_t u = 0; u < texCoord.size(); u++) {
		if (!texCoord[u].touched)
			continue;

		layout.texCoord[u] = arrayOf(texCoord[u]);
		layout.setTexCoordMask |= (1u << u);
		layout.finalTexCoord[u] = texCoord[u].current;
	}

	layout.setColor = color.touched;
	layout.finalColor = color.current;
	layout.setNormal = normal.touched;
	layout.finalNormal = normal.current;
	layout.setSecondaryColor = secondaryColor.touched;
	layout.finalSecondaryColor = secondaryColor.current;
	layout.setFogCoord = fogCoord.touched;
	layout.finalFogCoord = fogCoord.current[0];
	layout.setEdgeFlag = edgeFlag.touched;
	layout.finalEdgeFlag = (edgeFlag.current[0] != 0.0f);

	return layout;
}
