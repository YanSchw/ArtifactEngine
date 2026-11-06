#include "AudioClip.h"
#include "Core/EngineConfig.h"
#include "Serialization/ChunkedBinary.h"
#include "Serialization/Assets/AudioClipImporter.h"

#include <algorithm>

AudioClip::AudioClip() {
    m_StreamType = AssetStreamType::AlwaysLoaded;
}

bool AudioClip::ImportSource(AudioSampleBuffer& OutSamples) const {
    const String path = EngineConfig::ResolveContentPath(m_AudioPath);

    AudioSampleBuffer decoded;
    AudioClipImporter* importer = AssetImporter::Find<AudioClipImporter>(path);
    if (!importer || !importer->Import(path, decoded) || decoded.IsEmpty()) {
        AE_WARN("Importing audio from file {0} was unsuccessful!", path);
        return false;
    }

    AudioFormat format;
    format.SampleRate = m_MaxSampleRate > 0 ? std::min(decoded.GetSampleRate(), m_MaxSampleRate) : decoded.GetSampleRate();
    format.Channels = m_ForceMono ? 1 : std::min(decoded.GetChannels(), 2u);
    format.SampleFormat = AudioSampleFormat::Float32;

    OutSamples = decoded.ConvertTo(format);
    OutSamples.ApplyGain(m_Volume);
    OutSamples = OutSamples.WithSampleFormat(m_SampleFormat);
    return true;
}

void AudioClip::Load() {
#if defined(AE_PACKAGED)
    SharedObjectPtr<ChunkedBinary> binary = GetChunkedBinary();
    if (!binary || !binary->HasChunk(1)) {
        m_Samples = std::make_shared<const AudioSampleBuffer>();
        return;
    }
    ChunkReader chunkReader = binary->GetChunk(1);
    AudioFormat format;
    uint8_t sampleFormat = 0;
    uint64_t frameCount = 0;
    chunkReader >> format.SampleRate >> format.Channels >> sampleFormat >> frameCount;
    format.SampleFormat = (AudioSampleFormat)sampleFormat;

    std::shared_ptr<AudioSampleBuffer> samples = std::make_shared<AudioSampleBuffer>(format, frameCount);
    chunkReader.ReadBytes(samples->GetData().Data(), samples->GetData().Size());
    m_Samples = samples;
#else
    AudioSampleBuffer samples;
    ImportSource(samples);
    m_Samples = std::make_shared<const AudioSampleBuffer>(std::move(samples));
#endif
}

void AudioClip::Reimport() {
    if (IsLoaded()) {
        Load();
    }
}

void AudioClip::Unload() {
    m_Samples = nullptr;
}

void AudioClip::Cook(ChunkedBinary& OutChunkedBinary) {
    Super::Cook(OutChunkedBinary);

    AudioSampleBuffer samples;
    if (!ImportSource(samples)) {
        return;
    }

    ChunkWriter chunkWriter;
    chunkWriter << samples.GetSampleRate();
    chunkWriter << samples.GetChannels();
    chunkWriter << (uint8_t)samples.GetFormat().SampleFormat;
    chunkWriter << samples.GetFrameCount();
    chunkWriter.WriteBytes(samples.GetData().Data(), samples.GetData().Size());
    OutChunkedBinary.AddChunk(1, chunkWriter);
}

bool AudioClip::IsLoaded() const {
    return Super::IsLoaded() && m_Samples != nullptr;
}

double AudioClip::GetDuration() const {
    return m_Samples ? m_Samples->GetDuration() : 0.0;
}
