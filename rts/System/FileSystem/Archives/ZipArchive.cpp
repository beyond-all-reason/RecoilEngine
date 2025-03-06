/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ZipArchive.h"

#include <algorithm>
#include <stdexcept>
#include <cassert>

#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/Threading/ThreadPool.h"

IArchive* CZipArchiveFactory::DoCreateArchive(const std::string& filePath) const
{
	return new CZipArchive(filePath);
}

static_assert(ThreadPool::MAX_THREADS == CZipArchive::MAX_THREADS, "MAX_THREADS mismatch");

CZipArchive::CZipArchive(const std::string& archiveName)
	: CBufferedArchive(archiveName)
{
	std::scoped_lock lck(archiveLock);

	unzFile zip = nullptr;

	if ((zip = unzOpen(archiveName.c_str())) == nullptr) {
		LOG_L(L_ERROR, "[%s] error opening \"%s\"", __func__, archiveName.c_str());
		return;
	}

	unz_global_info64 globalZipInfo;

	memset(&globalZipInfo, 0, sizeof(globalZipInfo));
	unzGetGlobalInfo64(zip, &globalZipInfo);

	// We need to map file positions to speed up opening later
	fileEntries.reserve(globalZipInfo.number_entry);

	for (int ret = unzGoToFirstFile(zip); ret == UNZ_OK; ret = unzGoToNextFile(zip)) {
		unz_file_info info;
		char fName[512];

		unzGetCurrentFileInfo(zip, &info, fName, sizeof(fName), nullptr, 0, nullptr, 0);

		if (fName[0] == 0)
			continue;

		const size_t fNameLen = strlen(fName);

		// exclude directory names
		if ((fName[fNameLen - 1] == '/') || (fName[fNameLen - 1] == '\\'))
			continue;

		FileEntry fd;
		unzGetFilePos(zip, &fd.fp);

		fd.size = info.uncompressed_size;
		fd.origName = fName;
		fd.crc = info.crc;

		lcNameIndex.emplace(StringToLower(fd.origName), fileEntries.size());
		fileEntries.emplace_back(std::move(fd));
	}

	zipPerThread[0] = zip;
}

CZipArchive::~CZipArchive()
{
	std::scoped_lock lck(archiveLock);

	for (auto& zip : zipPerThread) {
		if (zip) {
			unzClose(zip);
			zip = nullptr;
		}
	}
}


void CZipArchive::FileInfo(unsigned int fid, std::string& name, int& size) const
{
	assert(IsFileId(fid));

	name = fileEntries[fid].origName;
	size = fileEntries[fid].size;
}

// To simplify things, files are always read completely into memory from
// the zip-file, since zlib does not provide any way of reading more
// than one file at a time
int CZipArchive::GetFileImpl(uint32_t fid, std::vector<std::uint8_t>& buffer)
{
	unzFile& thisThreadZip = zipPerThread[ThreadPool::GetThreadNum()];
	if (!thisThreadZip) {
		thisThreadZip = unzOpen(GetArchiveFile().c_str());
	}

	// Prevent opening files on missing/invalid archives
	if (thisThreadZip == nullptr)
		return -4;

	// assert(archiveLock.locked());
	assert(IsFileId(fid));

	unzGoToFilePos(thisThreadZip, &fileEntries[fid].fp);

	unz_file_info fi;
	unzGetCurrentFileInfo(thisThreadZip, &fi, nullptr, 0, nullptr, 0, nullptr, 0);

	if (unzOpenCurrentFile(thisThreadZip) != UNZ_OK)
		return -3;

	buffer.clear();
	buffer.resize(fi.uncompressed_size);

	int ret = 1;

	if (!buffer.empty() && unzReadCurrentFile(thisThreadZip, buffer.data(), buffer.size()) != buffer.size())
		ret -= 2;
	if (unzCloseCurrentFile(thisThreadZip) == UNZ_CRCERROR)
		ret -= 1;

	if (ret != 1)
		buffer.clear();

	return ret;
}

