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

	void SetMaxSize(uint32_t xsize, uint32_t ysize) { maxsize = uint2(xsize, ysize); }
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

	inline int GetPadding() const
	{
		return 1 << (numLevels - 1);
	}

	void SizeRoundUp()
	{
		atlasSize.x = std::min(AlignUp(atlasSize.x, GetPadding()), maxsize.x);
		atlasSize.y = std::min(AlignUp(atlasSize.y, GetPadding()), maxsize.y);
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

	// pixel center based UV
	AtlasedTexture GetTexCoordsCntr(const spring::unordered_map<std::string, SAtlasEntry>::const_iterator& it)
	{
		if (it == entries.end())
			return AtlasedTexture::DefaultAtlasTexture;

		const AtlasedTexture& a = it->second.texCoords; // all coords are inclusive
		AtlasedTexture uv = a;

		const float invW = 1.0f / atlasSize.x;
		const float invH = 1.0f / atlasSize.y;

		// Convert inclusive texel indices -> normalized coordinates of texel centers.
		uv.x1 = (a.x1 + 0.5f) * invW;
		uv.y1 = (a.y1 + 0.5f) * invH;
		uv.x2 = (a.x2 + 0.5f) * invW;
		uv.y2 = (a.y2 + 0.5f) * invH;

		return uv;
	}

	// pixel edges based UV
	AtlasedTexture GetTexCoordsEdge(const spring::unordered_map<std::string, SAtlasEntry>::const_iterator& it) const
	{
		if (it == entries.end())
			return AtlasedTexture::DefaultAtlasTexture;

		const AtlasedTexture& a = it->second.texCoords; // all coords are inclusive
		AtlasedTexture uv = a;

		const float invW = 1.0f / atlasSize.x;
		const float invH = 1.0f / atlasSize.y;

		// Edges in texel space are [x1, x2+1] and [y1, y2+1] because x2/y2 are inclusive.
		uv.x1 = (a.x1       ) * invW;
		uv.y1 = (a.y1       ) * invH;
		uv.x2 = (a.x2 + 1.0f) * invW;
		uv.y2 = (a.y2 + 1.0f) * invH;

		return uv;
	}

	AtlasedTexture GetTexCoordsCntr(const std::string& name)
	{
		return GetTexCoordsCntr(FindEntry(name));
	}

	AtlasedTexture GetTexCoordsEdge(const std::string& name)
	{
		return GetTexCoordsEdge(FindEntry(name));
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

	uint2 atlasSize;
	uint2 maxsize = {2048, 2048};
	int numLevels = std::numeric_limits<int>::max();
	int minDim = std::numeric_limits<int>::max();
};