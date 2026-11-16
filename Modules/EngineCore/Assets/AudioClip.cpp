#include "AudioClip.h"
#include "Core/EngineConfig.h"
#include "Core/EngineSettings.h"
#include "Serialization/ChunkedBinary.h"
#include "Serialization/Assets/AudioClipImporter.h"
#include "Serialization/Compression/AdpcmCodec.h"

#include <algorithm>

namespace {

void EncodeAdpcm(const AudioSampleBuffer& InSamples, ChunkWriter& OutWriter) {
    const AudioSampleBuffer pcm = InSamples.WithSampleFormat(AudioSampleFormat::Int16);
    AdpcmCodec::Encode((const int16_t*)pcm.GetData().Data(), pcm.GetFrameCount() * pcm.GetChannels(), pcm.GetChannels(), OutWriter);
}

AudioSampleBuffer DecodeAdpcm(ChunkReader& InReader, const AudioFormat& InFormat, uint64_t InFrameCount) {
    AudioSampleBuffer pcm({ InFormat.SampleRate, InFormat.Channels, AudioSampleFormat::Int16 }, InFrameCount);
    AdpcmCodec::Decode(InReader, (int16_t*)pcm.GetData().Data(), InFrameCount * InFormat.Channels, InFormat.Channels);
    return pcm.WithSampleFormat(InFormat.SampleFormat);
}

} // namespace

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
    AudioCompression compression = AudioCompression::Uncompressed;
    chunkReader >> format.SampleRate >> format.Channels >> sampleFormat >> frameCount >> compression;
    format.SampleFormat = (AudioSampleFormat)sampleFormat;

    if (compression == AudioCompression::ADPCM) {
        m_Samples = std::make_shared<const AudioSampleBuffer>(DecodeAdpcm(chunkReader, format, frameCount));
        return;
    }
    std::shared_ptr<AudioSampleBuffer> samples = std::make_shared<AudioSampleBuffer>(format, frameCount);
    chunkReader.ReadBytes(samples->GetData().Data(), samples->GetData().Size());
    m_Samples = samples;
#else
    AudioSampleBuffer samples;
    if (ImportSource(samples) && CompressionSettings::Resolve(m_Compression) == AudioCompression::ADPCM) {
        ChunkWriter chunkWriter;
        EncodeAdpcm(samples, chunkWriter);
        ChunkReader chunkReader(chunkWriter.GetData());
        samples = DecodeAdpcm(chunkReader, samples.GetFormat(), samples.GetFrameCount());
    }
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

    const AudioCompression compression = CompressionSettings::Resolve(m_Compression);

    ChunkWriter chunkWriter;
    chunkWriter << samples.GetSampleRate();
    chunkWriter << samples.GetChannels();
    chunkWriter << (uint8_t)samples.GetFormat().SampleFormat;
    chunkWriter << samples.GetFrameCount();
    chunkWriter << compression;
    if (compression == AudioCompression::ADPCM) {
        EncodeAdpcm(samples, chunkWriter);
    } else {
        chunkWriter.WriteBytes(samples.GetData().Data(), samples.GetData().Size());
    }
    OutChunkedBinary.AddChunk(1, chunkWriter);
}

bool AudioClip::IsLoaded() const {
    return Super::IsLoaded() && m_Samples != nullptr;
}

double AudioClip::GetDuration() const {
    return m_Samples ? m_Samples->GetDuration() : 0.0;
}
