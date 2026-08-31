// Does the driver actually honour glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE)?
//
// The engine builds its screen-space projection with ClipPerspProj(..., 1.0)
// whenever supportClipSpaceControl is set, which it is because Zink advertises
// GL_ARB_clip_control under the forced MESA_GL_VERSION_OVERRIDE=4.6. If the
// extension is advertised but not applied, every screen-space draw gets a depth
// value from the wrong convention, and UI geometry can fail the depth test and
// vanish while the world, which does not use those matrices, is unaffected.
//
// The test does not trust the extension string. It draws the same clip-space z
// under each convention and reads the depth buffer back:
//
//   NEGATIVE_ONE_TO_ONE   depth = 0.5 * z + 0.5
//   ZERO_TO_ONE           depth = z
//
// At z = 0.5 those are 0.75 and 0.5, far outside any rounding.
//
// build:
//   gcc -o clip_probe clip_probe.c \
//     -I$HOME/dev/mesa-install/include -L$HOME/dev/mesa-install/lib \
//     -lEGL -Wl,-rpath,$HOME/dev/mesa-install/lib
// run with the same environment as run-macos.sh

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef float GLfloat;
typedef double GLclampd;
typedef void GLvoid;

#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_DEPTH_TEST       0x0B71
#define GL_ALWAYS           0x0207
#define GL_TRIANGLES        0x0004
#define GL_MODELVIEW        0x1700
#define GL_PROJECTION       0x1701
#define GL_VERSION          0x1F02
#define GL_RENDERER         0x1F01
#define GL_EXTENSIONS       0x1F03
#define GL_DEPTH_COMPONENT  0x1902
#define GL_FLOAT            0x1406
#define GL_LOWER_LEFT       0x8CA1
#define GL_ZERO_TO_ONE      0x935F

#define SIZE 64
#define TEST_Z 0.5f

static void (*p_glClear)(GLbitfield);
static void (*p_glClearDepth)(GLclampd);
static void (*p_glEnable)(GLenum);
static void (*p_glDepthFunc)(GLenum);
static void (*p_glDepthRange)(GLclampd, GLclampd);
static void (*p_glViewport)(GLint, GLint, GLsizei, GLsizei);
static void (*p_glMatrixMode)(GLenum);
static void (*p_glLoadIdentity)(void);
static void (*p_glBegin)(GLenum);
static void (*p_glEnd)(void);
static void (*p_glVertex3f)(GLfloat, GLfloat, GLfloat);
static void (*p_glFinish)(void);
static void (*p_glReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*);
static const GLubyte* (*p_glGetString)(GLenum);
static GLenum (*p_glGetError)(void);
static void (*p_glClipControl)(GLenum, GLenum);

#define LOAD(name) \
	p_##name = (void*)eglGetProcAddress(#name); \
	if (p_##name == NULL) { fprintf(stderr, "missing %s\n", #name); return 1; }

static float drawAndReadDepth(void)
{
	p_glClearDepth(1.0);
	p_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	p_glEnable(GL_DEPTH_TEST);
	p_glDepthFunc(GL_ALWAYS);

	// identity matrices, so the vertex z below IS the clip-space z with w = 1
	p_glMatrixMode(GL_PROJECTION);
	p_glLoadIdentity();
	p_glMatrixMode(GL_MODELVIEW);
	p_glLoadIdentity();

	p_glBegin(GL_TRIANGLES);
		p_glVertex3f(-1.0f, -1.0f, TEST_Z);
		p_glVertex3f( 3.0f, -1.0f, TEST_Z);
		p_glVertex3f(-1.0f,  3.0f, TEST_Z);
	p_glEnd();

	p_glFinish();

	float depth = -1.0f;
	p_glReadPixels(SIZE / 2, SIZE / 2, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
	return depth;
}

int main(void)
{
	EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	EGLint major, minor;
	if (!eglInitialize(dpy, &major, &minor)) {
		fprintf(stderr, "eglInitialize failed\n");
		return 1;
	}
	eglBindAPI(EGL_OPENGL_API);

	const EGLint cfgAttrs[] = {
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_NONE
	};
	EGLConfig cfg;
	EGLint numCfg = 0;
	eglChooseConfig(dpy, cfgAttrs, &cfg, 1, &numCfg);
	if (numCfg == 0) {
		fprintf(stderr, "no config with a depth buffer\n");
		return 1;
	}

	const EGLint surfAttrs[] = { EGL_WIDTH, SIZE, EGL_HEIGHT, SIZE, EGL_NONE };
	EGLSurface surf = eglCreatePbufferSurface(dpy, cfg, surfAttrs);

	const EGLint ctxAttrs[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 0,
		EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT,
		EGL_NONE
	};
	EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxAttrs);
	if (!eglMakeCurrent(dpy, surf, surf, ctx)) {
		fprintf(stderr, "makeCurrent failed\n");
		return 1;
	}

	LOAD(glClear); LOAD(glClearDepth); LOAD(glEnable); LOAD(glDepthFunc);
	LOAD(glDepthRange); LOAD(glViewport); LOAD(glMatrixMode); LOAD(glLoadIdentity);
	LOAD(glBegin); LOAD(glEnd); LOAD(glVertex3f); LOAD(glFinish);
	LOAD(glReadPixels); LOAD(glGetString); LOAD(glGetError);

	printf("GL_RENDERER = %s\n", p_glGetString(GL_RENDERER));
	printf("GL_VERSION  = %s\n", p_glGetString(GL_VERSION));

	const char* exts = (const char*)p_glGetString(GL_EXTENSIONS);
	const int advertised = (exts != NULL && strstr(exts, "GL_ARB_clip_control") != NULL);
	printf("GL_ARB_clip_control advertised: %s\n", advertised ? "yes" : "no");

	p_glClipControl = (void*)eglGetProcAddress("glClipControl");
	printf("glClipControl entry point: %s\n", p_glClipControl ? "present" : "MISSING");

	p_glViewport(0, 0, SIZE, SIZE);
	p_glDepthRange(0.0, 1.0);

	const float defaultDepth = drawAndReadDepth();
	printf("\ndefault convention: depth = %.4f (expect %.4f)\n",
		defaultDepth, 0.5f * TEST_Z + 0.5f);

	if (p_glClipControl == NULL)
		return 2;

	p_glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	printf("glClipControl(ZERO_TO_ONE) glError = 0x%04x\n", p_glGetError());

	const float zeroToOneDepth = drawAndReadDepth();
	printf("zero-to-one:        depth = %.4f (expect %.4f)\n\n",
		zeroToOneDepth, TEST_Z);

	const int honoured = (zeroToOneDepth > TEST_Z - 0.05f && zeroToOneDepth < TEST_Z + 0.05f);
	const int unchanged = (zeroToOneDepth > defaultDepth - 0.01f && zeroToOneDepth < defaultDepth + 0.01f);

	if (honoured)
		printf("VERDICT: glClipControl is honoured.\n");
	else if (unchanged)
		printf("VERDICT: glClipControl is ADVERTISED BUT IGNORED. Depth did not move.\n");
	else
		printf("VERDICT: glClipControl changed depth to an unexpected value.\n");

	return honoured ? 0 : 3;
}
