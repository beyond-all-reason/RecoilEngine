/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RAS_FILE_HANDLER_H
#define RAS_FILE_HANDLER_H

#include <deque>

#include "RasFile.h"
#include "System/UnorderedMap.hpp"

class CRasFileHandler
{
public:
	void Init() { rasFileHandles.reserve(256); }
	void Kill() {
		// never explicitly iterated, can simply clear
		rasFileHandles.clear();
		rasFileObjects.clear();
	}

	CRasFile* GetRasFile(const std::string& name);
	CRasFile* ReloadRasFile(const std::string& name);
	const CRasFile* GetScriptFile(const std::string& name) const;

private:
	spring::unordered_map<std::string, size_t> rasFileHandles;
	std::deque<CRasFile> rasFileObjects;
};

extern CRasFileHandler* rasFileHandler;

#endif

