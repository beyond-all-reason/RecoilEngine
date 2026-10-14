/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AutoFill.h"

#include <CoreFoundation/CoreFoundation.h>
#include <objc/message.h>
#include <objc/runtime.h>

namespace AutoFill {

// With text input active, AppKit's AutoFill heuristics start a Safari helper
// process for the window. It never exits, so after quitting the engine stays
// listed as running in the background, and every start adds another helper.
// Registered defaults only apply in this process and lose to the user's own.
void Disable()
{
	const void* keys[] = {CFSTR("NSAutoFillHeuristicsEnabled")};
	const void* vals[] = {kCFBooleanFalse};
	CFDictionaryRef dict = CFDictionaryCreate(nullptr, keys, vals, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

	using GetDefaults = id (*)(Class, SEL);
	using RegisterDefaults = void (*)(id, SEL, CFDictionaryRef);
	id defaults = reinterpret_cast<GetDefaults>(objc_msgSend)(objc_getClass("NSUserDefaults"), sel_registerName("standardUserDefaults"));
	reinterpret_cast<RegisterDefaults>(objc_msgSend)(defaults, sel_registerName("registerDefaults:"), dict);

	CFRelease(dict);
}

};
