#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/Types.h"
#include "EngineConfig.gen.h"

class EngineConfig : public Object {
public:
    ARTIFACT_CLASS();
    EngineConfig() = delete;

public:
    static bool IsPackagedBuild();
    static bool SetConfigVar(const String& InName, const String& InValue);
    template<typename T>
    static T GetConfigVar(const String& InName);

    static Class EngineClass();
    static Class RenderPipelineClass();
    static String ContentDir() { return s_ContentDir; }

private:
    static void Initialize(const Array<String>& InArgs);
    static void LoadConfigVars(const Array<String>& InArgs);
    static void ResolvePaths();

    inline static String s_ContentDir = String("Content");

    friend int ArtifactMain(const Array<String>& InArgs);
};