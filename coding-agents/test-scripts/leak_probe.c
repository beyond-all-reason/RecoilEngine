// Does a plain GL draw loop leak memory on this driver?
//
// The engine grows about 5 GiB/s of phys_footprint on Zink over KosmicKrisp and
// is flat on llvmpipe with the same binary and content. Growth continues through
// load phases that issue no GL of their own, and disabling the present readback
// makes it worse rather than better, so the suspicion is ordinary drawing.
//
// This draws frames in a loop and prints a marker every 100 so an external
// sampler can line footprint up against frame count. Nothing is presented and
// nothing is read back.
//
// build:
//   gcc -o leak_probe leak_probe.c \
//     -I$HOME/dev/mesa-install/include -L$HOME/dev/mesa-install/lib \
//     -lEGL -Wl,-rpath,$HOME/dev/mesa-install/lib \
//     -Wl,-rpath,/opt/homebrew/opt/vulkan-loader/lib
//
// usage: leak_probe [frames] [batches-per-frame]
//
// SPRING_PROBE_MIPMAP switches to the shape the engine actually leaks on. The
// engine reaches tens of GiB during a level load, and the growth tracks the
// number of Metal command buffers kosmickrisp creates rather than the number
// of frames drawn: about six IOAccelerator regions per command buffer, none of
// them returned when it is released. glGenerateMipmap is one blit per mip
// level, each blit a render pass and so a command buffer, which is why
// RecoilBuildMipmaps over a map's worth of textures is what triggers it.

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef float GLfloat;

#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_QUADS            0x0007
#define GL_RENDERER         0x1F01
#define GL_DEPTH_TEST       0x0B71
#define GL_MODELVIEW        0x1700
#define GL_PROJECTION       0x1701

#define GL_TEXTURE_2D       0x0DE1
#define GL_RGBA8            0x8058
#define GL_RGBA             0x1908
#define GL_UNSIGNED_BYTE    0x1401
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_LINEAR_MIPMAP_LINEAR 0x2703

#define SIZE 512
#define TEXSIZE 512

static void (*p_glClearColor)(GLfloat, GLfloat, GLfloat, GLfloat);
static void (*p_glClear)(GLbitfield);
static void (*p_glViewport)(GLint, GLint, GLsizei, GLsizei);
static void (*p_glBegin)(GLenum);
static void (*p_glEnd)(void);
static void (*p_glVertex2f)(GLfloat, GLfloat);
static void (*p_glColor3f)(GLfloat, GLfloat, GLfloat);
static void (*p_glTexCoord2f)(GLfloat, GLfloat);
static void (*p_glFinish)(void);
static void (*p_glFlush)(void);
static void (*p_glDisable)(GLenum);
static void (*p_glMatrixMode)(GLenum);
static void (*p_glLoadIdentity)(void);
static const GLubyte* (*p_glGetString)(GLenum);
static void (*p_glGenTextures)(GLsizei, unsigned int*);
static void (*p_glDeleteTextures)(GLsizei, const unsigned int*);
static void (*p_glBindTexture)(GLenum, unsigned int);
static void (*p_glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
static void (*p_glTexParameteri)(GLenum, GLenum, GLint);
static void (*p_glGenerateMipmap)(GLenum);

#define LOAD(n) \
	p_##n = (void*)eglGetProcAddress(#n); \
	if (!p_##n) { fprintf(stderr, "missing %s\n", #n); return 1; }

int main(int argc, char** argv)
{
	const int frames = (argc > 1) ? atoi(argv[1]) : 100000;
	const int batches = (argc > 2) ? atoi(argv[2]) : 200;

	EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	EGLint major, minor;
	if (!eglInitialize(dpy, &major, &minor)) { fprintf(stderr, "eglInitialize failed\n"); return 1; }
	eglBindAPI(EGL_OPENGL_API);

	const EGLint cfgAttrs[] = {
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
		EGL_NONE
	};
	EGLConfig cfg; EGLint numCfg = 0;
	eglChooseConfig(dpy, cfgAttrs, &cfg, 1, &numCfg);

	const EGLint surfAttrs[] = { EGL_WIDTH, SIZE, EGL_HEIGHT, SIZE, EGL_NONE };
	EGLSurface surf = eglCreatePbufferSurface(dpy, cfg, surfAttrs);

	const EGLint ctxAttrs[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 0,
		EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT,
		EGL_NONE
	};
	EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxAttrs);
	if (!eglMakeCurrent(dpy, surf, surf, ctx)) { fprintf(stderr, "makeCurrent failed\n"); return 1; }

	LOAD(glClearColor); LOAD(glClear); LOAD(glViewport); LOAD(glBegin); LOAD(glEnd);
	LOAD(glVertex2f); LOAD(glColor3f); LOAD(glTexCoord2f); LOAD(glFinish); LOAD(glFlush);
	LOAD(glDisable); LOAD(glMatrixMode); LOAD(glLoadIdentity); LOAD(glGetString);
	LOAD(glGenTextures); LOAD(glDeleteTextures); LOAD(glBindTexture);
	LOAD(glTexImage2D); LOAD(glTexParameteri); LOAD(glGenerateMipmap);

	if (getenv("SPRING_PROBE_MIPMAP") != NULL) {
		// what RecoilBuildMipmaps does for every texture a map or unit set
		// needs: allocate every mip level, then generate them
		unsigned char* pixels = malloc((size_t)TEXSIZE * TEXSIZE * 4);
		memset(pixels, 0x80, (size_t)TEXSIZE * TEXSIZE * 4);

		printf("mipmap mode, %d textures of %dx%d, pid %d\n", frames, TEXSIZE, TEXSIZE, (int)getpid());
		fflush(stdout);

		for (int t = 0; t < frames; t++) {
			unsigned int tex = 0;
			p_glGenTextures(1, &tex);
			p_glBindTexture(GL_TEXTURE_2D, tex);

			for (int level = 0, w = TEXSIZE, h = TEXSIZE; w >= 1 || h >= 1; level++) {
				p_glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA8, w, h, 0,
				               GL_RGBA, GL_UNSIGNED_BYTE, level ? NULL : pixels);
				if (w == 1 && h == 1)
					break;
				w = (w > 1) ? w / 2 : 1;
				h = (h > 1) ? h / 2 : 1;
			}
			p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			// the control that separates "creating textures leaks" from "the
			// blits behind glGenerateMipmap leak"
			if (getenv("SPRING_PROBE_NO_GENMIP") == NULL)
				p_glGenerateMipmap(GL_TEXTURE_2D);

			// the engine keeps its textures, but deleting here makes the point
			// sharper: nothing the probe holds can account for the growth
			p_glBindTexture(GL_TEXTURE_2D, 0);
			p_glDeleteTextures(1, &tex);

			if ((t % 100) == 0) {
				printf("texture %d\n", t);
				fflush(stdout);
			}
		}

		p_glFinish();
		free(pixels);
		printf("done\n");
		return 0;
	}

	printf("GL_RENDERER = %s\n", p_glGetString(GL_RENDERER));
	printf("frames=%d batches=%d, pid %d\n", frames, batches, (int)getpid());
	fflush(stdout);

	p_glViewport(0, 0, SIZE, SIZE);
	p_glDisable(GL_DEPTH_TEST);
	p_glMatrixMode(GL_PROJECTION); p_glLoadIdentity();
	p_glMatrixMode(GL_MODELVIEW);  p_glLoadIdentity();

	for (int f = 0; f < frames; f++) {
		p_glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		p_glClear(GL_COLOR_BUFFER_BIT);

		// the shape the engine draws: a run of batches, some carrying a texcoord
		for (int b = 0; b < batches; b++) {
			const float x = -1.0f + 2.0f * ((b % 16) / 16.0f);
			const float y = -1.0f + 2.0f * ((b / 16) / 16.0f);

			p_glBegin(GL_QUADS);
				if (b & 1) p_glTexCoord2f(0.5f, 0.5f);
				p_glColor3f(1.0f, 0.2f, 0.2f);
				p_glVertex2f(x, y);
				p_glVertex2f(x + 0.1f, y);
				p_glVertex2f(x + 0.1f, y + 0.1f);
				p_glVertex2f(x, y + 0.1f);
			p_glEnd();
		}

		// SPRING_PROBE_NO_FINISH tests the theory that the driver's threaded
		// context queues without bound when nothing synchronises per frame. The
		// engine has no glFinish, and its only sync was the present readback,
		// which is exactly what disabling made worse.
		static int noFinish = -1;
		if (noFinish < 0) noFinish = (getenv("SPRING_PROBE_NO_FINISH") != NULL);
		if (!noFinish) p_glFinish();

		if ((f % 100) == 0) {
			printf("frame %d\n", f);
			fflush(stdout);
		}
	}

	return 0;
}
