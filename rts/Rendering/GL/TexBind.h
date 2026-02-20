/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "myGL.h"
#include "glHelpers.h"

namespace GL
{

// A texture bind control object, that automatically unbinds texture and restores active tex unit upon destruction
// When constructed without exact slot, will also restore the previously bound texture to the contemporary slot
class TexBind {
public:
	inline TexBind() {} // can be used to pop the binding
	inline TexBind(uint32_t slot, GLenum target, GLuint textureID)
		: stateTexUnit(GL::FetchEffectualStateAttribValue<GLenum>(GL_ACTIVE_TEXTURE))
		, texUnit(GL_TEXTURE0+slot)
		, target(target)
		, restoredTextureID(0)
	{
		if (stateTexUnit != texUnit)
			glActiveTexture(texUnit);

		restoredTextureID = GL::FetchCurrentSlotBoundTextureID(target);
		glBindTexture(target, textureID);
	}
	inline TexBind(GLenum target, GLuint textureID)
		: stateTexUnit(GL::FetchEffectualStateAttribValue<GLenum>(GL_ACTIVE_TEXTURE))
		, texUnit(stateTexUnit)
		, target(target)
		, restoredTextureID(GL::FetchCurrentSlotBoundTextureID(target))
	{
		glBindTexture(target, textureID);
	}

	inline TexBind(const TexBind&) = delete;
	inline TexBind(TexBind&& other) noexcept { *this = std::move(other); }

	inline TexBind& operator=(const TexBind&) = delete;
	inline TexBind& operator=(TexBind&& other) noexcept {
		std::swap(stateTexUnit, other.stateTexUnit);
		std::swap(texUnit, other.texUnit);
		std::swap(target, other.target);
		std::swap(restoredTextureID, other.restoredTextureID);

		return *this;
	}

	inline ~TexBind()
	{
		if (target == 0)
			return;

		glActiveTexture(texUnit);
		glBindTexture(target, restoredTextureID);
		if (stateTexUnit != texUnit) glActiveTexture(stateTexUnit);
	}

	inline auto GetLastActiveTextureSlot() const { return stateTexUnit; }
private:
	GLenum stateTexUnit = 0;
	GLenum texUnit = 0;
	GLenum target = 0;
	GLuint restoredTextureID = 0;
};

}
