/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaImmediateBatch.h"

#include "Rendering/GL/myGL.h"


void LuaImmediateBatch::ReadGLValue(BatchAttrib attrib, unsigned unit, float* out)
{
	switch (attrib) {
		case BatchAttrib::COLOR:
			glGetFloatv(GL_CURRENT_COLOR, out);
			break;
		case BatchAttrib::NORMAL:
			glGetFloatv(GL_CURRENT_NORMAL, out);
			break;
		case BatchAttrib::SECONDARY_COLOR:
			glGetFloatv(GL_CURRENT_SECONDARY_COLOR, out);
			break;
		case BatchAttrib::FOG_COORD:
			glGetFloatv(GL_CURRENT_FOG_COORD, out);
			break;
		case BatchAttrib::EDGE_FLAG: {
			GLboolean e = GL_TRUE;
			glGetBooleanv(GL_EDGE_FLAG, &e);
			out[0] = (e != GL_FALSE) ? 1.0f : 0.0f;
		} break;
		case BatchAttrib::TEXCOORD: {
			// GL_CURRENT_TEXTURE_COORDS follows the server side active texture unit
			GLint active = GL_TEXTURE0;
			glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
			glActiveTexture(GL_TEXTURE0 + unit);
			glGetFloatv(GL_CURRENT_TEXTURE_COORDS, out);
			glActiveTexture(active);
		} break;
	}
}


void LuaImmediateBatch::Issue(const BatchLayout& l)
{
	if (l.draw) {
		// client pointers are read as offsets into a bound buffer, and would
		// overwrite the attribute setup of a bound VAO
		GLint prevVAO = 0;
		GLint prevArrayBuffer = 0;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevArrayBuffer);
		if (prevVAO != 0)
			glBindVertexArray(0);
		if (prevArrayBuffer != 0)
			glBindBuffer(GL_ARRAY_BUFFER, 0);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(BatchLayout::POSITION_SIZE, GL_FLOAT, 0, l.position);

		if (l.color != nullptr) {
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(BatchLayout::COLOR_SIZE, GL_FLOAT, 0, l.color);
		}
		if (l.normal != nullptr) {
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, 0, l.normal);
		}
		if (l.secondaryColor != nullptr) {
			glEnableClientState(GL_SECONDARY_COLOR_ARRAY);
			glSecondaryColorPointer(BatchLayout::SECONDARY_COLOR_SIZE, GL_FLOAT, 0, l.secondaryColor);
		}
		if (l.fogCoord != nullptr) {
			glEnableClientState(GL_FOG_COORD_ARRAY);
			glFogCoordPointer(GL_FLOAT, 0, l.fogCoord);
		}
		if (l.edgeFlag != nullptr) {
			glEnableClientState(GL_EDGE_FLAG_ARRAY);
			glEdgeFlagPointer(0, l.edgeFlag);
		}

		bool texArrays = false;
		for (int u = 0; u < BatchLayout::MAX_TEXCOORD_UNITS; u++) {
			if (l.texCoord[u] == nullptr)
				continue;

			glClientActiveTexture(GL_TEXTURE0 + u);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(BatchLayout::TEXCOORD_SIZE, GL_FLOAT, 0, l.texCoord[u]);
			texArrays = true;
		}

		glDrawArrays(l.mode, 0, l.vertexCount);

		// the arrays must not leak into later draws, which expect client state off
		if (texArrays) {
			for (int u = 0; u < BatchLayout::MAX_TEXCOORD_UNITS; u++) {
				if (l.texCoord[u] == nullptr)
					continue;

				glClientActiveTexture(GL_TEXTURE0 + u);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
			glClientActiveTexture(GL_TEXTURE0);
		}
		if (l.edgeFlag != nullptr)
			glDisableClientState(GL_EDGE_FLAG_ARRAY);
		if (l.fogCoord != nullptr)
			glDisableClientState(GL_FOG_COORD_ARRAY);
		if (l.secondaryColor != nullptr)
			glDisableClientState(GL_SECONDARY_COLOR_ARRAY);
		if (l.normal != nullptr)
			glDisableClientState(GL_NORMAL_ARRAY);
		if (l.color != nullptr)
			glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);

		if (prevArrayBuffer != 0)
			glBindBuffer(GL_ARRAY_BUFFER, prevArrayBuffer);
		if (prevVAO != 0)
			glBindVertexArray(prevVAO);
	}

	// current values are undefined after drawing from an array, and a block
	// that set them in immediate mode leaves the last value behind
	if (l.setColor)
		glColor4fv(l.finalColor.data());
	if (l.setNormal)
		glNormal3fv(l.finalNormal.data());
	if (l.setSecondaryColor)
		glSecondaryColor3fv(l.finalSecondaryColor.data());
	if (l.setFogCoord)
		glFogCoordf(l.finalFogCoord);
	if (l.setEdgeFlag)
		glEdgeFlag(l.finalEdgeFlag ? GL_TRUE : GL_FALSE);

	for (int u = 0; u < BatchLayout::MAX_TEXCOORD_UNITS; u++) {
		if ((l.setTexCoordMask & (1u << u)) == 0)
			continue;

		const auto& t = l.finalTexCoord[u];
		glMultiTexCoord4f(GL_TEXTURE0 + u, t[0], t[1], t[2], t[3]);
	}
}
