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
    static Class EngineClass() { return s_EngineClass; }
    static Class RenderPipelineClass() { return s_RenderPipelineClass; }
    static String ContentDir() { return s_ContentDir; }

private:
    static void Initialize(const Array<String>& InArgs);
    static void ResolvePaths();

    inline static Class s_EngineClass = Class("Engine");
    inline static Class s_RenderPipelineClass = Class("ArtifactRenderPipeline");
    inline static String s_ContentDir = String("Content");

    friend int ArtifactMain(const Array<String>& InArgs);
};