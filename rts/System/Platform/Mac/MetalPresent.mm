/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#import <AppKit/AppKit.h>
#import <IOSurface/IOSurface.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <stdint.h>
#include <string.h>

#include "MetalPresent.h"

#include "System/Log/ILog.h"

// Reference counting is manual here, ARC is not enabled for this file. The
// Metal objects below are created once and kept for the lifetime of the
// window; everything per-frame is autoreleased, so each entry point that
// touches Objective-C opens a pool of its own. The engine's main loop has none.

namespace {
	id<MTLDevice> device = nil;
	id<MTLCommandQueue> queue = nil;
	CAMetalLayer* layer = nil;

	id<MTLRenderPipelineState> pipeline = nil;
	id<MTLSamplerState> sampler = nil;

	IOSurfaceRef surface = nullptr;
	id<MTLTexture> surfaceTex = nil;
	int surfaceW = 0;
	int surfaceH = 0;
	bool surfaceLocked = false;

	// the last presented frame, held so the next acquire can wait for the GPU
	// to finish sampling the surface before the caller overwrites it
	id<MTLCommandBuffer> lastPresent = nil;

	// CAMetalLayer's own default, kept here so the setting can arrive before the
	// layer does and still be applied
	bool vsync = true;

	constexpr uint32_t bgraPixelFormat = 0x42475241; // 'BGRA'

	NSString* const presentShaderSrc = @R"(
#include <metal_stdlib>
using namespace metal;

struct Vertex {
	float4 pos [[position]];
	float2 uv;
};

vertex Vertex present_vs(uint id [[vertex_id]], constant float& flipY [[buffer(0)]])
{
	// one oversized triangle covering the drawable, uv spanning 0-1 across it
	const float2 corner = float2((id << 1) & 2, id & 2);

	Vertex v;
	v.pos = float4(corner * 2.0 - 1.0, 0.0, 1.0);
	// clip space counts up the screen, Metal texture coordinates count down it
	v.uv = float2(corner.x, mix(1.0 - corner.y, corner.y, flipY));
	return v;
}

fragment float4 present_fs(Vertex v [[stage_in]], texture2d<float> image [[texture(0)]], sampler smp [[sampler(0)]])
{
	return image.sample(smp, v.uv);
}
)";

	const char* ErrText(NSError* err)
	{
		return (err != nil) ? [[err localizedDescription] UTF8String] : "no error";
	}

	bool BuildPipeline()
	{
		if (pipeline != nil)
			return true;

		NSError* err = nil;
		id<MTLLibrary> lib = [device newLibraryWithSource: presentShaderSrc options: nil error: &err];

		if (lib == nil) {
			LOG_L(L_ERROR, "[MetalPresent::%s] the present shader did not compile: %s", __func__, ErrText(err));
			return false;
		}

		id<MTLFunction> vertexFn = [lib newFunctionWithName: @"present_vs"];
		id<MTLFunction> fragmentFn = [lib newFunctionWithName: @"present_fs"];

		MTLRenderPipelineDescriptor* pipeDesc = [[MTLRenderPipelineDescriptor alloc] init];
		pipeDesc.vertexFunction = vertexFn;
		pipeDesc.fragmentFunction = fragmentFn;
		pipeDesc.colorAttachments[0].pixelFormat = layer.pixelFormat;

		pipeline = [device newRenderPipelineStateWithDescriptor: pipeDesc error: &err];

		[pipeDesc release];
		[fragmentFn release];
		[vertexFn release];
		[lib release];

		if (pipeline == nil) {
			LOG_L(L_ERROR, "[MetalPresent::%s] no present pipeline: %s", __func__, ErrText(err));
			return false;
		}

		// the image is only scaled when its size and the drawable's diverge,
		// which the size contract is meant to stop happening
		MTLSamplerDescriptor* smpDesc = [[MTLSamplerDescriptor alloc] init];
		smpDesc.minFilter = MTLSamplerMinMagFilterLinear;
		smpDesc.magFilter = MTLSamplerMinMagFilterLinear;
		smpDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
		smpDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;

		sampler = [device newSamplerStateWithDescriptor: smpDesc];

		[smpDesc release];

		return (sampler != nil);
	}

	void ReleaseSurface()
	{
		if (surfaceLocked) {
			IOSurfaceUnlock(surface, 0, nullptr);
			surfaceLocked = false;
		}

		[surfaceTex release];
		surfaceTex = nil;

		if (surface != nullptr) {
			CFRelease(surface);
			surface = nullptr;
		}

		surfaceW = 0;
		surfaceH = 0;
	}

	bool EnsureSurface(int w, int h)
	{
		if (surface != nullptr && surfaceW == w && surfaceH == h)
			return true;

		ReleaseSurface();

		NSDictionary* props = @{
			(id) kIOSurfaceWidth:           @(w),
			(id) kIOSurfaceHeight:          @(h),
			(id) kIOSurfaceBytesPerElement: @(4),
			(id) kIOSurfacePixelFormat:     @(bgraPixelFormat),
		};

		if ((surface = IOSurfaceCreate((CFDictionaryRef) props)) == nullptr) {
			LOG_L(L_ERROR, "[MetalPresent::%s] no %dx%d IOSurface", __func__, w, h);
			return false;
		}

		MTLTextureDescriptor* texDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat: MTLPixelFormatBGRA8Unorm
		                                                                                  width: w
		                                                                                 height: h
		                                                                              mipmapped: NO];
		texDesc.usage = MTLTextureUsageShaderRead;
		texDesc.storageMode = MTLStorageModeShared;

		if ((surfaceTex = [device newTextureWithDescriptor: texDesc iosurface: surface plane: 0]) == nil) {
			LOG_L(L_ERROR, "[MetalPresent::%s] no texture for the %dx%d IOSurface", __func__, w, h);
			ReleaseSurface();
			return false;
		}

		surfaceW = w;
		surfaceH = h;

		LOG("[MetalPresent::%s] %dx%d image, %zu bytes per row", __func__, w, h, IOSurfaceGetBytesPerRow(surface));
		return true;
	}
}


bool MacMetalPresent_Init(void* nsWindow, bool hiDPI)
{
	if (device != nil)
		return true;

	if (nsWindow == nullptr)
		return false;

	@autoreleasepool {
		if ((device = MTLCreateSystemDefaultDevice()) == nil) {
			LOG_L(L_ERROR, "[MetalPresent::%s] no Metal device", __func__);
			return false;
		}

		if ((queue = [device newCommandQueue]) == nil) {
			LOG_L(L_ERROR, "[MetalPresent::%s] no Metal command queue", __func__);
			return false;
		}

		NSWindow* window = (NSWindow*) nsWindow;
		NSView* view = window.contentView;

		layer = [[CAMetalLayer alloc] init];
		layer.device = device;
		layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
		layer.framebufferOnly = YES;
		layer.opaque = YES;
		const CGSize points = view.bounds.size;

		// the knob is contentsScale rather than drawableSize, because AppKit
		// derives the drawable from the scale when it lays a hosted layer out,
		// so a drawable set behind its back would not survive a resize
		const CGFloat scale = hiDPI ? window.backingScaleFactor : 1.0;

		layer.contentsScale = scale;
		layer.displaySyncEnabled = vsync;
		layer.frame = view.bounds;

		// AppKit has not laid the layer out yet, and the caller needs the size
		// now to match its framebuffer to it, so say what it is rather than
		// waiting to be told
		layer.drawableSize = CGSizeMake(points.width * scale, points.height * scale);

		// hosting rather than adding a sublayer, so AppKit resizes the layer
		// with the view and keeps its drawable size in step
		view.layer = layer;
		view.wantsLayer = YES;

		LOG("[MetalPresent::%s] %s, %.0fx%.0f points at %.1fx, %.0fx%.0f drawable, display sync %s", __func__, device.name.UTF8String,
			points.width, points.height, scale, layer.drawableSize.width, layer.drawableSize.height, vsync ? "on" : "off");
	}

	return true;
}


void MacMetalPresent_SetVSync(bool enabled)
{
	vsync = enabled;

	// Logged either way. VSync is applied at startup and the layer may not exist
	// yet, and a setting that silently went nowhere is the failure this whole
	// harness exists to catch.
	if (layer == nil) {
		LOG("[MetalPresent::%s] display sync %s, held until the layer exists", __func__, enabled ? "on" : "off");
		return;
	}

	@autoreleasepool {
		layer.displaySyncEnabled = enabled;
	}

	LOG("[MetalPresent::%s] display sync %s", __func__, enabled ? "on" : "off");
}


void MacMetalPresent_GetDrawableSize(int* outW, int* outH)
{
	// a CAMetalLayer sizes its drawable from its bounds only until something
	// sets the drawable itself, which Init has to do before AppKit's first
	// layout, so from then on it is ours to keep up to date
	if (layer != nil && layer.bounds.size.width > 0.0 && layer.bounds.size.height > 0.0) {
		const CGSize points = layer.bounds.size;
		const CGFloat scale = layer.contentsScale;

		layer.drawableSize = CGSizeMake(points.width * scale, points.height * scale);
	}

	const CGSize size = (layer != nil) ? layer.drawableSize : CGSizeZero;

	if (outW != nullptr)
		*outW = (int) size.width;
	if (outH != nullptr)
		*outH = (int) size.height;
}


void* MacMetalPresent_AcquireIOSurfaceBuffer(int w, int h, size_t* outRowBytes)
{
	if (outRowBytes != nullptr)
		*outRowBytes = 0;

	if (queue == nil || w <= 0 || h <= 0)
		return nullptr;

	@autoreleasepool {
		if (!EnsureSurface(w, h))
			return nullptr;
		if (!BuildPipeline())
			return nullptr;

		// one surface serves both the caller and the GPU, so writing to it
		// before the last frame is off the GPU would tear
		if (lastPresent != nil) {
			[lastPresent waitUntilCompleted];
			[lastPresent release];
			lastPresent = nil;
		}

		if (IOSurfaceLock(surface, 0, nullptr) != kIOReturnSuccess) {
			LOG_L(L_ERROR, "[MetalPresent::%s] could not lock the IOSurface for writing", __func__);
			return nullptr;
		}
	}

	surfaceLocked = true;

	if (outRowBytes != nullptr)
		*outRowBytes = IOSurfaceGetBytesPerRow(surface);

	return IOSurfaceGetBaseAddress(surface);
}


void MacMetalPresent_PresentIOSurface(bool flipY)
{
	if (queue == nil || surfaceTex == nil || pipeline == nil)
		return;

	if (surfaceLocked) {
		IOSurfaceUnlock(surface, 0, nullptr);
		surfaceLocked = false;
	}

	@autoreleasepool {
		id<CAMetalDrawable> drawable = [layer nextDrawable];

		if (drawable == nil) {
			LOG_L(L_WARNING, "[MetalPresent::%s] no drawable, frame dropped", __func__);
			return;
		}

		MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
		passDesc.colorAttachments[0].texture = drawable.texture;
		passDesc.colorAttachments[0].loadAction = MTLLoadActionDontCare;
		passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;

		const float flip = flipY ? 1.0f : 0.0f;

		id<MTLCommandBuffer> cmdBuf = [queue commandBuffer];
		id<MTLRenderCommandEncoder> encoder = [cmdBuf renderCommandEncoderWithDescriptor: passDesc];

		[encoder setRenderPipelineState: pipeline];
		[encoder setVertexBytes: &flip length: sizeof(flip) atIndex: 0];
		[encoder setFragmentTexture: surfaceTex atIndex: 0];
		[encoder setFragmentSamplerState: sampler atIndex: 0];
		[encoder drawPrimitives: MTLPrimitiveTypeTriangle vertexStart: 0 vertexCount: 3];
		[encoder endEncoding];

		[cmdBuf presentDrawable: drawable];
		[cmdBuf commit];

		lastPresent = [cmdBuf retain];
	}
}


void MacMetalPresent_PresentBGRA(int w, int h, const void* pixels, bool flipY)
{
	if (pixels == nullptr)
		return;

	size_t dstRowBytes = 0;
	uint8_t* dst = (uint8_t*) MacMetalPresent_AcquireIOSurfaceBuffer(w, h, &dstRowBytes);

	if (dst == nullptr)
		return;

	const uint8_t* src = (const uint8_t*) pixels;
	const size_t srcRowBytes = (size_t) w * 4;

	for (int y = 0; y < h; ++y)
		memcpy(dst + y * dstRowBytes, src + y * srcRowBytes, srcRowBytes);

	MacMetalPresent_PresentIOSurface(flipY);
}
