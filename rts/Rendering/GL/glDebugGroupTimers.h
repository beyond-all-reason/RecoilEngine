/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

namespace GL {
	// GPU timing of every SCOPED_GL_DEBUGGROUP, enabled by the GLDebugGroupTimers config variable
	// (N > 0: log a table of GPU milliseconds per frame per group every N frames). Each group issues a
	// GL_TIMESTAMP query at its start and end; the results are read three frames later, so timing
	// never stalls the pipeline. Nested groups are counted separately, so parents include children.
	class DebugGroupTimers {
	public:
		static bool Enabled();
		static void Begin(const char* name);
		static void End();
		static void EndFrame();
		static void Kill();
	};
}
