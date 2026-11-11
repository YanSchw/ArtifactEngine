#pragma once
#include "Common/Types.h"
#include "Object/Object.h"
#include "ScriptLibrary.gen.h"

class ScriptLibrary : public Object {
public:
    ARTIFACT_CLASS();

    FUNCTION(Category = "Utilities")
    static void PrintString(const String& InText);

    FUNCTION(Pure, Category = "Utilities")
    static Vec3 MakeVec3(float InX, float InY, float InZ);
};
