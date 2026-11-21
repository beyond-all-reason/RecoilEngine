/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

namespace PointerWarp {
	// Spring.WarpMouse leaves the system pointer alone only while the user is in another
	// window: ours has no focus and the pointer is not over it (alt-tabbed out). An unfocused
	// window under the pointer, as behind an overlay, still warps.
	constexpr bool MovesSystemPointer(bool windowFocused, bool pointerOverWindow)
	{
		return windowFocused || pointerOverWindow;
	}
}
