#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/Types.h"
#include "Common/Array.h"
#include "Common/Map.h"
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

    static void MountContent(const String& InKey, const String& InDir);
    static String GetEngineContentDir();
    static String GetProjectContentDir();
    static String GetContentDir(const String& InKey);
    static String ResolveContentPath(const String& InRelativePath);
    static Array<String> GetContentMountDirs();
    static Array<String> GetContentMountKeys();

private:
    static void Initialize(const Array<String>& InArgs);
    static void LoadConfigVars(const Array<String>& InArgs);
    static void ResolvePaths();

    inline static Map<String, String> s_ContentMounts;
    inline static Array<String> s_ContentMountKeys; // registration order, for deterministic search

    friend int ArtifactMain(const Array<String>& InArgs);
};