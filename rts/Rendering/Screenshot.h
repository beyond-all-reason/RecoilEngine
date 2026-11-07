/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <string>

void TakeScreenshot(std::string type, unsigned quality);
void ScreenshotReadbackBegin(); // no-op, reserved for future use
void ScreenshotReadbackEnd();   // call at END of Draw(), before SwapBuffers — reads back the finished frame

#endif
