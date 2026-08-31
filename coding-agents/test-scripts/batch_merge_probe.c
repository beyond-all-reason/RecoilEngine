// Does the KosmicKrisp batch merging defect also affect consecutive glDrawArrays?
//
// The engine separates every immediate-mode batch with a glFlush because two
// consecutive glBegin batches otherwise merge, and the second primitive is drawn
// joined to the first. That flush costs about 7.5% of the frame. The proposed fix
// is for LuaOpenGL to buffer each gl.BeginEnd block and replay it as one
// glDrawArrays, which only helps if glDrawArrays does not merge the same way.
// Nobody has tested that. This does, without an engine run.
//
// Draws a grid of 2000 identical LINE_LOOP squares with a gap between them. A
// merged batch draws a connecting line across a gap, so it lights pixels the
// correct image leaves black. Ground truth is the driver's own output with a
// flush between batches, which is the configuration measured clean.
//
// Read the positive control first. If "immediate, no separator" does not differ
// from the reference, the defect is not being exercised and nothing else in the
// run means anything.
//
//   PREFIX=$HOME/dev/mesa-install-premtl4
//   gcc -O1 -I$PREFIX/include -o /tmp/batch_merge_probe batch_merge_probe.c -L$PREFIX/lib -lEGL
//   DYLD_LIBRARY_PATH=/opt/homebrew/lib EGL_PLATFORM=surfaceless \
//     MESA_LOADER_DRIVER_OVERRIDE=zink GALLIUM_DRIVER=zink \
//     LIBGL_DRIVERS_PATH=$PREFIX/lib \
//     VK_DRIVER_FILES=$PREFIX/share/vulkan/icd.d/kosmickrisp_mesa_icd.aarch64.json \
//     MESA_GL_VERSION_OVERRIDE=4.6 MESA_GLSL_VERSION_OVERRIDE=460 \
//     /tmp/batch_merge_probe

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef float GLfloat;
typedef void GLvoid;

#define GL_COLOR_BUFFER_BIT   0x00004000
#define GL_LINE_LOOP          0x0002
#define GL_FLOAT              0x1406
#define GL_RGBA               0x1908
#define GL_UNSIGNED_BYTE      0x1401
#define GL_PACK_ALIGNMENT     0x0D05
#define GL_VERTEX_ARRAY       0x8074
#define GL_RENDERER           0x1F01
#define GL_VERSION            0x1F02
#define GL_DEPTH_TEST         0x0B71
#define GL_BLEND              0x0BE2
#define GL_CULL_FACE          0x0B44
#define GL_SRC_ALPHA          0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303

static const GLubyte* (*p_glGetString)(GLenum);
static GLenum (*p_glGetError)(void);
static void (*p_glClearColor)(GLfloat, GLfloat, GLfloat, GLfloat);
static void (*p_glClear)(GLbitfield);
static void (*p_glViewport)(GLint, GLint, GLsizei, GLsizei);
static void (*p_glBegin)(GLenum);
static void (*p_glEnd)(void);
static void (*p_glVertex2f)(GLfloat, GLfloat);
static void (*p_glVertex4f)(GLfloat, GLfloat, GLfloat, GLfloat);
static void (*p_glColor4f)(GLfloat, GLfloat, GLfloat, GLfloat);
static void (*p_glFinish)(void);
static void (*p_glFlush)(void);
static void (*p_glDisable)(GLenum);
static void (*p_glEnable)(GLenum);
static void (*p_glBlendFunc)(GLenum, GLenum);
static void (*p_glEnableClientState)(GLenum);
static void (*p_glDisableClientState)(GLenum);
static void (*p_glVertexPointer)(GLint, GLenum, GLsizei, const GLvoid*);
static void (*p_glDrawArrays)(GLenum, GLint, GLsizei);
static void (*p_glReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*);
static void (*p_glPixelStorei)(GLenum, GLint);

#define LOAD(name) do { \
	p_##name = (void*) eglGetProcAddress(#name); \
	if (p_##name == NULL) printf("MISSING entry point: %s\n", #name); \
} while (0)

// The artefact amplifier's grid, which is the configuration already measured
// positive: 2000 identical 16 segment LINE_LOOP circles of radius 7, spaced 22
// pixels apart. Every number here is copied from widget_loop_amp.lua.
//
// The spacing carries the measurement. A merged pair draws a segment between two
// circle centres, so at 22 pixels it is long and cannot be confused with the
// circles. The measured median was 28 stray pixels a frame, which is about one
// merge event per frame, so the defect fires roughly once per 2000 adjacent
// pairs. That rate is why the grid has to be this large to see anything.
#define DIVS     16
#define RADIUS   7.0f
#define SPACING  22
#define MARGIN   20
#define COLS     55
#define BATCHES  2000
#define ROWS     ((BATCHES + COLS - 1) / COLS)

#define SURF_W 1280
#define SURF_H 900

// Two copies of the same geometry. The 4 float form is what the engine emits
// since the arity mitigation, the 2 float form is what it emitted before.
static float vert2[BATCHES * DIVS * 2];
static float vert4[BATCHES * DIVS * 4];

static unsigned char* reference;
static unsigned char* current;

static float ndcX(float px) { return (px / (float) SURF_W) * 2.0f - 1.0f; }
static float ndcY(float py) { return (py / (float) SURF_H) * 2.0f - 1.0f; }

static void buildGeometry(void)
{
	float cosines[DIVS];
	float sines[DIVS];

	for (int d = 0; d < DIVS; d++) {
		const float a = ((d + 1) / (float) DIVS) * 3.14159265358979f * 2.0f;
		cosines[d] = cosf(a) * RADIUS;
		sines[d]   = sinf(a) * RADIUS;
	}

	for (int b = 0; b < BATCHES; b++) {
		const float cx = MARGIN + RADIUS + (b % COLS) * SPACING;
		const float cy = MARGIN + RADIUS + (b / COLS) * SPACING;

		for (int d = 0; d < DIVS; d++) {
			const int i = b * DIVS + d;

			const float x = ndcX(cx + cosines[d]);
			const float y = ndcY(cy + sines[d]);

			vert2[i * 2 + 0] = x;
			vert2[i * 2 + 1] = y;

			vert4[i * 4 + 0] = x;
			vert4[i * 4 + 1] = y;
			vert4[i * 4 + 2] = 0.0f;
			vert4[i * 4 + 3] = 1.0f;
		}
	}
}

enum Mode {
	MODE_IMMEDIATE,        // glBegin per batch, every vertex widened to 4 floats
	MODE_IMMEDIATE_ARITY,  // glBegin per batch, arity alternating 2 and 4
	MODE_ARRAYS,           // one glDrawArrays per batch, pointer size always 4
	MODE_ARRAYS_ARITY      // one glDrawArrays per batch, pointer size alternating 2 and 4
};

enum Separator {
	SEP_NONE,
	SEP_FLUSH,
	SEP_FINISH
};

static void separate(enum Separator sep)
{
	switch (sep) {
		case SEP_FLUSH:  p_glFlush();  break;
		case SEP_FINISH: p_glFinish(); break;
		default: break;
	}
}

static void render(enum Mode mode, enum Separator sep)
{
	p_glClear(GL_COLOR_BUFFER_BIT);
	p_glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	const int usesArrays = (mode == MODE_ARRAYS || mode == MODE_ARRAYS_ARITY);

	if (usesArrays)
		p_glEnableClientState(GL_VERTEX_ARRAY);

	for (int b = 0; b < BATCHES; b++) {
		// A narrow batch is every other one, so a defect that needs two unlike
		// neighbours has them, and one that needs two like neighbours also has them.
		const int narrow = (b & 1);

		separate(sep);

		if (usesArrays) {
			// The engine's buffered path would set the pointer once per gl.BeginEnd
			// block and issue one draw, so the probe does the same per batch.
			if (mode == MODE_ARRAYS_ARITY && narrow)
				p_glVertexPointer(2, GL_FLOAT, 0, &vert2[b * DIVS * 2]);
			else
				p_glVertexPointer(4, GL_FLOAT, 0, &vert4[b * DIVS * 4]);

			p_glDrawArrays(GL_LINE_LOOP, 0, DIVS);
			continue;
		}

		p_glBegin(GL_LINE_LOOP);
		for (int c = 0; c < DIVS; c++) {
			const float* v = &vert4[(b * DIVS + c) * 4];

			if (mode == MODE_IMMEDIATE_ARITY && narrow)
				p_glVertex2f(v[0], v[1]);
			else
				p_glVertex4f(v[0], v[1], v[2], v[3]);
		}
		p_glEnd();
	}

	if (usesArrays)
		p_glDisableClientState(GL_VERTEX_ARRAY);

	p_glFinish();
}

static void readback(unsigned char* dst)
{
	p_glReadPixels(0, 0, SURF_W, SURF_H, GL_RGBA, GL_UNSIGNED_BYTE, dst);
}

// Counts pixels that differ from the reference, split by direction. Extra means
// the configuration lit a pixel the reference leaves black, which is the shape a
// merged batch makes. Missing means it failed to draw something.
//
// Reported per frame rather than as a total, because the defect is probabilistic
// at roughly one event per 2000 adjacent pairs, and the published figures are
// "stray in N of M frames, median 28 pixels". A frame count is comparable to
// those, a sum is not.
static void compare(const char* label, enum Mode mode, enum Separator sep, int iterations)
{
	int dirtyFrames = 0;
	int totalExtra = 0;
	int totalMissing = 0;
	int worstExtra = 0;

	for (int it = 0; it < iterations; it++) {
		render(mode, sep);
		readback(current);

		int extra = 0;
		int missing = 0;

		for (int p = 0; p < SURF_W * SURF_H; p++) {
			const int refLit = (reference[p * 4] | reference[p * 4 + 1] | reference[p * 4 + 2]) != 0;
			const int curLit = (current[p * 4] | current[p * 4 + 1] | current[p * 4 + 2]) != 0;

			if (curLit && !refLit) extra++;
			if (refLit && !curLit) missing++;
		}

		if (extra > 0 || missing > 0) dirtyFrames++;
		if (extra > worstExtra) worstExtra = extra;

		totalExtra += extra;
		totalMissing += missing;
	}

	printf("%-34s  dirty %3d/%-3d  extra %7d total %6d worst  missing %7d   err 0x%04x\n",
		label, dirtyFrames, iterations, totalExtra, worstExtra, totalMissing, p_glGetError());
}

int main(int argc, char** argv)
{
	// 30 frames by default, so a case is directly comparable to the published
	// "stray in 31 of 31 frames" and "0 of 30" figures.
	const int iterations = (argc > 1) ? atoi(argv[1]) : 30;

	EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	EGLint major = 0, minor = 0;

	if (!eglInitialize(dpy, &major, &minor)) { fprintf(stderr, "eglInitialize failed\n"); return 1; }
	eglBindAPI(EGL_OPENGL_API);

	const EGLint cfgAttrs[] = {
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
		EGL_NONE
	};

	EGLConfig cfg;
	EGLint numCfg = 0;
	eglChooseConfig(dpy, cfgAttrs, &cfg, 1, &numCfg);

	const EGLint surfAttrs[] = { EGL_WIDTH, SURF_W, EGL_HEIGHT, SURF_H, EGL_NONE };
	EGLSurface surf = eglCreatePbufferSurface(dpy, cfg, surfAttrs);

	const EGLint ctxAttrs[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 0,
		EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT,
		EGL_NONE
	};

	EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxAttrs);
	if (!eglMakeCurrent(dpy, surf, surf, ctx)) { fprintf(stderr, "makeCurrent failed\n"); return 1; }

	LOAD(glGetString); LOAD(glGetError); LOAD(glClearColor); LOAD(glClear);
	LOAD(glViewport); LOAD(glBegin); LOAD(glEnd); LOAD(glVertex2f); LOAD(glVertex4f);
	LOAD(glColor4f); LOAD(glFinish); LOAD(glFlush); LOAD(glDisable);
	LOAD(glEnable); LOAD(glBlendFunc);
	LOAD(glEnableClientState); LOAD(glDisableClientState); LOAD(glVertexPointer);
	LOAD(glDrawArrays); LOAD(glReadPixels); LOAD(glPixelStorei);

	printf("GL_RENDERER = %s\n", (const char*) p_glGetString(GL_RENDERER));
	printf("GL_VERSION  = %s\n", (const char*) p_glGetString(GL_VERSION));
	printf("%d LINE_LOOP batches on a %dx%d surface, %d iterations a case\n\n",
		BATCHES, SURF_W, SURF_H, iterations);

	p_glViewport(0, 0, SURF_W, SURF_H);
	p_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	p_glDisable(GL_DEPTH_TEST);
	p_glDisable(GL_CULL_FACE);
	p_glPixelStorei(GL_PACK_ALIGNMENT, 1);

	// The amplifier draws inside gl.DrawScreen, where the engine has blending on.
	// Matching it costs nothing and removes one difference from the configuration
	// that is known to reproduce.
	p_glEnable(GL_BLEND);
	p_glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	buildGeometry();

	reference = malloc(SURF_W * SURF_H * 4);
	current   = malloc(SURF_W * SURF_H * 4);

	// Ground truth is immediate mode with a flush between batches and uniform
	// vertex arity, which is what the engine ships and what measured 0 of 30
	// frames stray.
	render(MODE_IMMEDIATE, SEP_FLUSH);
	readback(reference);

	printf("controls\n");
	compare("immediate, flush   (reference)", MODE_IMMEDIATE, SEP_FLUSH, iterations);
	compare("arrays,    finish  (2nd truth)", MODE_ARRAYS,    SEP_FINISH, iterations);

	printf("\npositive control, must show extra pixels\n");
	compare("immediate, no separator",        MODE_IMMEDIATE, SEP_NONE, iterations);

	printf("\nthe questions\n");
	compare("arrays,    no separator",        MODE_ARRAYS,       SEP_NONE, iterations);
	compare("arrays,    flush",               MODE_ARRAYS,       SEP_FLUSH, iterations);
	compare("arrays,    varying pointer size", MODE_ARRAYS_ARITY, SEP_NONE, iterations);
	compare("immediate, varying arity, flush", MODE_IMMEDIATE_ARITY, SEP_FLUSH, iterations);

	return 0;
}
