/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef REQUEST_PATH_INTERNAL_SYSTEM_H__
#define REQUEST_PATH_INTERNAL_SYSTEM_H__

namespace QTPFS {

class RequestPathInternalSystem {
public:
    static void Init();
    static void Update();
    static void Shutdown();
};

}

#endif