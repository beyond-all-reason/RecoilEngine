/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#if defined(RECOIL_RMLUI_RENDERER_GL3) && defined(RECOIL_RMLUI_RENDERER_GL4)
	#error "Select exactly one interactive RmlUi renderer"
#endif

#if !defined(RECOIL_RMLUI_RENDERER_GL3) && !defined(RECOIL_RMLUI_RENDERER_GL4)
	#define RECOIL_RMLUI_RENDERER_GL3 1
#endif

#ifndef RECOIL_RMLUI_GL4_TEXT_RUN_ATLAS
	#define RECOIL_RMLUI_GL4_TEXT_RUN_ATLAS 0
#endif

#ifndef RECOIL_RMLUI_GL4_STRICT_VALIDATION
	#define RECOIL_RMLUI_GL4_STRICT_VALIDATION 0
#endif

#ifndef RECOIL_RMLUI_GL4_DOCUMENT_BATCH_BARRIERS
	#define RECOIL_RMLUI_GL4_DOCUMENT_BATCH_BARRIERS 1
#endif
