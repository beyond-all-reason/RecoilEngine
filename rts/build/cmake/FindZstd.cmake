# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# - Find the Zstandard (zstd) library
# Find the native zstd includes and library (static or shared)
#
#  ZSTD_INCLUDE_DIR - where to find zstd.h
#  ZSTD_LIBRARIES   - List of libraries when using zstd.
#  ZSTD_FOUND       - True if zstd was found.

Include(FindPackageHandleStandardArgs)

If     (ZSTD_INCLUDE_DIR)
  # Already in cache, be silent
  Set(ZSTD_FIND_QUIETLY TRUE)
EndIf  (ZSTD_INCLUDE_DIR)

Find_Path(ZSTD_INCLUDE_DIR zstd.h)

Set(ZSTD_NAMES zstd)
Find_Library(ZSTD_LIBRARY NAMES ${ZSTD_NAMES})

# handle the QUIETLY and REQUIRED arguments and set ZSTD_FOUND to TRUE if
# all listed variables are TRUE
Find_Package_Handle_Standard_Args(Zstd DEFAULT_MSG ZSTD_LIBRARY ZSTD_INCLUDE_DIR)

If     (ZSTD_FOUND)
  Set(ZSTD_LIBRARIES ${ZSTD_LIBRARY})
Else   (ZSTD_FOUND)
  Set(ZSTD_LIBRARIES)
EndIf  (ZSTD_FOUND)

Mark_As_Advanced(ZSTD_LIBRARY ZSTD_INCLUDE_DIR)
