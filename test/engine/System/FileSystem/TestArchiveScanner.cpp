/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include <nowide/cstdlib.hpp>
#include <sys/stat.h>

namespace us {
	#include "../../../../tools/unitsync/unitsync_api.h"
}

namespace {

class TestDataDir {
public:
	TestDataDir()
		: oldCwd(std::filesystem::current_path())
		, path(std::filesystem::temp_directory_path() / ("spring-archive-scanner-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
	{
		std::filesystem::create_directories(path);
		SaveEnvironmentVariable("SPRING_WRITEDIR", oldWriteDir);
		SaveEnvironmentVariable("SPRING_ISOLATED", oldIsolation);
		const std::string pathString = path.string();
		nowide::setenv("SPRING_WRITEDIR", pathString.c_str(), 1);
		nowide::unsetenv("SPRING_ISOLATED");
	}

	~TestDataDir()
	{
		us::UnInit();
		RestoreEnvironmentVariable("SPRING_WRITEDIR", oldWriteDir);
		RestoreEnvironmentVariable("SPRING_ISOLATED", oldIsolation);
		std::filesystem::current_path(oldCwd);
		std::filesystem::remove_all(path);
	}

	const std::filesystem::path& GetPath() const { return path; }

private:
	static void SaveEnvironmentVariable(const char* name, std::string& value)
	{
		if (const char* current = nowide::getenv(name); current != nullptr)
			value = current;
	}

	static void RestoreEnvironmentVariable(const char* name, const std::string& value)
	{
		if (value.empty())
			nowide::unsetenv(name);
		else
			nowide::setenv(name, value.c_str(), 1);
	}

	const std::filesystem::path oldCwd;
	const std::filesystem::path path;
	std::string oldWriteDir;
	std::string oldIsolation;
};

class TemporaryFile {
public:
	TemporaryFile(const std::filesystem::path& path, const std::string& contents)
		: path(path)
	{
		std::ofstream file(this->path);
		file << contents;
	}

	~TemporaryFile()
	{
		std::filesystem::remove(path);
	}

private:
	const std::filesystem::path path;
};

std::string ArchiveCache(const std::string& archiveName, const std::string& archivePath, uint32_t modified, const std::string& fileEntries)
{
	return "return {\n"
		   "\tinternalver = 22,\n"
		   "\tarchives = {\n"
		   "\t\t{\n"
		   "\t\t\tname = \"" + archiveName + "\",\n"
		   "\t\t\tpath = \"" + archivePath + "\",\n"
		   "\t\t\tmodified = \"" + std::to_string(modified) + "\",\n"
		   "\t\t\tchecksum = \"\",\n"
		   "\t\t\tfilesInfo = {\n"
		   + fileEntries +
		   "\t\t\t},\n"
		   "\t\t},\n"
		   "\t},\n"
		   "\tbrokenArchives = {},\n"
		   "}\n";
}

std::string DirectoryOf(const std::filesystem::path& path)
{
	const std::string genericPath = path.generic_string();
	return genericPath.substr(0, genericPath.find_last_of('/') + 1);
}

uint32_t ModificationTimeOf(const std::filesystem::path& path)
{
	struct stat info;
	const std::string pathString = path.string();
	REQUIRE(stat(pathString.c_str(), &info) == 0);
	return info.st_mtime;
}

int InitializeUnitsync()
{
	return us::Init(false, 0);
}

}

TEST_CASE("ArchiveScanner accepts unreadable cached archive files")
{
	TestDataDir dataDir;
	const auto archivePath = dataDir.GetPath() / "unreadable.sdd";
	std::filesystem::create_directories(archivePath);
	std::filesystem::create_symlink(archivePath / "missing.txt", archivePath / "broken.txt");

	const auto cachePath = dataDir.GetPath() / "cache" / "ArchiveCache22.lua";
	std::filesystem::create_directories(cachePath.parent_path());
	const auto archiveDirectory = DirectoryOf(archivePath);
	const auto cache = ArchiveCache(
		archivePath.filename().string(),
		archiveDirectory,
		ModificationTimeOf(archivePath),
		"\t\t\t\t{ fileName = \"broken.txt\", size = \"-1\", modTime = \"0\", checksum = \"\" },\n"
	);
	TemporaryFile cacheFile(cachePath, cache);

	REQUIRE(InitializeUnitsync() != 0);
	// The unreadable entry must be discarded without aborting the scan.  The
	// scanner still returns the checksum assembled from the archive's file
	// names, even though the unreadable file itself was not hashed.
	const std::string archivePathString = archivePath.string();
	CHECK(us::GetArchiveChecksum(archivePathString.c_str()) != 0);
}

TEST_CASE("ArchiveScanner overwrites duplicate cached file entries")
{
	TestDataDir dataDir;
	const auto archivePath = dataDir.GetPath() / "duplicate.sdd";
	std::filesystem::create_directories(archivePath / "data");
	{
		std::ofstream file(archivePath / "data" / "test.txt");
		file << "archive cache regression";
	}

	const auto cachePath = dataDir.GetPath() / "cache" / "ArchiveCache22.lua";
	std::filesystem::create_directories(cachePath.parent_path());
	const auto archiveDirectory = DirectoryOf(archivePath);
	const auto cache = ArchiveCache(
		archivePath.filename().string(),
		archiveDirectory,
		ModificationTimeOf(archivePath),
		"\t\t\t\t{ fileName = \"data/test.txt\", size = \"1\", modTime = \"1\", checksum = \"\" },\n"
		"\t\t\t\t{ fileName = \"data/test.txt\", size = \"2\", modTime = \"2\", checksum = \"\" },\n"
	);
	TemporaryFile cacheFile(cachePath, cache);

	REQUIRE(InitializeUnitsync() != 0);
	const std::string archivePathString = archivePath.string();
	CHECK(us::GetArchiveChecksum(archivePathString.c_str()) != 0);
}
