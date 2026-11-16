#pragma once
#include "ProjectSettings.h"
#include "Object/SubclassOf.h"
#include "Common/UUID.h"
#include "Rendering/RenderPipeline.h"
#include "EngineSettings.gen.h"

class GameSettings : public ProjectSettings {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    String ProjectName = "Artifact";

    PROPERTY(AssetType = Scene)
    UUID DefaultScene = UUID::FromString("a1b2c3d4-0001-4a00-9000-000000000001");

    PROPERTY(AssetType = Font)
    UUID DefaultFont = UUID::FromString("f0e1d2c3-b4a5-4967-8899-aabbccddeeff");
};

class RenderingSettings : public ProjectSettings {
public:
    ARTIFACT_CLASS();

    virtual String GetConfigFile() const override { return "Engine"; }

    PROPERTY()
    SubclassOf<RenderPipeline> RenderPipelineClass = Class("ArtifactRenderPipeline");

    PROPERTY(Range = (0, 1000))
    int32_t CapFPS = 60;
};

class AudioSettings : public ProjectSettings {
public:
    ARTIFACT_CLASS();

    virtual String GetConfigFile() const override { return "Engine"; }

    PROPERTY(Range = (8000, 192000))
    int32_t AudioSampleRate = 48000;

    PROPERTY(Range = (64, 8192))
    int32_t AudioBufferFrames = 512;

    PROPERTY(Range = (1, 1024))
    int32_t AudioMaxVoices = 128;
};

class PackagingSettings : public ProjectSettings {
public:
    ARTIFACT_CLASS();

    virtual String GetConfigFile() const override { return "Engine"; }

    PROPERTY()
    Array<String> FrozenConfigVars;
};
