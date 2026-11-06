#pragma once
#include "Serialization/Assets/AudioClipImporter.h"
#include "MiniaudioAudioClipImporter.gen.h"

class MiniaudioAudioClipImporter : public AudioClipImporter {
public:
    ARTIFACT_CLASS();

    virtual bool CanImport(const String& InExtension) const override;
    virtual bool Import(const String& InFilePath, AudioSampleBuffer& OutSamples) override;
};
