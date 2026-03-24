#pragma once

#include "Common/Types.h"

class Version {
public:
    static int32_t Major();
    static int32_t Minor();
    static int32_t Patch();

    static String GetVersionString();
};