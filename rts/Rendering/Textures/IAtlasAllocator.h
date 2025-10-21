/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <string>
#include <limits>
#include <cstdint>

#include "AtlasedTexture.hpp"
#include "System/type2.h"
#include "System/UnorderedMap.hpp"
#include "System/StringHash.h"
#include "System/SpringMath.h"


class IAtlasAllocator
{
public:
	struct SAtlasEntry
	{
		SAtlasEntry() = default;
		SAtlasEntry(const int2 _size, std::string _name)
			: size(_size)
			, name(std::move(_name))
			, texCoords()
		{}

		int2 size;
		std::string name;
		AtlasedTexture texCoords;
	};
public:
	IAtlasAllocator() = default;
	virtual ~IAtlasAllocator() {}

	void SetMaxSize(int xsize, int ysize) { maxsize = int2(xsize, ysize); }
public:
	virtual bool Allocate() = 0;
	virtual int GetNumTexLevels() const = 0;
	virtual int GetReqNumTexLevels() const = 0;
	virtual uint32_t GetNumPages() const = 0;
	void SetMaxTexLevel(int maxLevels) { numLevels = maxLevels; };
public:
	void AddEntry(const SAtlasEntry& ae) { AddEntry(ae.name, ae.size); }
	void AddEntry(const std::string& name, const int2& size)
	{
		minDim = argmin(minDim, size.x, size.y);
		entries[name] = SAtlasEntry(size, name);
	}

	auto FindEntry(const std::string& name) const
	{
		return entries.find(name);
	}

	const auto& GetEntry(const spring::unordered_map<std::string, SAtlasEntry>::const_iterator& it) const {
		if (it == entries.end())
			return AtlasedTexture::DefaultAtlasTexture;

		return it->second.texCoords;
	}
	const auto& GetEntry(const std::string& name) const { return GetEntry(FindEntry(name));	}
	const auto& GetEntries() const { return entries; }

	AtlasedTexture GetTexCoords(const spring::unordered_map<std::string, SAtlasEntry>::const_iterator& it)
	{
		if (it == entries.end())
			return AtlasedTexture::DefaultAtlasTexture;

		AtlasedTexture uv = it->second.texCoords;

		uv.x1 /= atlasSize.x;
		uv.y1 /= atlasSize.y;
		uv.x2 /= atlasSize.x;
		uv.y2 /= atlasSize.y;

		// adjust texture coordinates by half a texel (opengl uses centeroids)
		uv.x1 += 0.5f / atlasSize.x;
		uv.y1 += 0.5f / atlasSize.y;
		uv.x2 += 0.5f / atlasSize.x;
		uv.y2 += 0.5f / atlasSize.y;

		return uv;
	}
	AtlasedTexture GetTexCoords(const std::string& name)
	{
		return GetTexCoords(FindEntry(name));
	}

	bool contains(const std::string& name) const
	{
		return entries.contains(name);
	}

	//! note: it doesn't clear the atlas! it only clears the entry db!
	void clear()
	{
		minDim = std::numeric_limits<int>::max();
		entries.clear();
	}

	int GetMinDim() const { return minDim < std::numeric_limits<int>::max() ? minDim : 1; }

	const auto& GetMaxSize() const { return maxsize; }
	const auto& GetAtlasSize() const { return atlasSize; }
protected:
	spring::unordered_map<std::string, SAtlasEntry> entries;

	int2 atlasSize;
	int2 maxsize = {2048, 2048};
	int numLevels = std::numeric_limits<int>::max();
	int minDim = std::numeric_limits<int>::max();
};