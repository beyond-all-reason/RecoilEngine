/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DUMPHISTORY_H
#define DUMPHISTORY_H

#include <nowide/fstream.hpp>

extern void DumpHistory(nowide::fstream& file, int frameNum, bool serverRequest);

#endif /* DUMPHISTORY_H */
