#include <string>
#include <nowide/cstdio.hpp>
#include <sys/stat.h>
#include "System/Log/ILog.h"

#include <catch_amalgamated.hpp>

// needs to be included after catch
#include "System/FileSystem/FileSystem.h"

namespace {
	struct PrepareFileSystem {
		PrepareFileSystem() {
			oldDir = FileSystem::GetCwd();
			testCwd = GetTempDir();
			if (!testCwd.empty()) {
				FileSystem::ChDir(testCwd);

				// create files and dirs to test
				FileSystem::CreateDirectory("testDir");
				WriteFile("testFile.txt", "a");
			}
		}
		~PrepareFileSystem() {
			if (!testCwd.empty()) {
				// delete files and dirs created in the ctor
				FileSystem::DeleteFile("testFile.txt");
				FileSystem::DeleteFile("testDir");
			}

			FileSystem::ChDir(oldDir);
			if (!testCwd.empty()) {
				FileSystem::DeleteFile(testCwd);
			}
		}
		static void WriteFile(const std::string& filePath, const std::string& content) {
			FILE* testFile = nowide::fopen(filePath.c_str(), "w");
			if (testFile != nullptr) {
				fprintf(testFile, "%s", content.c_str());
				fclose(testFile);
				testFile = nullptr;
			} else {
				FAIL("Failed to create test-file " + filePath);
			}
		}
		std::string GetTempDir() {
			if (testCwd.empty()) {
				char* tmpDir = std::tmpnam(nullptr);
				if (tmpDir != nullptr) {
					testCwd = tmpDir;
					FileSystem::CreateDirectory(testCwd);
					if (!FileSystem::DirIsWritable(testCwd)) {
						FAIL("Failed to create temporary test dir");
					}
				} else {
					FAIL("Failed to get temporary file name");
				}
			}
			return testCwd;
		}

		std::string testCwd;
	private:
		std::string oldDir;
	};
}

PrepareFileSystem pfs;

TEST_CASE("FileExists")
{
	CHECK(FileSystem::FileExists(u8"testFile.txt"));
	CHECK_FALSE(FileSystem::FileExists(u8"testFile99.txt"));
	CHECK_FALSE(FileSystem::FileExists(u8"testDir"));
	CHECK_FALSE(FileSystem::FileExists(u8"testDir99"));
}


TEST_CASE("GetFileSize")
{
	CHECK(FileSystem::GetFileSize("testFile.txt") == 1);
	CHECK(FileSystem::GetFileSize("testFile99.txt") == -1);
	CHECK(FileSystem::GetFileSize("testDir") == -1);
	CHECK(FileSystem::GetFileSize("testDir99") == -1);
}


TEST_CASE("GetFileModificationDate")
{
	CHECK(FileSystem::GetFileModificationDate("testDir") != "");
	CHECK(FileSystem::GetFileModificationDate("testFile.txt") != "");
	CHECK(FileSystem::GetFileModificationDate("not_there") == "");
}


TEST_CASE("CreateDirectory")
{
	// create & exists
	CHECK(FileSystem::DirIsWritable("./"));
	CHECK(FileSystem::DirExists(u8"testDir"));
	CHECK(FileSystem::DirExists(u8"testDir///"));
	CHECK(FileSystem::DirExists(u8"testDir////./"));
	CHECK(FileSystem::ComparePaths("testDir", "testDir////./"));
	CHECK_FALSE(FileSystem::ComparePaths("testDir", "test Dir2"));
	CHECK(FileSystem::CreateDirectory("testDir")); // already exists
	CHECK(FileSystem::CreateDirectory("testDir1")); // should be created
	CHECK(FileSystem::CreateDirectory("test Dir2")); // should be created

	// check if exists & no overwrite
	CHECK(FileSystem::CreateDirectory("test Dir2")); // already exists
	CHECK(FileSystem::DirIsWritable("test Dir2"));
	CHECK_FALSE(FileSystem::CreateDirectory("testFile.txt")); // file with this name already exists

	// delete temporaries
	CHECK(FileSystem::DeleteFile("testDir1"));
	CHECK(FileSystem::DeleteFile("test Dir2"));

	// check if really deleted
	CHECK_FALSE(FileSystem::DirExists(u8"testDir1"));
	CHECK_FALSE(FileSystem::DirExists(u8"test Dir2"));
}


TEST_CASE("GetDirectory")
{
#define CHECK_DIR_EXTRACTION(path, dir) \
		CHECK(FileSystem::GetDirectory(path) == dir)

	CHECK_DIR_EXTRACTION("testFile.txt", "");
	CHECK_DIR_EXTRACTION("./foo/testFile.txt", "./foo/");

#undef CHECK_DIR_EXTRACTION
}


#define CHECK_NORM_PATH(path, normPath) \
		CHECK(FileSystem::GetNormalizedPath(path) == normPath)

TEST_CASE("GetNormalizedPath - basic paths") 
{
	CHECK_NORM_PATH("foo/bar", "foo/bar");
	CHECK_NORM_PATH("foo\\bar", "foo/bar");
	CHECK_NORM_PATH("/foo/bar", "/foo/bar");
	CHECK_NORM_PATH("C:/foo/bar", "C:/foo/bar");
}

TEST_CASE("GetNormalizedPath - multiple slashes") 
{
	CHECK_NORM_PATH("foo///bar", "foo/bar");
	CHECK_NORM_PATH("foo\\\\\\bar", "foo/bar");
	CHECK_NORM_PATH("//foo//bar//", "/foo/bar");
	CHECK_NORM_PATH("C:\\\\foo\\\\bar", "C:/foo/bar");
}

TEST_CASE("GetNormalizedPath - current directory") 
{
	CHECK_NORM_PATH("./foo/bar", "foo/bar");
	CHECK_NORM_PATH(".\\foo\\bar", "foo/bar");
	CHECK_NORM_PATH("foo/./bar", "foo/bar");
	CHECK_NORM_PATH("foo/.", "foo");
	CHECK_NORM_PATH(".", ".");
	CHECK_NORM_PATH("./", ".");
	CHECK_NORM_PATH("./.", ".");
}

TEST_CASE("GetNormalizedPath - parent directory") 
{
	CHECK_NORM_PATH("foo/bar/..", "foo");
	CHECK_NORM_PATH("foo/bar/../baz", "foo/baz");
	CHECK_NORM_PATH("foo/../bar", "bar");
	CHECK_NORM_PATH("./foo/../bar", "bar");
	CHECK_NORM_PATH("foo/bar/../../baz", "baz");
	CHECK_NORM_PATH("../foo", "../foo");
	CHECK_NORM_PATH("../../foo", "../../foo");
	CHECK_NORM_PATH("..", "..");
}

TEST_CASE("GetNormalizedPath - mixed cases") 
{
	CHECK_NORM_PATH("./foo/./bar/../baz", "foo/baz");
	CHECK_NORM_PATH("foo//./bar//..//baz", "foo/baz");
	CHECK_NORM_PATH("./a/b/c/../../d", "a/d");
	CHECK_NORM_PATH("C:\\foo\\.\\bar\\..\\baz", "C:/foo/baz");
}

TEST_CASE("GetNormalizedPath - Windows drives") 
{
	CHECK_NORM_PATH("C:/", "C:");
	CHECK_NORM_PATH("C:\\", "C:");
	CHECK_NORM_PATH("D:\\foo\\bar", "D:/foo/bar");
	CHECK_NORM_PATH("C:/foo/../bar", "C:/bar");
	CHECK_NORM_PATH("C:\\..\\foo", "C:/foo");
}

TEST_CASE("GetNormalizedPath - absolute paths") 
{
	CHECK_NORM_PATH("/", "/");
	CHECK_NORM_PATH("/foo", "/foo");
	CHECK_NORM_PATH("/foo/../bar", "/bar");
	CHECK_NORM_PATH("/foo/./bar", "/foo/bar");
}

TEST_CASE("GetNormalizedPath - trailing slashes") 
{
	CHECK_NORM_PATH("foo/bar/", "foo/bar");
	CHECK_NORM_PATH("foo/bar//", "foo/bar");
	CHECK_NORM_PATH("./foo/", "foo");
	CHECK_NORM_PATH("foo\\bar\\", "foo/bar");
	CHECK_NORM_PATH("foo\\bar\\\\", "foo/bar");
	CHECK_NORM_PATH(".\\foo\\", "foo");
	CHECK_NORM_PATH("C:\\foo\\", "C:/foo");
}

TEST_CASE("GetNormalizedPath - with file extensions") 
{
	CHECK_NORM_PATH("./foo/bar.txt", "foo/bar.txt");
	CHECK_NORM_PATH("foo/../bar.log", "bar.log");
	CHECK_NORM_PATH("./a/b/../c.txt", "a/c.txt");
}

TEST_CASE("GetNormalizedPath - UTF-8 support") 
{
	CHECK_NORM_PATH("./文档/测试.txt", "文档/测试.txt");
	CHECK_NORM_PATH("папка/файл.log", "папка/файл.log");
	CHECK_NORM_PATH("./日本語/../テスト", "テスト");
	CHECK_NORM_PATH("C:\\français\\café\\..\\thé.txt", "C:/français/thé.txt");
	CHECK_NORM_PATH("./مجلد/ملف.txt", "مجلد/ملف.txt");
}

TEST_CASE("GetNormalizedPath - spaces") 
{
	CHECK_NORM_PATH("foo bar/baz", "foo bar/baz");
	CHECK_NORM_PATH("./my folder/test.txt", "my folder/test.txt");
	CHECK_NORM_PATH("C:\\Program Files\\app", "C:/Program Files/app");
}

TEST_CASE("GetNormalizedPath - special characters") 
{
	CHECK_NORM_PATH("foo-bar_baz", "foo-bar_baz");
	CHECK_NORM_PATH("./file (1).txt", "file (1).txt");
	CHECK_NORM_PATH("foo@bar/baz#123", "foo@bar/baz#123");
}

TEST_CASE("GetNormalizedPath - edge cases with .. at boundaries") 
{
	CHECK_NORM_PATH("./..", "..");
	CHECK_NORM_PATH("foo/..", ".");
	CHECK_NORM_PATH("foo/../..", "..");
	CHECK_NORM_PATH("./foo/bar/../../..", "..");
}

TEST_CASE("GetNormalizedPath - original failing tests")
{
	CHECK_NORM_PATH("/home/userX/.spring/foo/bar///./../test.log", "/home/userX/.spring/foo/test.log");
	CHECK_NORM_PATH("./symLinkToHome/foo/bar///./../test.log", "symLinkToHome/foo/test.log");
	CHECK_NORM_PATH(".\\symLinkToHome\\foo\\bar\\\\\\.\\..\\test.log", "symLinkToHome/foo/test.log");
	CHECK_NORM_PATH("C:\\foo\\bar\\\\\\.\\..\\test.log", "C:/foo/test.log");
}

#undef CHECK_NORM_PATH