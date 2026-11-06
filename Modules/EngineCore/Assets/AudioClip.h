#pragma once
#include "Asset.h"
#include "Audio/AudioSampleBuffer.h"
#include "AudioClip.gen.h"

class AudioClip : public Asset {
public:
    ARTIFACT_CLASS();
    AudioClip();
    virtual ~AudioClip() = default;

protected:
    virtual void Load() override;
    virtual void Unload() override;
    virtual void Cook(class ChunkedBinary& OutChunkedBinary) override;

public:
    virtual bool IsLoaded() const override;
    virtual String GetDisplayName() const override { return IsSubAsset() ? Asset::GetDisplayName() : DisplayNameFromPath(m_AudioPath); }
    virtual String GetSourcePath() const override { return m_AudioPath; }
    virtual void SetSourcePath(const String& InPath) override { m_AudioPath = InPath; }

    const SharedAudioSamples& GetSamples() const { return m_Samples; }
    double GetDuration() const;

    void Reimport();

private:
    bool ImportSource(AudioSampleBuffer& OutSamples) const;

    SharedAudioSamples m_Samples;

    PROPERTY()
    String m_AudioPath;

    PROPERTY()
    AudioSampleFormat m_SampleFormat = AudioSampleFormat::Int16;

    PROPERTY()
    uint32_t m_MaxSampleRate = 48000;

    PROPERTY()
    bool m_ForceMono = false;

    PROPERTY()
    float m_Volume = 1.0f;
};
