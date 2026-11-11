#include "ScriptLibrary.h"
#include "Core/Log.h"

void ScriptLibrary::PrintString(const String& InText) {
    AE_INFO("{0}", InText);
}

Vec3 ScriptLibrary::MakeVec3(float InX, float InY, float InZ) {
    return Vec3(InX, InY, InZ);
}
