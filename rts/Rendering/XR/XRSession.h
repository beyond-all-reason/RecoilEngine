/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <memory>

/**
 * Pose and projection of one eye, as located by the OpenXR runtime
 * (XR "local" space: metres, +x right, +y up, -z forward).
 */
struct XREye {
	float pos[3] = {0.0f, 0.0f, 0.0f};
	// orientation quaternion, x y z w
	float rot[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	// frustum edges as tangents at the near plane's unit distance (as in a headset's fov)
	float tanLeft = -1.0f;
	float tanRight = 1.0f;
	float tanBottom = -1.0f;
	float tanTop = 1.0f;
};

/**
 * Thin wrapper around an OpenXR session using the OpenGL binding: owns the
 * instance, session and one swapchain per eye, and runs the frame loop
 * (wait/begin/locate, acquire/release eye images, end).
 *
 * Only functional in Windows builds with USE_OPENXR; elsewhere every call
 * reports failure.
 *
 * Usage per frame: BeginFrame(), then per eye AcquireEyeImage() / render /
 * ReleaseEyeImage(), then EndFrame().
 *
 * See coding-agents/MULTI_RENDER_TARGETS.md for the multi-view groundwork.
 */
class CXRSession {
public:
	static CXRSession& GetInstance();

	~CXRSession();

	// creates the instance, session and swapchains; needs a current GL context.
	// False (and logged) when there is no runtime or headset. The eye targets
	// get the recommended size scaled by the given percentage.
	bool Start(int resolutionScalePercent);
	void Stop();

	bool IsCreated() const;
	// the runtime asked the application to quit, or the session was lost
	bool IsLost() const;

	// polls events and, while the session is running, waits for the runtime's
	// frame pacing (this blocks), begins the frame and locates both eyes.
	// True when the eyes should be rendered this frame.
	bool BeginFrame();
	void EndFrame();

	// size of the per-eye render target the runtime recommends
	int GetEyeWidth() const;
	int GetEyeHeight() const;
	const XREye& GetEye(int eye) const;

	// GL texture the eye should be rendered into, or 0 on failure
	uint32_t AcquireEyeImage(int eye);
	void ReleaseEyeImage(int eye);

private:
	CXRSession();
	CXRSession(const CXRSession&) = delete;
	CXRSession& operator = (const CXRSession&) = delete;

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};
