#include "RasFileHandler.h"
#include "System/FileSystem/FileHandler.h"

#include "System/Misc/TracyDefs.h"

CRasFile* CRasFileHandler::GetRasFile(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = rasFileHandles.find(name);

	if (it != rasFileHandles.end())
		return &rasFileObjects[it->second];

	CFileHandler f(name);

	if (!f.FileExists())
		return nullptr;

	rasFileHandles[name] = rasFileObjects.size();
	rasFileObjects.emplace_back(CRasFile(f, name));

	return &rasFileObjects[rasFileObjects.size() - 1];
}


CRasFile* CRasFileHandler::ReloadRasFile(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = rasFileHandles.find(name);

	if (it == rasFileHandles.end())
		return (GetRasFile(name));

	CFileHandler f(name);
	assert(f.FileExists());

	rasFileObjects[it->second] = CRasFile(f, name);
	return &rasFileObjects[it->second];
}


const CRasFile* CRasFileHandler::GetScriptFile(const std::string& name) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = rasFileHandles.find(name);

	if (it != rasFileHandles.end())
		return &rasFileObjects[it->second];

	return nullptr;
}

