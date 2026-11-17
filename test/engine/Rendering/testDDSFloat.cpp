/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// Tests that nv_dds loads uncompressed half-float DDS images (R16F, RG16F, RGBA16F; DX10 and legacy
// headers), uploads keep every mip tightly packed, and existing uncompressed formats
// keep their old GL description.
//
// nv_dds reads through CFileHandler (the VFS), which is far too heavy to link into a unit test.
// The stub below serves one in-memory file instead, either streamed (file on disk) or
// as a buffer (file in an archive), the two paths CDDSImage::load takes.

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/nv_dds.h"
#include "System/FileSystem/FileHandler.h"

#include <catch_amalgamated.hpp>

namespace {
	std::vector<uint8_t> g_fileData;
	bool g_asBuffer = false;

	struct UploadCall {
		GLenum target;
		GLint level, internalFormat;
		GLsizei width, height, depth;
		GLenum format, dataType;
		GLint unpackAlignment;
	};
	std::vector<UploadCall> g_uploads;
	GLint g_unpackAlignment = 4;

	void APIENTRY CaptureGetIntegerv(GLenum name, GLint* value)
	{
		REQUIRE(name == GL_UNPACK_ALIGNMENT);
		*value = g_unpackAlignment;
	}

	void APIENTRY CapturePixelStorei(GLenum name, GLint value)
	{
		REQUIRE(name == GL_UNPACK_ALIGNMENT);
		g_unpackAlignment = value;
	}

	void APIENTRY CaptureTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width,
		GLint, GLenum format, GLenum dataType, const void*)
	{
		g_uploads.push_back({target, level, internalFormat, width, 1, 1, format, dataType, g_unpackAlignment});
	}

	void APIENTRY CaptureTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width,
		GLsizei height, GLint, GLenum format, GLenum dataType, const void*)
	{
		g_uploads.push_back({target, level, internalFormat, width, height, 1, format, dataType, g_unpackAlignment});
	}

	void APIENTRY CaptureTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width,
		GLsizei height, GLsizei depth, GLint, GLenum format, GLenum dataType, const void*)
	{
		g_uploads.push_back({target, level, internalFormat, width, height, depth, format, dataType, g_unpackAlignment});
	}

	// Exercise the real upload functions without creating an OpenGL context.
	struct ScopedUploadCapture {
		decltype(glad_glGetIntegerv) getIntegerv = std::exchange(glad_glGetIntegerv, CaptureGetIntegerv);
		decltype(glad_glPixelStorei) pixelStorei = std::exchange(glad_glPixelStorei, CapturePixelStorei);
		decltype(glad_glTexImage1D) texImage1D = std::exchange(glad_glTexImage1D, CaptureTexImage1D);
		decltype(glad_glTexImage2D) texImage2D = std::exchange(glad_glTexImage2D, CaptureTexImage2D);
		decltype(glad_glTexImage3D) texImage3D = std::exchange(glad_glTexImage3D, CaptureTexImage3D);

		explicit ScopedUploadCapture(GLint alignment)
		{
			g_unpackAlignment = alignment;
			g_uploads.clear();
		}

		~ScopedUploadCapture()
		{
			glad_glGetIntegerv = getIntegerv;
			glad_glPixelStorei = pixelStorei;
			glad_glTexImage1D = texImage1D;
			glad_glTexImage2D = texImage2D;
			glad_glTexImage3D = texImage3D;
		}
	};
}

CFileHandler::CFileHandler(const std::string& name, const std::string& modes) { Open(name, modes); }

void CFileHandler::Open(const std::string& name, const std::string&)
{
	fileName = name;
	filePos = 0;
	fileSize = static_cast<int>(g_fileData.size());
	loadCode = g_asBuffer ? 1 : -3;
	fileBuffer.clear();
	if (g_asBuffer)
		fileBuffer = g_fileData;
}
void CFileHandler::Close() { fileBuffer.clear(); fileSize = -1; filePos = 0; }
int CFileHandler::GetPos() { return filePos; }
int CFileHandler::Read(void* buf, int length)
{
	const int n = std::min<int>(length, fileSize - filePos);
	if (n <= 0)
		return 0;
	if (g_asBuffer)
		std::memcpy(buf, fileBuffer.data() + filePos, n);
	else
		std::memcpy(buf, g_fileData.data() + filePos, n);
	filePos += n;
	return n;
}
bool CFileHandler::TryReadFromPWD(const std::string&) { return false; }
bool CFileHandler::TryReadFromRawFS(const std::string&) { return false; }
bool CFileHandler::TryReadFromVFS(const std::string&, int) { return false; }

namespace {
	void Put32(std::vector<uint8_t>& v, uint32_t x) { for (int i = 0; i < 4; ++i) v.push_back(uint8_t(x >> (8 * i))); }
	void Put16(std::vector<uint8_t>& v, uint16_t x) { v.push_back(uint8_t(x)); v.push_back(uint8_t(x >> 8)); }

	// exact for the values used here: multiples of 1/16 below 2048, which are exact binary16 numbers
	uint16_t ToHalf(float f)
	{
		if (f == 0.0f)
			return 0;
		uint32_t bits;
		std::memcpy(&bits, &f, sizeof(bits));
		const int exponent = int((bits >> 23) & 0xFF) - 127 + 15;
		return uint16_t(((bits >> 16) & 0x8000) | (exponent << 10) | ((bits >> 13) & 0x3FF));
	}

	// the value stored for component c of texel (x, y) in mip level `mip`
	float TexelValue(int mip, int x, int y, int c) { return float(1 + mip * 64 + y * 16 + x * 4 + c) / 16.0f; }

	// Builds a DDS in g_fileData. legacyFourCC != 0: legacy header with that FourCC; otherwise a DX10 header with dxgi.
	void MakeDDS(uint32_t w, uint32_t h, uint32_t mips, uint32_t components, uint32_t legacyFourCC, uint32_t dxgi, uint32_t depth = 1)
	{
		std::vector<uint8_t>& v = (g_fileData = {});
		Put32(v, 0x20534444); // "DDS "
		Put32(v, 124);
		Put32(v, nv_dds::DDSF_HEIGHT | nv_dds::DDSF_WIDTH | nv_dds::DDSF_PIXELFORMAT |
			nv_dds::DDSF_MIPMAPCOUNT | nv_dds::DDSF_PITCH | (depth > 1 ? nv_dds::DDSF_DEPTH : 0));
		Put32(v, h);
		Put32(v, w);
		Put32(v, w * components * 2);
		Put32(v, depth > 1 ? depth : 0);
		Put32(v, mips);
		for (int i = 0; i < 11; ++i) Put32(v, 0);
		Put32(v, 32);
		Put32(v, nv_dds::DDSF_FOURCC);
		Put32(v, legacyFourCC != 0 ? legacyFourCC : nv_dds::FOURCC_DX10);
		for (int i = 0; i < 5; ++i) Put32(v, 0);
		Put32(v, nv_dds::DDSF_TEXTURE | nv_dds::DDSF_COMPLEX | nv_dds::DDSF_MIPMAP);
		Put32(v, depth > 1 ? nv_dds::DDSF_VOLUME : 0);
		for (int i = 0; i < 3; ++i) Put32(v, 0);
		if (legacyFourCC == 0) {
			Put32(v, dxgi);
			Put32(v, depth > 1 ? nv_dds::DX10_DIMENSION_TEXTURE3D : nv_dds::DX10_DIMENSION_TEXTURE2D);
			Put32(v, 0);
			Put32(v, 1);
			Put32(v, 0);
		}
		for (uint32_t m = 0; m < mips; ++m) {
			for (uint32_t z = 0; z < std::max(1u, depth >> m); ++z)
				for (uint32_t y = 0; y < std::max(1u, h >> m); ++y)
					for (uint32_t x = 0; x < std::max(1u, w >> m); ++x)
						for (uint32_t c = 0; c < components; ++c)
							Put16(v, ToHalf(TexelValue(int(m), int(x), int(y), int(c))));
		}
	}

	struct Case { const char* name; uint32_t legacyFourCC, dxgi, components; unsigned glFormat, glInternal; };
	const Case cases[] = {
		{"DX10 R16G16_FLOAT",        0,                              nv_dds::DXGI_FORMAT_R16G16_FLOAT,       2, GL_RG,   GL_RG16F},
		{"legacy G16R16F",           nv_dds::D3DFMT_G16R16F,         0,                                      2, GL_RG,   GL_RG16F},
		{"DX10 R16_FLOAT",           0,                              nv_dds::DXGI_FORMAT_R16_FLOAT,          1, GL_RED,  GL_R16F},
		{"legacy R16F",              nv_dds::D3DFMT_R16F,            0,                                      1, GL_RED,  GL_R16F},
		{"DX10 R16G16B16A16_FLOAT",  0,                              nv_dds::DXGI_FORMAT_R16G16B16A16_FLOAT, 4, GL_RGBA, GL_RGBA16F},
		{"legacy A16B16G16R16F",     nv_dds::D3DFMT_A16B16G16R16F,   0,                                      4, GL_RGBA, GL_RGBA16F},
	};

	// reads half float c of texel (x, y) from tightly packed rows of `width` texels
	uint16_t Half(const unsigned char* data, uint32_t width, uint32_t components, uint32_t x, uint32_t y, uint32_t c)
	{
		const size_t idx = ((size_t(y) * width + x) * components + c) * 2;
		return uint16_t(data[idx] | (data[idx + 1] << 8));
	}

	// checks all texels of one mip level; flipped images have their rows in reverse order
	void CheckLevel(const unsigned char* data, uint32_t w, uint32_t h, uint32_t components, int mip, bool flipped)
	{
		for (uint32_t y = 0; y < h; ++y)
			for (uint32_t x = 0; x < w; ++x)
				for (uint32_t c = 0; c < components; ++c) {
					const uint32_t srcY = flipped ? (h - 1 - y) : y;
					REQUIRE(Half(data, w, components, x, y, c) == ToHalf(TexelValue(mip, int(x), int(srcY), int(c))));
				}
	}
}

// An existing uncompressed layout must keep describing itself exactly as before the float formats existed.
TEST_CASE("nv_dds keeps 8-bit uncompressed images unchanged")
{
	for (const bool asBuffer: {false, true}) {
		g_asBuffer = asBuffer;

		std::vector<uint8_t>& v = (g_fileData = {});
		Put32(v, 0x20534444);
		Put32(v, 124);
		Put32(v, nv_dds::DDSF_HEIGHT | nv_dds::DDSF_WIDTH | nv_dds::DDSF_PIXELFORMAT);
		Put32(v, 2); Put32(v, 3); Put32(v, 3 * 4); Put32(v, 0); Put32(v, 1);
		for (int i = 0; i < 11; ++i) Put32(v, 0);
		Put32(v, 32); Put32(v, nv_dds::DDSF_RGBA); Put32(v, 0); Put32(v, 32);
		Put32(v, 0x00FF0000); Put32(v, 0x0000FF00); Put32(v, 0x000000FF); Put32(v, 0xFF000000);
		Put32(v, nv_dds::DDSF_TEXTURE);
		for (int i = 0; i < 4; ++i) Put32(v, 0);
		for (uint32_t i = 0; i < 3 * 2 * 4; ++i) v.push_back(uint8_t(i));

		nv_dds::CDDSImage img;
		REQUIRE(img.load("test.dds", false));
		CHECK_FALSE(img.is_float());
		CHECK_FALSE(img.is_compressed());
		CHECK(img.get_format() == GL_BGRA);
		CHECK(img.get_internal_format() == 4); // the component count, as before
		CHECK(img.get_data_type() == GL_UNSIGNED_BYTE);
		CHECK(img.get_bytes_per_component() == 1);
		CHECK(img.get_size() == 3 * 2 * 4);
		const unsigned char* px = img;
		for (uint32_t i = 0; i < 3 * 2 * 4; ++i)
			CHECK(px[i] == uint8_t(i));
	}
}

TEST_CASE("nv_dds rejects unsupported float layouts")
{
	g_asBuffer = false;
	// DXGI_FORMAT_R32_FLOAT (41) is not supported
	MakeDDS(4, 4, 1, 2, 0, 41);
	nv_dds::CDDSImage img;
	CHECK_FALSE(img.load("test.dds", false));
	// a saved float image would be written as 8-bit data, so saving is refused
	MakeDDS(4, 4, 1, 2, 0, nv_dds::DXGI_FORMAT_R16G16_FLOAT);
	REQUIRE(img.load("test.dds", false));
	CHECK_FALSE(img.save("unused.dds"));
}

TEST_CASE("nv_dds loads uncompressed half-float images")
{
	for (const bool asBuffer: {false, true}) {
		g_asBuffer = asBuffer;
		for (const Case& tc: cases) {
			// odd, non-square size: 5x3 with 2 mips (5x3, 2x1) so that row padding assumptions would show
			const uint32_t w = 5, h = 3, mips = 2;

			for (const bool flip: {false, true}) {
				DYNAMIC_SECTION(tc.name << (asBuffer ? ", buffered" : ", streamed") << (flip ? ", flipped" : ", as stored")) {
					MakeDDS(w, h, mips, tc.components, tc.legacyFourCC, tc.dxgi);

					nv_dds::CDDSImage img;
					REQUIRE(img.load("test.dds", flip));
					CHECK(img.is_valid());
					CHECK(img.get_type() == nv_dds::TextureFlat);
					CHECK_FALSE(img.is_compressed());
					CHECK(img.is_float());
					CHECK(img.get_format() == tc.glFormat);
					CHECK(img.get_internal_format() == tc.glInternal);
					CHECK(img.get_data_type() == GL_HALF_FLOAT);
					CHECK(img.get_components() == tc.components);
					CHECK(img.get_width() == w);
					CHECK(img.get_height() == h);
					CHECK(img.get_size() == w * h * tc.components * 2);
					REQUIRE(img.get_num_mipmaps() == mips - 1);

					// every texel of both levels holds the value written to it
					CheckLevel(img, w, h, tc.components, 0, flip);
					const nv_dds::CSurface& mip1 = img.get_mipmap(0);
					CHECK(mip1.get_width() == 2);
					CHECK(mip1.get_height() == 1);
					CHECK(mip1.get_size() == 2 * 1 * tc.components * 2);
					CheckLevel(mip1, mip1.get_width(), mip1.get_height(), tc.components, 1, flip);
				}
			}
		}
	}
}

TEST_CASE("nv_dds uploads tightly packed half-float mipmaps and restores unpack alignment")
{
	g_asBuffer = false;
	for (const Case& tc: cases) {
		for (const uint32_t dimensions: {1u, 2u, 3u}) {
			for (const uint32_t w: {4u, 5u, 6u}) {
				for (const GLint alignment: {1, 2, 4, 8}) {
					DYNAMIC_SECTION(tc.name << ", " << dimensions << "D, width " << w << ", alignment " << alignment) {
						const uint32_t h = dimensions >= 2 ? 8 : 1;
						const uint32_t d = dimensions == 3 ? 8 : 1;
						const uint32_t mips = dimensions >= 2 ? 4 : 3;
						MakeDDS(w, h, mips, tc.components, tc.legacyFourCC, tc.dxgi, d);
						nv_dds::CDDSImage img;
						REQUIRE(img.load("test.dds", false));

						ScopedUploadCapture capture(alignment);
						const GLenum target = dimensions == 1 ? GL_TEXTURE_1D : dimensions == 2 ? GL_TEXTURE_2D : GL_TEXTURE_3D;
						switch (dimensions) {
							case 1: REQUIRE(img.upload_texture1D()); break;
							case 2: REQUIRE(img.upload_texture2D(0, target)); break;
							case 3: REQUIRE(img.upload_texture3D()); break;
						}
						CHECK(g_unpackAlignment == alignment);
						REQUIRE(g_uploads.size() == mips);
						for (uint32_t level = 0; level < mips; ++level) {
							const UploadCall& upload = g_uploads[level];
							CHECK(upload.target == target);
							CHECK(upload.level == level);
							CHECK(upload.internalFormat == tc.glInternal);
							CHECK(upload.format == tc.glFormat);
							CHECK(upload.dataType == GL_HALF_FLOAT);
							CHECK(upload.width == std::max(1u, w >> level));
							CHECK(upload.height == std::max(1u, h >> level));
							CHECK(upload.depth == std::max(1u, d >> level));
							if (upload.height > 1 || upload.depth > 1) {
								// GL must advance by the tightly packed row size at every level.
								// In particular, 4x8 R16F has a 1x2 mip with two-byte rows.
								const size_t rowBytes = upload.width * tc.components * sizeof(uint16_t);
								const size_t rowAlignment = std::max<size_t>(sizeof(uint16_t), upload.unpackAlignment);
								CHECK(rowBytes % rowAlignment == 0);
							}
						}
					}
				}
			}
		}
	}
}
