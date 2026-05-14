#pragma once
#include "Common/String.h"

struct Enum {
    String Name;
    
    Enum(const String& name) : Name(name) {}
};

#ifndef GENERATED_BODY /* in-case Object.h is included */
#define GENERATED_BODY_CONCAT(line) _GENERATED_BODY_##line
#define GENERATED_BODY(line) GENERATED_BODY_CONCAT(line)
#endif

#define ARTIFACT_ENUM(...) GENERATED_BODY(__LINE__);
