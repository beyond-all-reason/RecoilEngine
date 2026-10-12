/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "XRSession.h"

#if defined(USE_OPENXR) && defined(_WIN32)

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <unknwn.h>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include "System/Log/ILog.h"

// GL enums, so that this file does not need the GL headers
static constexpr int64_t XR_GL_RGBA8 = 0x8058;
static constexpr int64_t XR_GL_SRGB8_ALPHA8 = 0x8C43;

#define XR_CHECK(call) \
	do { \
		const XrResult xrRes = (call); \
		if (XR_FAILED(xrRes)) { \
			char xrResStr[XR_MAX_RESULT_STRING_SIZE] = {0}; \
			xrResultToString(instance, xrRes, xrResStr); \
			LOG_L(L_ERROR, "[XRSession::%s] %s failed: %s", __func__, #call, xrResStr); \
			return false; \
		} \
	} while (0)

struct CXRSession::Impl {
	struct Eye {
		XrSwapchain swapchain = XR_NULL_HANDLE;
		std::vector<XrSwapchainImageOpenGLKHR> images;
		bool imageAcquired = false;
		bool submitted = false;
	};

	bool Start(int resolutionScalePercent);
	void Stop();
	bool PollEvents();
	bool BeginFrame();
	void EndFrame();
	uint32_t AcquireEyeImage(int eye);
	void ReleaseEyeImage(int eye);

	XrInstance instance = XR_NULL_HANDLE;
	XrSystemId systemId = XR_NULL_SYSTEM_ID;
	XrSession session = XR_NULL_HANDLE;
	XrSpace space = XR_NULL_HANDLE;
	XrSessionState state = XR_SESSION_STATE_UNKNOWN;

	bool running = false;
	bool lost = false;
	bool frameBegun = false;
	bool shouldRender = false;

	int eyeWidth = 0;
	int eyeHeight = 0;

	Eye eyes[2];
	XREye eyePoses[2];
	XrView views[2];
	XrFrameState frameState{XR_TYPE_FRAME_STATE};
};


bool CXRSession::Impl::Start(int resolutionScalePercent)
{
	if (session != XR_NULL_HANDLE)
		return true;

	lost = false;

	const char* extensions[] = {XR_KHR_OPENGL_ENABLE_EXTENSION_NAME};

	XrInstanceCreateInfo instanceInfo{XR_TYPE_INSTANCE_CREATE_INFO};
	std::strncpy(instanceInfo.applicationInfo.applicationName, "Recoil", XR_MAX_APPLICATION_NAME_SIZE - 1);
	std::strncpy(instanceInfo.applicationInfo.engineName, "Recoil", XR_MAX_ENGINE_NAME_SIZE - 1);
	instanceInfo.applicationInfo.applicationVersion = 1;
	instanceInfo.applicationInfo.engineVersion = 1;
	instanceInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
	instanceInfo.enabledExtensionCount = 1;
	instanceInfo.enabledExtensionNames = extensions;

	{
		const XrResult res = xrCreateInstance(&instanceInfo, &instance);
		if (XR_FAILED(res)) {
			LOG_L(L_ERROR, "[XRSession::%s] no OpenXR runtime available (xrCreateInstance result %d)", __func__, int(res));
			instance = XR_NULL_HANDLE;
			return false;
		}
	}

	XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
	systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	{
		const XrResult res = xrGetSystem(instance, &systemInfo, &systemId);
		if (XR_FAILED(res)) {
			LOG_L(L_ERROR, "[XRSession::%s] no headset found (xrGetSystem result %d)", __func__, int(res));
			Stop();
			return false;
		}
	}

	// the runtime insists on this being queried before a session is created
	PFN_xrGetOpenGLGraphicsRequirementsKHR getGLRequirements = nullptr;
	XR_CHECK(xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&getGLRequirements)));
	XrGraphicsRequirementsOpenGLKHR glRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
	XR_CHECK(getGLRequirements(instance, systemId, &glRequirements));

	uint32_t viewCount = 0;
	XR_CHECK(xrEnumerateViewConfigurationViews(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr));
	if (viewCount != 2) {
		LOG_L(L_ERROR, "[XRSession::%s] expected a stereo view configuration, got %u views", __func__, viewCount);
		Stop();
		return false;
	}

	XrViewConfigurationView configViews[2] = {{XR_TYPE_VIEW_CONFIGURATION_VIEW}, {XR_TYPE_VIEW_CONFIGURATION_VIEW}};
	XR_CHECK(xrEnumerateViewConfigurationViews(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &viewCount, configViews));

	const int scale = std::clamp(resolutionScalePercent, 10, 200);
	eyeWidth = std::clamp(int(configViews[0].recommendedImageRectWidth) * scale / 100, 16, int(configViews[0].maxImageRectWidth));
	eyeHeight = std::clamp(int(configViews[0].recommendedImageRectHeight) * scale / 100, 16, int(configViews[0].maxImageRectHeight));

	XrGraphicsBindingOpenGLWin32KHR binding{XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
	binding.hDC = wglGetCurrentDC();
	binding.hGLRC = wglGetCurrentContext();

	XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
	sessionInfo.next = &binding;
	sessionInfo.systemId = systemId;
	XR_CHECK(xrCreateSession(instance, &sessionInfo, &session));

	XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
	spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	spaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
	XR_CHECK(xrCreateReferenceSpace(session, &spaceInfo, &space));

	// prefer sRGB: the compositor then decodes what we wrote, same as the window would be
	uint32_t formatCount = 0;
	XR_CHECK(xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr));
	std::vector<int64_t> formats(formatCount);
	XR_CHECK(xrEnumerateSwapchainFormats(session, formatCount, &formatCount, formats.data()));

	int64_t format = formats.empty() ? XR_GL_RGBA8 : formats[0];
	for (const int64_t candidate: {XR_GL_SRGB8_ALPHA8, XR_GL_RGBA8}) {
		if (std::find(formats.begin(), formats.end(), candidate) != formats.end()) {
			format = candidate;
			break;
		}
	}

	for (Eye& eye: eyes) {
		XrSwapchainCreateInfo swapchainInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
		swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
		swapchainInfo.format = format;
		swapchainInfo.sampleCount = 1;
		swapchainInfo.width = eyeWidth;
		swapchainInfo.height = eyeHeight;
		swapchainInfo.faceCount = 1;
		swapchainInfo.arraySize = 1;
		swapchainInfo.mipCount = 1;
		XR_CHECK(xrCreateSwapchain(session, &swapchainInfo, &eye.swapchain));

		uint32_t imageCount = 0;
		XR_CHECK(xrEnumerateSwapchainImages(eye.swapchain, 0, &imageCount, nullptr));
		eye.images.assign(imageCount, XrSwapchainImageOpenGLKHR{XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR});
		XR_CHECK(xrEnumerateSwapchainImages(eye.swapchain, imageCount, &imageCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(eye.images.data())));
	}

	for (XrView& view: views)
		view = XrView{XR_TYPE_VIEW};

	LOG("[XRSession] started: %dx%d per eye, swapchain format 0x%llx", eyeWidth, eyeHeight, static_cast<unsigned long long>(format));
	return true;
}

void CXRSession::Impl::Stop()
{
	for (Eye& eye: eyes) {
		if (eye.swapchain != XR_NULL_HANDLE)
			xrDestroySwapchain(eye.swapchain);
		eye = Eye{};
	}
	if (space != XR_NULL_HANDLE)
		xrDestroySpace(space);
	if (session != XR_NULL_HANDLE)
		xrDestroySession(session);
	if (instance != XR_NULL_HANDLE)
		xrDestroyInstance(instance);

	instance = XR_NULL_HANDLE;
	session = XR_NULL_HANDLE;
	space = XR_NULL_HANDLE;
	systemId = XR_NULL_SYSTEM_ID;
	state = XR_SESSION_STATE_UNKNOWN;
	running = false;
	frameBegun = false;
	shouldRender = false;
}

bool CXRSession::Impl::PollEvents()
{
	XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};

	while (xrPollEvent(instance, &event) == XR_SUCCESS) {
		switch (event.type) {
			case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
				lost = true;
			} break;
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
				const auto* changed = reinterpret_cast<const XrEventDataSessionStateChanged*>(&event);
				state = changed->state;

				if (state == XR_SESSION_STATE_READY) {
					XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
					beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
					const bool began = XR_SUCCEEDED(xrBeginSession(session, &beginInfo));
					running = began;
					lost |= !began;
				} else if (state == XR_SESSION_STATE_STOPPING) {
					xrEndSession(session);
					running = false;
				} else if (state == XR_SESSION_STATE_EXITING || state == XR_SESSION_STATE_LOSS_PENDING) {
					running = false;
					lost = true;
				}
			} break;
			default: break;
		}
		event = XrEventDataBuffer{XR_TYPE_EVENT_DATA_BUFFER};
	}
	return !lost;
}

bool CXRSession::Impl::BeginFrame()
{
	shouldRender = false;
	if (session == XR_NULL_HANDLE)
		return false;

	PollEvents();
	if (!running)
		return false;

	XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
	frameState = XrFrameState{XR_TYPE_FRAME_STATE};
	if (XR_FAILED(xrWaitFrame(session, &waitInfo, &frameState))) {
		lost = true;
		return false;
	}

	XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
	if (XR_FAILED(xrBeginFrame(session, &beginInfo))) {
		lost = true;
		return false;
	}
	frameBegun = true;

	for (Eye& eye: eyes) {
		eye.imageAcquired = false;
		eye.submitted = false;
	}

	if (!frameState.shouldRender)
		return false;

	XrViewLocateInfo locateInfo{XR_TYPE_VIEW_LOCATE_INFO};
	locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	locateInfo.displayTime = frameState.predictedDisplayTime;
	locateInfo.space = space;

	XrViewState viewState{XR_TYPE_VIEW_STATE};
	uint32_t located = 0;
	if (XR_FAILED(xrLocateViews(session, &locateInfo, &viewState, 2, &located, views)) || located != 2)
		return false;

	constexpr XrViewStateFlags needed = XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT;
	if ((viewState.viewStateFlags & needed) != needed)
		return false;

	for (int i = 0; i < 2; ++i) {
		const XrPosef& pose = views[i].pose;
		const XrFovf& fov = views[i].fov;
		XREye& out = eyePoses[i];

		out.pos[0] = pose.position.x;
		out.pos[1] = pose.position.y;
		out.pos[2] = pose.position.z;
		out.rot[0] = pose.orientation.x;
		out.rot[1] = pose.orientation.y;
		out.rot[2] = pose.orientation.z;
		out.rot[3] = pose.orientation.w;
		out.tanLeft = std::tan(fov.angleLeft);
		out.tanRight = std::tan(fov.angleRight);
		out.tanBottom = std::tan(fov.angleDown);
		out.tanTop = std::tan(fov.angleUp);
	}

	shouldRender = true;
	return true;
}

void CXRSession::Impl::EndFrame()
{
	if (!frameBegun)
		return;
	frameBegun = false;

	// the swapchain images may still be held if rendering was cut short
	for (Eye& eye: eyes) {
		if (eye.imageAcquired) {
			XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
			xrReleaseSwapchainImage(eye.swapchain, &releaseInfo);
			eye.imageAcquired = false;
		}
	}

	XrCompositionLayerProjectionView projViews[2];
	for (int i = 0; i < 2; ++i) {
		projViews[i] = XrCompositionLayerProjectionView{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
		projViews[i].pose = views[i].pose;
		projViews[i].fov = views[i].fov;
		projViews[i].subImage.swapchain = eyes[i].swapchain;
		projViews[i].subImage.imageRect.offset = {0, 0};
		projViews[i].subImage.imageRect.extent = {eyeWidth, eyeHeight};
		projViews[i].subImage.imageArrayIndex = 0;
	}

	XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
	layer.space = space;
	layer.viewCount = 2;
	layer.views = projViews;

	const bool submitLayer = shouldRender && eyes[0].submitted && eyes[1].submitted;
	const XrCompositionLayerBaseHeader* layers[1] = {reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer)};

	XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
	endInfo.displayTime = frameState.predictedDisplayTime;
	endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	endInfo.layerCount = submitLayer ? 1 : 0;
	endInfo.layers = submitLayer ? layers : nullptr;

	if (XR_FAILED(xrEndFrame(session, &endInfo)))
		lost = true;
}

uint32_t CXRSession::Impl::AcquireEyeImage(int eye)
{
	Eye& e = eyes[eye];
	if (!frameBegun || !shouldRender || e.swapchain == XR_NULL_HANDLE || e.imageAcquired)
		return 0;

	uint32_t index = 0;
	XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
	if (XR_FAILED(xrAcquireSwapchainImage(e.swapchain, &acquireInfo, &index)))
		return 0;
	e.imageAcquired = true;

	XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
	waitInfo.timeout = XR_INFINITE_DURATION;
	if (XR_FAILED(xrWaitSwapchainImage(e.swapchain, &waitInfo)) || index >= e.images.size())
		return 0;

	return e.images[index].image;
}

void CXRSession::Impl::ReleaseEyeImage(int eye)
{
	Eye& e = eyes[eye];
	if (!e.imageAcquired)
		return;

	XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
	e.submitted = XR_SUCCEEDED(xrReleaseSwapchainImage(e.swapchain, &releaseInfo));
	e.imageAcquired = false;
}


CXRSession::CXRSession() : impl(std::make_unique<Impl>()) {}
CXRSession::~CXRSession() = default;

CXRSession& CXRSession::GetInstance()
{
	static CXRSession instance;
	return instance;
}

bool CXRSession::Start(int resolutionScalePercent) { return impl->Start(resolutionScalePercent); }
void CXRSession::Stop() { impl->Stop(); }
bool CXRSession::IsCreated() const { return impl->session != XR_NULL_HANDLE; }
bool CXRSession::IsLost() const { return impl->lost; }
bool CXRSession::BeginFrame() { return impl->BeginFrame(); }
void CXRSession::EndFrame() { impl->EndFrame(); }
int CXRSession::GetEyeWidth() const { return impl->eyeWidth; }
int CXRSession::GetEyeHeight() const { return impl->eyeHeight; }
const XREye& CXRSession::GetEye(int eye) const { return impl->eyePoses[eye]; }
uint32_t CXRSession::AcquireEyeImage(int eye) { return impl->AcquireEyeImage(eye); }
void CXRSession::ReleaseEyeImage(int eye) { impl->ReleaseEyeImage(eye); }

#else // no OpenXR in this build

struct CXRSession::Impl {};

CXRSession::CXRSession() {}
CXRSession::~CXRSession() = default;

CXRSession& CXRSession::GetInstance()
{
	static CXRSession instance;
	return instance;
}

bool CXRSession::Start(int) { return false; }
void CXRSession::Stop() {}
bool CXRSession::IsCreated() const { return false; }
bool CXRSession::IsLost() const { return false; }
bool CXRSession::BeginFrame() { return false; }
void CXRSession::EndFrame() {}
int CXRSession::GetEyeWidth() const { return 0; }
int CXRSession::GetEyeHeight() const { return 0; }
const XREye& CXRSession::GetEye(int) const { static const XREye none; return none; }
uint32_t CXRSession::AcquireEyeImage(int) { return 0; }
void CXRSession::ReleaseEyeImage(int) {}

#endif
