// MGL Integration: bridges BAR's SDL window to MGL's Metal renderer
#include <SDL.h>
#include <SDL_syswm.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

// MGL API - must be declared before ObjC usage
extern "C" {
    typedef struct GLMContextRec *GLMContext;
    extern GLMContext _ctx;
    GLMContext createGLMContext(unsigned format, unsigned type,
                                unsigned depth_format, unsigned depth_type,
                                unsigned stencil_format, unsigned stencil_type);
}

// MGL's ObjC renderer
@interface MGLRenderer : NSObject
- (void) createMGLRendererAndBindToContext: (GLMContext) glm_ctx view: (NSView *) view;
@end

#include <dlfcn.h>
#include <glad/glad.h>

void MGL_InitWithSDLWindow(SDL_Window *sdlWindow)
{
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(sdlWindow, &wmInfo)) {
        fprintf(stderr, "MGL: SDL_GetWindowWMInfo failed\n");
        return;
    }
    NSWindow *nsWindow = wmInfo.info.cocoa.window;
    NSView *sdlView = nsWindow.contentView;
    
    // Create MGL context
    if (_ctx == NULL) {
        _ctx = createGLMContext(0x1908, 0x1401, 0x81A6, 0x1405, 0x822A, 0x1401);
    }
    
    // Create MGL renderer and bind to SDL's content view (don't replace it!)
    MGLRenderer *renderer = [[MGLRenderer alloc] init];
    [sdlView setWantsLayer:YES];
    [renderer createMGLRendererAndBindToContext: _ctx view: sdlView];
    
    fprintf(stderr, "MGL: Bound to SDL window via content view (%dx%d)\n",
            (int)sdlView.bounds.size.width, (int)sdlView.bounds.size.height);
}

// Override GLAD function pointers with MGL's Metal-backed implementations
// Also hook SDL_GL_GetProcAddress to route ALL GL functions through MGL
void MGL_OverrideGLADPointers(void)
{
    void *mgl = dlopen("@executable_path/libmgl.dylib", RTLD_NOW | RTLD_GLOBAL | RTLD_FIRST);
    if (!mgl) {
        mgl = dlopen("/Volumes/Extreme SSD/bar-macos-port/MGL/build_hb/libmgl.dylib", 
                     RTLD_NOW | RTLD_GLOBAL | RTLD_FIRST);
    }
    if (!mgl) {
        fprintf(stderr, "MGL: Cannot load libmgl.dylib: %s\n", dlerror());
        return;
    }
    
    // Override critical functions individually
    #define MGL_OVERRIDE(name) do { \
        void *fn = dlsym(mgl, "gl" #name); \
        if (fn) glad_gl##name = (decltype(glad_gl##name))fn; \
    } while(0)

    // Drawing
    MGL_OVERRIDE(DrawArrays);
    MGL_OVERRIDE(DrawElements);
    MGL_OVERRIDE(DrawElementsInstanced);
    MGL_OVERRIDE(DrawRangeElements);
    MGL_OVERRIDE(Clear);
    MGL_OVERRIDE(ClearColor);
    MGL_OVERRIDE(Viewport);
    MGL_OVERRIDE(Scissor);
    // Textures
    MGL_OVERRIDE(BindTexture);
    MGL_OVERRIDE(ActiveTexture);
    MGL_OVERRIDE(GenTextures);
    MGL_OVERRIDE(DeleteTextures);
    MGL_OVERRIDE(TexImage2D);
    MGL_OVERRIDE(TexSubImage2D);
    MGL_OVERRIDE(TexParameteri);
    // Buffers
    MGL_OVERRIDE(BindBuffer);
    MGL_OVERRIDE(GenBuffers);
    MGL_OVERRIDE(DeleteBuffers);
    MGL_OVERRIDE(BufferData);
    MGL_OVERRIDE(BufferSubData);
    // Shaders & Programs
    MGL_OVERRIDE(UseProgram);
    MGL_OVERRIDE(CreateProgram);
    MGL_OVERRIDE(DeleteProgram);
    MGL_OVERRIDE(CreateShader);
    MGL_OVERRIDE(DeleteShader);
    MGL_OVERRIDE(ShaderSource);
    MGL_OVERRIDE(CompileShader);
    MGL_OVERRIDE(GetShaderiv);
    MGL_OVERRIDE(GetShaderInfoLog);
    MGL_OVERRIDE(AttachShader);
    MGL_OVERRIDE(LinkProgram);
    MGL_OVERRIDE(GetProgramiv);
    MGL_OVERRIDE(GetProgramInfoLog);
    MGL_OVERRIDE(ValidateProgram);
    MGL_OVERRIDE(BindAttribLocation);
    MGL_OVERRIDE(GetAttribLocation);
    MGL_OVERRIDE(GetUniformLocation);
    // Uniforms
    MGL_OVERRIDE(Uniform1i);
    MGL_OVERRIDE(Uniform1f);
    MGL_OVERRIDE(Uniform2f);
    MGL_OVERRIDE(Uniform3f);
    MGL_OVERRIDE(Uniform4f);
    MGL_OVERRIDE(UniformMatrix4fv);
    // Vertex Arrays
    MGL_OVERRIDE(EnableVertexAttribArray);
    MGL_OVERRIDE(DisableVertexAttribArray);
    MGL_OVERRIDE(VertexAttribPointer);
    MGL_OVERRIDE(BindVertexArray);
    MGL_OVERRIDE(GenVertexArrays);
    MGL_OVERRIDE(DeleteVertexArrays);
    // Framebuffers
    MGL_OVERRIDE(BindFramebuffer);
    MGL_OVERRIDE(GenFramebuffers);
    MGL_OVERRIDE(DeleteFramebuffers);
    MGL_OVERRIDE(FramebufferTexture2D);
    MGL_OVERRIDE(CheckFramebufferStatus);
    // State
    MGL_OVERRIDE(Enable);
    MGL_OVERRIDE(Disable);
    MGL_OVERRIDE(BlendFunc);
    MGL_OVERRIDE(DepthFunc);
    MGL_OVERRIDE(DepthMask);
    MGL_OVERRIDE(ColorMask);
    MGL_OVERRIDE(CullFace);
    MGL_OVERRIDE(FrontFace);
    // Misc
    MGL_OVERRIDE(GetError);
    MGL_OVERRIDE(GetIntegerv);
    MGL_OVERRIDE(GetString);
    MGL_OVERRIDE(GetStringi);
    MGL_OVERRIDE(Flush);
    MGL_OVERRIDE(Finish);
    MGL_OVERRIDE(GetFloatv);
    MGL_OVERRIDE(ReadPixels);
    MGL_OVERRIDE(PixelStorei);
    
    fprintf(stderr, "MGL: ALL GLAD function pointers overridden with MGL Metal implementations\n");
}

