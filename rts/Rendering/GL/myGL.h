/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MY_GL_H
#define _MY_GL_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <array>
#include <cstring>

#include <glad/glad.h>

#include "System/float3.h"
#include "System/float4.h"
#include "System/type2.h"
#include "System/UnorderedMap.hpp"

#include "glStateDebug.h"
#include "glDebugGroup.hpp"

#if       defined(HEADLESS)
	#define IS_GL_FUNCTION_AVAILABLE(functionName) true
#else
	#define IS_GL_FUNCTION_AVAILABLE(functionName) (functionName != nullptr)
#endif

// macOS core profile matrix tracking
// On Apple's Metal GL wrapper, fixed-function matrix ops are no-ops on core profile.
// We track them ourselves so shaders can get the correct MVP via glGetFloatv.
#if defined(__APPLE__)
namespace spring_matrix {
	static GLint g_matMode = GL_MODELVIEW;
	static float g_modelview[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
	static float g_projection[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
	// Per-mode matrix stacks – each matrix mode has its OWN stack in real OpenGL
	static constexpr int kMatrixStackSize = 16;
	static float g_modelviewStack[kMatrixStackSize][16];
	static float g_projectionStack[kMatrixStackSize][16];
	static int g_modelviewStackTop = 0;
	static int g_projectionStackTop = 0;
	// Backup: last ortho projection (saved before 3D perspective or identity overrides it)
	static float g_lastOrthoProj[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
	static bool  g_hasOrthoProj = false;
	// Returns true if current projection isn't a 2D ortho — we should use the backup
	static bool shouldUseOrthoBackup() {
		if (!spring_matrix::g_hasOrthoProj) return false;
		const float* p = spring_matrix::g_projection;
		// Identity projection (all 1s on diagonal, 0s elsewhere) → use ortho
		if (p[0] == 1.0f && p[5] == 1.0f && p[10] == 1.0f && p[15] == 1.0f &&
		    p[1] == 0.0f && p[2] == 0.0f && p[3] == 0.0f &&
		    p[4] == 0.0f && p[6] == 0.0f && p[7] == 0.0f &&
		    p[8] == 0.0f && p[9] == 0.0f && p[11] == 0.0f &&
		    p[12] == 0.0f && p[13] == 0.0f && p[14] == 0.0f)
			return true;
		// 3D perspective (large near/far depth values) → use ortho
		if (fabsf(p[14]) > 100.0f) return true;
		return false;
	}

	inline void multMatrix(float* dst, const float* a, const float* b) {
		float tmp[16] = {0};
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				for (int k = 0; k < 4; k++)
					tmp[i*4+j] += a[i*4+k] * b[k*4+j];
		memcpy(dst, tmp, 16*sizeof(float));
	}

	inline void loadIdentity(float* m) {
		memset(m, 0, 16*sizeof(float));
		m[0] = m[5] = m[10] = m[15] = 1.0f;
	}

	inline void ortho(float* m, double l, double r, double b, double t, double n, double f) {
		loadIdentity(m);
		m[0] = 2.0f / (r - l);
		m[5] = 2.0f / (t - b);
		m[10] = -2.0f / (f - n);
		m[12] = -(r + l) / (r - l);
		m[13] = -(t + b) / (t - b);
		m[14] = -(f + n) / (f - n);
	}

	inline void translate(float* m, float x, float y, float z) {
		float t[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, x,y,z,1};
		multMatrix(m, m, t);
	}

	inline void scale(float* m, float x, float y, float z) {
		float s[16] = {x,0,0,0, 0,y,0,0, 0,0,z,0, 0,0,0,1};
		multMatrix(m, m, s);
	}

	inline void rotate(float* m, float angle, float x, float y, float z) {
		float rad = angle * 3.14159265f / 180.0f;
		float c = cosf(rad), s = sinf(rad);
		float r[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
		float len = sqrtf(x*x + y*y + z*z);
		if (len > 0) { x /= len; y /= len; z /= len; }
		r[0] = c + x*x*(1-c); r[1] = x*y*(1-c) + z*s; r[2] = x*z*(1-c) - y*s;
		r[4] = x*y*(1-c) - z*s; r[5] = c + y*y*(1-c); r[6] = y*z*(1-c) + x*s;
		r[8] = x*z*(1-c) + y*s; r[9] = y*z*(1-c) - x*s; r[10] = c + z*z*(1-c);
		multMatrix(m, m, r);
	}
}

	#include <cstdio>

	// macOS core profile glBegin/glEnd emulation
	// On Apple's Metal GL wrapper, immediate-mode functions (glBegin/glEnd/glVertex/glTexCoord/glColor)
	// are available as function pointers but are no-ops on core profile.
	// We capture all vertex data during glBegin...glEnd blocks and flush as modern VBO draws.
	namespace spring_immediate {
		static constexpr int kMaxVertices = 8192;
		static constexpr int kMaxConverted = kMaxVertices * 2;

	struct IMVertex {
		float x, y, z;
		float r, g, b, a;
		float s, t;
	};

	static GLenum   g_primMode = 0;
	static bool     g_inBegin = false;
	static int      g_vertCount = 0;
	static IMVertex g_vertices[kMaxVertices];
	static IMVertex g_converted[kMaxConverted];

	// Current attribute state (persists between vertices like legacy GL)
	static float g_curColor[4]    = {1.0f, 1.0f, 1.0f, 1.0f};
	static float g_curTexCoord[2] = {0.0f, 0.0f};
	static float g_curNormal[3]   = {0.0f, 0.0f, 1.0f};

	// Persistent GL objects (lazy-init)
	static GLuint g_vbo     = 0;
	static GLuint g_vao     = 0;
	static GLuint g_program = 0;

	// Simple shader sources
	static const char* kImmVS = R"glsl(#version 150 core
		uniform mat4 u_mvpMatrix;
		in vec3 a_position;
		in vec4 a_color;
		in vec2 a_texcoord;
		out vec4 v_color;
		out vec2 v_texcoord;
		void main() {
			gl_Position = u_mvpMatrix * vec4(a_position, 1.0);
			v_color = a_color;
			v_texcoord = a_texcoord;
		}
	)glsl";

	static const char* kImmFS = R"glsl(#version 150 core
		uniform sampler2D u_texture;
		uniform bool u_useTexture;
		in vec4 v_color;
		in vec2 v_texcoord;
		out vec4 fragColor;
		void main() {
			vec4 texColor = u_useTexture ? texture(u_texture, v_texcoord) : vec4(1.0);
			fragColor = v_color * texColor;
		}
	)glsl";

	static GLuint compileShader(GLenum type, const char* src) {
		GLuint sh = glCreateShader(type);
		if (sh == 0) { fprintf(stderr, "[spring_immediate] glCreateShader(type=%d) returned 0\n", type); return 0; }
		glShaderSource(sh, 1, &src, nullptr);
		glCompileShader(sh);
		GLint ok = 0;
		glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
		if (!ok) {
			char log[1024] = {};
			glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
			fprintf(stderr, "[spring_immediate] Shader compile error (type=%d):\n%s\n", type, log);
			glDeleteShader(sh);
			return 0;
		}
		return sh;
	}

	static int g_debugFlushCount = 0;
	static int g_debugMatrixCount = 0;
	static int g_debugPushPopCount = 0;
	static int g_debugVertCount = 0;

	static void init() {
		if (g_program != 0) return;
		fprintf(stderr, "[spring_immediate] init() creating shader program\n");
		GLuint vs = compileShader(GL_VERTEX_SHADER,   kImmVS);
		GLuint fs = compileShader(GL_FRAGMENT_SHADER, kImmFS);
		if (vs == 0 || fs == 0) {
			fprintf(stderr, "[spring_immediate] init() FAILED: shader compilation error\n");
			return;
		}
		g_program = glCreateProgram();
		glAttachShader(g_program, vs);
		glAttachShader(g_program, fs);
		glBindAttribLocation(g_program, 0, "a_position");
		glBindAttribLocation(g_program, 1, "a_color");
		glBindAttribLocation(g_program, 2, "a_texcoord");
		glLinkProgram(g_program);
		GLint ok = 0;
		glGetProgramiv(g_program, GL_LINK_STATUS, &ok);
		if (!ok) {
			char log[1024] = {};
			glGetProgramInfoLog(g_program, sizeof(log), nullptr, log);
			fprintf(stderr, "[spring_immediate] Program link error:\n%s\n", log);
			glDeleteProgram(g_program);
			g_program = 0;
		}
		glDeleteShader(vs);
		glDeleteShader(fs);

		glGenVertexArrays(1, &g_vao);
		glGenBuffers(1, &g_vbo);
		glBindVertexArray(g_vao);
		glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
		glBufferData(GL_ARRAY_BUFFER, kMaxConverted * sizeof(IMVertex), nullptr, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(IMVertex), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(IMVertex), (void*)(3*sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(IMVertex), (void*)(7*sizeof(float)));
		glEnableVertexAttribArray(2);
		glBindVertexArray(0);
		fprintf(stderr, "[spring_immediate] init() complete: program=%d vao=%d vbo=%d\n", g_program, g_vao, g_vbo);
	}

	static void flush() {
		if (g_vertCount == 0) return;
		init();
		if (g_program == 0) return; // shader failed to compile/link

		if (g_debugFlushCount < 60) { g_debugFlushCount++; }

		// Convert GL_QUADS to GL_TRIANGLES (core profile doesn't support GL_QUADS in glDrawArrays)
		int outCount = g_vertCount;
		GLenum drawMode = g_primMode;
		const IMVertex* drawVerts = g_vertices;

		if (g_primMode == GL_QUADS) {
			outCount = (g_vertCount / 4) * 6;
			for (int i = 0, o = 0; i < g_vertCount; i += 4, o += 6) {
				g_converted[o+0] = g_vertices[i+0];
				g_converted[o+1] = g_vertices[i+1];
				g_converted[o+2] = g_vertices[i+2];
				g_converted[o+3] = g_vertices[i+0];
				g_converted[o+4] = g_vertices[i+2];
				g_converted[o+5] = g_vertices[i+3];
			}
			drawMode = GL_TRIANGLES;
			drawVerts = g_converted;
		}

		// Compute MVP matrix from tracked matrices
		float mvp[16];
		// If projection is a 3D perspective (large |m[11]/m[15]|), use saved 2D ortho instead
		// Perspective matrix has non-zero at column 3, row 2 (index m[3*4+2] = m[14])
		const float* useProj = spring_matrix::g_projection;
		const float* useModel = spring_matrix::g_modelview;
		float identityModel[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
		if (spring_matrix::shouldUseOrthoBackup()) {
			useProj = spring_matrix::g_lastOrthoProj;
			useModel = identityModel;
		}
		spring_matrix::multMatrix(mvp, useProj, useModel);
		// macOS fix: flip Y axis (Metal framebuffer has inverted Y vs OpenGL)
		mvp[5] = -mvp[5];  // negate Y scale
		mvp[13] = -mvp[13]; // negate Y translation

		// Save current VAO and program
		GLint oldVAO = 0, oldProg = 0;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVAO);
		glGetIntegerv(GL_CURRENT_PROGRAM, &oldProg);

		glBindVertexArray(g_vao);
		glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, outCount * sizeof(IMVertex), drawVerts);

		glUseProgram(g_program);
		glDisable(GL_DEPTH_TEST); // ensure depth doesn't cull 2D UI
		glUniformMatrix4fv(glGetUniformLocation(g_program, "u_mvpMatrix"), 1, GL_FALSE, mvp);
		// Check if a texture is bound (non-zero texture binding)
		GLint boundTex = 0;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);
		// GREEN TEST: first 5 flushes force green color to verify flush is working
		if (g_debugFlushCount < 5) {
			glUniform1i(glGetUniformLocation(g_program, "u_useTexture"), 0);
		} else {
			glUniform1i(glGetUniformLocation(g_program, "u_useTexture"), (boundTex != 0) ? 1 : 0);
		}
		if (boundTex != 0)
			glUniform1i(glGetUniformLocation(g_program, "u_texture"), 0); // texture unit 0

		glDrawArrays(drawMode, 0, outCount);

		glUseProgram(oldProg);
		glBindVertexArray(oldVAO);
	}

	static void begin(GLenum mode) {
		g_primMode = mode;
		g_inBegin = true;
		g_vertCount = 0;
		g_curColor[0] = g_curColor[1] = g_curColor[2] = g_curColor[3] = 1.0f;
		g_curTexCoord[0] = g_curTexCoord[1] = 0.0f;
	}

	static void end() {
		g_inBegin = false;
		flush();
	}

	static void vertex2f(float x, float y) {
		if (!g_inBegin || g_vertCount >= kMaxVertices) return;
		IMVertex& v = g_vertices[g_vertCount++];
		v.x = x; v.y = y; v.z = 0.0f;
		v.r = g_curColor[0]; v.g = g_curColor[1]; v.b = g_curColor[2]; v.a = g_curColor[3];
		v.s = g_curTexCoord[0]; v.t = g_curTexCoord[1];
	}

	static void vertex3f(float x, float y, float z) {
		if (!g_inBegin || g_vertCount >= kMaxVertices) return;
		IMVertex& v = g_vertices[g_vertCount++];
		v.x = x; v.y = y; v.z = z;
		v.r = g_curColor[0]; v.g = g_curColor[1]; v.b = g_curColor[2]; v.a = g_curColor[3];
		v.s = g_curTexCoord[0]; v.t = g_curTexCoord[1];
	}

	static void color3f(float r, float g, float b) {
		g_curColor[0] = r; g_curColor[1] = g; g_curColor[2] = b; g_curColor[3] = 1.0f;
	}

	static void color4f(float r, float g, float b, float a) {
		g_curColor[0] = r; g_curColor[1] = g; g_curColor[2] = b; g_curColor[3] = a;
	}

	static void texCoord2f(float s, float t) {
		g_curTexCoord[0] = s; g_curTexCoord[1] = t;
	}

	static void normal3f(float nx, float ny, float nz) {
		g_curNormal[0] = nx; g_curNormal[1] = ny; g_curNormal[2] = nz;
	}
}
#endif

struct TextureParameters {
	GLint intFmt;
	GLint sizeX;
	GLint sizeY;
	GLint sizeZ;
	GLint bpp;
	GLint chNum;
	GLint imageSize;
	GLint prefDataType;
	GLboolean isNormalizedDepth;
	GLboolean isCompressed;
};

static inline void glVertexf3(const float3& v)    { glVertex3f(v.r, v.g, v.b); }
static inline void glColorf3(const float3& v)     { glColor3f(v.r, v.g, v.b); }
static inline void glColorf4(const float4& v)     { glColor4f(v.r, v.g, v.b, v.a); }
static inline void glTranslatef3(const float3& v) { glTranslatef(v.r, v.g, v.b); }
static inline void glColorf4(const float3& v, const float alpha) { glColor4f(v.r, v.g, v.b, alpha); }

typedef   void   (*   glOrthoFuncPtr) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near, GLdouble far);
typedef   void   (*gluOrtho2DFuncPtr) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);
typedef   void   (* glFrustumFuncPtr) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near, GLdouble far);

static inline void __spring_glOrtho_noCC(GLdouble l, GLdouble r,  GLdouble b, GLdouble t,  GLdouble n, GLdouble f) { glOrtho(l, r, b, t, n, f); }
static inline void __spring_glOrtho     (GLdouble l, GLdouble r,  GLdouble b, GLdouble t,  GLdouble n, GLdouble f) {
	#ifndef UNIT_TEST
	glTranslatef(0.0f, 0.0f, 0.5f);
	glScalef(1.0f, 1.0f, 0.5f);
	glOrtho(l, r,  b, t,  n, f);
	#else
	#error myGL.h included in unit-test?
	#endif
}

static inline void __spring_gluOrtho2D_noCC(GLdouble l, GLdouble r,  GLdouble b, GLdouble t) { __spring_glOrtho_noCC(l, r, b, t, -1.0, 1.0); }
static inline void __spring_gluOrtho2D     (GLdouble l, GLdouble r,  GLdouble b, GLdouble t) { __spring_glOrtho     (l, r, b, t, -1.0, 1.0); }

static inline void __spring_glFrustum_noCC(GLdouble l, GLdouble r,  GLdouble b, GLdouble t,  GLdouble n, GLdouble f) { glFrustum(l, r, b, t, n, f); }
static inline void __spring_glFrustum     (GLdouble l, GLdouble r,  GLdouble b, GLdouble t,  GLdouble n, GLdouble f) {
	#ifndef UNIT_TEST
	glTranslatef(0.0f, 0.0f, 0.5f);
	glScalef(1.0f, 1.0f, 0.5f);
	glFrustum(l, r,  b, t,  n, f);
	#endif
}

static constexpr    glOrthoFuncPtr    glOrthoFuncs[2] = {__spring_glOrtho_noCC, __spring_glOrtho};
static constexpr gluOrtho2DFuncPtr gluOrtho2DFuncs[2] = {__spring_gluOrtho2D_noCC, __spring_gluOrtho2D};
static constexpr  glFrustumFuncPtr  glFrustumFuncs[2] = {__spring_glFrustum_noCC, __spring_glFrustum};

#undef glOrtho
#undef gluOrtho2D
#undef glFrustum

#define glOrtho       glOrthoFuncs[globalRendering->supportClipSpaceControl]
#define gluOrtho2D gluOrtho2DFuncs[globalRendering->supportClipSpaceControl]
#define glFrustum   glFrustumFuncs[globalRendering->supportClipSpaceControl]

// macOS core profile: override fixed-function matrix ops with tracked matrices
// because Apple's Metal GL wrapper makes these no-ops on core profile
#if defined(__APPLE__)
	#undef glPushMatrix
	#undef glPopMatrix
	#undef glMatrixMode
	#undef glLoadIdentity
	#undef glLoadMatrixf
	#undef glTranslatef
	#undef glScalef
	#undef glRotatef
	#undef glMultMatrixf
	#undef glGetFloatv
	#undef glOrtho
	#undef gluOrtho2D
	#undef glBegin
	#undef glEnd
	#undef glVertex2f
	#undef glVertex3f
	#undef glVertex2fv
	#undef glVertex3fv
	#undef glTexCoord2f
	#undef glTexCoord2fv
	#undef glColor3f
	#undef glColor4f
	#undef glColor3fv
	#undef glColor4fv
	#undef glNormal3f
	#undef glNormal3fv
	#undef glBindTexture

	static GLuint g_whiteTex2D = 0;
	static inline void __apple_glBindTexture(GLenum target, GLuint texture) {
		if (target == GL_TEXTURE_2D && texture == 0) {
			if (g_whiteTex2D == 0) {
				glad_glGenTextures(1, &g_whiteTex2D);
				unsigned char w[4] = {255,255,255,255};
				glad_glBindTexture(GL_TEXTURE_2D, g_whiteTex2D);
				glad_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, w);
				glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
			glad_glBindTexture(GL_TEXTURE_2D, g_whiteTex2D);
			return;
		}
		glad_glBindTexture(target, texture);
	}

	static inline void __apple_glMatrixMode(GLenum mode) {
		if (spring_immediate::g_debugMatrixCount < 50) {
			fprintf(stderr, "[spring_matrix] glMatrixMode(%s)\n", mode == GL_PROJECTION ? "PROJECTION" : mode == GL_MODELVIEW ? "MODELVIEW" : "OTHER");
			spring_immediate::g_debugMatrixCount++;
		}
		spring_matrix::g_matMode = mode;
	}
	static inline void __apple_glLoadIdentity() {
		float* m = (spring_matrix::g_matMode == GL_PROJECTION) ? spring_matrix::g_projection : spring_matrix::g_modelview;
		spring_matrix::loadIdentity(m);
		if (spring_immediate::g_debugMatrixCount < 50) {
			fprintf(stderr, "[spring_matrix] glLoadIdentity on %s\n", spring_matrix::g_matMode == GL_PROJECTION ? "PROJECTION" : "MODELVIEW");
			spring_immediate::g_debugMatrixCount++;
		}
	}
	static inline void __apple_glLoadMatrixf(const GLfloat* m) {
		float* dst = (spring_matrix::g_matMode == GL_PROJECTION) ? spring_matrix::g_projection : spring_matrix::g_modelview;
		memcpy(dst, m, 16*sizeof(float));
		if (spring_immediate::g_debugMatrixCount < 30 && spring_matrix::g_matMode == GL_PROJECTION) {
			fprintf(stderr, "[spring_matrix] glLoadMatrixf on PROJECTION: [%.2f %.2f %.2f %.2f]\n", m[0], m[4], m[8], m[12]);
			spring_immediate::g_debugMatrixCount++;
		}
	}
	static inline void __apple_glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
		float* m = (spring_matrix::g_matMode == GL_PROJECTION) ? spring_matrix::g_projection : spring_matrix::g_modelview;
		spring_matrix::translate(m, x, y, z);
	}
	static inline void __apple_glScalef(GLfloat x, GLfloat y, GLfloat z) {
		float* m = (spring_matrix::g_matMode == GL_PROJECTION) ? spring_matrix::g_projection : spring_matrix::g_modelview;
		spring_matrix::scale(m, x, y, z);
	}
	static inline void __apple_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
		float* m = (spring_matrix::g_matMode == GL_PROJECTION) ? spring_matrix::g_projection : spring_matrix::g_modelview;
		spring_matrix::rotate(m, angle, x, y, z);
	}
	static inline void __apple_glMultMatrixf(const GLfloat* m) {
		float* dst = (spring_matrix::g_matMode == GL_PROJECTION) ? spring_matrix::g_projection : spring_matrix::g_modelview;
		spring_matrix::multMatrix(dst, dst, m);
	}
	static inline void __apple_glPushMatrix() {
		if (spring_immediate::g_debugPushPopCount < 50) {
			fprintf(stderr, "[spring_matrix] glPushMatrix(%s) stackTop=%d\n",
				spring_matrix::g_matMode == GL_PROJECTION ? "PROJ" : "MODEL",
				spring_matrix::g_matMode == GL_PROJECTION ? spring_matrix::g_projectionStackTop : spring_matrix::g_modelviewStackTop);
			spring_immediate::g_debugPushPopCount++;
		}
		if (spring_matrix::g_matMode == GL_PROJECTION && spring_matrix::g_projectionStackTop < spring_matrix::kMatrixStackSize) {
			memcpy(spring_matrix::g_projectionStack[spring_matrix::g_projectionStackTop], spring_matrix::g_projection, 16*sizeof(float));
			spring_matrix::g_projectionStackTop++;
		} else if (spring_matrix::g_matMode == GL_MODELVIEW && spring_matrix::g_modelviewStackTop < spring_matrix::kMatrixStackSize) {
			memcpy(spring_matrix::g_modelviewStack[spring_matrix::g_modelviewStackTop], spring_matrix::g_modelview, 16*sizeof(float));
			spring_matrix::g_modelviewStackTop++;
		}
	}
	static inline void __apple_glPopMatrix() {
		if (spring_immediate::g_debugPushPopCount < 50) {
			fprintf(stderr, "[spring_matrix] glPopMatrix(%s) stackTop=%d\n",
				spring_matrix::g_matMode == GL_PROJECTION ? "PROJ" : "MODEL",
				spring_matrix::g_matMode == GL_PROJECTION ? spring_matrix::g_projectionStackTop : spring_matrix::g_modelviewStackTop);
			spring_immediate::g_debugPushPopCount++;
		}
		if (spring_matrix::g_matMode == GL_PROJECTION && spring_matrix::g_projectionStackTop > 0) {
			spring_matrix::g_projectionStackTop--;
			memcpy(spring_matrix::g_projection, spring_matrix::g_projectionStack[spring_matrix::g_projectionStackTop], 16*sizeof(float));
		} else if (spring_matrix::g_matMode == GL_MODELVIEW && spring_matrix::g_modelviewStackTop > 0) {
			spring_matrix::g_modelviewStackTop--;
			memcpy(spring_matrix::g_modelview, spring_matrix::g_modelviewStack[spring_matrix::g_modelviewStackTop], 16*sizeof(float));
		}
	}
	static inline void __apple_glGetFloatv(GLenum pname, GLfloat* params) {
		if (pname == GL_MODELVIEW_MATRIX) {
			if (spring_matrix::shouldUseOrthoBackup()) {
				float id[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
				memcpy(params, id, 16*sizeof(float));
				return;
			}
			memcpy(params, spring_matrix::g_modelview, 16*sizeof(float)); return;
		}
		if (pname == GL_PROJECTION_MATRIX) {
			if (spring_matrix::shouldUseOrthoBackup()) {
				memcpy(params, spring_matrix::g_lastOrthoProj, 16*sizeof(float));
				return;
			}
			memcpy(params, spring_matrix::g_projection, 16*sizeof(float));
			return;
		}
		if (glad_glGetFloatv != nullptr) glad_glGetFloatv(pname, params);
		else memset(params, 0, sizeof(GLfloat) * (pname == GL_MODELVIEW_MATRIX || pname == GL_PROJECTION_MATRIX ? 16 : 4));
	}
	// Redefine glOrtho/gluOrtho2D to also track matrices
	static inline void __apple_glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
		float* m = (spring_matrix::g_matMode == GL_PROJECTION) ? spring_matrix::g_projection : spring_matrix::g_modelview;
		spring_matrix::ortho(m, l, r, b, t, n, f);
		// Save as 2D ortho backup for UI rendering
		if (spring_matrix::g_matMode == GL_PROJECTION) {
			memcpy(spring_matrix::g_lastOrthoProj, m, 16*sizeof(float));
			spring_matrix::g_hasOrthoProj = true;
		}
		if (spring_immediate::g_debugMatrixCount < 50) {
			fprintf(stderr, "[spring_matrix] glOrtho(l=%.1f r=%.1f b=%.1f t=%.1f n=%.1f f=%.1f) on %s\n",
				l, r, b, t, n, f, spring_matrix::g_matMode == GL_PROJECTION ? "PROJECTION" : "MODELVIEW");
			spring_immediate::g_debugMatrixCount++;
		}
	}
	static inline void __apple_gluOrtho2D(GLdouble l, GLdouble r, GLdouble b, GLdouble t) {
		__apple_glOrtho(l, r, b, t, -1.0, 1.0);
	}

	// Immediate-mode emulation overrides (glBegin/glEnd are no-ops on macOS core profile)
	static inline void __apple_glBegin(GLenum mode) { spring_immediate::begin(mode); }
	static inline void __apple_glEnd()              { spring_immediate::end(); }

	static inline void __apple_glVertex2f(GLfloat x, GLfloat y)            { spring_immediate::vertex2f(x, y); }
	static inline void __apple_glVertex3f(GLfloat x, GLfloat y, GLfloat z) { spring_immediate::vertex3f(x, y, z); }
	static inline void __apple_glVertex2fv(const GLfloat* v)               { spring_immediate::vertex2f(v[0], v[1]); }
	static inline void __apple_glVertex3fv(const GLfloat* v)               { spring_immediate::vertex3f(v[0], v[1], v[2]); }

	static inline void __apple_glTexCoord2f(GLfloat s, GLfloat t)          { spring_immediate::texCoord2f(s, t); }
	static inline void __apple_glTexCoord2fv(const GLfloat* v)             { spring_immediate::texCoord2f(v[0], v[1]); }

	static inline void __apple_glColor3f(GLfloat r, GLfloat g, GLfloat b)              { spring_immediate::color3f(r, g, b); }
	static inline void __apple_glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)   { spring_immediate::color4f(r, g, b, a); }
	static inline void __apple_glColor3fv(const GLfloat* v)                            { spring_immediate::color3f(v[0], v[1], v[2]); }
	static inline void __apple_glColor4fv(const GLfloat* v)                            { spring_immediate::color4f(v[0], v[1], v[2], v[3]); }

	static inline void __apple_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)          { spring_immediate::normal3f(nx, ny, nz); }
	static inline void __apple_glNormal3fv(const GLfloat* v)                           { spring_immediate::normal3f(v[0], v[1], v[2]); }

	#define glMatrixMode   __apple_glMatrixMode
	#define glLoadIdentity __apple_glLoadIdentity
	#define glLoadMatrixf  __apple_glLoadMatrixf
	#define glTranslatef   __apple_glTranslatef
	#define glScalef       __apple_glScalef
	#define glRotatef      __apple_glRotatef
	#define glMultMatrixf  __apple_glMultMatrixf
	#define glPushMatrix   __apple_glPushMatrix
	#define glPopMatrix    __apple_glPopMatrix
	#define glGetFloatv    __apple_glGetFloatv
	#define glOrtho        __apple_glOrtho
	#define gluOrtho2D     __apple_gluOrtho2D
	#define glBegin        __apple_glBegin
	#define glEnd          __apple_glEnd
	#define glVertex2f     __apple_glVertex2f
	#define glVertex3f     __apple_glVertex3f
	#define glVertex2fv    __apple_glVertex2fv
	#define glVertex3fv    __apple_glVertex3fv
	#define glTexCoord2f   __apple_glTexCoord2f
	#define glTexCoord2fv  __apple_glTexCoord2fv
	#define glColor3f      __apple_glColor3f
	#define glColor4f      __apple_glColor4f
	#define glColor3fv     __apple_glColor3fv
	#define glColor4fv     __apple_glColor4fv
	#define glNormal3f     __apple_glNormal3f
	#define glNormal3fv    __apple_glNormal3fv
	#define glBindTexture  __apple_glBindTexture
#endif



void WorkaroundATIPointSizeBug();

void glSaveTextureArray(const GLuint textureID, const char* filename, int level = 0, int page = 0);
void glSaveTexture(const GLuint textureID, const char* filename, int level = 0);

void RecoilGetTexParams(GLenum target, GLuint textureID, GLint level, TextureParameters& textureParameters);
void RecoilTexStorage2D(GLenum target, GLint levels, GLint internalFormat, GLsizei width, GLsizei height);
void RecoilTexStorage3D(GLenum target, GLint levels, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth);
void RecoilBuildMipmaps(const GLenum target, GLint internalFormat, const GLsizei width, const GLsizei height, const GLenum format, const GLenum type, const void* data, int32_t numLevels = 0);
bool glSpringBlitImages(
	GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ,
	GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ,
	GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth
);

void ClearScreen();

bool ProgramStringIsNative(GLenum target, const char* filename);
unsigned int LoadVertexProgram(const char* filename);
unsigned int LoadFragmentProgram(const char* filename);

void glClearErrors(const char* cls, const char* fnc, bool verbose = false);
void glSafeDeleteProgram(GLuint program);

bool CheckAvailableVideoModes();

bool GetAvailableVideoRAM(GLint* memory, const char* glVendor);
bool ShowDriverWarning(const char* glVendor);


class CVertexArray;
CVertexArray* GetVertexArray();

struct SDrawElementsIndirectCommand {
	SDrawElementsIndirectCommand() = default;
	SDrawElementsIndirectCommand(uint32_t indexCount_, uint32_t instanceCount_, uint32_t firstIndex_, uint32_t baseVertex_, uint32_t baseInstance_)
		: indexCount{ indexCount_ }
		, instanceCount{ instanceCount_ }
		, firstIndex{ firstIndex_ }
		, baseVertex{ baseVertex_ }
		, baseInstance{ baseInstance_ }
	{};

	uint32_t indexCount;
	uint32_t instanceCount;
	uint32_t firstIndex;
	uint32_t baseVertex;
	uint32_t baseInstance;
};

struct SInstanceData {
	SInstanceData() = default;
	SInstanceData(uint32_t matOffset_, uint16_t auxInfo_, uint16_t numPieces_, uint32_t uniOffset_, uint32_t bposeMatOffset_)
		: matOffset{ matOffset_ }            // updated during the following draw frames
		, uniOffset{ uniOffset_ }            // updated during the following draw frames
		, info{ auxInfo_, numPieces_ }       // not updated during the following draw frames
		, bposeMatOffset { bposeMatOffset_ } // updated during the following draw frames
	{}

	uint32_t matOffset;
	uint32_t uniOffset;
	std::array<uint16_t, 2> info; // u32 {paletteIndex[0:10], reserved[11:15], numPieces[16:31]}
	uint32_t bposeMatOffset;
};

#endif // _MY_GL_H
