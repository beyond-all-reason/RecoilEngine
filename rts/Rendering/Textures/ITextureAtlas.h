/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <string>

#include "AtlasedTexture.hpp"
#include "System/type2.h"

class ITextureAtlas
{
public:
	virtual ~ITextureAtlas() = default;

	virtual const std::string& GetName() const = 0;
	virtual uint32_t GetTexID() const = 0;
	virtual uint32_t GetTexTarget() const = 0;
	virtual int2 GetSize() const = 0;
	virtual uint32_t GetNumPages() const = 0;
	virtual int GetNumTexLevels() const = 0;

	virtual const AtlasedTexture* FindTexture(const std::string& name) const = 0;
	virtual AtlasedTexture* GetTexturePtr(const std::string& name) = 0;
	virtual std::string GetTextureName(const AtlasedTexture* texture) const = 0;

	virtual void ReloadTexture() = 0;
};
