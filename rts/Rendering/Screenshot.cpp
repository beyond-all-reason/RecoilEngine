/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Screenshot.h"

#include <future>
#include <vector>

#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/StringUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileHandler.h"
#include "System/TimeUtil.h"

#undef CreateDirectory

CONFIG(int, ScreenshotCounter).description("Deprecated, does nothing, but not marked as such to keep compatibility with older engine versions").defaultValue(0);

struct FunctionArgs
{
	std::vector<uint8_t> pixelbuf;
	std::string filename;
	unsigned quality;
	int x;
	int y;
};

static std::future<void> fut = {};

static std::string pendingType = {};
static unsigned pendingQuality = 0;

void TakeScreenshot(std::string type, unsigned quality)
{
	pendingType    = type.empty() ? "png" : type;
	pendingQuality = quality;
}

void ScreenshotReadbackBegin() {}

// Called at the END of CGame::Draw(), after all rendering, before SwapBuffers.
// glReadPixels must happen here: the back buffer contains the finished frame at
// this point on all platforms. After SwapBuffers its contents are undefined
// (on Linux/Wayland drivers correctly treat it as such, producing a black image).
// Compression and disk write are offloaded to a dedicated thread via std::async.
void ScreenshotReadbackEnd()
{
	if (pendingType.empty())
		return;

	if (!FileSystem::CreateDirectory("screenshots"))
		return;

	const std::string type = std::move(pendingType);
	pendingType = {};

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glReadBuffer(GL_BACK);

	int x = globalRendering->winSizeX;
	int y = globalRendering->winSizeY;
	x += ((4 - (x % 4)) * int((x % 4) != 0));

	FunctionArgs args;
	args.x        = x;
	args.y        = y;
	args.quality  = pendingQuality;
	args.filename = "screenshots/screen_" + CTimeUtil::GetCurrentTimeStr(true) + "." + type;
	args.pixelbuf.resize(static_cast<size_t>(x) * static_cast<size_t>(y) * 4u);

	glReadPixels(0, 0, x, y, GL_RGBA, GL_UNSIGNED_BYTE, args.pixelbuf.data());

	if (fut.valid())
		fut.get();

	fut = std::async(std::launch::async, [](FunctionArgs&& args) {
		CBitmap bmp(&args.pixelbuf[0], args.x, args.y);
		bmp.ReverseYAxis();
		bmp.Save(args.filename, true, true, args.quality);
	}, std::move(args));
}
