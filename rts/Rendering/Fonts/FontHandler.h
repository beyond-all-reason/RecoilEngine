/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FONT_HANDLER_H
#define _FONT_HANDLER_H

#include "System/Misc/NonCopyable.h"
/*
#include <vector>

#include "System/float3.h"
#include "System/creg/creg_cond.h"
#include "System/UnorderedSet.hpp"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/SimObjectIDPool.h"
*/

class CFontHandler : public spring::noncopyable
{
public:
	CFontHandler();
	bool Init();

	bool useNewColorIndicators = false;
};

extern CFontHandler fontHandler;


#endif // _FONT_HANDLER_H
