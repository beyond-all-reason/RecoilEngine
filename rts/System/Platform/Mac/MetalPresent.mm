/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <IOSurface/IOSurface.h>

#include "MetalPresent.h"
#include <vector>
#include <cstdint>
#include <cstring>

// Written to be MRC-safe (no ARC assumptions): long-lived objects come from
// +new/Create (owned, +1) and are intentionally kept for process lifetime;
// per-frame objects are autoreleased.

static id<MTLDevice>        g_device  = nil;
static id<MTLCommandQueue>  g_queue   = nil;
static CAMetalLayer*        g_layer   = nil;

// Path 1: CPU-staging texture for MacMetalPresent_PresentBGRA (early splash).
static id<MTLTexture>       g_staging = nil;
static int                  g_w = 0;
static int                  g_h = 0;
static std::vector<uint8_t> g_flipBuf;

// Path 2: IOSurface zero-copy. Engine writes pixels directly into the
// IOSurface base address via glReadPixels (no CPU intermediate buffer, no
// CPU-side Y flip, no replaceRegion upload). A small render pipeline samples
// the IOSurface-backed texture and writes it Y-flipped to the drawable.
static IOSurfaceRef               g_ioSurface       = nullptr;
static id<MTLTexture>             g_ioTexture       = nil;
static int                        g_ioW             = 0;
static int                        g_ioH             = 0;
static bool                       g_ioLocked        = false;
static id<MTLRenderPipelineState> g_presentPSO      = nil;
static id<MTLRenderPipelineState> g_presentPSO_flip = nil;
static id<MTLSamplerState>        g_linearSampler   = nil;

// Fullscreen-triangle vertex+fragment shader. Two PSOs (flipped + non-flipped)
// keep the per-frame Y-flip choice branch-free in the GPU shader.
static NSString* const kPresentShaderSrc = @R"(
#include <metal_stdlib>
using namespace metal;

struct VOut {
    float4 position [[position]];
    float2 uv;
};

vertex VOut vs_present(uint vid [[vertex_id]]) {
    // Fullscreen triangle covering NDC [-1,1]^2 with UVs [0,1]^2.
    const float2 pos[3] = { float2(-1.0,  3.0), float2(-1.0, -1.0), float2( 3.0, -1.0) };
    const float2 uv [3] = { float2( 0.0, -1.0), float2( 0.0,  1.0), float2( 2.0,  1.0) };
    VOut o;
    o.position = float4(pos[vid], 0.0, 1.0);
    o.uv = uv[vid];
    return o;
}

vertex VOut vs_present_flip(uint vid [[vertex_id]]) {
    // Same triangle but with V flipped at the source side.
    const float2 pos[3] = { float2(-1.0,  3.0), float2(-1.0, -1.0), float2( 3.0, -1.0) };
    const float2 uv [3] = { float2( 0.0,  2.0), float2( 0.0,  0.0), float2( 2.0,  0.0) };
    VOut o;
    o.position = float4(pos[vid], 0.0, 1.0);
    o.uv = uv[vid];
    return o;
}

fragment float4 fs_present(VOut in [[stage_in]],
                           texture2d<float> src [[texture(0)]],
                           sampler           s   [[sampler(0)]]) {
    return src.sample(s, in.uv);
}
)";

static bool BuildPresentPipelines()
{
    if (g_presentPSO != nil && g_presentPSO_flip != nil)
        return true;

    NSError* err = nil;
    id<MTLLibrary> lib = [g_device newLibraryWithSource:kPresentShaderSrc options:nil error:&err];
    if (lib == nil) {
        fprintf(stderr, "[MetalPresent] shader compile failed: %s\n",
                err ? [[err localizedDescription] UTF8String] : "(no error)");
        return false;
    }
    id<MTLFunction> vs     = [lib newFunctionWithName:@"vs_present"];
    id<MTLFunction> vsFlip = [lib newFunctionWithName:@"vs_present_flip"];
    id<MTLFunction> fs     = [lib newFunctionWithName:@"fs_present"];
    if (vs == nil || vsFlip == nil || fs == nil) {
        fprintf(stderr, "[MetalPresent] shader function lookup failed\n");
        return false;
    }

    auto makePSO = [&](id<MTLFunction> vertFn) -> id<MTLRenderPipelineState> {
        MTLRenderPipelineDescriptor* d = [[MTLRenderPipelineDescriptor alloc] init];
        d.vertexFunction   = vertFn;
        d.fragmentFunction = fs;
        d.colorAttachments[0].pixelFormat = g_layer.pixelFormat;
        d.colorAttachments[0].blendingEnabled = NO;
        NSError* e = nil;
        id<MTLRenderPipelineState> p = [g_device newRenderPipelineStateWithDescriptor:d error:&e];
        if (p == nil)
            fprintf(stderr, "[MetalPresent] PSO build failed: %s\n",
                    e ? [[e localizedDescription] UTF8String] : "(no error)");
        return p;
    };

    g_presentPSO      = makePSO(vs);
    g_presentPSO_flip = makePSO(vsFlip);

    MTLSamplerDescriptor* sd = [[MTLSamplerDescriptor alloc] init];
    sd.minFilter = MTLSamplerMinMagFilterNearest;
    sd.magFilter = MTLSamplerMinMagFilterNearest;
    sd.sAddressMode = MTLSamplerAddressModeClampToEdge;
    sd.tAddressMode = MTLSamplerAddressModeClampToEdge;
    g_linearSampler = [g_device newSamplerStateWithDescriptor:sd];

    return (g_presentPSO != nil && g_presentPSO_flip != nil && g_linearSampler != nil);
}

bool MacMetalPresent_Init(void* caMetalLayer)
{
	if (g_device != nil)
		return true;
	if (caMetalLayer == nullptr)
		return false;

	g_layer  = (CAMetalLayer*)caMetalLayer;
	g_device = MTLCreateSystemDefaultDevice();
	if (g_device == nil)
		return false;

	g_queue = [g_device newCommandQueue];
	if (g_queue == nil)
		return false;

	g_layer.device          = g_device;
	g_layer.pixelFormat     = MTLPixelFormatBGRA8Unorm;
	g_layer.framebufferOnly  = NO; // allow the drawable to be a blit destination
	return true;
}

void MacMetalPresent_PresentBGRA(int w, int h, const void* pixels, bool flipY)
{
	if (g_device == nil || g_queue == nil || g_layer == nil)
		return;
	if (w <= 0 || h <= 0 || pixels == nullptr)
		return;

	const uint8_t* src = (const uint8_t*)pixels;
	const size_t rowBytes = (size_t)w * 4;

	// OpenGL readback is bottom-up; Metal/CoreAnimation is top-down.
	if (flipY) {
		g_flipBuf.resize(rowBytes * (size_t)h);
		for (int y = 0; y < h; ++y)
			std::memcpy(&g_flipBuf[(size_t)y * rowBytes], src + (size_t)(h - 1 - y) * rowBytes, rowBytes);
		src = g_flipBuf.data();
	}

	if (g_staging == nil || g_w != w || g_h != h) {
		MTLTextureDescriptor* d =
			[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
															   width:(NSUInteger)w
															  height:(NSUInteger)h
														   mipmapped:NO];
		d.usage = MTLTextureUsageShaderRead;
		g_staging = [g_device newTextureWithDescriptor:d];
		g_w = w;
		g_h = h;
		g_layer.drawableSize = CGSizeMake((CGFloat)w, (CGFloat)h);
	}

	[g_staging replaceRegion:MTLRegionMake2D(0, 0, w, h)
				 mipmapLevel:0
				   withBytes:src
				 bytesPerRow:rowBytes];

	id<CAMetalDrawable> drawable = [g_layer nextDrawable];
	if (drawable == nil) {
		fprintf(stderr, "[MetalPresent] nextDrawable returned nil — frame not presented\n");
		return;
	}

	id<MTLCommandBuffer> cb = [g_queue commandBuffer];
	id<MTLBlitCommandEncoder> blit = [cb blitCommandEncoder];
	[blit copyFromTexture:g_staging
			  sourceSlice:0
			  sourceLevel:0
			 sourceOrigin:MTLOriginMake(0, 0, 0)
			   sourceSize:MTLSizeMake(w, h, 1)
				toTexture:drawable.texture
		 destinationSlice:0
		 destinationLevel:0
		destinationOrigin:MTLOriginMake(0, 0, 0)];
	[blit endEncoding];
	[cb presentDrawable:drawable];
	[cb commit];
}

static void ReleaseIOSurfaceBacking()
{
    if (g_ioLocked && g_ioSurface) {
        IOSurfaceUnlock(g_ioSurface, 0, nullptr);
        g_ioLocked = false;
    }
    g_ioTexture = nil; // autoreleased
    if (g_ioSurface) {
        CFRelease(g_ioSurface);
        g_ioSurface = nullptr;
    }
    g_ioW = 0;
    g_ioH = 0;
}

static bool EnsureIOSurfaceBacking(int w, int h)
{
    if (g_ioSurface && g_ioW == w && g_ioH == h)
        return true;

    ReleaseIOSurfaceBacking();

    NSDictionary* props = @{
        (id)kIOSurfaceWidth:           @(w),
        (id)kIOSurfaceHeight:          @(h),
        (id)kIOSurfaceBytesPerElement: @(4),
        (id)kIOSurfacePixelFormat:     @((uint32_t)'BGRA'),
    };
    g_ioSurface = IOSurfaceCreate((__bridge CFDictionaryRef)props);
    if (g_ioSurface == nullptr) {
        fprintf(stderr, "[MetalPresent] IOSurfaceCreate failed (%dx%d)\n", w, h);
        return false;
    }

    MTLTextureDescriptor* d =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                           width:(NSUInteger)w
                                                          height:(NSUInteger)h
                                                       mipmapped:NO];
    d.usage = MTLTextureUsageShaderRead;
    d.storageMode = MTLStorageModeShared;
    g_ioTexture = [g_device newTextureWithDescriptor:d iosurface:g_ioSurface plane:0];
    if (g_ioTexture == nil) {
        fprintf(stderr, "[MetalPresent] newTextureWithDescriptor:iosurface: failed\n");
        ReleaseIOSurfaceBacking();
        return false;
    }

    g_ioW = w;
    g_ioH = h;
    // Drawable size must match the layer's *natural* backing
    // pixel count, not the IOSurface size. With SPRING_MAC_NO_RETINA the
    // IOSurface is at logical (1x) res while the layer's backing is Retina
    // (2x). Setting drawableSize to the smaller IOSurface size makes
    // CoreAnimation place the drawable at 1:1 in a corner of the layer.
    const CGSize  lbSize  = g_layer.bounds.size;
    const CGFloat cs      = g_layer.contentsScale > 0 ? g_layer.contentsScale : 1.0;
    const CGFloat targetW = lbSize.width  * cs;
    const CGFloat targetH = lbSize.height * cs;
    const CGFloat finalW  = targetW > 0 ? targetW : (CGFloat)w;
    const CGFloat finalH  = targetH > 0 ? targetH : (CGFloat)h;
    g_layer.drawableSize = CGSizeMake(finalW, finalH);
    fprintf(stderr, "[MetalPresent/drawable] IOSurface=%dx%d layer.bounds=%.1fx%.1f pt "
                    "contentsScale=%.2f -> drawableSize=%.1fx%.1f px\n",
            w, h, (double)lbSize.width, (double)lbSize.height,
            (double)cs, (double)finalW, (double)finalH);
    return true;
}

void* MacMetalPresent_AcquireIOSurfaceBuffer(int w, int h, size_t* outRowBytes)
{
    if (outRowBytes) *outRowBytes = 0;
    if (g_device == nil || g_queue == nil || g_layer == nil)
        return nullptr;
    if (w <= 0 || h <= 0)
        return nullptr;
    if (!EnsureIOSurfaceBacking(w, h))
        return nullptr;
    if (!BuildPresentPipelines())
        return nullptr;

    static bool s_loggedOnce = false;
    if (!s_loggedOnce) {
        fprintf(stderr, "[MetalPresent] IOSurface zero-copy path active (%dx%d, rowBytes=%zu)\n",
                w, h, IOSurfaceGetBytesPerRow(g_ioSurface));
        s_loggedOnce = true;
    }

    // Acquire CPU access. AVERTING_PRECEDENT_DEADLOCK: the previous frame's
    // Metal command buffer may still hold a GPU reference to the surface;
    // IOSurfaceLock blocks until that read is retired.
    if (IOSurfaceLock(g_ioSurface, 0, nullptr) != kIOReturnSuccess) {
        fprintf(stderr, "[MetalPresent] IOSurfaceLock failed\n");
        return nullptr;
    }
    g_ioLocked = true;

    const size_t rb = IOSurfaceGetBytesPerRow(g_ioSurface);
    if (outRowBytes) *outRowBytes = rb;
    return IOSurfaceGetBaseAddress(g_ioSurface);
}

void MacMetalPresent_PresentIOSurface(bool flipY)
{
    if (g_device == nil || g_queue == nil || g_layer == nil)
        return;
    if (g_ioSurface == nullptr || g_ioTexture == nil)
        return;

    if (g_ioLocked) {
        IOSurfaceUnlock(g_ioSurface, 0, nullptr);
        g_ioLocked = false;
    }

    id<CAMetalDrawable> drawable = [g_layer nextDrawable];
    if (drawable == nil) {
        fprintf(stderr, "[MetalPresent] nextDrawable returned nil — frame not presented\n");
        return;
    }

    MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor renderPassDescriptor];
    rpd.colorAttachments[0].texture     = drawable.texture;
    rpd.colorAttachments[0].loadAction  = MTLLoadActionDontCare;
    rpd.colorAttachments[0].storeAction = MTLStoreActionStore;

    id<MTLCommandBuffer> cb = [g_queue commandBuffer];
    id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rpd];
    [enc setRenderPipelineState:flipY ? g_presentPSO_flip : g_presentPSO];
    [enc setFragmentTexture:g_ioTexture atIndex:0];
    [enc setFragmentSamplerState:g_linearSampler atIndex:0];
    [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    [enc endEncoding];
    [cb presentDrawable:drawable];
    [cb commit];
}
