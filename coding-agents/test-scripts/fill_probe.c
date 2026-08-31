// What fill rate does this driver stack actually reach?
//
// The engine is GPU bound and its cost is linear in pixel count at about 90 to 97
// Mpixel/s, measured at two resolutions on a frozen scene. On an M1 Pro that is
// roughly a hundred GPU cycles a pixel, so either the engine overdraws enormously
// or the Zink and KosmicKrisp path cannot go faster than this. Those need very
// different fixes, and this separates them without an engine run.
//
// Draws K full screen quads into a pbuffer of the given size, syncs, and reports
// the rate. Compare the result against the engine's 90 Mpixel/s.
//
//   PREFIX=$HOME/dev/mesa-install-premtl4
//   gcc -O1 -I$PREFIX/include -o /tmp/fill_probe fill_probe.c -L$PREFIX/lib -lEGL
//   DYLD_LIBRARY_PATH=/opt/homebrew/lib EGL_PLATFORM=surfaceless \
//     MESA_LOADER_DRIVER_OVERRIDE=zink GALLIUM_DRIVER=zink \
//     LIBGL_DRIVERS_PATH=$PREFIX/lib \
//     VK_DRIVER_FILES=$PREFIX/share/vulkan/icd.d/kosmickrisp_mesa_icd.aarch64.json \
//     MESA_GL_VERSION_OVERRIDE=4.6 MESA_GLSL_VERSION_OVERRIDE=460 \
//     /tmp/fill_probe 3024 1832

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef float GLfloat;

#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_QUADS            0x0007
#define GL_VERSION          0x1F02
#define GL_RENDERER         0x1F01
#define GL_DEPTH_TEST       0x0B71
#define GL_BLEND            0x0BE2
#define GL_CULL_FACE        0x0B44
#define GL_SRC_ALPHA        0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303

static const GLubyte* (*p_glGetString)(GLenum);
static GLenum (*p_glGetError)(void);
static void (*p_glClearColor)(GLfloat, GLfloat, GLfloat, GLfloat);
static void (*p_glClear)(GLbitfield);
static void (*p_glViewport)(GLint, GLint, GLsizei, GLsizei);
static void (*p_glBegin)(GLenum);
static void (*p_glEnd)(void);
static void (*p_glVertex4f)(GLfloat, GLfloat, GLfloat, GLfloat);
static void (*p_glColor4f)(GLfloat, GLfloat, GLfloat, GLfloat);
static void (*p_glFinish)(void);
static void (*p_glFlush)(void);
static void (*p_glDisable)(GLenum);
static void (*p_glEnable)(GLenum);
static void (*p_glBlendFunc)(GLenum, GLenum);

#define LOAD(name) do { \
	p_##name = (void*) eglGetProcAddress(#name); \
	if (p_##name == NULL) printf("MISSING entry point: %s\n", #name); \
} while (0)

static double now_seconds(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec * 1.0e-9;
}

// One full screen quad in clip space. Four-float vertices, which is what the
// engine emits since the arity mitigation.
static void quad(void)
{
	p_glBegin(GL_QUADS);
	p_glVertex4f(-1.0f, -1.0f, 0.0f, 1.0f);
	p_glVertex4f( 1.0f, -1.0f, 0.0f, 1.0f);
	p_glVertex4f( 1.0f,  1.0f, 0.0f, 1.0f);
	p_glVertex4f(-1.0f,  1.0f, 0.0f, 1.0f);
	p_glEnd();
}

// flushEach breaks the render pass between quads, which is what the engine's
// SPRING_BATCH_FLUSH does per immediate-mode batch. If a pass break costs a full
// attachment store and reload, this is where it shows.
static int flushEach = 0;

static void measure(const char* label, int w, int h, int quads, int frames)
{
	// One untimed frame, so shader and pipeline setup is not counted
	p_glClear(GL_COLOR_BUFFER_BIT);
	for (int q = 0; q < quads; q++)
		quad();
	p_glFinish();

	const double t0 = now_seconds();

	for (int f = 0; f < frames; f++) {
		p_glClear(GL_COLOR_BUFFER_BIT);
		for (int q = 0; q < quads; q++) {
			if (flushEach)
				p_glFlush();
			p_glColor4f(0.2f, 0.4f, 0.6f, 0.5f);
			quad();
		}
		p_glFinish();
	}

	const double dt = now_seconds() - t0;
	const double perFrame = dt / frames;
	const double megapixels = (double) w * h * quads * 1.0e-6;

	printf("%-28s %5d quads  %7.2f ms/frame  %8.1f Mpixel/s  err 0x%04x\n",
		label, quads, perFrame * 1000.0, megapixels / perFrame, p_glGetError());
}

int main(int argc, char** argv)
{
	const int w = (argc > 1) ? atoi(argv[1]) : 3024;
	const int h = (argc > 2) ? atoi(argv[2]) : 1832;
	const int frames = (argc > 3) ? atoi(argv[3]) : 20;

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

	const EGLint surfAttrs[] = { EGL_WIDTH, w, EGL_HEIGHT, h, EGL_NONE };
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
	LOAD(glViewport); LOAD(glBegin); LOAD(glEnd); LOAD(glVertex4f); LOAD(glColor4f);
	LOAD(glFinish); LOAD(glFlush); LOAD(glDisable); LOAD(glEnable); LOAD(glBlendFunc);

	printf("GL_RENDERER = %s\n", (const char*) p_glGetString(GL_RENDERER));
	printf("surface %dx%d = %.2f Mpixel, %d frames a measurement\n\n",
		w, h, w * h * 1.0e-6, frames);

	p_glViewport(0, 0, w, h);
	p_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	p_glDisable(GL_DEPTH_TEST);
	p_glDisable(GL_CULL_FACE);
	p_glDisable(GL_BLEND);

	// 0 quads isolates the clear and the per-frame submission from the fill
	measure("clear only, opaque", w, h, 0, frames);
	measure("opaque", w, h, 1, frames);
	measure("opaque", w, h, 8, frames);
	measure("opaque", w, h, 64, frames);

	// The engine draws almost everything blended
	p_glEnable(GL_BLEND);
	p_glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	measure("blended", w, h, 1, frames);
	measure("blended", w, h, 8, frames);
	measure("blended", w, h, 64, frames);

	// The same fill, but every quad in its own render pass
	flushEach = 1;
	measure("blended, pass a quad", w, h, 1, frames);
	measure("blended, pass a quad", w, h, 8, frames);
	measure("blended, pass a quad", w, h, 64, frames);

	return 0;
}
