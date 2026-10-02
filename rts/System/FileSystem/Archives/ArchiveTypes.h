/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ARCHIVE_TYPES_H
#define _ARCHIVE_TYPES_H

enum {
	ARCHIVE_TYPE_SDP = 0, // pool
	ARCHIVE_TYPE_SDD = 1, // dir
	ARCHIVE_TYPE_SDZ = 2, // zip
	ARCHIVE_TYPE_SD7 = 3, // 7zip
	ARCHIVE_TYPE_SDV = 4, // virtual
	ARCHIVE_TYPE_SDZST = 5, // zip with zstd entries (.sdzst files)
	ARCHIVE_TYPE_CNT = 6,
	ARCHIVE_TYPE_BUF = 7, // buffered, not created directly
};

#endif

