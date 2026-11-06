#pragma once
#include "AssetImporter.h"
#include "Audio/AudioSampleBuffer.h"
#include "AudioClipImporter.gen.h"

class AudioClipImporter : public AssetImporter {
public:
    ARTIFACT_CLASS();

    virtual bool Import(const String& InFilePath, AudioSampleBuffer& OutSamples) = 0;
};
