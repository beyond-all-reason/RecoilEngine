/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ZipArchive.h"

#include <algorithm>
#include <stdexcept>
#include <cassert>
#include <fstream>

#include <zstd.h>
#include <zlib.h>

#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/Threading/ThreadPool.h"
#include "System/TimeUtil.h"

namespace {
	// PKWARE APPNOTE compression method for Zstandard entries.
	constexpr uint16_t ZIP_METHOD_ZSTD = 93;

	uint16_t rdLE16(const uint8_t* p) { return uint16_t(p[0]) | (uint16_t(p[1]) << 8); }
	uint32_t rdLE32(const uint8_t* p) {
		return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
	}
	uint64_t rdLE64(const uint8_t* p) {
		return uint64_t(rdLE32(p)) | (uint64_t(rdLE32(p + 4)) << 32);
	}
}

IArchive* CZipArchiveFactory::DoCreateArchive(const std::string& filePath) const
{
	return new CZipArchive(filePath);
}

IArchive* CZipStdArchiveFactory::DoCreateArchive(const std::string& filePath) const
{
	// .sdzst is a regular zip whose entries use zstd (method 93); CZipArchive
	// reads both transparently, see CZipArchive::GetFileImpl.
	return new CZipArchive(filePath);
}

CZipArchive::CZipArchive(const std::string& archiveName)
	: CBufferedArchive(archiveName)
{
	static_assert(ThreadPool::MAX_THREADS <= CZipArchive::MAX_THREADS, "MAX_THREADS mismatch");
	static_assert(sizeof(decltype(afi)::ValueType) * 8 >= ThreadPool::MAX_THREADS);

	std::scoped_lock lck(archiveLock); //not needed?

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

		if (unzGetCurrentFileInfo(zip, &info, fName, sizeof(fName), nullptr, 0, nullptr, 0) != UNZ_OK)
			continue;

		if (fName[0] == 0)
			continue;

		const size_t fNameLen = strlen(fName);

		// exclude directory names
		if ((fName[fNameLen - 1] == '/') || (fName[fNameLen - 1] == '\\'))
			continue;

		unz_file_pos fp{};
		unzGetFilePos(zip, &fp);

		const auto& fd = fileEntries.emplace_back(
			std::move(fp), //fp
			info.uncompressed_size, //size
			fName, //origName
			info.crc, //crc
			static_cast<uint32_t>(CTimeUtil::DosTimeToTime64(info.dosDate)) //modTime
		);

		lcNameIndex.emplace(StringToLower(fd.origName), fileEntries.size() - 1);
	}

	zipPerThread[0] = zip;

	parallelAccessNum = ThreadPool::GetNumThreads(); // will open NumThreads parallel archives, this way GetFile() is no longer needs to be mutex locked
	sem = std::make_unique<decltype(sem)::element_type>(parallelAccessNum);
	const auto maxBitMask = (1u << parallelAccessNum) - 1;
	afi.SetMaxBitsMask(maxBitMask);
}

CZipArchive::~CZipArchive()
{
	std::scoped_lock lck(archiveLock); //not needed?

	for (auto& zip : zipPerThread) {
		if (zip) {
			unzClose(zip);
			zip = nullptr;
		}
	}
}


const std::string& CZipArchive::FileName(uint32_t fid) const
{
	assert(IsFileId(fid));
	return fileEntries[fid].origName;
}

int32_t CZipArchive::FileSize(uint32_t fid) const
{
	assert(IsFileId(fid));
	return fileEntries[fid].size;
}

IArchive::SFileInfo CZipArchive::FileInfo(uint32_t fid) const
{
	assert(IsFileId(fid));
	const auto& fe = fileEntries[fid];
	return IArchive::SFileInfo {
		.fileName = fe.origName,
		.specialFileName = "",
		.size = fe.size,
		.modTime = fe.modTime
	};
}

// Reads a zstd-compressed (method 93) entry. minizip cannot decode these and
// rejects them in its open path, so we locate the raw frame directly from the
// zip headers and decompress it with libzstd. The entry's central-directory
// position is the unz_file_pos minizip already gave us when scanning.
int CZipArchive::GetFileZstd(uint32_t fid, uint64_t compressedSize, uint64_t uncompressedSize, std::vector<std::uint8_t>& buffer)
{
	std::ifstream f(GetArchiveFile(), std::ios::binary);
	if (!f)
		return -4;

	// Central directory file header (46-byte fixed part) for this entry.
	const uint64_t cdOffset = fileEntries[fid].fp.pos_in_zip_directory;
	uint8_t cd[46];
	f.seekg(cdOffset);
	f.read(reinterpret_cast<char*>(cd), sizeof(cd));
	if (!f || rdLE32(cd) != 0x02014b50u)
		return -3;

	const uint16_t cdNameLen = rdLE16(cd + 28);
	const uint16_t cdExtraLen = rdLE16(cd + 30);
	uint64_t lhOffset = rdLE32(cd + 42); // relative offset of local header

	// Zip64: a 0xFFFFFFFF offset lives in the Zip64 extended-info extra field.
	if (lhOffset == 0xFFFFFFFFu) {
		std::vector<uint8_t> extra(cdExtraLen);
		f.seekg(cdOffset + 46 + cdNameLen);
		if (cdExtraLen != 0)
			f.read(reinterpret_cast<char*>(extra.data()), cdExtraLen);
		if (!f)
			return -3;

		// Within tag 0x0001 the 8-byte fields appear only for the 32-bit fields
		// that were set to 0xFFFFFFFF, in order: uncompressed, compressed, offset.
		const size_t skip = (rdLE32(cd + 24) == 0xFFFFFFFFu ? 8 : 0)
		                  + (rdLE32(cd + 20) == 0xFFFFFFFFu ? 8 : 0);
		bool found = false;
		for (size_t i = 0; i + 4 <= extra.size(); ) {
			const uint16_t tag = rdLE16(&extra[i]);
			const uint16_t sz = rdLE16(&extra[i + 2]);
			if (tag == 0x0001 && i + 4 + skip + 8 <= extra.size()) {
				lhOffset = rdLE64(&extra[i + 4 + skip]);
				found = true;
				break;
			}
			i += 4 + sz;
		}
		if (!found)
			return -3;
	}

	// Local file header (30-byte fixed part); name/extra lengths can differ
	// from the central-directory copy, so read them here.
	uint8_t lh[30];
	f.seekg(lhOffset);
	f.read(reinterpret_cast<char*>(lh), sizeof(lh));
	if (!f || rdLE32(lh) != 0x04034b50u)
		return -3;

	const uint64_t dataOffset = lhOffset + 30 + rdLE16(lh + 26) + rdLE16(lh + 28);

	std::vector<std::uint8_t> compressed(compressedSize);
	if (!compressed.empty()) {
		f.seekg(dataOffset);
		f.read(reinterpret_cast<char*>(compressed.data()), compressed.size());
		if (!f)
			return -1;
	}

	buffer.clear();
	buffer.resize(uncompressedSize);

	const size_t dSize = ZSTD_decompress(
		buffer.empty() ? nullptr : buffer.data(), buffer.size(),
		compressed.empty() ? nullptr : compressed.data(), compressed.size());
	if (ZSTD_isError(dSize) || dSize != buffer.size()) {
		buffer.clear();
		return -1;
	}

	if (crc32(0, buffer.data(), buffer.size()) != fileEntries[fid].crc) {
		buffer.clear();
		return 0;
	}

	return 1;
}

// To simplify things, files are always read completely into memory from
// the zip-file, since zlib does not provide any way of reading more
// than one file at a time
int CZipArchive::GetFileImpl(uint32_t fid, std::vector<std::uint8_t>& buffer)
{
	// this below will lead to expensive on-demand creation of thisThreadZip
	// in case actual number of parallel threads entering this function is
	// less than ThreadPool::GetThreadNum(). E.g. when counting_semaphore
	// dictates for less than ThreadPool::GetThreadNum() simultaneous IO operations
	//unzFile& thisThreadZip = zipPerThread[ThreadPool::GetThreadNum()];

	const auto tnum = afi.AcquireScoped();
	assert(tnum < parallelAccessNum);
	unzFile& thisThreadZip = zipPerThread[tnum];

	if (!thisThreadZip) {
		thisThreadZip = unzOpen(GetArchiveFile().c_str());
	}

	// Prevent opening files on missing/invalid archives
	if (thisThreadZip == nullptr)
		return -4;

	assert(IsFileId(fid));

	unzGoToFilePos(thisThreadZip, &fileEntries[fid].fp);

	unz_file_info fi;
	unzGetCurrentFileInfo(thisThreadZip, &fi, nullptr, 0, nullptr, 0, nullptr, 0);

	// minizip can enumerate but not decode zstd entries (method 93). It also
	// rejects method 93 in its open path, and the engine may link a prebuilt
	// minizip we cannot patch. So for zstd entries we bypass minizip's decoder
	// entirely: locate the raw frame via the zip headers and decompress it
	// ourselves (see GetFileZstd).
	if (fi.compression_method == ZIP_METHOD_ZSTD)
		return GetFileZstd(fid, fi.compressed_size, fi.uncompressed_size, buffer);

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

